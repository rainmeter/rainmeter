// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureFolderInfo.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Pcre.h"
#include "Skin.h"
#include "../Common/RawString.h"

#define UPDATE_TIME_MIN_MS (10000)

namespace {

class FolderInfo
{
public:
	FolderInfo();

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
	void CalculateSize();

	RawString m_Path;
	bool m_IncludeSubFolders;
	bool m_IncludeHiddenFiles;
	bool m_IncludeSystemFiles;
	UINT64 m_Size;
	UINT m_FileCount;
	UINT m_FolderCount;
	Pcre m_RegExpFilter;
	ULONGLONG m_LastUpdateTime;
};

FolderInfo::FolderInfo() :
	m_IncludeSubFolders(false),
	m_IncludeHiddenFiles(false),
	m_IncludeSystemFiles(false),
	m_Size(0),
	m_FileCount(0),
	m_FolderCount(0),
	m_LastUpdateTime(0)
{
}

void FolderInfo::Clear()
{
	m_Size = 0;
	m_FileCount = 0;
	m_FolderCount = 0;
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
				if (m_RegExpFilter.Execute(findData.cFileName, 0, nullptr, 0) != 0)
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
		m_LastUpdateTime = 0;
	}
}

void FolderInfo::SetRegExpFilter(const WCHAR* filter)
{
	m_RegExpFilter.Reset();
	if (*filter)
	{
		const char* error = nullptr;
		m_RegExpFilter.Compile(filter, &error);
	}
}

}  // namespace

struct FolderInfoParentMeasure
{
	FolderInfoParentMeasure(MeasureFolderInfo* measure) :
		folder(),
		owner(measure),
		measureCount(1)
	{
	}

	FolderInfo folder;
	MeasureFolderInfo* owner;
	UINT measureCount;
};

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
		if (--m_Parent->measureCount == 0)
		{
			delete m_Parent;
			m_Parent = nullptr;
		}
	}
}

void MeasureFolderInfo::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	static constexpr ConfigParser::EnumOption<Type> s_InfoTypes[] =
	{
		{ L"FolderSize", Type::FolderSize },
		{ L"FolderSizeStr", Type::FolderSize },
		{ L"FolderCount", Type::FolderCount },
		{ L"FolderCountStr", Type::FolderCount },
		{ L"FileCount", Type::FileCount },
		{ L"FileCountStr", Type::FileCount },
	};
	m_Type = parser.ReadEnum(section, L"InfoType", Type::FileCount, s_InfoTypes);

	const std::wstring_view folder = parser.ReadString(section, L"Folder", L"", { .sectionVariables = false });
	if (folder.starts_with(L'['))
	{
		if (m_Parent)
		{
			return;
		}

		// Folder starts with [ so use the ParentMeasure of the referenced section
		if (m_Skin && folder.length() >= 3 && folder.ends_with(L']'))
		{
			const std::wstring_view name = folder.substr(1, folder.length() - 2);
			Measure* measure = m_Skin->GetMeasure(name);
			if (measure && measure->GetTypeID() == TypeID<MeasureFolderInfo>())
			{
				auto* referenced = (MeasureFolderInfo*)measure;
				if (referenced->m_Parent && referenced->m_Parent->owner == referenced)
				{
					m_Parent = referenced->m_Parent;
					++m_Parent->measureCount;
					return;
				}
			}
		}

		LogWarningF(this, L"Invalid Folder=%.*s", (int)folder.length(), folder.data());
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
	}

	std::wstring path = parser.ReadString(section, L"Folder", L"");
	GetSkin()->MakePathAbsolute(path);
	m_Parent->folder.SetPath(path.c_str());

	const WCHAR* filter = parser.ReadString(section, L"RegExpFilter", L"").c_str();
	m_Parent->folder.SetRegExpFilter(filter);

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
