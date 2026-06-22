/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureFolderInfo.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Skin.h"
#include "../Common/RawString.h"
#include "pcre/config.h"
#include "pcre/pcre.h"

#define UPDATE_TIME_MIN_MS (10000ULL)

namespace {

class FolderInfo
{
public:
	FolderInfo();
	~FolderInfo();

	void SetPath(const WCHAR* path);
	void SetRegExpFilter(const WCHAR* filter);
	void SetSubFolders(bool flag) { m_IncludeSubFolders = flag; }
	void SetHiddenFiles(bool flag) { m_IncludeHiddenFiles = flag; }
	void SetSystemFiles(bool flag) { m_IncludeSystemFiles = flag; }

	UINT64 GetSize() { return m_Size; }
	UINT GetFileCount() { return m_FileCount; }
	UINT GetFolderCount() { return m_FolderCount; }

	void Update();

private:
	void Clear();
	void FreePcre();
	void CalculateSize();

	RawString m_Path;
	bool m_IncludeSubFolders;
	bool m_IncludeHiddenFiles;
	bool m_IncludeSystemFiles;
	UINT64 m_Size;
	UINT m_FileCount;
	UINT m_FolderCount;
	pcre16* m_RegExpFilter;
	ULONGLONG m_LastUpdateTime;
};

FolderInfo::FolderInfo() :
	m_IncludeSubFolders(false),
	m_IncludeHiddenFiles(false),
	m_IncludeSystemFiles(false),
	m_Size(0ULL),
	m_FileCount(0U),
	m_FolderCount(0U),
	m_RegExpFilter(nullptr),
	m_LastUpdateTime(0ULL)
{
}

FolderInfo::~FolderInfo()
{
	FreePcre();
}

void FolderInfo::Clear()
{
	m_Size = 0ULL;
	m_FileCount = 0U;
	m_FolderCount = 0U;
}

void FolderInfo::FreePcre()
{
	if (m_RegExpFilter)
	{
		pcre16_free(m_RegExpFilter);
		m_RegExpFilter = nullptr;
	}
}

void FolderInfo::Update()
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

void FolderInfo::CalculateSize()
{
	std::list<RawString> folderQueue;
	folderQueue.push_back(m_Path.c_str());

	WCHAR searchPattern[MAX_PATH + 10] = { 0 };
	WCHAR buffer[MAX_PATH] = { 0 };
	WIN32_FIND_DATA findData = { 0 };
	HANDLE findHandle = nullptr;
	while (!folderQueue.empty())
	{
		const RawString& ref = folderQueue.front();
		_snwprintf_s(searchPattern, _countof(searchPattern), L"%s%s", ref.c_str(), L"\\*.*");

		findHandle = FindFirstFile(searchPattern, &findData);
		if (INVALID_HANDLE_VALUE == findHandle)
		{
			folderQueue.pop_front();
			continue;
		}

		do
		{
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
				++m_FolderCount;
				if (m_IncludeSubFolders)
				{
					_snwprintf_s(buffer, _countof(buffer), L"%s\\%s", ref.c_str(), findData.cFileName);
					folderQueue.push_back(buffer);
				}
			}
			else
			{
				++m_FileCount;
				m_Size += ((UINT64)findData.nFileSizeHigh << 32) + findData.nFileSizeLow;
			}
		}
		while (FindNextFile(findHandle, &findData));
		FindClose(findHandle);
		findHandle = nullptr;

		folderQueue.pop_front();
	}
}

void FolderInfo::SetPath(const WCHAR* path)
{
	if (wcscmp(m_Path.c_str(), path) != 0)
	{
		m_Path = path;
		m_LastUpdateTime = 0ULL;
	}
}

void FolderInfo::SetRegExpFilter(const WCHAR* filter)
{
	FreePcre();

	if (*filter)
	{
		const char* error = nullptr;
		int erroffset = 0;
		m_RegExpFilter = pcre16_compile(
			(PCRE_SPTR16)filter, PCRE_UTF16, &error, &erroffset, nullptr);
	}
}

}  // namespace

struct FolderInfoParentMeasure
{
	FolderInfoParentMeasure(MeasureFolderInfo* measure) :
		folder(),
		owner(measure),
		measureCount(1U)
	{
	}

	FolderInfo folder;
	MeasureFolderInfo* owner;
	UINT measureCount;
};

static std::vector<FolderInfoParentMeasure*> g_ParentMeasures;

enum class MeasureFolderInfo::Type
{
	FileCount,
	FolderCount,
	FolderSize
};

MeasureFolderInfo::MeasureFolderInfo(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Parent(),
	m_Type(Type::FileCount)
{
}

MeasureFolderInfo::~MeasureFolderInfo()
{
	if (m_Parent)
	{
		if (--m_Parent->measureCount == 0U)
		{
			auto iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), m_Parent);
			g_ParentMeasures.erase(iter);

			delete m_Parent;
			m_Parent = nullptr;
		}
	}
}

void MeasureFolderInfo::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	const WCHAR* type = parser.ReadString(section, L"InfoType", L"").c_str();
	if (_wcsicmp(type, L"FolderSize") == 0 || _wcsicmp(type, L"FolderSizeStr") == 0)
	{
		m_Type = Type::FolderSize;
	}
	else if (_wcsicmp(type, L"FolderCount") == 0 || _wcsicmp(type, L"FolderCountStr") == 0)
	{
		m_Type = Type::FolderCount;
	}
	else if (_wcsicmp(type, L"FileCount") == 0 || _wcsicmp(type, L"FileCountStr") == 0 || !*type)
	{
		m_Type = Type::FileCount;
	}
	else
	{
		m_Type = Type::FileCount;
		LogErrorF(this, L"Invalid InfoType=%s", type);
	}

	const std::wstring& folder = parser.ReadString(section, L"Folder", L"", false);
	const WCHAR* str = folder.c_str();
	if (str[0] == L'[')
	{
		if (m_Parent)
		{
			return;
		}

		++str;
		size_t len = wcslen(str);
		if (len > 0 && str[len - 1] == L']')
		{
			--len;

			for (auto iter = g_ParentMeasures.begin(); iter != g_ParentMeasures.end(); ++iter)
			{
				if (GetSkin() == (*iter)->owner->GetSkin() &&
					_wcsnicmp(str, (*iter)->owner->GetName(), len) == 0)
				{
					m_Parent = (*iter);
					++m_Parent->measureCount;
					return;
				}
			}
		}

		LogWarningF(this, L"Invalid Folder=%s", folder.c_str());
		return;
	}

	if (m_Parent)
	{
		if (m_Parent->owner != this)
		{
			return;
		}
	}
	else
	{
		m_Parent = new FolderInfoParentMeasure(this);
		g_ParentMeasures.push_back(m_Parent);
	}

	std::wstring path = parser.ReadString(section, L"Folder", L"");
	GetSkin()->MakePathAbsolute(path);
	m_Parent->folder.SetPath(path.c_str());

	str = parser.ReadString(section, L"RegExpFilter", L"").c_str();
	m_Parent->folder.SetRegExpFilter(str);

	m_Parent->folder.SetSubFolders(parser.ReadBool(section, L"IncludeSubFolders", false));
	m_Parent->folder.SetHiddenFiles(parser.ReadBool(section, L"IncludeHiddenFiles", false));
	m_Parent->folder.SetSystemFiles(parser.ReadBool(section, L"IncludeSystemFiles", false));
}

void MeasureFolderInfo::UpdateValue()
{
	if (!m_Parent)
	{
		m_Value = 0.0;
		return;
	}

	if (m_Parent->owner == this)
	{
		m_Parent->folder.Update();
	}

	switch (m_Type)
	{
	case Type::FolderSize:
		m_Value = (double)m_Parent->folder.GetSize();
		break;

	case Type::FileCount:
		m_Value = (double)m_Parent->folder.GetFileCount();
		break;

	case Type::FolderCount:
		m_Value = (double)m_Parent->folder.GetFolderCount();
		break;
	}
}
