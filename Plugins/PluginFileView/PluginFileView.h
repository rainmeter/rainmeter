/*
  Copyright (C) 2012 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"

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