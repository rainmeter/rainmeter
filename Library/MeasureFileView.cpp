// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureFileView.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/CriticalSection.h"
#include "../Common/StringParser.h"
#include "../Common/StringUtil.h"
#include <queue>

enum MeasureFileView::MeasureType : BYTE
{
	TYPE_FOLDERPATH,
	TYPE_FOLDERSIZE,
	TYPE_FILECOUNT,
	TYPE_FOLDERCOUNT,
	TYPE_FILENAME,
	TYPE_FILETYPE,
	TYPE_FILESIZE,
	TYPE_FILEDATE,
	TYPE_FILEPATH,
	TYPE_PATHTOFILE,
	TYPE_ICON
};

enum MeasureFileView::DateType : BYTE
{
	DTYPE_MODIFIED,
	DTYPE_CREATED,
	DTYPE_ACCESSED
};

enum SortType
{
	STYPE_NAME,
	STYPE_SIZE,
	STYPE_TYPE,
	STYPE_DATE
};

enum RecursiveType
{
	RECURSIVE_NONE,
	RECURSIVE_PARTIAL,
	RECURSIVE_FULL
};

enum class ExtensionsFilter : BYTE
{
	Include,
	Exclude
};

struct FileInfo
{
	std::wstring fileName;
	std::wstring path;
	std::wstring ext;
	bool isFolder = false;
	UINT64 size = 0;
	FILETIME createdTime = {};
	FILETIME modifiedTime = {};
	FILETIME accessedTime = {};
	int iconIndex = -1;
};

struct FileViewParentData
{
	std::wstring path;
	std::wstring wildcardSearch;
	SortType sortType = STYPE_NAME;
	MeasureFileView::DateType sortDateType = MeasureFileView::DTYPE_MODIFIED;
	int count = 0;
	RecursiveType recursiveType = RECURSIVE_NONE;
	bool sortAscending = true;
	bool showDotDot = true;
	bool showFile = true;
	bool showFolder = true;
	bool showHidden = true;
	bool showSystem = false;
	bool hideExtension = false;
	std::vector<std::wstring> extensions;
	ExtensionsFilter extensionsFilter = ExtensionsFilter::Include;
	std::wstring finishAction;

	std::vector<MeasureFileView*> children;
	std::vector<FileInfo> files;
	int fileCount = 0;
	int folderCount = 0;
	UINT64 folderSize = 0;
	bool needsUpdating = true;
	bool needsIcons = true;
	int indexOffset = 0;
	MeasureFileView::UpdateTask* task = nullptr;

	HWND hwnd = nullptr;
	Skin* skin = nullptr;
	MeasureFileView* owner = nullptr;
};

static CriticalSection g_CriticalSection;

static void RemoveChildFromParent(FileViewParentData* parent, MeasureFileView* child)
{
	if (!parent) return;

	auto childIter = std::find(parent->children.begin(), parent->children.end(), child);
	if (childIter != parent->children.end())
	{
		parent->children.erase(childIter);
	}
}

int MeasureFileView::GetTrueIndex(const FileViewParentData* parent) const
{
	return m_IgnoreCount ? m_Index : ((m_Index % parent->count) + parent->indexOffset);
}

void MeasureFileView::SetParent(FileViewParentData* parent)
{
	if (m_Parent != parent)
	{
		RemoveChildFromParent(m_Parent, this);
		m_Parent = parent;
	}

	if (parent)
	{
		auto iter = std::find(parent->children.begin(), parent->children.end(), this);
		if (iter == parent->children.end())
		{
			parent->children.push_back(this);
		}
	}
}

class MeasureFileView::UpdateTask : public AsyncTask
{
public:
	static UpdateTask* Create(MeasureFileView* requestor, FileViewParentData* parent)
	{
		assert(parent);

		auto* task = new UpdateTask(requestor, parent);
		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	UpdateTask(MeasureFileView* requestor, FileViewParentData* parent) : AsyncTask(requestor),
		m_Path(parent->path),
		m_WildcardSearch(parent->wildcardSearch),
		m_SortType(parent->sortType),
		m_SortDateType(parent->sortDateType),
		m_RecursiveType(parent->recursiveType),
		m_SortAscending(parent->sortAscending),
		m_ShowDotDot(parent->showDotDot),
		m_ShowFile(parent->showFile),
		m_ShowFolder(parent->showFolder),
		m_ShowHidden(parent->showHidden),
		m_ShowSystem(parent->showSystem),
		m_Extensions(parent->extensions),
		m_ExtensionsFilter(parent->extensionsFilter),
		m_FinishAction(parent->finishAction),
		m_NeedsUpdating(parent->needsUpdating),
		m_NeedsIcons(parent->needsIcons)
	{
		if (m_NeedsIcons)
		{
			if (!m_NeedsUpdating) m_Files = parent->files;

			for (auto* child : parent->children)
			{
				if (child->m_Type == TYPE_ICON)
				{
					const int trueIndex = child->GetTrueIndex(parent);
					m_IconRequests.push_back(trueIndex);
				}
			}
		}
	}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;
	void GetFolderInfo(std::queue<std::wstring>& folderQueue, std::wstring& folder, RecursiveType rType);

	std::wstring m_Path;
	std::wstring m_WildcardSearch;
	SortType m_SortType = STYPE_NAME;
	DateType m_SortDateType = DTYPE_MODIFIED;
	RecursiveType m_RecursiveType = RECURSIVE_NONE;
	bool m_SortAscending = true;
	bool m_ShowDotDot = true;
	bool m_ShowFile = true;
	bool m_ShowFolder = true;
	bool m_ShowHidden = true;
	bool m_ShowSystem = false;
	std::vector<std::wstring> m_Extensions;
	ExtensionsFilter m_ExtensionsFilter = ExtensionsFilter::Include;
	std::wstring m_FinishAction;
	std::vector<FileInfo> m_Files;
	int m_FileCount = 0;
	int m_FolderCount = 0;
	UINT64 m_FolderSize = 0;
	bool m_NeedsUpdating = false;
	bool m_NeedsIcons = false;

	std::vector<int> m_IconRequests;

	bool m_TaskSuccessful = false;
};

static void GetParentFolder(std::wstring& path)
{
	size_t pos = path.find_last_not_of(L"\\");
	if (pos == std::wstring::npos)
	{
		path.clear();
		return;
	}

	pos = path.find_last_of(L"\\", pos);
	if (pos == std::wstring::npos)
	{
		path.clear();
		return;
	}

	path.erase(pos + 1);
}

static bool ShowContextMenu(HWND hwnd, const std::wstring& path)
{
	// Convert any relative paths
	WCHAR buffer[MAX_PATH] = { 0 };
	if (!_wfullpath(buffer, path.c_str(), MAX_PATH))
		return false;

	POINT pos = { 0 };
	GetCursorPos(&pos);

	// If the mouse is outside of the boundaries of
	// the skin, use the upper-left corner of the skin
	RECT rect = { 0 };
	GetWindowRect(hwnd, &rect);
	if (pos.x < rect.left || pos.x > rect.right ||
		pos.y < rect.top || pos.y > rect.bottom)
	{
		pos.x = rect.left;
		pos.y = rect.top;
	}

	ITEMIDLIST* id = nullptr;
	HRESULT result = SHParseDisplayName(buffer, nullptr, &id, 0, nullptr);
	if (!SUCCEEDED(result) || !id)
		return false;

	Microsoft::WRL::ComPtr<IShellFolder> iFolder = nullptr;
	LPCITEMIDLIST idChild = nullptr;
	result = SHBindToParent(id, IID_IShellFolder, (void**)&iFolder, &idChild);
	if (!SUCCEEDED(result) || !iFolder)
		return false;

	Microsoft::WRL::ComPtr<IContextMenu> iMenu = nullptr;
	result = iFolder->GetUIObjectOf(hwnd, 1, (const ITEMIDLIST**)&idChild, IID_IContextMenu, nullptr, (void**)&iMenu);
	if (!SUCCEEDED(result) || !iFolder)
		return false;

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu) return false;

	if (SUCCEEDED(iMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL)))
	{
		int iCmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD, pos.x, pos.y, hwnd, NULL);
		if (iCmd > 0)
		{
			CMINVOKECOMMANDINFOEX info = { 0 };
			info.cbSize = sizeof(info);
			info.fMask = CMIC_MASK_UNICODE | CMIC_MASK_ASYNCOK;
			info.hwnd = hwnd;
			info.lpVerb = MAKEINTRESOURCEA(iCmd - 1);
			info.lpVerbW = MAKEINTRESOURCEW(iCmd - 1);
			info.nShow = SW_SHOWNORMAL;

			iMenu->InvokeCommand((LPCMINVOKECOMMANDINFO)&info);
		}
	}

	DestroyMenu(hMenu);
	return true;
}

MeasureFileView::MeasureFileView(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(TYPE_FOLDERPATH),
	m_DateType(DTYPE_MODIFIED),
	m_IconSize(SHIL_EXTRALARGE),
	m_Index(1),
	m_IgnoreCount(false),
	m_Parent(nullptr)
{
}

MeasureFileView::~MeasureFileView()
{
	CriticalSectionLock lock(g_CriticalSection);
	FileViewParentData* parent = m_Parent;
	if (parent)
	{
		RemoveChildFromParent(parent, this);
	}

	if (parent && parent->owner == this)
	{
		if (parent->task)
		{
			parent->task->AbortWhenPossible();
			parent->task = nullptr;
		}

		for (auto* child : parent->children)
		{
			child->m_Parent = nullptr;
		}

		delete parent;
		parent = nullptr;
	}
}

void MeasureFileView::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	static constexpr ConfigParser::EnumOption<DateType> s_DateTypes[] =
	{
		{ L"MODIFIED", DTYPE_MODIFIED },
		{ L"CREATED", DTYPE_CREATED },
		{ L"ACCESSED", DTYPE_ACCESSED },
	};

	const std::wstring_view path = parser.ReadString(section, L"Path", L"", { .sectionVariables = false });
	if (path.starts_with(L'[') && path.ends_with(L']'))
	{
		// Path is a reference to another FileView measure, so share its parent data
		Skin* skin = GetSkin();
		if (skin && path.length() >= 3)
		{
			const std::wstring_view name = path.substr(1, path.length() - 2);
			Measure* measure = skin->GetMeasure(name);
			if (measure && measure->GetTypeID() == TypeID<MeasureFileView>())
			{
				auto* referenced = (MeasureFileView*)measure;
				FileViewParentData* parent = referenced->m_Parent;
				if (parent && parent->owner == referenced)
				{
					SetParent(parent);
				}
			}
		}

		if (!m_Parent)
		{
			LogErrorF(this, L"Invalid Path: \"%.*s\"", (int)path.length(), path.data());
			return;
		}
	}
	else
	{
		if (!m_Parent)
		{
			m_Parent = new FileViewParentData;
			m_Parent->skin = GetSkin();
			m_Parent->owner = this;
			m_Parent->hwnd = GetSkin()->GetWindow();
		}

		m_Parent->path = path;

		if (!m_Parent->path.empty() && !m_Parent->path.ends_with(L'\\'))
		{
			m_Parent->path += L'\\';
		}

		static constexpr ConfigParser::EnumOption<SortType> s_SortTypes[] =
		{
			{ L"NAME", STYPE_NAME },
			{ L"SIZE", STYPE_SIZE },
			{ L"TYPE", STYPE_TYPE },
			{ L"DATE", STYPE_DATE },
		};
		m_Parent->sortType = parser.ReadEnum(section, L"SortType", STYPE_NAME, s_SortTypes);

		if (m_Parent->sortType == STYPE_DATE)
		{
			m_Parent->sortDateType = parser.ReadEnum(section, L"SortDateType", DTYPE_MODIFIED, s_DateTypes);
		}

		int count = parser.ReadInt(section, L"Count", 1);
		m_Parent->count = count > 0 ? count : 1;

		int recursive = parser.ReadInt(section, L"Recursive", 0);
		switch (recursive)
		{
		default:
			LogWarningF(m_Parent->owner, L"Invalid Recursive type");

		case 0:
			m_Parent->recursiveType = RECURSIVE_NONE;
			break;

		case 1:
			m_Parent->recursiveType = RECURSIVE_PARTIAL;
			break;

		case 2:
			m_Parent->recursiveType = RECURSIVE_FULL;
			break;
		}

		m_Parent->sortAscending = parser.ReadBool(section, L"SortAscending", true);
		m_Parent->showDotDot = parser.ReadBool(section, L"ShowDotDot", true);
		m_Parent->showFile = parser.ReadBool(section, L"ShowFile", true);
		m_Parent->showFolder = parser.ReadBool(section, L"ShowFolder", true);
		m_Parent->showHidden = parser.ReadBool(section, L"ShowHidden", true);
		m_Parent->showSystem = parser.ReadBool(section, L"ShowSystem", false);
		m_Parent->hideExtension = parser.ReadBool(section, L"HideExtensions", false);
		const std::wstring* extensions = &parser.ReadString(section, L"Extensions", L"");
		if (!parser.GetLastDefaultUsed())
		{
			m_Parent->extensionsFilter = ExtensionsFilter::Include;
		}
		else
		{
			extensions = &parser.ReadString(section, L"ExcludeExtensions", L"");
			m_Parent->extensionsFilter = ExtensionsFilter::Exclude;
		}
		StringParser::Split(*extensions, L';', m_Parent->extensions);

		extensions = nullptr;

		parser.ReadString(m_Parent->wildcardSearch, section, L"WildcardSearch", L"*");

		parser.ReadString(m_Parent->finishAction, section, L"FinishAction", L"", { .sectionVariables = false });
	}

	SetParent(m_Parent);

	int index = parser.ReadInt(section, L"Index", 1) - 1;
	m_Index = index >= 0 ? index : 1;

	m_IgnoreCount = parser.ReadBool(section, L"IgnoreCount", false);

	const MeasureType previousType = m_Type;

	static constexpr ConfigParser::EnumOption<MeasureType> s_Types[] =
	{
		{ L"FOLDERPATH", TYPE_FOLDERPATH },
		{ L"FOLDERSIZE", TYPE_FOLDERSIZE },
		{ L"FILECOUNT", TYPE_FILECOUNT },
		{ L"FOLDERCOUNT", TYPE_FOLDERCOUNT },
		{ L"FILENAME", TYPE_FILENAME },
		{ L"FILETYPE", TYPE_FILETYPE },
		{ L"FILESIZE", TYPE_FILESIZE },
		{ L"FILEDATE", TYPE_FILEDATE },
		{ L"FILEPATH", TYPE_FILEPATH },
		{ L"PATHTOFILE", TYPE_PATHTOFILE },
		{ L"ICON", TYPE_ICON },
	};
	m_Type = parser.ReadEnum(section, L"Type", TYPE_FOLDERPATH, s_Types);

	if (m_Type == TYPE_FILEDATE)
	{
		m_DateType = parser.ReadEnum(section, L"DateType", DTYPE_MODIFIED, s_DateTypes);
	}
	else if (m_Type == TYPE_ICON)
	{
		if (previousType != TYPE_ICON)
		{
			// Remove icons written by versions that predate SystemImage support.
			const std::wstring defaultValue = fmt::format(L"icon{}.ico", m_Index + 1);
			std::wstring iconPath = parser.ReadString(section, L"IconPath", defaultValue.c_str());
			GetSkin()->MakePathAbsolute(iconPath);
			DeleteFile(iconPath.c_str());
		}

		static constexpr ConfigParser::EnumOption<int> s_IconSizes[] =
		{
			{ L"SMALL", SHIL_SMALL },
			{ L"MEDIUM", SHIL_LARGE },
			{ L"LARGE", SHIL_EXTRALARGE },
			{ L"EXTRALARGE", SHIL_JUMBO },
		};
		m_IconSize = parser.ReadEnum(section, L"IconSize", (int)SHIL_LARGE, s_IconSizes);
	}
}

void MeasureFileView::UpdateValue()
{
	FileViewParentData* parent = m_Parent;
	if (!parent)
	{
		m_Value = 0.0;
		return;
	}

	if (!parent->task && parent->owner == this && (parent->needsUpdating || parent->needsIcons))
	{
		parent->task = UpdateTask::Create(this, parent);
		parent->needsUpdating = false;
		parent->needsIcons = false;
	}

	const int trueIndex = GetTrueIndex(parent);
	double value = 0;

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		switch (m_Type)
		{
		case TYPE_FILESIZE:
			value = parent->files[trueIndex].size > 0 ? (double)parent->files[trueIndex].size : 0;
			break;

		case TYPE_FILEDATE:
		{
			FILETIME fTime = { 0 };
			SYSTEMTIME stUTC = { 0 }, stLOCAL = { 0 };
			ULARGE_INTEGER time = { 0 };

			switch (m_DateType)
			{
			default:
			case DTYPE_MODIFIED:
				fTime = parent->files[trueIndex].modifiedTime;
				break;

			case DTYPE_CREATED:
				fTime = parent->files[trueIndex].createdTime;
				break;

			case DTYPE_ACCESSED:
				fTime = parent->files[trueIndex].accessedTime;
				break;
			}

			FileTimeToSystemTime(&fTime, &stUTC);
			SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLOCAL);
			SystemTimeToFileTime(&stLOCAL, &fTime);

			time.LowPart = fTime.dwLowDateTime;
			time.HighPart = fTime.dwHighDateTime;

			value = (double)(time.QuadPart / 10000000);
		}
		break;
		}
	}

	switch (m_Type)
	{
	case TYPE_FILECOUNT:
		value = (double)parent->fileCount;
		break;

	case TYPE_FOLDERCOUNT:
		value = (double)parent->folderCount;
		break;

	case TYPE_FOLDERSIZE:
		value = (double)parent->folderSize;
		break;
	}

	m_Value = value;
}

const WCHAR* MeasureFileView::GetStringValue()
{
	FileViewParentData* parent = m_Parent;
	if (!parent) return CheckSubstitute(L"");

	const int trueIndex = GetTrueIndex(parent);
	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		switch (m_Type)
		{
		case TYPE_FILESIZE:
			m_StrValue.clear();

			// Force a numeric return (see the Update function)
			if (!parent->files[trueIndex].isFolder) return nullptr;
			break;

		case TYPE_FILENAME:
			m_StrValue = parent->files[trueIndex].fileName;
			if (parent->hideExtension && !parent->files[trueIndex].isFolder)
			{
				size_t pos = m_StrValue.find_last_of(L".");
				if (pos != std::wstring::npos)
				{
					m_StrValue.resize(pos);
				}
			}
			break;

		case TYPE_FILETYPE:
			m_StrValue = parent->files[trueIndex].ext;
			break;

		case TYPE_FILEDATE:
		{
			SYSTEMTIME stUTC, stLOCAL;
			FILETIME fTime;

			switch (m_DateType)
			{
			default:
			case DTYPE_MODIFIED:
				fTime = parent->files[trueIndex].modifiedTime;
				break;

			case DTYPE_CREATED:
				fTime = parent->files[trueIndex].createdTime;
				break;

			case DTYPE_ACCESSED:
				fTime = parent->files[trueIndex].accessedTime;
				break;
			}

			if (fTime.dwLowDateTime != 0 && fTime.dwHighDateTime != 0)
			{
				WCHAR temp[512];
				FileTimeToSystemTime(&fTime, &stUTC);
				SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLOCAL);
				GetDateFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, nullptr, temp, _countof(temp));
				m_StrValue = temp;
				m_StrValue += L" ";
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, nullptr, temp, _countof(temp));
				m_StrValue += temp;
			}
			else
			{
				m_StrValue.clear();
			}
		}
		break;

		case TYPE_ICON:
			if (parent->files[trueIndex].iconIndex >= 0)
			{
				m_StrValue = fmt::format(L"SystemImage:{},{}", parent->files[trueIndex].iconIndex, m_IconSize);
			}
			else if (!parent->task)
			{
				m_StrValue.clear();
			}
			break;

		case TYPE_FILEPATH:
			m_StrValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ?
				parent->path :
				parent->files[trueIndex].path + parent->files[trueIndex].fileName;
			break;

		case TYPE_PATHTOFILE:
			m_StrValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ?
				parent->path :
				parent->files[trueIndex].path;
			break;
		}
	}
	else
	{
		m_StrValue.clear();
	}

	switch (m_Type)
	{
	case TYPE_FILECOUNT:
	case TYPE_FOLDERCOUNT:
	case TYPE_FOLDERSIZE:
		return nullptr;	// Force numeric return (see the Update function)
		break;

	case TYPE_FOLDERPATH:
		m_StrValue = parent->path;
		break;
	}

	return CheckSubstitute(m_StrValue.c_str());
}

void MeasureFileView::Command(const std::wstring& command)
{
	LPCWSTR args = command.c_str();

	FileViewParentData* parent = m_Parent;
	if (!parent || parent->task) return;

	auto runFile = [&](std::wstring fileName, std::wstring dir, bool isProperty) -> void
	{
		std::wstring cmd = dir + fileName;

		SHELLEXECUTEINFO si = { sizeof(SHELLEXECUTEINFO) };
		si.nShow = SW_SHOWNORMAL;
		si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_ASYNCOK;
		si.lpDirectory = dir.c_str();
		si.lpFile = cmd.c_str();

		if (isProperty)
		{
			si.fMask |= SEE_MASK_INVOKEIDLIST;

			if (cmd.empty())
			{
				si.lpFile = L"ms-settings:about";
			}
			else
			{
				si.lpVerb = L"properties";
			}
		}

		ShellExecuteEx(&si);
	};

	// Parent only commands
	if (parent->owner == this)
	{
		if ((int)parent->files.size() > parent->count)
		{
			const WCHAR* cmdIndexUp = L"INDEXUP";
			const size_t lenIndexUp = wcslen(cmdIndexUp);
			const WCHAR* cmdIndexDown = L"INDEXDOWN";
			const size_t lenIndexDown = wcslen(cmdIndexDown);

			if (_wcsicmp(args, L"PAGEUP") == 0)
			{
				if ((parent->indexOffset - parent->count) >= 0)
				{
					parent->indexOffset -= parent->count;
					parent->needsIcons = true;
				}
				else
				{
					parent->indexOffset = 0;
					parent->needsIcons = true;
				}
				return;
			}
			else if (_wcsicmp(args, L"PAGEDOWN") == 0)
			{
				if ((parent->indexOffset + (2 * parent->count)) < (int)parent->files.size())
				{
					parent->indexOffset += parent->count;
					parent->needsIcons = true;
				}
				else
				{
					parent->indexOffset = (int)parent->files.size() - parent->count;
					parent->needsIcons = true;
				}
				return;
			}
			else if (_wcsnicmp(args, cmdIndexUp, lenIndexUp) == 0 && (args[lenIndexUp] == L' ' || args[lenIndexUp] == L'\0'))
			{
				const int shift = (args[lenIndexUp] == L'\0') ? 1 : std::max(_wtoi(args + lenIndexUp + 1), 1);
				parent->indexOffset = std::max(parent->indexOffset - shift, 0);
				parent->needsIcons = true;
				return;
			}
			else if (_wcsnicmp(args, cmdIndexDown, lenIndexDown) == 0 && (args[lenIndexDown] == L' ' || args[lenIndexDown] == L'\0'))
			{
				const int shift = (args[lenIndexDown] == L'\0') ? 1 : std::max(_wtoi(args + lenIndexDown + 1), 1);
				const int maxOffset = std::max((int)parent->files.size() - parent->count, 0);
				parent->indexOffset = std::min(parent->indexOffset + shift, maxOffset);
				parent->needsIcons = true;
				return;
			}
		}

		if (_wcsicmp(args, L"UPDATE") == 0)
		{
			parent->indexOffset = 0;
			parent->needsIcons = true;
			parent->needsUpdating = true;
		}
		else if (_wcsicmp(args, L"CONTEXTMENU") == 0)
		{
			if (!ShowContextMenu(parent->hwnd, parent->path))
			{
				LogErrorF(this, L"Cannot open context menu for \"%s\"", parent->path.c_str());
			}
		}
		else if (_wcsicmp(args, L"PROPERTIES") == 0)
		{
			runFile(L"", parent->path, true);
		}
		else if (parent->recursiveType != RECURSIVE_FULL && _wcsicmp(args, L"PREVIOUSFOLDER") == 0)
		{
			GetParentFolder(parent->path);

			parent->indexOffset = 0;
			parent->needsUpdating = true;
			parent->needsIcons = true;
		}
		else
		{
			// Special commands that allow for a user defined file/folder
			std::wstring arg = args;
			std::wstring::size_type pos = arg.find_first_of(L' ');
			if (pos != std::wstring::npos)
			{
				arg = arg.substr(pos);
				if (!arg.empty())
				{
					arg.erase(0, 1);	// Skip the space

					if (_wcsnicmp(args, L"CONTEXTMENU", 11) == 0)
					{
						if (!ShowContextMenu(parent->hwnd, arg))
						{
							LogErrorF(this, L"Cannot open context menu for \"%s\"", arg.c_str());
						}
					}
					else if (_wcsnicmp(args, L"PROPERTIES", 10) == 0)
					{
						runFile(arg, L"", true);
					}
					else
					{
						LogWarningF(this, L"!CommandMeasure: Unknown path: %s", arg.c_str());
					}
				}
				else
				{
					LogWarningF(this, L"!CommandMeasure: Unknown command: %s", args);
				}
			}
		}

		return;
	}

	// Child only commands
	const int trueIndex = GetTrueIndex(parent);
	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		if (_wcsicmp(args, L"OPEN") == 0)
		{
			runFile(parent->files[trueIndex].fileName, parent->files[trueIndex].path, false);
		}
		else if (_wcsicmp(args, L"CONTEXTMENU") == 0)
		{
			std::wstring path = parent->files[trueIndex].path;
			std::wstring fileName = parent->files[trueIndex].fileName;

			if (_wcsicmp(fileName.c_str(), L"..") == 0)
			{
				path = parent->path;
				GetParentFolder(path);
				fileName = L"";
			}

			path.append(fileName);

			if (!ShowContextMenu(parent->hwnd, path))
			{
				LogErrorF(this, L"Cannot open context menu for \"%s\"", path.c_str());
			}
		}
		else if (_wcsicmp(args, L"PROPERTIES") == 0)
		{
			std::wstring path = parent->files[trueIndex].path;
			std::wstring fileName = parent->files[trueIndex].fileName;

			if (_wcsicmp(fileName.c_str(), L"..") == 0)
			{
				path = parent->path;
				GetParentFolder(path);
				fileName = L"";
			}

			runFile(fileName, path, true);
		}
		else if (parent->recursiveType != RECURSIVE_FULL && _wcsicmp(args, L"FOLLOWPATH") == 0)
		{
			if (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0)
			{
				GetParentFolder(parent->path);

				parent->indexOffset = 0;
				parent->needsUpdating = true;
				parent->needsIcons = true;
			}
			else if (parent->files[trueIndex].isFolder)
			{
				parent->path += parent->files[trueIndex].fileName;
				if (parent->path[parent->path.size() - 1] != L'\\')
				{
					parent->path += L'\\';
				}

				parent->indexOffset = 0;
				parent->needsUpdating = true;
				parent->needsIcons = true;
			}
			else
			{
				runFile(parent->files[trueIndex].fileName, parent->files[trueIndex].path, false);
			}
		}
		else
		{
			LogWarningF(this, L"!CommandMeasure: Unknown command: %s", args);
		}

		return;
	}

	LogWarningF(this, L"!CommandMeasure: Unknown command: %s", args);
}

void MeasureFileView::UpdateTask::StartWorkOnWorkerThread()
{
	FileInfo file;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return;

	if (m_NeedsUpdating)
	{
		// If no path is specified, get all the drives instead
		if (m_Path.empty())
		{
			WCHAR drive[4] = L" :\\";
			DWORD driveMask = GetLogicalDrives();
			for (int i = 0; i < 32; ++i)
			{
				if ((driveMask << (31 - i) >> 31) > 0)
				{
					drive[0] = i + 'A';
					file.fileName = drive;
					file.isFolder = true;
					file.size = 0;

					++m_FolderCount;
					m_Files.push_back(file);
				}
			}
		}
		else
		{
			if (m_ShowDotDot && m_RecursiveType != RECURSIVE_FULL)
			{
				file.fileName = L"..";
				file.isFolder = true;

				m_Files.push_back(file);
			}

			std::queue<std::wstring> folderQueue;
			std::wstring folder = m_Path;

			RecursiveType rType = m_RecursiveType;
			GetFolderInfo(folderQueue, folder, (rType == RECURSIVE_PARTIAL) ? RECURSIVE_NONE : rType);

			while (rType != RECURSIVE_NONE && !folderQueue.empty() && !m_AbortRequested)
			{
				folder = folderQueue.front();
				GetFolderInfo(folderQueue, folder, rType);
				folderQueue.pop();
			}
		}

		if (m_AbortRequested)
		{
			CoUninitialize();
			return;
		}

		// Sort
		const int sortAsc = m_SortAscending ? 1 : -1;
		const auto& begin = (!m_Path.empty() &&
			(m_ShowDotDot && m_RecursiveType != RECURSIVE_FULL)) ? m_Files.begin() + 1 : m_Files.begin();

		switch (m_SortType)
		{
		case STYPE_NAME:
			std::sort(begin, m_Files.end(),
				[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
				{
					if (file1.isFolder && file2.isFolder)
					{
						return (sortAsc * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
					}
					else if (!file1.isFolder && !file2.isFolder)
					{
						return (sortAsc * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
					}
					return file1.isFolder;
				});
			break;

		case STYPE_SIZE:
			std::sort(begin, m_Files.end(),
				[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
				{
					if (file1.isFolder && file2.isFolder)
					{
						return (sortAsc * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
					}
					else if (!file1.isFolder && !file2.isFolder)
					{
						return (sortAsc > 0) ? (file1.size < file2.size) : (file1.size > file2.size);
					}
					return file1.isFolder;
				});
			break;

		case STYPE_TYPE:
			std::sort(begin, m_Files.end(),
				[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
				{
					if (file1.isFolder && file2.isFolder)
					{
						return (sortAsc * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
					}
					else if (!file1.isFolder && !file2.isFolder)
					{
						int result = (file1.ext.empty() && file2.ext.empty()) ? 0 : sortAsc * _wcsicmp(file1.ext.c_str(), file2.ext.c_str());
						return (0 != result) ? (result < 0) : (sortAsc * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
					}
					return file1.isFolder;
				});
			break;

		case STYPE_DATE:
			switch (m_SortDateType)
			{
			case DTYPE_MODIFIED:
				std::sort(begin, m_Files.end(),
					[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
					{
						if (file1.isFolder && file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.modifiedTime, &file2.modifiedTime) < 0);
						}
						else if (!file1.isFolder && !file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.modifiedTime, &file2.modifiedTime) < 0);
						}
						return file1.isFolder;
					});
				break;

			case DTYPE_CREATED:
				std::sort(begin, m_Files.end(),
					[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
					{
						if (file1.isFolder && file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.createdTime, &file2.createdTime) < 0);
						}
						else if (!file1.isFolder && !file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.createdTime, &file2.createdTime) < 0);
						}
						return file1.isFolder;
					});
				break;

			case DTYPE_ACCESSED:
				std::sort(begin, m_Files.end(),
					[&sortAsc](const FileInfo& file1, const FileInfo& file2) -> bool
					{
						if (file1.isFolder && file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.accessedTime, &file2.accessedTime) < 0);
						}
						else if (!file1.isFolder && !file2.isFolder)
						{
							return (sortAsc * CompareFileTime(&file1.accessedTime, &file2.accessedTime) < 0);
						}
						return file1.isFolder;
					});
				break;
			}
			break;
		}
	}

	for (const int trueIndex : m_IconRequests)
	{
		if (m_AbortRequested) break;

		if (trueIndex >= 0 && trueIndex < (int)m_Files.size())
		{
			auto& file = m_Files[trueIndex];
			file.iconIndex = -1;
			std::wstring filePath = file.path;
			filePath += (file.fileName == L"..") ? L"" : file.fileName;

			SHFILEINFO fileInfo = { 0 };
			if (SHGetFileInfo(filePath.c_str(), 0, &fileInfo, sizeof(fileInfo), SHGFI_SYSICONINDEX))
			{
				file.iconIndex = fileInfo.iIcon;
			}
		}
	}

	m_TaskSuccessful = true;
	CoUninitialize();
}

void MeasureFileView::UpdateTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested) return;

	auto* measure = (MeasureFileView*)m_Requestor;
	auto* parent = measure->m_Parent;
	if (parent->task == this)
	{
		parent->task = nullptr;

		if (m_TaskSuccessful && (m_NeedsUpdating || m_NeedsIcons))
		{
			parent->files = std::move(m_Files);
		}
		else if (m_NeedsUpdating)
		{
			parent->files.clear();
		}

		if (m_NeedsUpdating)
		{
			parent->fileCount = m_TaskSuccessful ? m_FileCount : 0;
			parent->folderCount = m_TaskSuccessful ? m_FolderCount : 0;
			parent->folderSize = m_TaskSuccessful ? m_FolderSize : 0;
		}

		if (m_TaskSuccessful && !m_FinishAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_FinishAction.c_str(), parent->skin);
		}
	}
}

void MeasureFileView::UpdateTask::GetFolderInfo(std::queue<std::wstring>& folderQueue, std::wstring& folder, RecursiveType rType)
{
	std::wstring path = folder;
	folder += (rType == RECURSIVE_NONE) ? m_WildcardSearch : L"*";

	WIN32_FIND_DATA fd = { 0 };
	HANDLE find = FindFirstFileEx(folder.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			FileInfo file;

			file.fileName = fd.cFileName;
			if (_wcsicmp(file.fileName.c_str(), L".") == 0 || _wcsicmp(file.fileName.c_str(), L"..") == 0)
			{
				continue;
			}

			file.isFolder = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
			bool isHidden = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) > 0;
			bool isSystem = (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) > 0;

			if (rType == RECURSIVE_FULL && m_WildcardSearch != L"*" && !file.isFolder)
			{
				if (!PathMatchSpec(file.fileName.c_str(), m_WildcardSearch.c_str()))
				{
					continue;
				}
			}

			if ((rType != RECURSIVE_PARTIAL) &&
				((rType != RECURSIVE_FULL && !m_ShowFile && !file.isFolder) ||
				(rType != RECURSIVE_FULL && !m_ShowFolder && file.isFolder) ||
				(!m_ShowHidden && isHidden) ||(!m_ShowSystem && isSystem)))
			{
				continue;
			}

			if (rType != RECURSIVE_PARTIAL && !file.isFolder)
			{
				size_t pos = file.fileName.find_last_of(L".");
				bool found = false;
				if (pos != std::wstring::npos)
				{
					file.ext = file.fileName.substr(pos + 1);

					for (const auto& iter : m_Extensions)
					{
						if (_wcsicmp(iter.c_str(), file.ext.c_str()) == 0)
						{
							found = true;
							break;
						}
					}
				}

				if (!m_Extensions.empty() &&
					((m_ExtensionsFilter == ExtensionsFilter::Include && !found) ||
					(m_ExtensionsFilter == ExtensionsFilter::Exclude && found)))
				{
					continue;
				}
			}

			if (file.isFolder)
			{
				if (rType != RECURSIVE_FULL)
				{
					++m_FolderCount;
				}

				folderQueue.push(path + file.fileName + L"\\");
			}
			else
			{
				++m_FileCount;
				file.size = ((UINT64)fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
			}

			m_FolderSize += file.size;

			file.createdTime = fd.ftCreationTime;
			file.modifiedTime = fd.ftLastWriteTime;
			file.accessedTime = fd.ftLastAccessTime;

			file.path = path;

			if (rType == RECURSIVE_NONE || (rType == RECURSIVE_FULL && !file.isFolder))
			{
				m_Files.push_back(file);
			}
		} while (FindNextFile(find, &fd) && !m_AbortRequested);
		FindClose(find);
	}
}
