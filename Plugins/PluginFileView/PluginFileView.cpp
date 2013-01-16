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

#include "PluginFileView.h"

#define MAX_LINE_LENGTH 4096
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

unsigned __stdcall SystemThreadProc(void* pParam);
void GetSubFolderSize(const std::vector<FileInfo> files, const std::wstring path, int& folderCount, int& fileCount, UINT64& folderSize);
void GetIcon(std::wstring fileName, std::wstring iconPath, IconSize iconSize);
HRESULT SaveIcon(HICON icon, FILE*fp);

std::vector<ParentMeasure*> g_ParentMeasures;
static CRITICAL_SECTION g_CriticalSection;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&g_CriticalSection);

		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls.
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		DeleteCriticalSection(&g_CriticalSection);
		break;
	}

	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	ChildMeasure* child = new ChildMeasure;
	*data = child;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	ChildMeasure* child = (ChildMeasure*)data;

	void* skin = RmGetSkin(rm);

	std::wstring path = RmReadString(rm, L"Path", L"", FALSE);
	if (path[0] == L'[' && path[path.size() - 1] == L']')
	{
		path = path.substr(1, path.size() - 2);
		
		for (auto iter = g_ParentMeasures.begin(); iter != g_ParentMeasures.end(); ++iter)
		{
			if (_wcsicmp((*iter)->name, path.c_str()) == 0 &&
				(*iter)->skin == skin)
			{
				child->parent = (*iter);
				break;
			}
		}

		if (!child->parent)
		{
			RmLog(LOG_ERROR, L"FileView.dll: Invalid Path");
		}
	}
	else
	{
		if (!child->parent)
		{
			child->parent = new ParentMeasure;
			child->parent->skin = skin;
			child->parent->name = RmGetMeasureName(rm);
			child->parent->ownerChild = child;
			g_ParentMeasures.push_back(child->parent);
		}

		// Add trailing "\" if none exists
		if (!path.empty() && path[path.size() - 1] != L'\\')
		{
			path += L'\\';
		}

		child->parent->path = path;

		LPCWSTR sort = RmReadString(rm, L"SortType", L"Name");
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

			LPCWSTR date = RmReadString(rm, L"SortDateType", L"Modified");
			if (_wcsicmp(date, L"MODIFIED") == 0)
			{
				child->parent->sortDateType = DTYPE_MODIFIED;
			}
			else if (_wcsicmp(date, L"CREATED") == 0)
			{
				child->parent->sortDateType = DTYPE_CREATED;
			}
			else if(_wcsicmp(date, L"ACCESSED") == 0)
			{
				child->parent->sortDateType = DTYPE_ACCESSED;
			}
		}

		int count = RmReadInt(rm, L"Count", 1);
		child->parent->count = count > 0 ? count : 1;

		child->parent->isRecursive = 0!=RmReadInt(rm, L"Recursive", 0);
		child->parent->sortAscending = 0!=RmReadInt(rm, L"SortAscending", 1);
		child->parent->showDotDot = 0!=RmReadInt(rm, L"ShowDotDot", 1);
		child->parent->showFile = 0!=RmReadInt(rm, L"ShowFile", 1);
		child->parent->showFolder = 0!=RmReadInt(rm, L"ShowFolder", 1);
		child->parent->showHidden = 0!=RmReadInt(rm, L"ShowHidden", 1);
		child->parent->showSystem = 0!=RmReadInt(rm, L"ShowSystem", 0);
		child->parent->hideExtension = 0!=RmReadInt(rm, L"HideExtensions", 0);		
		child->parent->extensions = Tokenize(RmReadString(rm, L"Extensions", L""), L";");

		child->parent->wildcardSearch = RmReadString(rm, L"WildcardSearch", L"*");

		child->parent->finishAction = RmReadString(rm, L"FinishAction", L"", false);
	}

	auto iter = std::find(child->parent->children.begin(), child->parent->children.end(), child);
	if (iter == child->parent->children.end())
	{
		child->parent->children.push_back(child);
	}

	int index = RmReadInt(rm, L"Index", 1) - 1;
	child->index = index >= 0 ? index : 1;

	child->ignoreCount = 0!=RmReadInt(rm, L"IgnoreCount", 0);

	LPCWSTR type = RmReadString(rm, L"Type", L"FOLDERPATH");
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

		LPCWSTR date = RmReadString(rm, L"DateType", L"Modified");
		if (_wcsicmp(date, L"MODIFIED") == 0)
		{
			child->date = DTYPE_MODIFIED;
		}
		else if (_wcsicmp(date, L"CREATED") == 0)
		{
			child->date = DTYPE_CREATED;
		}
		else if(_wcsicmp(date, L"ACCESSED") == 0)
		{
			child->date = DTYPE_ACCESSED;
		}
	}
	else if (_wcsicmp(type, L"ICON") == 0)
	{
		child->type = TYPE_ICON;

		std::wstring temp = L"icon";
		WCHAR buffer[MAX_PATH];
		_itow_s(child->index, buffer, 10);
		temp += buffer;
		temp += L".ico";		
		child->iconPath = RmReadPath(rm, L"IconPath", temp.c_str());

		LPCWSTR size = RmReadString(rm, L"IconSize", L"MEDIUM");
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
}

PLUGIN_EXPORT double Update(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (!parent)
	{
		return 0.0;
	}

	EnterCriticalSection(&g_CriticalSection);
	if (parent->ownerChild == child && (parent->needsUpdating || parent->needsIcons))
	{
		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, SystemThreadProc, child, 0, &id);
		if (thread)
		{
			parent->thread = thread;
		}
	}
	
	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
	child->value = 0;

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < parent->files.size())
	{
		switch (child->type)
		{
		case TYPE_FILESIZE:
			child->value = parent->files[trueIndex].size > 0 ? (double)parent->files[trueIndex].size : 0;
			break;

		case TYPE_FILEDATE:
			{
				FILETIME fTime;
				SYSTEMTIME stUTC, stLOCAL;
				ULARGE_INTEGER time;

				switch (child->date)
				{
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
				SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLOCAL);
				SystemTimeToFileTime(&stLOCAL, &fTime);

				time.LowPart = fTime.dwLowDateTime;
				time.HighPart = fTime.dwHighDateTime;

				child->value = (double)(time.QuadPart / 10000000);
			}
			break;
		}
	}
	
	switch (child->type)
	{
	case TYPE_FILECOUNT:
		child->value = (double)parent->fileCount;
		break;

	case TYPE_FOLDERCOUNT:
		child->value = (double)parent->folderCount;
		break;

	case TYPE_FOLDERSIZE:
		child->value = (double)parent->folderSize;
		break;
	}
	LeaveCriticalSection(&g_CriticalSection);

	return child->value;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;
	
	EnterCriticalSection(&g_CriticalSection);
	if (!parent)
	{
		LeaveCriticalSection(&g_CriticalSection);
		return L"";
	}

	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
	child->strValue = L"";

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < parent->files.size())
	{
		switch (child->type)
		{
		case TYPE_FILESIZE:
			if (!parent->files[trueIndex].isFolder)
			{
				LeaveCriticalSection(&g_CriticalSection);
				return NULL;	// Force a numeric return (see the Update function)
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
				WCHAR* temp = new WCHAR[MAX_LINE_LENGTH];
				SYSTEMTIME stUTC, stLOCAL;
				FILETIME fTime;

				switch (child->date)
				{
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

				if (fTime.dwLowDateTime != 0  && fTime.dwHighDateTime != 0)
				{
					FileTimeToSystemTime(&fTime, &stUTC);
					SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLOCAL);
					GetDateFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, NULL, temp, MAX_LINE_LENGTH);
					child->strValue = temp;
					child->strValue += L" ";
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, NULL, temp, MAX_LINE_LENGTH);
					child->strValue += temp;
				}
				else
				{
					child->strValue = L"";
				}
				delete [] temp;
			}
			break;

		case TYPE_ICON:
			child->strValue = child->iconPath;
			break;

		case TYPE_FILEPATH:
			child->strValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ? parent->path : parent->path + parent->files[trueIndex].fileName;
			break;
		}
	}

	switch (child->type)
	{
	case TYPE_FILECOUNT:
	case TYPE_FOLDERCOUNT:
	case TYPE_FOLDERSIZE:
		child->strValue = L"";
		LeaveCriticalSection(&g_CriticalSection);
		return NULL;	// Force numeric return (see the Update function)
		break;

	case TYPE_FOLDERPATH:
		child->strValue = parent->path;
		break;
	}

	LeaveCriticalSection(&g_CriticalSection);
	return child->strValue.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	EnterCriticalSection(&g_CriticalSection);
	if (!parent || parent->thread)
	{
		LeaveCriticalSection(&g_CriticalSection);
		return;
	}
	
	if (parent->ownerChild == child)
	{
		if (parent->files.size() > parent->count)
		{
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
				if ((parent->indexOffset + ( 2 * parent->count)) < parent->files.size())
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
			else if (_wcsicmp(args, L"INDEXUP") ==0)
			{
				if ((parent->indexOffset - 1) >= 0)
				{
					--parent->indexOffset;
					parent->needsIcons = true;
				}
				else
				{
					parent->indexOffset = 0;
					parent->needsIcons = true;
				}
			}
			else if (_wcsicmp(args, L"INDEXDOWN") == 0)
			{
				if ((parent->indexOffset + parent->count) < parent->files.size())
				{
					++parent->indexOffset;
					parent->needsIcons = true;
				}
				else
				{
					parent->indexOffset = (int)parent->files.size() - parent->count;
					parent->needsIcons = true;
				}
			}
		}
		
		if (_wcsicmp(args, L"UPDATE") == 0)
		{
			parent->needsIcons = true;
			parent->needsUpdating = true;
		}
		else if (_wcsicmp(args, L"PREVIOUSFOLDER") == 0)
		{
			std::vector<std::wstring> path = Tokenize(parent->path, L"\\");
			if (path.size() < 2)
			{
				parent->path.clear();
			}
			else
			{
				parent->path.clear();
				for (int i = 0; i < path.size() - 1; ++i)
				{
					parent->path += path[i];
					parent->path += L"\\";
				}
			}

			parent->indexOffset = 0;
			parent->needsUpdating = true;
			parent->needsIcons = true;
		}

		LeaveCriticalSection(&g_CriticalSection);
		return;
	}

	int trueIndex = child->ignoreCount ? child->index : ((child->index % parent->count) + parent->indexOffset);
	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < parent->files.size())
	{	
		if (_wcsicmp(args, L"OPEN") == 0)
		{
			std::wstring file = parent->path + parent->files[trueIndex].fileName;
			ShellExecute(NULL, NULL, file.c_str(), NULL, parent->path.c_str(), SW_SHOW);
		}
		else if (_wcsicmp(args, L"FOLLOWPATH") == 0)
		{
			if (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0)
			{
				std::vector<std::wstring> path = Tokenize(parent->path, L"\\");
				if (path.size() < 2)
				{
					parent->path.clear();
				}
				else
				{
					parent->path.clear();
					for (int i = 0; i < path.size() - 1; ++i)
					{
						parent->path += path[i];
						parent->path += L"\\";
					}
				}

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
				std::wstring file = parent->path + parent->files[trueIndex].fileName;
				ShellExecute(NULL, NULL, file.c_str(), NULL, parent->path.c_str(), SW_SHOW);
			}
		}

		LeaveCriticalSection(&g_CriticalSection);
		return;
	}

	LeaveCriticalSection(&g_CriticalSection);
	RmLog(LOG_ERROR, L"FileView.dll: Invalid command");
}

PLUGIN_EXPORT void Finalize(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	EnterCriticalSection(&g_CriticalSection);
	if (parent->thread)
	{
		TerminateThread(parent->thread, 0);
		parent->thread = NULL;
	}

	if (parent && parent->ownerChild == child)
	{
		CloseHandle(parent->thread);
		parent->thread = NULL;

		delete parent;

		auto iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
		g_ParentMeasures.erase(iter);
	}

	delete child;
	LeaveCriticalSection(&g_CriticalSection);
}

unsigned __stdcall SystemThreadProc(void* pParam)
{
	ChildMeasure* child = (ChildMeasure*)pParam;
	ParentMeasure* parent = child->parent;

	EnterCriticalSection(&g_CriticalSection);
	const bool needsUpdating = parent->needsUpdating;
	parent->needsUpdating = false;						// Set to false here in case skin is reloaded
	const bool needsIcons = parent->needsIcons;
	parent->needsIcons = false;							// Set to false here in case skin is reloaded

	const std::wstring parentPath = parent->path;
	const std::wstring wildcard = parent->wildcardSearch;
	const std::vector<std::wstring> extensions = parent->extensions;
	const bool isRecursive = parent->isRecursive;
	const bool showDotDot = parent->showDotDot;
	const bool showFile = parent->showFile;
	const bool showFolder = parent->showFolder;
	const bool showHidden = parent->showHidden;
	const bool showSystem = parent->showSystem;
	const SortType sortType = parent->sortType;
	const DateType sortDateType = parent->sortDateType;
	const bool sortAscending = parent->sortAscending;
	const std::vector<ChildMeasure*> children = parent->children;

	std::vector<FileInfo> files = parent->files;
	int fileCount = parent->fileCount;
	int folderCount = parent->folderCount;
	UINT64 folderSize = parent->folderSize;
	LeaveCriticalSection(&g_CriticalSection);

	std::wstring path = parentPath + wildcard;
	
	FileInfo file;

	if (needsUpdating)
	{
		fileCount = 0;
		folderCount = 0;
		folderSize = 0;

		EnterCriticalSection(&g_CriticalSection);
		parent->files.clear();
		files.clear();
		LeaveCriticalSection(&g_CriticalSection);

		// If no path is specified, get all the drives instead
		if (parentPath == L"" || parentPath.empty())
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

					files.push_back(file);
				}
			}
		}
		else
		{
			if (showDotDot)
			{
				file.fileName = L"..";
				file.isFolder = true;

				files.push_back(file);
			}

			WIN32_FIND_DATA fd;
			HANDLE find = FindFirstFileEx(path.c_str(),	FindExInfoStandard,	&fd, FindExSearchNameMatch,	NULL, 0);

			if (find != INVALID_HANDLE_VALUE)
			{
				do
				{
					file.Clear();

					file.fileName = fd.cFileName;
					if (_wcsicmp(file.fileName.c_str(), L".") == 0 || _wcsicmp(file.fileName.c_str(), L"..") == 0)
					{
						continue;
					}

					file.isFolder = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
					bool isHidden = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) > 0;
					bool isSystem = (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) > 0;

					if ((!showFile && !file.isFolder) || (!showFolder && file.isFolder) ||
						(!showHidden && isHidden) || (!showSystem && isSystem))
					{
						continue;
					}

					if (!file.isFolder)
					{
						size_t pos = file.fileName.find_last_of(L".");
						if (pos != file.fileName.npos)
						{
							file.ext = file.fileName.substr(pos + 1);

							if (extensions.size() > 0)
							{
								bool found = false;
								for (auto iter = extensions.begin(); iter != extensions.end(); ++iter)
								{
									if (_wcsicmp((*iter).c_str(), file.ext.c_str()) == 0)
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
						else if (extensions.size() > 0)
						{
							continue;
						}
					}

					if (file.isFolder)
					{
						++folderCount;
						file.size = 0;
					}
					else
					{
						++fileCount;
						file.size = ((UINT64)fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
						file.createdTime = fd.ftCreationTime;
						file.modifiedTime = fd.ftLastWriteTime;
						file.accessedTime = fd.ftLastAccessTime;
					}
					
					file.sortAscending = sortAscending;		// Used in sort functions
					folderSize += file.size;

					files.push_back(file);
				}
				while (FindNextFile(find, &fd));
				FindClose(find);

				// Sort
				auto begin = showDotDot ? files.begin() + 1: files.begin();
				switch (sortType)
				{
				case STYPE_NAME:
					std::sort(begin, files.end(), SortByName);
					break;

				case STYPE_SIZE:
					std::sort(begin, files.end(), SortBySize);
					break;

				case STYPE_TYPE:
					std::sort(begin, files.end(), SortByExtension);
					break;

				case STYPE_DATE:
					switch (sortDateType)
					{
					case DTYPE_MODIFIED:
						std::sort(begin, files.end(), SortByModifiedTime);
						break;

					case DTYPE_CREATED:
						std::sort(begin, files.end(), SortByCreatedTime);
						break;

					case DTYPE_ACCESSED:
						std::sort(begin, files.end(), SortByAccessedTime);
						break;
					}
					break;
				}

				if (isRecursive)
				{
					GetSubFolderSize(files, parentPath, folderCount, fileCount, folderSize);
				}
			}
		}
	}

	if (needsIcons)
	{
		for (auto iter = children.begin(); iter != children.end(); ++iter)
		{
			EnterCriticalSection(&g_CriticalSection);
			int trueIndex = (*iter)->ignoreCount ? (*iter)->index : (((*iter)->index % (*iter)->parent->count) + (*iter)->parent->indexOffset);
			
			if ((*iter)->type == TYPE_ICON && trueIndex >= 0 && trueIndex < files.size())
			{
				std::wstring filePath = parentPath;
				filePath += (files[trueIndex].fileName == L"..") ? L"" : files[trueIndex].fileName;
				GetIcon(filePath, (*iter)->iconPath, (*iter)->iconSize);
			}
			else if ((*iter)->type == TYPE_ICON)
			{
				GetIcon(INVALID_FILE, (*iter)->iconPath, (*iter)->iconSize);
			}
			LeaveCriticalSection(&g_CriticalSection);
		}
	}

	EnterCriticalSection(&g_CriticalSection);
	parent->files = files;
	parent->fileCount = fileCount;
	parent->folderCount = folderCount;
	parent->folderSize = folderSize;

	CloseHandle(parent->thread);
	parent->thread = NULL;
	LeaveCriticalSection(&g_CriticalSection);

	if (!parent->finishAction.empty())
	{
		RmExecute(parent->skin, parent->finishAction.c_str());
	}

	_endthreadex(0);
	return 0;
}

void GetSubFolderSize(const std::vector<FileInfo> files, const std::wstring path, int& folderCount, int& fileCount, UINT64& folderSize)
{
	std::list<std::wstring> folderQueue;
	std::wstring folder;

	// Get current folders first (if any)
	for (auto iter = files.begin(); iter != files.end(); ++iter)
	{
		if ((*iter).isFolder && _wcsicmp((*iter).fileName.c_str(), L"..") != 0)
		{
			folderQueue.push_back(path + (*iter).fileName);
		}
	}

	while (!folderQueue.empty())
	{
		std::list<std::wstring>::reference ref = folderQueue.front();
		folder = ref + L"\\*";

		WIN32_FIND_DATA fd;
		HANDLE find = FindFirstFileEx(folder.c_str(), FindExInfoStandard, &fd, FindExSearchNameMatch, NULL,	0);
		
		if (find != INVALID_HANDLE_VALUE)
		{
			do 
			{
				folder = fd.cFileName;

				if (_wcsicmp(folder.c_str(), L".") == 0 || _wcsicmp(folder.c_str(), L"..") == 0)
				{
					continue;
				}

				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)
				{
					++folderCount;
					folderQueue.push_back(ref + L"\\" + folder);
				}
				else
				{
					++fileCount;
					folderSize += ((UINT64)fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
				}
			}
			while (FindNextFile(find, &fd));
							
			FindClose(find);
		}
						
		folderQueue.pop_front();
	}
}

void GetIcon(std::wstring filePath, std::wstring iconPath, IconSize iconSize)
{
	SHFILEINFO shFileInfo;
	HICON icon = NULL;
	HIMAGELIST* hImageList = NULL;
	FILE* fp = NULL;

	// Special case for .url files
	if (filePath.size() > 3 && _wcsicmp(filePath.substr(filePath.size() - 4).c_str(), L".URL") == 0)
	{
		WCHAR buffer[MAX_PATH] = L"";
		GetPrivateProfileString(L"InternetShortcut", L"IconFile", L"", buffer, sizeof(buffer), filePath.c_str());
		if (*buffer)
		{
			std::wstring file = buffer;
			int iconIndex = 0;

			GetPrivateProfileString(L"InternetShortcut", L"IconIndex", L"-1", buffer, sizeof(buffer), filePath.c_str());
			if (buffer != L"-1")
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

			PrivateExtractIcons(file.c_str(), iconIndex, size, size, &icon, NULL, 1, LR_LOADTRANSPARENT);
		}
		else
		{
			std::wstring browser;
			WCHAR buffer[MAX_PATH];
			DWORD size = MAX_PATH;
			HKEY hKey;

			RegOpenKeyEx(HKEY_CLASSES_ROOT, L"http\\shell\\open\\command", 0, KEY_QUERY_VALUE, &hKey);
			RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)buffer, &size);
			RegCloseKey(hKey);

			//Strip quotes
			if (buffer[0] == L'"')
			{
				browser = buffer; browser = browser.substr(1);
				size_t pos = browser.find_first_of(L'"');
				browser = browser.substr(0, pos);
			}

			filePath = browser;
		}
	}
	
	if (icon == NULL)
	{
		SHGetFileInfo(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX);
		SHGetImageList(iconSize, IID_IImageList, (void**) &hImageList);
		((IImageList*)hImageList)->GetIcon(shFileInfo.iIcon, ILD_TRANSPARENT, &icon);
	}

	errno_t error = _wfopen_s(&fp, iconPath.c_str(), L"wb");
	if (filePath == INVALID_FILE || icon == NULL || (error == 0 && !SaveIcon(icon, fp)))
	{
		fwrite(iconPath.c_str(), 1, 1, fp);		// Clears previous icon
		fclose(fp);
	}

	DestroyIcon(icon);
}

HRESULT SaveIcon(HICON hIcon, FILE* fp)
{
	ICONINFO iconInfo;
	BITMAP bmColor;
	BITMAP bmMask;
	if (!fp || NULL == hIcon || !GetIconInfo(hIcon, &iconInfo) ||
		!GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
		!GetObject(iconInfo.hbmMask,  sizeof(bmMask),  &bmMask))
		return false;

	// support only 16/32 bit icon now
	if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32)
		return false;

	HDC dc = GetDC(NULL);
	BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
	BITMAPINFO* bmi = (BITMAPINFO*)bmiBytes;

	// color bits
	memset(bmi, 0, sizeof(BITMAPINFO));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, NULL, bmi, DIB_RGB_COLORS);
	int colorBytesCount = bmi->bmiHeader.biSizeImage;
	BYTE* colorBits = new BYTE[colorBytesCount];
	GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi, DIB_RGB_COLORS);

	// mask bits
	memset(bmi, 0, sizeof(BITMAPINFO));
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, NULL, bmi, DIB_RGB_COLORS);
	int maskBytesCount = bmi->bmiHeader.biSizeImage;
	BYTE* maskBits = new BYTE[maskBytesCount];
	GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi, DIB_RGB_COLORS);

	ReleaseDC(NULL, dc);

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
	ICONDIR dir;
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
	delete[] colorBits;
	delete[] maskBits;

	fclose(fp);

	return true;
}