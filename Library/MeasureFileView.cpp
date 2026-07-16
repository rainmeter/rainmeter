/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureFileView.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/CriticalSection.h"
#include "../Common/ParseUtil.h"
#include "../Common/StringUtil.h"
#include <commoncontrols.h>
#include <memory>
#include <queue>

#define INVALID_FILE L"/<>\\"

#pragma pack(push, 2)
typedef struct	// 16 bytes
{
	BYTE        bWidth;				// Width, in pixels, of the image
	BYTE        bHeight;			// Height, in pixels, of the image
	BYTE        bColorCount;		// Number of colors in image (0 if >=8bpp)
	BYTE        bReserved;			// Reserved ( must be 0)
	WORD        wPlanes;			// Color Planes
	WORD        wBitCount;			// Bits per pixel
	DWORD       dwBytesInRes;		// How many bytes in this resource?
	DWORD       dwImageOffset;		// Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct	// 22 bytes
{
	WORD           idReserved;		// Reserved (must be 0)
	WORD           idType;			// Resource Type (1 for icons)
	WORD           idCount;			// How many images?
	ICONDIRENTRY   idEntries[1];	// An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;
#pragma pack(pop)

enum MeasureType
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

enum DateType
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

enum IconSize
{
	IS_SMALL = 1,	// 16x16
	IS_MEDIUM = 0,	// 32x32
	IS_LARGE = 2,	// 48x48
	IS_EXLARGE = 4	// 256x256
};

enum RecursiveType
{
	RECURSIVE_NONE,
	RECURSIVE_PARTIAL,
	RECURSIVE_FULL
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
};

struct ParentMeasure;

struct ChildMeasure
{
	MeasureType type = TYPE_FOLDERPATH;
	DateType date = DTYPE_MODIFIED;
	IconSize iconSize = IS_LARGE;
	std::wstring iconPath;
	int index = 1;
	bool ignoreCount = false;

	std::wstring strValue;
	ParentMeasure* parent = nullptr;
	MeasureFileView* measure = nullptr;
};

struct ParentMeasure
{
	std::wstring path;
	std::wstring wildcardSearch;
	SortType sortType = STYPE_NAME;
	DateType sortDateType = DTYPE_MODIFIED;
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
	std::wstring finishAction;

	std::vector<ChildMeasure*> children;
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
	LPCWSTR name = nullptr;
	ChildMeasure* ownerChild = nullptr;
};

void GetIcon(std::wstring filePath, const std::wstring& iconPath, IconSize iconSize);
bool SaveIcon(HICON hIcon, FILE* fp);

static std::vector<ParentMeasure*> g_ParentMeasures;
static CriticalSection g_CriticalSection;
static std::wstring g_SysProperties;

static void RemoveChildFromParent(ParentMeasure* parent, ChildMeasure* child)
{
	if (!parent) return;

	auto childIter = std::find(parent->children.begin(), parent->children.end(), child);
	if (childIter != parent->children.end())
	{
		parent->children.erase(childIter);
	}
}

static void SetChildParent(ChildMeasure* child, ParentMeasure* parent)
{
	if (child->parent != parent)
	{
		RemoveChildFromParent(child->parent, child);
		child->parent = parent;
	}

	if (parent)
	{
		auto iter = std::find(parent->children.begin(), parent->children.end(), child);
		if (iter == parent->children.end())
		{
			parent->children.push_back(child);
		}
	}
}

class MeasureFileView::UpdateTask : public AsyncTask
{
public:
	static UpdateTask* Create(MeasureFileView* requestor, ParentMeasure* parent)
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
	UpdateTask(MeasureFileView* requestor, ParentMeasure* parent) : AsyncTask(requestor),
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
		m_FinishAction(parent->finishAction),
		m_NeedsUpdating(parent->needsUpdating),
		m_NeedsIcons(parent->needsIcons)
	{
		if (m_NeedsIcons)
		{
			if (!m_NeedsUpdating) m_Files = parent->files;

			for (auto* child : parent->children)
			{
				if (child->type == TYPE_ICON)
				{
					const int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
					m_IconRequests.emplace_back(child->iconPath, child->iconSize, trueIndex);
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
	std::wstring m_FinishAction;
	std::vector<FileInfo> m_Files;
	int m_FileCount = 0;
	int m_FolderCount = 0;
	UINT64 m_FolderSize = 0;
	bool m_NeedsUpdating = false;
	bool m_NeedsIcons = false;

	struct IconRequest
	{
		std::wstring path;
		IconSize size;
		int trueIndex;
	};

	std::vector<IconRequest> m_IconRequests;

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
	m_Child(new ChildMeasure)
{
	m_Child->measure = this;

	if (g_SysProperties.empty())
	{
		if (IsWindows10OrGreater())
		{
			g_SysProperties = L"ms-settings:about";
		}
		else
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			ExpandEnvironmentStrings(L"%WINDIR%", buffer, _countof(buffer));
			g_SysProperties = buffer;
			g_SysProperties += L"\\system32\\control.exe system";
		}
	}
}

MeasureFileView::~MeasureFileView()
{
	ChildMeasure* child = m_Child;

	CriticalSectionLock lock(g_CriticalSection);
	ParentMeasure* parent = child->parent;
	if (parent)
	{
		RemoveChildFromParent(parent, child);
	}

	if (parent && parent->ownerChild == child)
	{
		if (parent->task)
		{
			parent->task->AbortWhenPossible();
			parent->task = nullptr;
		}

		for (auto iter : parent->children)
		{
			iter->parent = nullptr;
		}

		auto iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
		g_ParentMeasures.erase(iter);

		delete parent;
		parent = nullptr;
	}

	delete child;
	child = nullptr;
}

void MeasureFileView::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	ChildMeasure* child = m_Child;

	std::wstring path = parser.ReadString(section, L"Path", L"", false);
	if (!path.empty() && path[0] == L'[' && path[path.size() - 1] == L']')
	{
		path = path.substr(1, path.size() - 2);

		for (auto iter : g_ParentMeasures)
		{
			if (_wcsicmp(iter->name, path.c_str()) == 0 && iter->skin == GetSkin())
			{
				SetChildParent(child, iter);
				break;
			}
		}

		if (!child->parent)
		{
			LogErrorF(this, L"Invalid Path: \"%s\"", path.c_str());
			return;
		}
	}
	else
	{
		if (!child->parent)
		{
			child->parent = new ParentMeasure;
			child->parent->skin = GetSkin();
			child->parent->name = GetName();
			child->parent->ownerChild = child;
			child->parent->hwnd = GetSkin()->GetWindow();
			g_ParentMeasures.push_back(child->parent);
		}

		// Add trailing "\" if none exists
		if (!path.empty() && path[path.size() - 1] != L'\\')
		{
			path += L'\\';
		}

		child->parent->path = path;

		LPCWSTR sort = parser.ReadString(section, L"SortType", L"Name").c_str();
		if (_wcsicmp(sort, L"NAME") == 0)
		{
			child->parent->sortType = STYPE_NAME;
		}
		else if (_wcsicmp(sort, L"SIZE") == 0)
		{
			child->parent->sortType = STYPE_SIZE;
		}
		else if (_wcsicmp(sort, L"TYPE") == 0)
		{
			child->parent->sortType = STYPE_TYPE;
		}
		else if (_wcsicmp(sort, L"DATE") == 0)
		{
			child->parent->sortType = STYPE_DATE;

			LPCWSTR date = parser.ReadString(section, L"SortDateType", L"Modified").c_str();
			if (_wcsicmp(date, L"MODIFIED") == 0)
			{
				child->parent->sortDateType = DTYPE_MODIFIED;
			}
			else if (_wcsicmp(date, L"CREATED") == 0)
			{
				child->parent->sortDateType = DTYPE_CREATED;
			}
			else if (_wcsicmp(date, L"ACCESSED") == 0)
			{
				child->parent->sortDateType = DTYPE_ACCESSED;
			}
		}

		int count = parser.ReadInt(section, L"Count", 1);
		child->parent->count = count > 0 ? count : 1;

		int recursive = parser.ReadInt(section, L"Recursive", 0);
		switch (recursive)
		{
		default:
			LogWarningF(child->parent->ownerChild->measure, L"Invalid Recursive type");

		case 0:
			child->parent->recursiveType = RECURSIVE_NONE;
			break;

		case 1:
			child->parent->recursiveType = RECURSIVE_PARTIAL;
			break;

		case 2:
			child->parent->recursiveType = RECURSIVE_FULL;
			break;
		}

		child->parent->sortAscending = parser.ReadBool(section, L"SortAscending", true);
		child->parent->showDotDot = parser.ReadBool(section, L"ShowDotDot", true);
		child->parent->showFile = parser.ReadBool(section, L"ShowFile", true);
		child->parent->showFolder = parser.ReadBool(section, L"ShowFolder", true);
		child->parent->showHidden = parser.ReadBool(section, L"ShowHidden", true);
		child->parent->showSystem = parser.ReadBool(section, L"ShowSystem", false);
		child->parent->hideExtension = parser.ReadBool(section, L"HideExtensions", false);
		child->parent->extensions = ParseUtil::Tokenize(parser.ReadString(section, L"Extensions", L""), L";");

		child->parent->wildcardSearch = parser.ReadString(section, L"WildcardSearch", L"*");

		child->parent->finishAction = parser.ReadString(section, L"FinishAction", L"", false);
	}

	SetChildParent(child, child->parent);

	int index = parser.ReadInt(section, L"Index", 1) - 1;
	child->index = index >= 0 ? index : 1;

	child->ignoreCount = parser.ReadBool(section, L"IgnoreCount", false);

	LPCWSTR type = parser.ReadString(section, L"Type", L"FOLDERPATH").c_str();
	if (_wcsicmp(type, L"FOLDERPATH") == 0)
	{
		child->type = TYPE_FOLDERPATH;
	}
	else if (_wcsicmp(type, L"FOLDERSIZE") == 0)
	{
		child->type = TYPE_FOLDERSIZE;
	}
	else if (_wcsicmp(type, L"FILECOUNT") == 0)
	{
		child->type = TYPE_FILECOUNT;
	}
	else if (_wcsicmp(type, L"FOLDERCOUNT") == 0)
	{
		child->type = TYPE_FOLDERCOUNT;
	}
	else if (_wcsicmp(type, L"FILENAME") == 0)
	{
		child->type = TYPE_FILENAME;
	}
	else if (_wcsicmp(type, L"FILETYPE") == 0)
	{
		child->type = TYPE_FILETYPE;
	}
	else if (_wcsicmp(type, L"FILESIZE") == 0)
	{
		child->type = TYPE_FILESIZE;
	}
	else if (_wcsicmp(type, L"FILEDATE") == 0)
	{
		child->type = TYPE_FILEDATE;

		LPCWSTR date = parser.ReadString(section, L"DateType", L"Modified").c_str();
		if (_wcsicmp(date, L"MODIFIED") == 0)
		{
			child->date = DTYPE_MODIFIED;
		}
		else if (_wcsicmp(date, L"CREATED") == 0)
		{
			child->date = DTYPE_CREATED;
		}
		else if (_wcsicmp(date, L"ACCESSED") == 0)
		{
			child->date = DTYPE_ACCESSED;
		}
	}
	else if (_wcsicmp(type, L"ICON") == 0)
	{
		child->type = TYPE_ICON;

		std::wstring temp = L"icon";
		WCHAR buffer[MAX_PATH] = { 0 };
		_itow_s(child->index + 1, buffer, 10);
		temp += buffer;
		temp += L".ico";
		child->iconPath = parser.ReadString(section, L"IconPath", temp.c_str());
		GetSkin()->MakePathAbsolute(child->iconPath);

		LPCWSTR size = parser.ReadString(section, L"IconSize", L"MEDIUM").c_str();
		if (_wcsicmp(size, L"SMALL") == 0)
		{
			child->iconSize = IS_SMALL;
		}
		else if (_wcsicmp(size, L"MEDIUM") == 0)
		{
			child->iconSize = IS_MEDIUM;
		}
		else if (_wcsicmp(size, L"LARGE") == 0)
		{
			child->iconSize = IS_LARGE;
		}
		else if (_wcsicmp(size, L"EXTRALARGE") == 0)
		{
			child->iconSize = IS_EXLARGE;
		}
	}
	else if (_wcsicmp(type, L"FILEPATH") == 0)
	{
		child->type = TYPE_FILEPATH;
	}
	else if (_wcsicmp(type, L"PATHTOFILE") == 0)
	{
		child->type = TYPE_PATHTOFILE;
	}
}

void MeasureFileView::UpdateValue()
{
	ChildMeasure* child = m_Child;
	ParentMeasure* parent = child->parent;
	if (!parent)
	{
		m_Value = 0.0;
		return;
	}

	if (!parent->task && parent->ownerChild == child && (parent->needsUpdating || parent->needsIcons))
	{
		parent->task = UpdateTask::Create(this, parent);
		parent->needsUpdating = false;
		parent->needsIcons = false;
	}

	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
	double value = 0;

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		switch (child->type)
		{
		case TYPE_FILESIZE:
			value = parent->files[trueIndex].size > 0 ? (double)parent->files[trueIndex].size : 0;
			break;

		case TYPE_FILEDATE:
		{
			FILETIME fTime = { 0 };
			SYSTEMTIME stUTC = { 0 }, stLOCAL = { 0 };
			ULARGE_INTEGER time = { 0 };

			switch (child->date)
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

	switch (child->type)
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
	ChildMeasure* child = m_Child;

	ParentMeasure* parent = child->parent;
	if (!parent) return CheckSubstitute(L"");

	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
	child->strValue = L"";

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		switch (child->type)
		{
		case TYPE_FILESIZE:
			if (!parent->files[trueIndex].isFolder)
			{
				return nullptr;	// Force a numeric return (see the Update function)
			}
			break;

		case TYPE_FILENAME:
		{
			std::wstring temp = parent->files[trueIndex].fileName;
			if (parent->hideExtension && !parent->files[trueIndex].isFolder)
			{
				size_t pos = temp.find_last_of(L".");
				if (pos != temp.npos)
				{
					child->strValue = temp.substr(0, pos);
				}
			}
			else
			{
				child->strValue = temp;
			}
		}
		break;

		case TYPE_FILETYPE:
			child->strValue = parent->files[trueIndex].ext;
			break;

		case TYPE_FILEDATE:
		{
			SYSTEMTIME stUTC, stLOCAL;
			FILETIME fTime;

			switch (child->date)
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
				child->strValue = temp;
				child->strValue += L" ";
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, nullptr, temp, _countof(temp));
				child->strValue += temp;
			}
			else
			{
				child->strValue = L"";
			}
		}
		break;

		case TYPE_ICON:
			child->strValue = child->iconPath;
			break;

		case TYPE_FILEPATH:
			child->strValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ?
				parent->path :
				parent->files[trueIndex].path + parent->files[trueIndex].fileName;
			break;

		case TYPE_PATHTOFILE:
			child->strValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ?
				parent->path :
				parent->files[trueIndex].path;
			break;
		}
	}

	switch (child->type)
	{
	case TYPE_FILECOUNT:
	case TYPE_FOLDERCOUNT:
	case TYPE_FOLDERSIZE:
		return nullptr;	// Force numeric return (see the Update function)
		break;

	case TYPE_FOLDERPATH:
		child->strValue = parent->path;
		break;
	}

	return CheckSubstitute(child->strValue.c_str());
}

void MeasureFileView::Command(const std::wstring& command)
{
	ChildMeasure* child = m_Child;
	LPCWSTR args = command.c_str();

	ParentMeasure* parent = child->parent;
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
				si.lpFile = g_SysProperties.c_str();
			}
			else
			{
				si.lpVerb = L"properties";
			}
		}

		ShellExecuteEx(&si);
	};

	// Parent only commands
	if (parent->ownerChild == child)
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
			}
			else if (_wcsnicmp(args, cmdIndexUp, lenIndexUp) == 0 && (args[lenIndexUp] == L' ' || args[lenIndexUp] == L'\0'))
			{
				const int shift = (args[lenIndexUp] == L'\0') ? 1 : max(_wtoi(args + lenIndexUp + 1), 1);
				parent->indexOffset = max(parent->indexOffset - shift, 0);
				parent->needsIcons = true;
			}
			else if (_wcsnicmp(args, cmdIndexDown, lenIndexDown) == 0 && (args[lenIndexDown] == L' ' || args[lenIndexDown] == L'\0'))
			{
				const int shift = (args[lenIndexDown] == L'\0') ? 1 : max(_wtoi(args + lenIndexDown + 1), 1);
				const int maxOffset = max((int)parent->files.size() - parent->count, 0);
				parent->indexOffset = min(parent->indexOffset + shift, maxOffset);
				parent->needsIcons = true;
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
	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
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

	for (const auto& iconRequest : m_IconRequests)
	{
		if (m_AbortRequested) break;

		if (iconRequest.trueIndex >= 0 && iconRequest.trueIndex < (int)m_Files.size())
		{
			const auto& file = m_Files[iconRequest.trueIndex];
			std::wstring filePath = file.path;
			filePath += (file.fileName == L"..") ? L"" : file.fileName;
			GetIcon(filePath, iconRequest.path, iconRequest.size);
		}
		else
		{
			GetIcon(INVALID_FILE, iconRequest.path, iconRequest.size);
		}
	}

	m_TaskSuccessful = true;
	CoUninitialize();
}

void MeasureFileView::UpdateTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested) return;

	auto* measure = (MeasureFileView*)m_Requestor;
	auto* parent = measure->m_Child->parent;
	if (parent->task == this)
	{
		parent->task = nullptr;

		if (m_NeedsUpdating)
		{
			parent->files = m_TaskSuccessful ? std::move(m_Files) : std::vector<FileInfo>();
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
				if (pos != std::wstring::npos)
				{
					file.ext = file.fileName.substr(pos + 1);

					if (m_Extensions.size() > 0)
					{
						bool found = false;
						for (const auto& iter : m_Extensions)
						{
							if (_wcsicmp(iter.c_str(), file.ext.c_str()) == 0)
							{
								found = true;
								break;
							}
						}

						if (!found)
						{
							continue;
						}
					}
				}
				else if (m_Extensions.size() > 0)
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

void GetIcon(std::wstring filePath, const std::wstring& iconPath, IconSize iconSize)
{
	SHFILEINFO shFileInfo = { 0 };
	HICON icon = nullptr;
	HIMAGELIST* hImageList = nullptr;
	FILE* fp = nullptr;

	// Special case for .url files
	if (filePath.size() > 3 && _wcsicmp(filePath.substr(filePath.size() - 4).c_str(), L".URL") == 0)
	{
		WCHAR buffer[MAX_PATH] = { 0 };
		GetPrivateProfileString(L"InternetShortcut", L"IconFile", L"", buffer, _countof(buffer), filePath.c_str());
		if (*buffer)
		{
			std::wstring file = buffer;
			int iconIndex = 0;

			GetPrivateProfileString(L"InternetShortcut", L"IconIndex", L"-1", buffer, _countof(buffer), filePath.c_str());
			if (wcscmp(buffer, L"-1") != 0)
			{
				iconIndex = _wtoi(buffer);
			}

			int size = 16;
			switch (iconSize)
			{
			case IS_EXLARGE: size = 256; break;
			case IS_LARGE: size = 48; break;
			case IS_MEDIUM: size = 32; break;
			}

			PrivateExtractIcons(file.c_str(), iconIndex, size, size, &icon, nullptr, 1, LR_LOADTRANSPARENT);
		}
	}

	if (icon == nullptr)
	{
		SHGetFileInfo(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX);
		SHGetImageList(iconSize, IID_IImageList, (void**)&hImageList);
		((IImageList*)hImageList)->GetIcon(shFileInfo.iIcon, ILD_TRANSPARENT, &icon);
	}

	errno_t error = _wfopen_s(&fp, iconPath.c_str(), L"wb");
	if (filePath == INVALID_FILE || icon == nullptr || (error == 0 && !SaveIcon(icon, fp)))
	{
		if (fp)
		{
			fwrite(iconPath.c_str(), 1, 1, fp);		// Clears previous icon
			fclose(fp);
		}
	}

	DestroyIcon(icon);
}

bool SaveIcon(HICON hIcon, FILE* fp)
{
	ICONINFO iconInfo = { 0 };
	BITMAP bmColor = { 0 };
	BITMAP bmMask = { 0 };
	if (!fp || nullptr == hIcon || !GetIconInfo(hIcon, &iconInfo) ||
		!GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
		!GetObject(iconInfo.hbmMask,  sizeof(bmMask),  &bmMask))
		return false;

	// support only 16/32 bit icon now
	if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32)
		return false;

	HDC dc = GetDC(nullptr);
	BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = { 0 };
	BITMAPINFO* bmi = (BITMAPINFO*)bmiBytes;

	// color bits
	memset(bmi, 0, sizeof(BITMAPINFO));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
	int colorBytesCount = bmi->bmiHeader.biSizeImage;
	BYTE* colorBits = new BYTE[colorBytesCount];
	GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi, DIB_RGB_COLORS);

	// mask bits
	memset(bmi, 0, sizeof(BITMAPINFO));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
	int maskBytesCount = bmi->bmiHeader.biSizeImage;
	BYTE* maskBits = new BYTE[maskBytesCount];
	GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi, DIB_RGB_COLORS);

	ReleaseDC(nullptr, dc);

	// icon data
	BITMAPINFOHEADER bmihIcon;
	memset(&bmihIcon, 0, sizeof(bmihIcon));
	bmihIcon.biSize      = sizeof(BITMAPINFOHEADER);
	bmihIcon.biWidth     = bmColor.bmWidth;
	bmihIcon.biHeight    = bmColor.bmHeight * 2;	// icXOR + icAND
	bmihIcon.biPlanes    = bmColor.bmPlanes;
	bmihIcon.biBitCount  = bmColor.bmBitsPixel;
	bmihIcon.biSizeImage = colorBytesCount + maskBytesCount;

	// icon header
	ICONDIR dir = { 0 };
	dir.idReserved = 0;		// must be 0
	dir.idType = 1;			// 1 for icons
	dir.idCount = 1;
	dir.idEntries[0].bWidth        = (BYTE)bmColor.bmWidth;
	dir.idEntries[0].bHeight       = (BYTE)bmColor.bmHeight;
	dir.idEntries[0].bColorCount   = 0;		// 0 if >= 8bpp
	dir.idEntries[0].bReserved     = 0;		// must be 0
	dir.idEntries[0].wPlanes       = bmColor.bmPlanes;
	dir.idEntries[0].wBitCount     = bmColor.bmBitsPixel;
	dir.idEntries[0].dwBytesInRes  = sizeof(bmihIcon) + bmihIcon.biSizeImage;
	dir.idEntries[0].dwImageOffset = sizeof(ICONDIR);

	fwrite(&dir,      sizeof(dir),      1, fp);
	fwrite(&bmihIcon, sizeof(bmihIcon), 1, fp);
	fwrite(colorBits, colorBytesCount,  1, fp);
	fwrite(maskBits,  maskBytesCount,   1, fp);

	// Clean up
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);
	delete [] colorBits;
	colorBits = nullptr;
	delete [] maskBits;
	maskBits = nullptr;

	fclose(fp);

	return true;
}
