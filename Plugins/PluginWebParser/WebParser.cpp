/*
  Copyright (C) 2005 Kimmo Pekkola

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// Note: To compile this you need the PCRE library (http://www.pcre.org/).
// See: http://www.perldoc.com/perl5.8.0/pod/perlre.html

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <Wininet.h>
#include <shlwapi.h>
#include "pcre-6.4/pcre.h"
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

struct UrlData
{
	std::wstring url;
	std::wstring regExp;
	std::wstring resultString;
	std::wstring errorString;
	std::wstring proxy;
	int codepage;
	int stringIndex;
	int stringIndex2;
	UINT updateRate;
	UINT updateCounter;
	std::wstring section;
	std::wstring finishAction;
	bool download;
	bool forceReload;
	std::wstring downloadFile;
	std::wstring downloadedFile;
	std::wstring iniFile;
	int debug;
	std::wstring debugFileLocation;
	HANDLE threadHandle;
	HANDLE dlThreadHandle;
};

BYTE* DownloadUrl(std::wstring& url, DWORD* dwSize, bool forceReload);
void ShowError(int lineNumber, WCHAR* errorMsg = NULL);
DWORD WINAPI NetworkThreadProc(LPVOID pParam);
DWORD WINAPI NetworkDownloadThreadProc(LPVOID pParam);
void Log(const WCHAR* string);
void ParseData(UrlData* urlData, LPCSTR parseData);

CRITICAL_SECTION g_CriticalSection; 
bool g_Initialized = false;

static std::map<UINT, UrlData*> g_UrlData;
static bool g_Debug = false;
static HINTERNET hRootHandle = NULL;

#define OVECCOUNT 300    // should be a multiple of 3
#define MUTEX_ERROR "Unable to obtain the mutex."

std::string ConvertToUTF8(LPCWSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str) + 1;
		int bufLen = WideCharToMultiByte(CP_UTF8, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			char* tmpSz = new char[bufLen];
			tmpSz[0] = 0;
			WideCharToMultiByte(CP_UTF8, 0, str, strLen, tmpSz, bufLen, NULL, NULL);
			szAscii = tmpSz;
			delete [] tmpSz;
		}
	}
	return szAscii;
}

std::string ConvertToUTF8(LPCSTR str, int codepage)
{
	std::string szUTF8;

	if (str && *str)
	{
		int strLen = (int)strlen(str) + 1;
		int bufLen = MultiByteToWideChar(codepage, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			WCHAR* wideSz = new WCHAR[bufLen];
			wideSz[0] = 0;
			MultiByteToWideChar(codepage, 0, str, strLen, wideSz, bufLen);
			szUTF8 = ConvertToUTF8(wideSz);
			delete [] wideSz;
		}
	}
	return szUTF8;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str) + 1;
		int bufLen = MultiByteToWideChar(CP_UTF8, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			WCHAR* wideSz = new WCHAR[bufLen];
			wideSz[0] = 0;
			MultiByteToWideChar(CP_UTF8, 0, str, strLen, wideSz, bufLen);
			szWide = wideSz;
			delete [] wideSz;
		}
	}
	return szWide;
}

HWND FindMeterWindow()
{
	HWND wnd = FindWindow(L"RainmeterMeterWindow", NULL);
	if (wnd == NULL)
	{
		// Check if all windows are "On Desktop"
		HWND ProgmanHwnd = FindWindow(L"Progman", L"Program Manager");
		if (ProgmanHwnd)
		{
			wnd = FindWindowEx(ProgmanHwnd, NULL, L"RainmeterMeterWindow", NULL);
			if (wnd == NULL)
			{
				ProgmanHwnd = FindWindowEx(FindWindowEx(ProgmanHwnd, NULL, L"SHELLDLL_DefView", L""), NULL, L"SysListView32", L"FolderView");
				if (ProgmanHwnd)
				{
					wnd = FindWindowEx(ProgmanHwnd, NULL, L"RainmeterMeterWindow", NULL);
				}
			}
		}

		if (wnd == NULL)
		{
			HWND WorkerWHwnd = NULL;
			while ((WorkerWHwnd = FindWindowEx(NULL, WorkerWHwnd, L"WorkerW", L"")) != NULL)
			{
				ProgmanHwnd = FindWindowEx(FindWindowEx(WorkerWHwnd, NULL, L"SHELLDLL_DefView", L""), NULL, L"SysListView32", L"FolderView");
				if (ProgmanHwnd)
				{
					wnd = FindWindowEx(ProgmanHwnd, NULL, L"RainmeterMeterWindow", NULL);
					break;
				}
			}
		}
	}
	return wnd;
}

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	LPCTSTR tmpSz; 

	if (!g_Initialized)
	{
		InitializeCriticalSection(&g_CriticalSection);
		g_Initialized = true;
	}

	UrlData* data = new UrlData;
	data->section = section;
	data->updateRate = 1;
	data->updateCounter = 0;
	data->iniFile = iniFile;

	/* Read our own settings from the ini-file */

	data->url = ReadConfigString(section, L"Url", L"");
	data->regExp = ReadConfigString(section, L"RegExp", L"");
	data->finishAction = ReadConfigString(section, L"FinishAction", L"");
	data->errorString = ReadConfigString(section, L"ErrorString", L"");
	data->proxy = ReadConfigString(section, L"ProxyServer", L"");
	data->downloadFile = ReadConfigString(section, L"DownloadFile", L"");
	data->debugFileLocation = ReadConfigString(section, L"Debug2File", L"c:\\WebParserDump.txt");
	
	if(data->debugFileLocation.find(L"\\") == std::wstring::npos)
	{
		std::wstring str = data->iniFile.substr(0,data->iniFile.find_last_of(L"\\")+1); 
		str += data->debugFileLocation;
		Log(str.c_str());
		data->debugFileLocation = str;
	}

	tmpSz = ReadConfigString(section, L"StringIndex", L"0");
	if (tmpSz)
	{
		data->stringIndex = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"StringIndex2", L"0");
	if (tmpSz)
	{
		data->stringIndex2 = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"UpdateRate", L"600");
	if (tmpSz)
	{
		data->updateRate = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"Download", L"0");
	if (tmpSz)
	{
		data->download = 1 == _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"ForceReload", L"0");
	if (tmpSz)
	{
		data->forceReload = 1 == _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"Debug", L"0");
	if (tmpSz)
	{
		data->debug = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"CodePage", L"0");
	if (tmpSz)
	{
		data->codepage = _wtoi(tmpSz);
	}

	if (hRootHandle == NULL)
	{
		if (data->proxy.empty())
		{
			hRootHandle = InternetOpen(L"Rainmeter WebParser plugin",
										INTERNET_OPEN_TYPE_PRECONFIG,
										NULL,
										NULL,
										0);
		}
		else
		{
			hRootHandle = InternetOpen(L"Rainmeter WebParser plugin",
										INTERNET_OPEN_TYPE_PROXY,
										data->proxy.c_str(),
										NULL,
										0);
		}

		if (hRootHandle == NULL)
		{
			ShowError(__LINE__);
			return NULL;
		}
	}

	data->threadHandle = 0;
	data->dlThreadHandle = 0;

	// During initialization there is no threads yet so no need to do any locking
	g_UrlData[id] = data;

	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	double value = 0;
	UrlData* urlData = NULL;

	// Find the data for this instance (the data structure is not changed by anyone so this should be safe)
	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);
	if(urlIter != g_UrlData.end())
	{
		urlData = (*urlIter).second;
	}

	if (urlData) 
	{
		if (urlData->download && urlData->regExp.empty() && urlData->url.find(L'[') == std::wstring::npos)
		{
			// If RegExp is empty download the file that is pointed by the Url
			if (urlData->dlThreadHandle == 0)
			{
				if (urlData->updateCounter == 0)
				{
					// Launch a new thread to fetch the web data
					DWORD id;
					urlData->dlThreadHandle = CreateThread(NULL, 0, NetworkDownloadThreadProc, urlData, 0, &id);
				}

				urlData->updateCounter++;
				if (urlData->updateCounter >= urlData->updateRate)
				{
					urlData->updateCounter = 0;
				}
			}

			// Else download the file pointed by the result string (this is done later)
		}
		else
		{
			// Make sure that the thread is not writing to the result at the same time
			EnterCriticalSection(&g_CriticalSection);
			
			if (!urlData->resultString.empty())
			{
				value = wcstod(urlData->resultString.c_str(), NULL);
			}

			LeaveCriticalSection(&g_CriticalSection);

			if (urlData->url.size() > 0 && urlData->url.find(L'[') == std::wstring::npos)
			{
				// This is not a reference; need to update.
				if (urlData->threadHandle == 0)
				{
					if (urlData->updateCounter == 0)
					{
						// Launch a new thread to fetch the web data
						DWORD id;
						urlData->threadHandle = CreateThread(NULL, 0, NetworkThreadProc, urlData, 0, &id);
					}

					urlData->updateCounter++;
					if (urlData->updateCounter >= urlData->updateRate)
					{
						urlData->updateCounter = 0;
					}
				}
			}
		}
	}

	return value;
}



/*
  Thread that fetches the data from the net and parses the page.
*/
DWORD WINAPI NetworkThreadProc(LPVOID pParam)
{
	UrlData* urlData = (UrlData*)pParam;
	DWORD dwSize = 0;

	BYTE* data = DownloadUrl(urlData->url, &dwSize, urlData->forceReload);

	if (data)
	{
		if (urlData->debug == 2)
		{
			// Dump to a file

			// Convert to a narrow string
			std::string path(urlData->debugFileLocation.begin(), urlData->debugFileLocation.end());

			FILE* file = fopen(path.c_str(), "wb");
			fwrite(data, sizeof(BYTE), dwSize, file);
			fclose(file);
		}

		ParseData(urlData, (LPCSTR)data);

		delete [] data;
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(urlData->threadHandle);
	urlData->threadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

    return 0;   // thread completed successfully
}

void ParseData(UrlData* urlData, LPCSTR parseData)
{
	size_t dwSize = 0;

	// Parse the value from the data
	pcre* re;
	const char* error;
	int erroffset;
	int ovector[OVECCOUNT];
	int rc;
	int flags = PCRE_UTF8;
	
	if (urlData->codepage == 0)
	{
		flags = 0;
	}
	
	// Compile the regular expression in the first argument
	re = pcre_compile(
		ConvertToUTF8(urlData->regExp.c_str()).c_str(),   // the pattern
		flags,					  // default options
		&error,               // for error message
		&erroffset,           // for error offset
		NULL);                // use default character tables
	
	if (re != NULL)
	{
		// Compilation succeeded: match the subject in the second argument
		std::string utf8Data;

		if (urlData->codepage != CP_UTF8 && urlData->codepage != 0)		// 0 = CP_ACP
		{
			// Must convert the data to utf8
			utf8Data = ConvertToUTF8(parseData, urlData->codepage);
			parseData = utf8Data.c_str();
		}

		dwSize = strlen(parseData);
		
		rc = pcre_exec(
			re,                   // the compiled pattern
			NULL,                 // no extra data - we didn't study the pattern
			parseData,			  // the subject string
			dwSize,			      // the length of the subject
			0,                    // start at offset 0 in the subject
			0,					  // default options
			ovector,              // output vector for substring information
			OVECCOUNT);           // number of elements in the output vector
		
		
		if (rc >= 0)
		{
			if (rc == 0)
			{
				// The output vector wasn't big enough
				Log(L"WebParser: Too many substrings!");
			}
			else
			{
				if (urlData->stringIndex < rc)
				{
					if (urlData->debug != 0)
					{
						for (int i = 0; i < rc; i++)
						{
							WCHAR buffer[1024];
							char* substring_start = (char*)(parseData + ovector[2 * i]);
							int substring_length = ovector[2 * i + 1] - ovector[2 * i];
							substring_length = min(substring_length, 256);
							std::string tmpStr(substring_start, substring_length);
							wsprintf(buffer, L"WebParser: (Index %2d) %s", i, ConvertToWide(tmpStr.c_str()).c_str());
							Log(buffer);
						}
					}

					const char* substring_start = parseData + ovector[2 * urlData->stringIndex];
					int substring_length = ovector[2 * urlData->stringIndex + 1] - ovector[2 * urlData->stringIndex];

					EnterCriticalSection(&g_CriticalSection);
					std::string szResult((char*)substring_start, substring_length);
					urlData->resultString = ConvertToWide(szResult.c_str());
					LeaveCriticalSection(&g_CriticalSection);
				}
				else
				{
					Log(L"WebParser: Not enough substrings!");
				}

				// Update the references
				std::map<UINT, UrlData*>::iterator i = g_UrlData.begin();
				std::wstring compareStr = L"[";
				compareStr += urlData->section;
				compareStr += L"]";
				for ( ; i != g_UrlData.end(); i++)
				{
					if ((((*i).second)->url.find(compareStr) != std::wstring::npos) && (urlData->iniFile == ((*i).second)->iniFile))
					{
						if (((*i).second)->stringIndex < rc)
						{
							const char* substring_start = parseData + ovector[2 * ((*i).second)->stringIndex];
							int substring_length = ovector[2 * ((*i).second)->stringIndex + 1] - ovector[2 * ((*i).second)->stringIndex];

							std::string szResult((char*)substring_start, substring_length);

							if (!((*i).second)->regExp.empty()) 
							{
								// Change the index and parse the substring
								int index = (*i).second->stringIndex;
								(*i).second->stringIndex = (*i).second->stringIndex2;
								ParseData((*i).second, szResult.c_str());
								(*i).second->stringIndex = index;
							}
							else
							{
								// Set the result 
								EnterCriticalSection(&g_CriticalSection);
								
								// Substitude the [measure] with szResult
								std::wstring wzResult = ConvertToWide(szResult.c_str());
								std::wstring wzUrl = ((*i).second)->url;

								wzUrl.replace(wzUrl.find(compareStr), compareStr.size(), wzResult);
								((*i).second)->resultString = wzUrl;

								// Start download threads for the references
								if (((*i).second)->download)
								{
									// Start the download thread
									DWORD id;
									((*i).second)->dlThreadHandle = CreateThread(NULL, 0, NetworkDownloadThreadProc, ((*i).second), 0, &id);
								}

								LeaveCriticalSection(&g_CriticalSection);
							}
						}
						else
						{
							Log(L"WebParser: Not enough substrings!");
						}
					}
				}
			}
		}
		else
		{
			// Matching failed: handle error cases
			WCHAR buffer[1024];
			wsprintf(buffer, L"WebParser: Matching error! (%d)\n", rc);
			Log(buffer);

			EnterCriticalSection(&g_CriticalSection);
			urlData->resultString = urlData->errorString;

			// Update the references
			std::map<UINT, UrlData*>::iterator i = g_UrlData.begin();
			std::wstring compareStr = L"[";
			compareStr += urlData->section;
			compareStr += L"]";
			for ( ; i != g_UrlData.end(); i++)
			{
				if ((((*i).second)->url.find(compareStr) != std::wstring::npos) && (urlData->iniFile == ((*i).second)->iniFile))
				{
					((*i).second)->resultString = ((*i).second)->errorString;
				}
			}
			LeaveCriticalSection(&g_CriticalSection);
		}
	}
	else
	{
		// Compilation failed: print the error message and exit
		WCHAR buffer[1024];
		wsprintf(buffer, L"WebParser: PCRE compilation failed at offset %d: %s\n", erroffset, error);
		Log(buffer);
	}

	if (urlData->download)
	{
		// Start the download thread
		DWORD id;
		urlData->dlThreadHandle = CreateThread(NULL, 0, NetworkDownloadThreadProc, urlData, 0, &id);
	}
	else
	{
		if (!urlData->finishAction.empty()) 
		{
			HWND wnd = FindMeterWindow();

			if (wnd != NULL)
			{
				COPYDATASTRUCT copyData;

				copyData.dwData = 1;
				copyData.cbData = (DWORD)(urlData->finishAction.size() + 1) * sizeof(WCHAR);
				copyData.lpData = (void*)urlData->finishAction.c_str();

				// Send the bang to the Rainmeter window
				SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
			}
		}
	}
}

/*
  Thread that downloads the file from the new.
*/
DWORD WINAPI NetworkDownloadThreadProc(LPVOID pParam)
{
	UrlData* urlData = (UrlData*)pParam;

	std::wstring url;

	if (urlData->regExp.empty() && urlData->resultString.empty())
	{
		url = urlData->url;
	}
	else
	{
		EnterCriticalSection(&g_CriticalSection);
		url = urlData->resultString;
		LeaveCriticalSection(&g_CriticalSection);
	
		std::wstring::size_type pos = url.find(L':');
		if (pos == std::wstring::npos && !url.empty())	// No protocol
		{
			// Add the base url to the string
			if (url[0] == L'/')
			{
				// Absolute path
				pos = urlData->url.find(L'/', 7);	// Assume "http://" (=7)
				if (pos != std::wstring::npos)
				{
					std::wstring path(urlData->url.substr(0, pos));
					url = path + url;
				}
			}
			else
			{
				// Relative path

				pos = urlData->url.rfind(L'/');
				if (pos != std::wstring::npos)
				{
					std::wstring path(urlData->url.substr(0, pos + 1));
					url = path + url;
				}
			}
		}
	}

	if (!url.empty())
	{
		bool download = !urlData->downloadFile.empty();

		// Create the filename
		WCHAR buffer[MAX_PATH] = {0};
		std::wstring fullpath, directory;

		if (download)  // download mode
		{
			PathCanonicalize(buffer, urlData->downloadFile.c_str());

			std::wstring path = buffer;
			std::wstring::size_type pos = path.find_first_not_of(L'\\');
			if (pos != std::wstring::npos)
			{
				path = path.substr(pos);
			}

			PathCanonicalize(buffer, urlData->iniFile.substr(0, urlData->iniFile.find_last_of(L'\\') + 1).c_str());  // "#CURRENTPATH#"
			wcscat(buffer, L"DownloadFile\\");  // "#CURRENTPATH#DownloadFile\"
			CreateDirectory(buffer, NULL);	// Make sure that the folder exists

			wcscat(buffer, path.c_str());

			if (buffer[wcslen(buffer)-1] != L'\\')  // path is a file
			{
				fullpath = buffer;
				PathRemoveFileSpec(buffer);
			}
			PathAddBackslash(buffer);
		}
		else  // cache mode
		{
			GetTempPath(MAX_PATH, buffer);
			wcscat(buffer, L"Rainmeter-Cache\\");  // "%TEMP%\Rainmeter-Cache\"
		}
		CreateDirectory(buffer, NULL);	// Make sure that the folder exists
		directory = buffer;

		if (fullpath.empty())
		{
			fullpath = directory;

			std::wstring::size_type pos2 = url.find_first_of(L"?#");
			std::wstring::size_type pos1 = url.find_last_of(L'/', pos2);
			pos1 = (pos1 != std::wstring::npos) ? pos1 + 1 : 0;

			std::wstring name;
			if (pos2 != std::wstring::npos)
			{
				name = url.substr(pos1, pos2 - pos1);
			}
			else
			{
				name = url.substr(pos1);
			}

			if (!name.empty())
			{
				// Replace reserved characters to "_"
				pos1 = 0;
				while ((pos1 = name.find_first_of(L"\\/:*?\"<>|", pos1)) != std::wstring::npos)
				{
					name[pos1] = L'_';
				}
				fullpath += name;
			}
			else
			{
				fullpath += L"index";
			}
		}

		bool ready = true;
		if (download)  // download mode
		{
			std::wstring error;

			if (!PathFileExists(directory.c_str()) || !PathIsDirectory(directory.c_str()))
			{
				ready = false;

				error = L"WebParser: Directory not exists: ";
				error += directory;
				error += L"\n";
				Log(error.c_str());
			}
			else if (PathIsDirectory(fullpath.c_str()))
			{
				ready = false;

				error = L"WebParser: Path is a directory, not a file: ";
				error += fullpath;
				error += L"\n";
				Log(error.c_str());
			}
			else
			{
				DeleteFile(fullpath.c_str());

				if (PathFileExists(fullpath.c_str()))
				{
					ready = false;

					DWORD attr = GetFileAttributes(fullpath.c_str());
					if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
					{
						error = L"WebParser: File is READ-ONLY: ";
					}
					else
					{
						error = L"WebParser: Failed to delete file: ";
					}
					error += fullpath;
					error += L"\n";
					Log(error.c_str());
				}
			}
		}
		else  // cache mode
		{
			if (!urlData->downloadedFile.empty())
			{
				DeleteFile(urlData->downloadedFile.c_str());
			}

			EnterCriticalSection(&g_CriticalSection);

			if (PathFileExists(fullpath.c_str()))
			{
				std::wstring::size_type pos = fullpath.find_last_of(L'.');

				std::wstring path, ext;
				if (pos != std::wstring::npos)
				{
					path = fullpath.substr(0, pos);
					ext = fullpath.substr(pos);
				}
				else
				{
					path = fullpath;
				}

				// Assign a serial number
				size_t i = 1;
				do
				{
					wsprintf(buffer, L"_%i", i++);

					fullpath = path;
					fullpath += buffer;
					if (!ext.empty())
					{
						fullpath += ext;
					}
				} while (PathFileExists(fullpath.c_str()));
			}

			// Create empty file
			HANDLE hFile = CreateFile(fullpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (ready)
		{
			// Delete IE cache before download if "SyncMode5" is not 3 (every visit to the page)
			{
				// Check "Temporary Internet Files" sync mode (SyncMode5)
				// Values:
				//   Every visit to the page                 3
				//   Every time you start Internet Explorer  2
				//   Automatically (default)                 4
				//   Never                                   0
				// http://support.microsoft.com/kb/263070/en

				HKEY hKey;
				LONG ret;
				DWORD mode;

				ret = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_QUERY_VALUE, &hKey);
				if (ret == ERROR_SUCCESS)
				{
					DWORD size = sizeof(mode);
					ret = RegQueryValueEx(hKey, L"SyncMode5", NULL, NULL, (LPBYTE)&mode, &size);
					RegCloseKey(hKey);
				}

				if (ret != ERROR_SUCCESS || mode != 3)
				{
					std::wstring::size_type pos = url.find_first_of(L'#');

					if (pos != std::wstring::npos)
					{
						DeleteUrlCacheEntry(url.substr(0, pos).c_str());
					}
					else
					{
						DeleteUrlCacheEntry(url.c_str());
					}
				}
			}

			// Write some log info
			std::wstring info = L"WebParser: Downloading url ";
			info += url;
			info += L" to ";
			info += fullpath;
			info += L"\n";
			Log(info.c_str());

			HRESULT resultCoInitialize = CoInitialize(NULL);  // requires before calling URLDownloadToFile function

			// Download the file
			HRESULT result = URLDownloadToFile(NULL, url.c_str(), fullpath.c_str(), NULL, NULL);
			if (result == S_OK)
			{
				EnterCriticalSection(&g_CriticalSection);

				// Convert LFN to 8.3 filename if the path contains blank character
				if (fullpath.find_first_of(L' ') != std::wstring::npos)
				{
					DWORD size = GetShortPathName(fullpath.c_str(), buffer, MAX_PATH);
					if (size > 0 && size <= MAX_PATH)
					{
						fullpath = buffer;
					}
				}
				urlData->downloadedFile = fullpath;

				LeaveCriticalSection(&g_CriticalSection);
	
				if (!urlData->finishAction.empty()) 
				{
					HWND wnd = FindMeterWindow();

					if (wnd != NULL)
					{
						COPYDATASTRUCT copyData;

						copyData.dwData = 1;
						copyData.cbData = (DWORD)(urlData->finishAction.size() + 1) * sizeof(WCHAR);
						copyData.lpData = (void*)urlData->finishAction.c_str();

						// Send the bang to the Rainmeter window
						SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
					}
				}
			}
			else
			{
				if (!download)  // cache mode
				{
					// Delete empty file
					DeleteFile(fullpath.c_str());
				}

				wsprintf(buffer, L"result=0x%08X, COM=0x%08X", result, resultCoInitialize);
				std::wstring error = L"WebParser: Download failed (";
				error += buffer;
				error += L"): ";
				error += url;
				Log(error.c_str());
			}

			if (SUCCEEDED(resultCoInitialize))
			{
				CoUninitialize();
			}
		}
		else
		{
			std::wstring error = L"WebParser: Download failed: ";
			error += url;
			Log(error.c_str());
		}
	}
	else
	{
		Log(L"WebParser: The url is empty.\n");
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(urlData->dlThreadHandle);
	urlData->dlThreadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

    return 0;   // thread completed successfully
}

/*
  This function is called when the value should be
  returned as a string.
*/
LPCTSTR GetString(UINT id, UINT flags) 
{
	static std::wstring resultString;

	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);

	if(urlIter != g_UrlData.end())
	{
		EnterCriticalSection(&g_CriticalSection);
		if (((*urlIter).second)->download)
		{
			resultString = ((*urlIter).second)->downloadedFile;
		}
		else
		{
			resultString = ((*urlIter).second)->resultString;
		}
		LeaveCriticalSection(&g_CriticalSection);

		return resultString.c_str();
	}

	return NULL;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);

	if(urlIter != g_UrlData.end())
	{
		if (((*urlIter).second)->threadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(((*urlIter).second)->threadHandle, 0);
			(*urlIter).second->threadHandle = NULL;
			
			LeaveCriticalSection(&g_CriticalSection);
		}

		if (((*urlIter).second)->dlThreadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(((*urlIter).second)->dlThreadHandle, 0);
			(*urlIter).second->dlThreadHandle = NULL;
			
			LeaveCriticalSection(&g_CriticalSection);
		}

		if (((*urlIter).second)->downloadFile.empty())  // cache mode
		{
			if (!((*urlIter).second)->downloadedFile.empty())
			{
				// Delete the file
				DeleteFile(((*urlIter).second)->downloadedFile.c_str());
			}
		}

		delete (*urlIter).second;
		g_UrlData.erase(urlIter);
	}

	if (g_UrlData.empty())
	{
		// Last one, close all handles
		if (hRootHandle)
		{
			InternetCloseHandle(hRootHandle);
			hRootHandle = NULL;
		}

		// Last instance deletes the critical section
		DeleteCriticalSection(&g_CriticalSection);
		g_Initialized = false;
	}
}

/*
	Downloads the given url and returns the webpage as dynamically allocated string.
	You need to delete the returned string after use!
*/
BYTE* DownloadUrl(std::wstring& url, DWORD* dwDataSize, bool forceReload)
{
	HINTERNET hUrlDump;
	DWORD dwSize;
	BYTE* lpData;
	BYTE* lpOutPut;
	BYTE* lpHolding = NULL;
	int nCounter = 1;
	int nBufferSize;
	const int CHUNK_SIZE = 8192;

	std::wstring err = L"WebParser: Fetching URL: ";
	err += url;
	Log(err.c_str());
	
	DWORD flags = INTERNET_FLAG_RESYNCHRONIZE;
	if (forceReload)
	{
		flags = INTERNET_FLAG_RELOAD;
	}

	hUrlDump = InternetOpenUrl(hRootHandle, url.c_str(), NULL, NULL, flags, 0);
	if (hUrlDump == NULL)
	{
		ShowError(__LINE__); 
		return NULL;
	}

	*dwDataSize = 0;

	do
	{
		// Allocate the buffer.
		lpData = new BYTE[CHUNK_SIZE];
		
		// Read the data.
		if (!InternetReadFile(hUrlDump, (LPVOID)lpData, CHUNK_SIZE, &dwSize))
		{
			ShowError(__LINE__);
			delete [] lpData;
			break;
		}
		else
		{
			// Check if all of the data has been read.  This should
			// never get called on the first time through the loop.
			if (dwSize == 0)
			{
				// Delete the existing buffers.
				delete [] lpData;
				break;
			}
			
			// Determine the buffer size to hold the new data and the data
			// already written (if any).
			nBufferSize = nCounter * CHUNK_SIZE;
			
			// Allocate the output buffer.
			lpOutPut = new BYTE[nBufferSize + 2];
			
			// Make sure the buffer is not the initial buffer.
			if (lpHolding != NULL)
			{
				// Copy the data in the holding buffer.
				memcpy(lpOutPut, lpHolding, *dwDataSize);

				// Delete the old buffer
				delete [] lpHolding;

				lpHolding = lpOutPut;
				lpOutPut = lpOutPut + *dwDataSize;
			}
			else
			{
				lpHolding = lpOutPut;
			}

			// Copy the data buffer.
			memcpy(lpOutPut, lpData, dwSize);

			*dwDataSize += dwSize;

			// End with double null
			lpOutPut[dwSize] = 0;
			lpOutPut[dwSize + 1] = 0;
			
			// Increment the number of buffers read.
			nCounter++;

			// Delete the buffer
			delete [] lpData;
		}
	} while (TRUE);
	
	// Close the HINTERNET handle.
	InternetCloseHandle(hUrlDump);

	err = L"WebParser: Finished URL: ";
	err += url;
	Log(err.c_str());

	// Return.
	return lpHolding;
}

/*
  Writes the last error to log.
*/
void ShowError(int lineNumber, WCHAR* errorMsg)
{
	WCHAR szBuffer[4096];
	LPVOID lpMsgBuf = NULL;

	WCHAR buffer[16];
	wsprintf(buffer, L"%i", lineNumber);

	std::wstring err = L"WebParser (";
	err += buffer;
	err += L") ";

	if (errorMsg == NULL)
	{
		if (GetLastError() == ERROR_INTERNET_EXTENDED_ERROR) 
		{
			DWORD dwError, dwLen = 4096;
			if (InternetGetLastResponseInfo(&dwError, szBuffer, &dwLen))
			{
				err += szBuffer;
			}
		}
		else
		{
			DWORD dwErr = GetLastError();

			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwErr,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
			);

			if (lpMsgBuf == NULL) 
			{
				err += L"Unknown error: ";
				wsprintf(buffer, L"%i", dwErr);
				err += buffer;
			}
			else
			{
				err += (LPTSTR)lpMsgBuf;
				LocalFree(lpMsgBuf);
			}
		}
	}
	else
	{
		err += errorMsg;
	}

	Log(err.c_str());
}

/*
  Writes the log to a file (logging is thread safe (I think...)).
*/
void Log(const WCHAR* string)
{
	// Todo: put logging into critical section
	LSLog(LOG_DEBUG, L"Rainmeter", string);
}

UINT GetPluginVersion()
{
	return 1012;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}