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

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <time.h>
#include <tchar.h>
#include <shlwapi.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

struct quoteData
{
	std::wstring pathname;
	std::wstring separator;
	std::vector<std::wstring> fileFilters;
	std::vector<std::wstring> files;
	std::wstring value;
};

void ScanFolder(quoteData& qData, bool bSubfolders, const std::wstring& path);

static std::map<UINT, quoteData> g_Values;

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str) + 1;
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			char* tmpSz = new char[bufLen];
			tmpSz[0] = 0;
			WideCharToMultiByte(CP_ACP, 0, str, strLen, tmpSz, bufLen, NULL, NULL);
			szAscii = tmpSz;
			delete [] tmpSz;
		}
	}
	return szAscii;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str) + 1;
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			WCHAR* wideSz = new WCHAR[bufLen];
			wideSz[0] = 0;
			MultiByteToWideChar(CP_ACP, 0, str, strLen, wideSz, bufLen);
			szWide = wideSz;
			delete [] wideSz;
		}
	}
	return szWide;
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
	quoteData qData;
	LPCTSTR data;
	bool bSubfolders = false;

	data = ReadConfigString(section, L"Subfolders", L"0");
	if (data && _ttoi(data) == 1)
	{
		bSubfolders = true;
	}

	data = ReadConfigString(section, L"Separator", L"\n");
	if (data)
	{
		qData.separator = data;
	}

	data = ReadConfigString(section, L"FileFilter", L"");
	if (data && wcslen(data) > 0)
	{
		std::wstring ext = data;

		size_t start = 0;
		size_t pos = ext.find(L';');
		while (pos != std::wstring::npos) 
		{
			qData.fileFilters.push_back(ext.substr(start, pos - start));
			start = pos + 1;
			pos = ext.find(L';', pos + 1);
		}
		qData.fileFilters.push_back(ext.substr(start));

		qData.separator = data;
	}

	/* Read our own settings from the ini-file */
	data = ReadConfigString(section, L"PathName", L"");
	if (data && wcslen(data) > 0)
	{
		qData.pathname = data;

		if (qData.pathname.find(':') == -1)		// Not found
		{
			std::wstring path = iniFile;
			size_t pos = path.rfind('\\');
			if (pos >= 0)
			{
				path.erase(pos + 1);
				qData.pathname = path + qData.pathname;
			}
		}

		if (PathIsDirectory(qData.pathname.c_str())) 
		{
			if (qData.pathname[qData.pathname.size() - 1] != L'\\') 
			{
				qData.pathname += L"\\";
			}

			// Scan files
			ScanFolder(qData, bSubfolders, qData.pathname);
		}
	}
	
	if (!qData.pathname.empty())
	{
		g_Values[id] = qData;
	}

	// TODO: Random=0, load stuff sequentially (store to somewhere)

	srand( (unsigned)time( NULL ) );

	return 0;
}

void ScanFolder(quoteData& qData, bool bSubfolders, const std::wstring& path)
{
	// Get folder listing
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	std::wstring searchPath = path + L"*";

	hSearch = FindFirstFile(searchPath.c_str(), &fileData);
	do
	{
		if(hSearch == INVALID_HANDLE_VALUE) break;    // No more files found

		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			if (bSubfolders) 
			{
				if (wcscmp(fileData.cFileName, L".") != 0 && wcscmp(fileData.cFileName, L"..") != 0)
				{
					ScanFolder(qData, bSubfolders, path + fileData.cFileName + L"\\");
				}
			}
		}
		else
		{
			if (!qData.fileFilters.empty()) 
			{
				for (int i = 0; i < qData.fileFilters.size(); i++) 
				{
					if (!qData.fileFilters[i].empty() && PathMatchSpec(fileData.cFileName, qData.fileFilters[i].c_str())) 
					{
						qData.files.push_back(path + fileData.cFileName);
						break;
					}
				}
			}
			else
			{
				qData.files.push_back(path + fileData.cFileName);
			}
		}
	}
	while(FindNextFile(hSearch, &fileData));
}

#define BUFFER_SIZE 4096

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{
	std::map<UINT, quoteData>::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		quoteData& qData = (*i).second;

		if (qData.files.empty())
		{
			BYTE buffer[BUFFER_SIZE + 2];
			buffer[BUFFER_SIZE] = 0;

			// Read the file
			FILE* file = _wfopen(qData.pathname.c_str(), L"r");
			if (file)
			{
				// Check if the file is unicode or ascii
				fread(buffer, sizeof(WCHAR), 1, file);

				fseek(file, 0, SEEK_END);
				int size = ftell(file);

				if (size > 0)
				{
					// Go to a random place
					int pos = rand() % size;
					fseek(file, (pos / 2) * 2, SEEK_SET);

					qData.value.erase();

					if (0xFEFF == *(WCHAR*)buffer) 
					{
						// It's unicode
						WCHAR* wBuffer = (WCHAR*)buffer;

						// Read until we find the first separator
						WCHAR* sepPos1 = NULL;
						WCHAR* sepPos2 = NULL;
						do 
						{
							size_t len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
							buffer[len] = 0;
							buffer[len + 1] = 0;

							sepPos1 = wcsstr(wBuffer, qData.separator.c_str());
							if (sepPos1 == NULL)
							{
								// The separator wasn't found
								if (feof(file))
								{
									// End of file reached -> read from start
									fseek(file, 2, SEEK_SET);
									len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
									buffer[len] = 0;
									buffer[len + 1] = 0;
									sepPos1 = wBuffer;
								}
								// else continue reading
							}
							else
							{
								sepPos1 += qData.separator.size();
							}
						}
						while (sepPos1 == NULL);

						// Find the second separator
						do 
						{
							sepPos2 = wcsstr(sepPos1, qData.separator.c_str());
							if (sepPos2 == NULL)
							{
								// The separator wasn't found
								if (feof(file))
								{
									// End of file reached -> read the rest
									qData.value += sepPos1;
									break;
								}
								else
								{
									qData.value += sepPos1;

									// else continue reading
									size_t len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
									buffer[len] = 0;
									buffer[len + 1] = 0;
									sepPos1 = wBuffer;
								}
							}
							else
							{
								if (sepPos2)
								{
									*sepPos2 = 0;
								}

								// Read until we find the second separator
								qData.value += sepPos1;
							}
						}
						while (sepPos2 == NULL);
					}
					else
					{
						// It's ascii
						char* aBuffer = (char*)buffer;

						// Read until we find the first separator
						char* sepPos1 = NULL;
						char* sepPos2 = NULL;
						do 
						{
							size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
							aBuffer[len] = 0;

							sepPos1 = strstr(aBuffer, ConvertToAscii(qData.separator.c_str()).c_str());
							if (sepPos1 == NULL)
							{
								// The separator wasn't found
								if (feof(file))
								{
									// End of file reached -> read from start
									fseek(file, 0, SEEK_SET);
									len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
									aBuffer[len] = 0;
									sepPos1 = aBuffer;
								}
								// else continue reading
							}
							else
							{
								sepPos1 += qData.separator.size();
							}
						}
						while (sepPos1 == NULL);

						// Find the second separator
						do 
						{
							sepPos2 = strstr(sepPos1, ConvertToAscii(qData.separator.c_str()).c_str());
							if (sepPos2 == NULL)
							{
								// The separator wasn't found
								if (feof(file))
								{
									// End of file reached -> read the rest
									qData.value += ConvertToWide(sepPos1);
									break;
								}
								else
								{
									qData.value += ConvertToWide(sepPos1);

									// else continue reading
									size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
									aBuffer[len] = 0;
									sepPos1 = aBuffer;
								}
							}
							else
							{
								if (sepPos2)
								{
									*sepPos2 = 0;
								}

								// Read until we find the second separator
								qData.value += ConvertToWide(sepPos1);
							}
						}
						while (sepPos2 == NULL);
					}
				}

				fclose(file);
			}
		}
		else
		{
			// Select the filename
			if (qData.files.size() > 0)
			{
				qData.value = qData.files[rand() % qData.files.size()];
			}
		}
	}
	
	return 0;
}

LPCTSTR GetString(UINT id, UINT flags) 
{
	std::map<UINT, quoteData>::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		return ((*i).second).value.c_str();
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
	std::map<UINT, quoteData>::iterator i1 = g_Values.find(id);
	if (i1 != g_Values.end())
	{
		g_Values.erase(i1);
	}
}

UINT GetPluginVersion()
{
	return 1001;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}