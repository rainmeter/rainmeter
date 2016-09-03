/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include <wrl/client.h>

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
	bool isFolder;
	UINT64 size;
	FILETIME createdTime;
	FILETIME modifiedTime;
	FILETIME accessedTime;

	FileInfo() :
		fileName(L""),
		path(L""),
		ext(L""),
		isFolder(false),
		size(0),
		createdTime(),
		modifiedTime(),
		accessedTime() { }
};

struct ChildMeasure;

struct ParentMeasure
{
	std::wstring path;
	std::wstring wildcardSearch;
	SortType sortType;
	DateType sortDateType;
	int count;
	RecursiveType recursiveType;
	bool sortAscending;
	bool showDotDot;
	bool showFile;
	bool showFolder;
	bool showHidden;
	bool showSystem;
	bool hideExtension;
	std::vector<std::wstring> extensions;
	std::wstring finishAction;

	std::vector<ChildMeasure*> iconChildren;
	std::vector<FileInfo> files;
	int fileCount;
	int folderCount;
	UINT64 folderSize;
	bool needsUpdating;
	bool needsIcons;
	int indexOffset;
	HANDLE thread;

	void* rm;
	HWND hwnd;
	void* skin;
	LPCWSTR name;
	ChildMeasure* ownerChild;

	ParentMeasure() :
		path(),
		wildcardSearch(),
		folderSize(0),
		sortType(STYPE_NAME),
		sortDateType(DTYPE_MODIFIED),
		count(0),
		sortAscending(true),
		showDotDot(true),
		showFile(true),
		showFolder(true),
		showHidden(true),
		showSystem(false),
		hideExtension(false),
		extensions(),
		finishAction(),
		iconChildren(),
		files(),
		skin(nullptr),
		name(),
		ownerChild(nullptr),
		rm(),
		hwnd(),
		thread(nullptr),
		fileCount(0),
		folderCount(0),
		needsUpdating(true),
		needsIcons(true),
		indexOffset(0),
		recursiveType(RECURSIVE_NONE) { }
};

struct ChildMeasure
{
	MeasureType type;
	DateType date;
	IconSize iconSize;
	std::wstring iconPath;
	int index;
	bool ignoreCount;

	std::wstring strValue;
	ParentMeasure* parent;

	ChildMeasure() :
		type(TYPE_FOLDERPATH),
		date(DTYPE_MODIFIED),
		iconSize(IS_LARGE),
		iconPath(),
		index(1),
		ignoreCount(false),
		strValue(),
		parent(nullptr) { }
};

std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);
	std::wstring::size_type pos = str.find_first_of(delimiters, lastPos);

	while (std::wstring::npos != pos || std::wstring::npos != lastPos)
	{
		tokens.emplace_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}

	return tokens;
}

void GetParentFolder(std::wstring& path)
{
	std::vector<std::wstring> tokens = Tokenize(path, L"\\");
	if (tokens.size() < 2)
	{
		path.clear();
	}
	else
	{
		path.clear();
		for (size_t i = 0; i < tokens.size() - 1; ++i)
		{
			path += tokens[i];
			path += L"\\";
		}
	}
}

bool ShowContextMenu(HWND hwnd, std::wstring& path)
{
	POINT pos;
	GetCursorPos(&pos);

	// If the mouse is outside of the boundaries of
	// the skin, use the upper-left corner of the skin
	RECT rect;
	GetWindowRect(hwnd, &rect);
	if (pos.x < rect.left || pos.x > rect.right ||
		pos.y < rect.top || pos.y > rect.bottom)
	{
		pos.x = rect.left;
		pos.y = rect.top;
	}

	ITEMIDLIST* id = nullptr;
	HRESULT result = SHParseDisplayName(path.c_str(), nullptr, &id, 0, nullptr);
	if (!SUCCEEDED(result) || !id)
		return false;

	Microsoft::WRL::ComPtr<IShellFolder> iFolder = nullptr;
	LPCITEMIDLIST idChild = nullptr;
	result = SHBindToParent(id, IID_IShellFolder, (void**)&iFolder, &idChild);
	if (!SUCCEEDED(result) || !iFolder)
		return false;

	Microsoft::WRL::ComPtr<IContextMenu> iMenu = nullptr;
	result = iFolder->GetUIObjectOf(hwnd, 1, (const ITEMIDLIST **)&idChild, IID_IContextMenu, nullptr, (void**)&iMenu);
	if (!SUCCEEDED(result) || !iFolder)
		return false;

	HMENU hMenu = CreatePopupMenu();
	if (!hMenu)
		return false;

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

/*std::wstring UINT64_To_String(UINT64 value)
{
	std::wstring result;
	result.reserve(20); // Max of 20 digits possible
	do
	{
		result += "0123456789"[value % 10];
		value /= 10;
	}
	while(value);
	
	std::reverse(result.begin(), result.end());

	return result;
}*/
