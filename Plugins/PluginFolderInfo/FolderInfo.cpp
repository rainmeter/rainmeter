/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FolderInfo.h"
#include <windows.h>
#include <list>
#include "../API/RainmeterAPI.h"

#define UPDATE_TIME_MIN_MS (10000ULL)

CFolderInfo::CFolderInfo(void* ownerSkin) :
	m_InstanceCount(1),
	m_Skin(ownerSkin),
	m_IncludeSubFolders(false),
	m_IncludeHiddenFiles(false),
	m_IncludeSystemFiles(false),
	m_Size(),
	m_FileCount(),
	m_FolderCount(),
	m_RegExpFilter(),
	m_LastUpdateTime()
{
}

CFolderInfo::~CFolderInfo()
{
	FreePcre();
}

void CFolderInfo::AddInstance()
{
	++m_InstanceCount;
}

void CFolderInfo::RemoveInstance()
{
	--m_InstanceCount;
	if (m_InstanceCount == 0)
	{
		delete this;
	}
}

void CFolderInfo::Clear()
{
	m_Size = 0;
	m_FileCount = 0;
	m_FolderCount = 0;
}

void CFolderInfo::FreePcre()
{
	if (m_RegExpFilter)
	{
		pcre16_free(m_RegExpFilter);
		m_RegExpFilter = nullptr;
	}
}

void CFolderInfo::Update()
{
	ULONGLONG now = GetTickCount64();
	if (now - m_LastUpdateTime > UPDATE_TIME_MIN_MS)
	{
		Clear();

		if (!m_Path.empty())
		{
			CalculateSize();
		}

		m_LastUpdateTime = now;
	}
}

void CFolderInfo::CalculateSize()
{
	std::list<RawString> folderQueue;
	folderQueue.push_back(m_Path.c_str());

	WCHAR searchPattern[MAX_PATH + 10];
	WCHAR buffer[MAX_PATH];
	WIN32_FIND_DATA findData;
	HANDLE findHandle;
	while (!folderQueue.empty())
	{
		const RawString& ref = folderQueue.front();
		wsprintf(searchPattern, L"%s%s", ref.c_str(), L"\\*.*");

		findHandle = FindFirstFile(searchPattern, &findData);
		if (INVALID_HANDLE_VALUE == findHandle)
		{
			folderQueue.pop_front();
			continue;
		}

		do
		{
			// special case for "." and ".."
			if (wcscmp(findData.cFileName, L".") == 0 ||
				wcscmp(findData.cFileName, L"..") == 0)
			{
				continue;
			}

			bool isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

			if (!m_IncludeHiddenFiles && (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				continue;
			}
			else if (!m_IncludeSystemFiles && (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
			{
				continue;
			}
			else if (!isFolder && m_RegExpFilter)
			{
				if (pcre16_exec(
						m_RegExpFilter, nullptr,
						(PCRE_SPTR16)findData.cFileName, (int)wcslen(findData.cFileName),
						0, 0, nullptr, 0) != 0)
				{
					continue;
				}
			}

			if (isFolder)
			{
				m_FolderCount++;
				if (m_IncludeSubFolders)
				{
					wsprintf(buffer, L"%s\\%s", ref.c_str(), findData.cFileName);
					folderQueue.push_back(buffer);
				}
			}
			else
			{
				m_FileCount++;
				m_Size += ((UINT64)findData.nFileSizeHigh << 32) + findData.nFileSizeLow;
			}
		}
		while (FindNextFile(findHandle, &findData));
		FindClose(findHandle);

		folderQueue.pop_front();
	}
}

void CFolderInfo::SetPath(LPCWSTR path)
{
	if (wcscmp(m_Path.c_str(), path) != 0)
	{
		m_Path = path;

		// Force update next time
		m_LastUpdateTime = 0ULL;
	}
}

void CFolderInfo::SetRegExpFilter(LPCWSTR filter)
{
	FreePcre();

	if (*filter)
	{
		const char* error;
		int erroffset;
		m_RegExpFilter = pcre16_compile(
			(PCRE_SPTR16)filter, PCRE_UTF16, &error, &erroffset, nullptr);
	}
}
