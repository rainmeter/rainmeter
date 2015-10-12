/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <vector>
#include <algorithm>
#include "FolderInfo.h"
#include "../API/RainmeterAPI.h"

enum MeasureType
{
	MEASURE_FILECOUNT,
	MEASURE_FOLDERCOUNT,
	MEASURE_FOLDERSIZE
};

struct MeasureData
{
	LPCWSTR section;
	CFolderInfo* folder;
	MeasureType type;
	bool parent;

	MeasureData(LPCWSTR section) :
		section(section),
		folder(),
		type(MEASURE_FILECOUNT),
		parent(false)
	{
	}
};

std::vector<MeasureData*> g_Measures;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData(RmGetMeasureName(rm));
	*data = measure;
	g_Measures.push_back(measure);

	void* skin = RmGetSkin(rm);

	LPCWSTR str = RmReadString(rm, L"Folder", L"", FALSE);
	if (*str == L'[')
	{
		const size_t len = wcslen(str);
		for (auto iter = g_Measures.cbegin(); iter != g_Measures.cend(); ++iter)
		{
			if ((*iter)->folder &&
				(*iter)->folder->GetSkin() == skin &&
				wcsncmp(&str[1], (*iter)->section, len - 2) == 0)
			{
				measure->folder = (*iter)->folder;
				measure->folder->AddInstance();
				return;
			}
		}
	}

	measure->folder = new CFolderInfo(skin);
	measure->parent = true;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;
	CFolderInfo* folder = measure->folder;

	if (!folder)
	{
		return;
	}

	LPCWSTR str = RmReadString(rm, L"InfoType", L"");
	if (_wcsicmp(str, L"FolderSize") == 0 || _wcsicmp(str, L"FolderSizeStr") == 0)
	{
		measure->type = MEASURE_FOLDERSIZE;
	}
	else if (_wcsicmp(str, L"FolderCount") == 0 || _wcsicmp(str, L"FolderCountStr") == 0)
	{
		measure->type = MEASURE_FOLDERCOUNT;
	}
	else if (_wcsicmp(str, L"FileCount") == 0 || _wcsicmp(str, L"FileCountStr") == 0)
	{
		measure->type = MEASURE_FILECOUNT;
	}

	if (measure->parent)
	{
		str = RmReadPath(rm, L"Folder", L"");
		folder->SetPath(str);

		str = RmReadString(rm, L"RegExpFilter", L"");
		folder->SetRegExpFilter(str);
		
		folder->SetSubFolders(RmReadInt(rm, L"IncludeSubFolders", 0) == 1);
		folder->SetHiddenFiles(RmReadInt(rm, L"IncludeHiddenFiles", 0) == 1);
		folder->SetSystemFiles(RmReadInt(rm, L"IncludeSystemFiles", 0) == 1);
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	if (!measure->folder)
	{
		return 0.0;
	}

	if (measure->parent)
	{
		measure->folder->Update();
	}

	switch (measure->type)
	{
	case MEASURE_FOLDERSIZE:
		return (double)measure->folder->GetSize();

	case MEASURE_FILECOUNT:
		return measure->folder->GetFileCount();

	case MEASURE_FOLDERCOUNT:
		return measure->folder->GetFolderCount();
	}

	return 0.0;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->folder)
	{
		measure->folder->RemoveInstance();
	}

	delete measure;

	std::vector<MeasureData*>::iterator iter = std::find(g_Measures.begin(), g_Measures.end(), measure);
	g_Measures.erase(iter);
}
