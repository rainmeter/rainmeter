/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREFILEVIEW_H_
#define RM_LIBRARY_MEASUREFILEVIEW_H_

#include "Measure.h"

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

class MeasureFileView : public Measure
{
public:
	MeasureFileView(Skin* skin, const WCHAR* name);
	virtual ~MeasureFileView();

	MeasureFileView(const MeasureFileView& other) = delete;
	MeasureFileView& operator=(MeasureFileView other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureFileView>(); }
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ChildMeasure* m_Child;
};

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

	HWND hwnd;
	Skin* skin;
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
	MeasureFileView* measure;

	ChildMeasure() :
		type(TYPE_FOLDERPATH),
		date(DTYPE_MODIFIED),
		iconSize(IS_LARGE),
		iconPath(),
		index(1),
		ignoreCount(false),
		strValue(),
		parent(nullptr),
		measure(nullptr) { }
};

#endif
