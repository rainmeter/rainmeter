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
	TYPE_FOLDERPATH,	// Current folder complete path
	TYPE_FOLDERSIZE,	// Current folder size

	TYPE_FILECOUNT,		// Number of files of current folder
	TYPE_FOLDERCOUNT,	// Number of sub-folders under the current folder

	TYPE_FILENAME,		// Name of file
	TYPE_FILETYPE,		// Type of file (ie "Text Document", not .txt)
	TYPE_FILESIZE,		// Size of file
	TYPE_FILEDATE,		// Date of file - Can be "Created Date", "Modified Date" etc.
	TYPE_FILEPATH,		// Full path of the file

	TYPE_ICON			// Icon of file
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

struct FileInfo
{
	std::wstring fileName;
	std::wstring typeName;	// File type description
	std::wstring ext;
	bool isFolder;
	bool sortAscending;	// Used for sorting function (since we cannot pass other values to a sort function)
	UINT64 size;
	FILETIME createdTime;
	FILETIME modifiedTime;
	FILETIME accessedTime;

	FileInfo(): sortAscending(false)
	{
		Clear();
	}

	void Clear()
	{
		fileName = L"";
		typeName = L"";
		ext = L"";
		isFolder = false;
		size = 0;

		createdTime.dwLowDateTime = 0;
		createdTime.dwHighDateTime = 0;
		modifiedTime.dwLowDateTime = 0;
		modifiedTime.dwHighDateTime = 0;
		accessedTime.dwLowDateTime = 0;
		accessedTime.dwHighDateTime = 0;
	}
};

struct ChildMeasure;

struct ParentMeasure
{
	// Options from the .ini
	std::wstring path;
	std::wstring wildcardSearch;
	SortType sortType;
	DateType sortDateType;
	int count;
	bool isRecursive;
	bool sortAscending;
	bool showDotDot;
	bool showFile;
	bool showFolder;
	bool showHidden;
	bool showSystem;
	bool hideExtension;
	std::vector<std::wstring> extensions;	// only show these extensions

	// Internal values
	std::vector<ChildMeasure*> children;
	std::vector<FileInfo> files;
	int fileCount;
	int folderCount;
	UINT64 folderSize;
	bool needsUpdating;
	bool needsIcons;
	int indexOffset;
	HANDLE thread;

	// References and identifying values
	void* skin;
	LPCWSTR name;
	ChildMeasure* ownerChild;

	ParentMeasure() :
		path(L""),
		wildcardSearch(L""),
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
		children(NULL),
		files(NULL),
		skin(NULL),
		name(L""),
		ownerChild(NULL),
		thread(NULL),
		fileCount(0),
		folderCount(0),
		needsUpdating(true),
		needsIcons(true),
		indexOffset(0),
		isRecursive(false) { }
};

struct ChildMeasure
{
	// Options from the .ini
	MeasureType type;
	DateType date;
	IconSize iconSize;
	std::wstring iconPath;
	int index;
	bool ignoreCount;
	bool needsIcon;

	// Internal values
	double value;			// numerical value of the value (if available)
	std::wstring strValue;	// string value of the value

	// References
	ParentMeasure* parent;

	ChildMeasure() :
		type(TYPE_FOLDERPATH),
		date(DTYPE_MODIFIED),
		iconSize(IS_LARGE),
		iconPath(L""),
		index(0),
		ignoreCount(false),
		needsIcon(true),
		value(0.0),
		strValue(),
		parent(NULL) { }
};

std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);	// skip delimiters at beginning.
	std::wstring::size_type pos = str.find_first_of(delimiters, lastPos);	// find first "non-delimiter".

	while (std::wstring::npos != pos || std::wstring::npos != lastPos)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));    	// found a token, add it to the vector.
		lastPos = str.find_first_not_of(delimiters, pos);    	// skip delimiters.  Note the "not_of"
		pos = str.find_first_of(delimiters, lastPos);    	// find next "non-delimiter"
	}

	return tokens;
}

bool SortByName(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		return (sort * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
	}

	return file1.isFolder;
}

bool SortByExtension(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		int result = (file1.ext.empty() && file2.ext.empty()) ? 0 : sort * _wcsicmp(file1.ext.c_str(), file2.ext.c_str());
		return (0 != result) ? (result < 0) : (sort * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
	}
    
	return file1.isFolder;
}

bool SortBySize(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * _wcsicmp(file1.fileName.c_str(), file2.fileName.c_str()) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		return (sort > 0) ? (file1.size < file2.size) : (file1.size > file2.size);
	}
    
	return file1.isFolder;
}

bool SortByAccessedTime(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.accessedTime, &file2.accessedTime) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.accessedTime, &file2.accessedTime) < 0);
	}

	return file1.isFolder;
}

bool SortByCreatedTime(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.createdTime, &file2.createdTime) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.createdTime, &file2.createdTime) < 0);
	}

	return file1.isFolder;
}

bool SortByModifiedTime(const FileInfo& file1, const FileInfo& file2)
{
	int sort = file1.sortAscending ? 1 : -1;

	if (file1.isFolder && file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.modifiedTime, &file2.modifiedTime) < 0);
	}
	else if (!file1.isFolder && !file2.isFolder)
	{
		return (sort * CompareFileTime(&file1.modifiedTime, &file2.modifiedTime) < 0);
	}

	return file1.isFolder;
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