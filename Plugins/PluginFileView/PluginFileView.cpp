/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PluginFileView.h"
#include "../../Common/StringUtil.h"

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
void GetFolderInfo(std::queue<std::wstring>& folderQueue, std::wstring& folder, ParentMeasure* parent, RecursiveType rType);
void GetIcon(std::wstring filePath, const std::wstring& iconPath, IconSize iconSize);
HRESULT SaveIcon(HICON hIcon, FILE* fp);

static std::vector<ParentMeasure*> g_ParentMeasures;
static CRITICAL_SECTION g_CriticalSection;
static std::string g_SysProperties;

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

	if (g_SysProperties.empty())
	{
		std::wstring dir = RmReplaceVariables(rm, L"%WINDIR%");
		dir.append(L"\\system32\\control.exe");
		dir.append(L" system");

		g_SysProperties = StringUtil::Narrow(dir);
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	ChildMeasure* child = (ChildMeasure*)data;

	void* skin = RmGetSkin(rm);

	std::wstring path = RmReadString(rm, L"Path", L"", FALSE);
	if (path[0] == L'[' && path[path.size() - 1] == L']')
	{
		path = path.substr(1, path.size() - 2);
		
		for (auto iter : g_ParentMeasures)
		{
			if (_wcsicmp(iter->name, path.c_str()) == 0 && iter->skin == skin)
			{
				child->parent = iter;
				break;
			}
		}

		if (!child->parent)
		{
			RmLogF(rm, LOG_ERROR, L"Invalid Path: \"%s\"", path.c_str());
			return;
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
			child->parent->hwnd = RmGetSkinWindow(rm);
			child->parent->rm = rm;
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

		int recursive = RmReadInt(rm, L"Recursive", 0);
		switch (recursive)
		{
		default:
			RmLog(child->parent->rm, LOG_WARNING, L"Invalid Recursive type");

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
		_itow_s(child->index + 1, buffer, 10);
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

		auto iter = std::find(child->parent->iconChildren.begin(), child->parent->iconChildren.end(), child);
		if (iter == child->parent->iconChildren.end())
		{
			child->parent->iconChildren.push_back(child);
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

PLUGIN_EXPORT double Update(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (!parent)
	{
		return 0.0;
	}

	EnterCriticalSection(&g_CriticalSection);
	if (!parent->thread && parent->ownerChild == child && (parent->needsUpdating || parent->needsIcons))
	{
		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, SystemThreadProc, parent, 0, &id);
		if (thread)
		{
			parent->thread = thread;
		}
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
	LeaveCriticalSection(&g_CriticalSection);

	return value;
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

	if (!parent->files.empty() && trueIndex >= 0 && trueIndex < (int)parent->files.size())
	{
		switch (child->type)
		{
		case TYPE_FILESIZE:
			if (!parent->files[trueIndex].isFolder)
			{
				LeaveCriticalSection(&g_CriticalSection);
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
					SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLOCAL);
					GetDateFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, nullptr, temp, MAX_LINE_LENGTH);
					child->strValue = temp;
					child->strValue += L" ";
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stLOCAL, nullptr, temp, MAX_LINE_LENGTH);
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
			child->strValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ? parent->path : parent->files[trueIndex].path + parent->files[trueIndex].fileName;
			break;

		case TYPE_PATHTOFILE:
			child->strValue = (_wcsicmp(parent->files[trueIndex].fileName.c_str(), L"..") == 0) ? parent->path : parent->files[trueIndex].path;
			break;
		}
	}

	switch (child->type)
	{
	case TYPE_FILECOUNT:
	case TYPE_FOLDERCOUNT:
	case TYPE_FOLDERSIZE:
		LeaveCriticalSection(&g_CriticalSection);
		return nullptr;	// Force numeric return (see the Update function)
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

	auto runFile = [&](std::wstring fileName, std::wstring dir, bool isProperty) -> void
	{
		// Display computer system properties
		if (isProperty && dir.empty() && fileName.empty())
		{
			WinExec(g_SysProperties.c_str(), SW_SHOWNORMAL);
			return;
		}

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		std::wstring file = dir + fileName;
		SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};

		si.lpVerb = isProperty ? L"properties" : nullptr;
		si.lpFile = file.c_str();
		si.nShow = SW_SHOWNORMAL;
		si.lpDirectory = dir.c_str();
		si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_ASYNCOK;
		
		if (isProperty) si.fMask |= SEE_MASK_INVOKEIDLIST;

		ShellExecuteEx(&si);
		CoUninitialize();
	};

	// Parent only commands
	if (parent->ownerChild == child)
	{
		if ((int)parent->files.size() > parent->count)
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
				if ((parent->indexOffset + parent->count) < (int)parent->files.size())
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
			parent->indexOffset = 0;
			parent->needsIcons = true;
			parent->needsUpdating = true;
		}
		else if (_wcsicmp(args, L"CONTEXTMENU") == 0)
		{
			if (!ShowContextMenu(parent->hwnd, parent->path))
			{
				RmLogF(parent->rm, LOG_ERROR, L"Cannot open context menu for \"%s\"", parent->path.c_str());
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
							RmLogF(parent->rm, LOG_ERROR, L"Cannot open context menu for \"%s\"", arg.c_str());
						}
					}
					else if (_wcsnicmp(args, L"PROPERTIES", 10) == 0)
					{
						runFile(arg, L"", true);
					}
					else
					{
						RmLogF(parent->rm, LOG_WARNING, L"!CommandMeasure: Unknown path: %s", arg.c_str());
					}
				}
				else
				{
					RmLogF(parent->rm, LOG_WARNING, L"!CommandMeasure: Unknown command: %s", args);
				}
			}
		}

		LeaveCriticalSection(&g_CriticalSection);
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
				RmLogF(parent->rm, LOG_ERROR, L"Cannot open context menu for \"%s\"", path.c_str());
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
			RmLogF(parent->rm, LOG_WARNING, L"!CommandMeasure: Unknown command: %s", args);
		}

		LeaveCriticalSection(&g_CriticalSection);
		return;
	}

	LeaveCriticalSection(&g_CriticalSection);
	RmLogF(parent->rm, LOG_WARNING, L"!CommandMeasure: Unknown command: %s", args);
}

PLUGIN_EXPORT void Finalize(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	EnterCriticalSection(&g_CriticalSection);
	if (parent && parent->ownerChild == child)
	{
		if (parent->thread)
		{
			TerminateThread(parent->thread, 0);
			parent->thread = nullptr;
		}

		auto iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
		g_ParentMeasures.erase(iter);

		delete parent;
	}

	delete child;
	LeaveCriticalSection(&g_CriticalSection);
}

unsigned __stdcall SystemThreadProc(void* pParam)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	ParentMeasure* parent = (ParentMeasure*)pParam;

	EnterCriticalSection(&g_CriticalSection);
	ParentMeasure* tmp = new ParentMeasure (*parent);
	parent->needsUpdating = false;						// Set to false here in case skin is reloaded
	parent->needsIcons = false;							// Set to false here in case skin is reloaded
	LeaveCriticalSection(&g_CriticalSection);
	
	FileInfo file;

	if (tmp->needsUpdating)
	{
		EnterCriticalSection(&g_CriticalSection);
		parent->files.clear();
		parent->fileCount = 0;
		parent->folderCount = 0;
		parent->folderSize = 0;
		LeaveCriticalSection(&g_CriticalSection);

		tmp->files.clear();
		tmp->fileCount = 0;
		tmp->folderCount = 0;
		tmp->folderSize = 0;

		// If no path is specified, get all the drives instead
		if (tmp->path.empty())
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

					++tmp->folderCount;
					tmp->files.push_back(file);
				}
			}
		}
		else
		{
			if (tmp->showDotDot && tmp->recursiveType != RECURSIVE_FULL)
			{
				file.fileName = L"..";
				file.isFolder = true;

				tmp->files.push_back(file);
			}

			std::queue<std::wstring> folderQueue;
			std::wstring folder = tmp->path;
			
			RecursiveType rType = tmp->recursiveType;
			GetFolderInfo(folderQueue, folder, tmp, (rType == RECURSIVE_PARTIAL) ? RECURSIVE_NONE : rType);

			while (rType != RECURSIVE_NONE && !folderQueue.empty())
			{
				folder = folderQueue.front();
				GetFolderInfo(folderQueue, folder, tmp, rType);
				folderQueue.pop();
			}
		}

		// Sort
		const int sortAsc = tmp->sortAscending ? 1 : -1;
		auto begin = (!tmp->path.empty() && 
			(tmp->showDotDot && tmp->recursiveType != RECURSIVE_FULL)) ? tmp->files.begin() + 1: tmp->files.begin();

		switch (tmp->sortType)
		{
		case STYPE_NAME:
			std::sort(begin, tmp->files.end(),
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
			std::sort(begin, tmp->files.end(),
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
			std::sort(begin, tmp->files.end(),
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
			switch (tmp->sortDateType)
			{
			case DTYPE_MODIFIED:
				std::sort(begin, tmp->files.end(),
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
				std::sort(begin, tmp->files.end(),
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
				std::sort(begin, tmp->files.end(),
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

		EnterCriticalSection(&g_CriticalSection);
		parent->files = tmp->files;
		parent->files.shrink_to_fit();
		parent->fileCount = tmp->fileCount;
		parent->folderCount = tmp->folderCount;
		parent->folderSize = tmp->folderSize;
		LeaveCriticalSection(&g_CriticalSection);
	}

	if (tmp->needsIcons)
	{
		for (auto iter : tmp->iconChildren)
		{
			EnterCriticalSection(&g_CriticalSection);
			int trueIndex = iter->ignoreCount ? iter->index : ((iter->index % iter->parent->count) + iter->parent->indexOffset);

			if (iter->type == TYPE_ICON && trueIndex >= 0 && trueIndex < (int)tmp->files.size())
			{
				std::wstring filePath = tmp->files[trueIndex].path;
				filePath += (tmp->files[trueIndex].fileName == L"..") ? L"" :tmp->files[trueIndex].fileName;
				GetIcon(filePath, iter->iconPath, iter->iconSize);
			}
			else if (iter->type == TYPE_ICON)
			{
				GetIcon(INVALID_FILE, iter->iconPath, iter->iconSize);
			}
			LeaveCriticalSection(&g_CriticalSection);
		}
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(parent->thread);
	parent->thread = nullptr;
	LeaveCriticalSection(&g_CriticalSection);

	if (!tmp->finishAction.empty())
	{
		RmExecute(tmp->skin, tmp->finishAction.c_str());
	}

	delete tmp;

	CoUninitialize();
	return 0;
}

void GetFolderInfo(std::queue<std::wstring>& folderQueue, std::wstring& folder, ParentMeasure* parent, RecursiveType rType)
{
	std::wstring path = folder;
	folder += (rType == RECURSIVE_NONE) ? parent->wildcardSearch : L"*";

	WIN32_FIND_DATA fd;
	HANDLE find = FindFirstFileEx(folder.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
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

			if (rType == RECURSIVE_FULL && parent->wildcardSearch != L"*" && !file.isFolder)
			{
				if (!PathMatchSpec(file.fileName.c_str(), parent->wildcardSearch.c_str()))
				{
					continue;
				}
			}

			if ((rType != RECURSIVE_PARTIAL) &&
				((rType != RECURSIVE_FULL && !parent->showFile && !file.isFolder) ||
				(rType != RECURSIVE_FULL && !parent->showFolder && file.isFolder) ||
				(!parent->showHidden && isHidden) ||(!parent->showSystem && isSystem)))
			{
				continue;
			}

			if (rType != RECURSIVE_PARTIAL && !file.isFolder)
			{
				size_t pos = file.fileName.find_last_of(L".");
				if (pos != std::wstring::npos)
				{
					file.ext = file.fileName.substr(pos + 1);

					if (parent->extensions.size() > 0)
					{
						bool found = false;
						for (auto iter : parent->extensions)
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
				else if (parent->extensions.size() > 0)
				{
					continue;
				}
			}

			if (file.isFolder)
			{
				if (rType != RECURSIVE_FULL)
				{
					++parent->folderCount;
				}

				folderQueue.push(path + file.fileName + L"\\");
			}
			else
			{
				++parent->fileCount;
				file.size = ((UINT64)fd.nFileSizeHigh << 32) + fd.nFileSizeLow;
			}

			parent->folderSize += file.size;

			file.createdTime = fd.ftCreationTime;
			file.modifiedTime = fd.ftLastWriteTime;
			file.accessedTime = fd.ftLastAccessTime;

			file.path = path;

			if (rType == RECURSIVE_NONE || (rType == RECURSIVE_FULL && !file.isFolder))
			{
				parent->files.push_back(file);
			}
		}
		while (FindNextFile(find, &fd));
		FindClose(find);
	}
}

void GetIcon(std::wstring filePath, const std::wstring& iconPath, IconSize iconSize)
{
	SHFILEINFO shFileInfo;
	HICON icon = nullptr;
	HIMAGELIST* hImageList = nullptr;
	FILE* fp = nullptr;

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

			PrivateExtractIcons(file.c_str(), iconIndex, size, size, &icon, nullptr, 1, LR_LOADTRANSPARENT);
		}
		else
		{
			std::wstring browser;
			WCHAR buffer[MAX_PATH];
			DWORD size = MAX_PATH;
			HKEY hKey;

			RegOpenKeyEx(HKEY_CLASSES_ROOT, L"http\\shell\\open\\command", 0, KEY_QUERY_VALUE, &hKey);
			RegQueryValueEx(hKey, nullptr, nullptr, nullptr, (LPBYTE)buffer, &size);
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

	if (icon == nullptr)
	{
		SHGetFileInfo(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), SHGFI_SYSICONINDEX);
		SHGetImageList(iconSize, IID_IImageList, (void**) &hImageList);
		((IImageList*)hImageList)->GetIcon(shFileInfo.iIcon, ILD_TRANSPARENT, &icon);
	}

	errno_t error = _wfopen_s(&fp, iconPath.c_str(), L"wb");
	if (filePath == INVALID_FILE || icon == nullptr || (error == 0 && !SaveIcon(icon, fp)))
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
	if (!fp || nullptr == hIcon || !GetIconInfo(hIcon, &iconInfo) ||
		!GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
		!GetObject(iconInfo.hbmMask,  sizeof(bmMask),  &bmMask))
		return false;

	// support only 16/32 bit icon now
	if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32)
		return false;

	HDC dc = GetDC(nullptr);
	BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
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
