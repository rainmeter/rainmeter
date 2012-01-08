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

#include <windows.h>
#include <string>
#include <vector>
#include <time.h>
#include <shlwapi.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

#define BUFFER_SIZE 4096

struct MeasureData
{
	std::wstring pathname;
	std::wstring separator;
	std::vector<std::wstring> files;
	std::wstring value;
};

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str);
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			szAscii.resize(bufLen);
			WideCharToMultiByte(CP_ACP, 0, str, strLen, &szAscii[0], bufLen, NULL, NULL);
		}
	}
	return szAscii;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(CP_ACP, 0, str, strLen, &szWide[0], bufLen);
		}
	}
	return szWide;
}

void ScanFolder(std::vector<std::wstring>& files, std::vector<std::wstring>& filters, bool bSubfolders, const std::wstring& path)
{
	// Get folder listing
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	std::wstring searchPath = path + L"*";

	hSearch = FindFirstFile(searchPath.c_str(), &fileData);
	do
	{
		if (hSearch == INVALID_HANDLE_VALUE) break;    // No more files found

		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (bSubfolders &&
				wcscmp(fileData.cFileName, L".") != 0 &&
				wcscmp(fileData.cFileName, L"..") != 0)
			{
				ScanFolder(files, filters, bSubfolders, path + fileData.cFileName + L"\\");
			}
		}
		else
		{
			if (!filters.empty())
			{
				for (int i = 0; i < filters.size(); ++i)
				{
					if (!filters[i].empty() && PathMatchSpec(fileData.cFileName, filters[i].c_str()))
					{
						files.push_back(path + fileData.cFileName);
						break;
					}
				}
			}
			else
			{
				files.push_back(path + fileData.cFileName);
			}
		}
	}
	while (FindNextFile(hSearch, &fileData));
}

PLUGIN_EXPORT void Initialize(void** data)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	measure->pathname = RmReadPath(rm, L"PathName", L"");

	if (PathIsDirectory(measure->pathname.c_str()))
	{
		std::vector<std::wstring> fileFilters;
		LPCWSTR filter = RmReadString(rm, L"FileFilter", L"");
		if (*filter)
		{
			std::wstring ext = filter;

			size_t start = 0;
			size_t pos = ext.find(L';');
			while (pos != std::wstring::npos)
			{
				fileFilters.push_back(ext.substr(start, pos - start));
				start = pos + 1;
				pos = ext.find(L';', pos + 1);
			}
			fileFilters.push_back(ext.substr(start));
		}

		if (measure->pathname[measure->pathname.size() - 1] != L'\\')
		{
			measure->pathname += L"\\";
		}

		// Scan files
		measure->files.clear();
		bool bSubfolders = RmReadInt(rm, L"Subfolders", 1) == 1;
		ScanFolder(measure->files, fileFilters, bSubfolders, measure->pathname);
	}
	else
	{
		measure->separator = RmReadString(rm, L"Separator", L"\n");
	}

	srand((unsigned)time(NULL));
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->files.empty())
	{
		BYTE buffer[BUFFER_SIZE + 2];
		buffer[BUFFER_SIZE] = 0;

		// Read the file
		FILE* file = _wfopen(measure->pathname.c_str(), L"r");
		if (file)
		{
			// Check if the file is unicode or ascii
			fread(buffer, sizeof(WCHAR), 1, file);

			fseek(file, 0, SEEK_END);
			long size = ftell(file);

			if (size > 0)
			{
				// Go to a random place
				int pos = rand() % size;
				fseek(file, (pos / 2) * 2, SEEK_SET);

				measure->value.clear();

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

						sepPos1 = wcsstr(wBuffer, measure->separator.c_str());
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
							sepPos1 += measure->separator.size();
						}
					}
					while (sepPos1 == NULL);

					// Find the second separator
					do
					{
						sepPos2 = wcsstr(sepPos1, measure->separator.c_str());
						if (sepPos2 == NULL)
						{
							// The separator wasn't found
							if (feof(file))
							{
								// End of file reached -> read the rest
								measure->value += sepPos1;
								break;
							}
							else
							{
								measure->value += sepPos1;

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
							measure->value += sepPos1;
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

						sepPos1 = strstr(aBuffer, ConvertToAscii(measure->separator.c_str()).c_str());
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
							sepPos1 += measure->separator.size();
						}
					}
					while (sepPos1 == NULL);

					// Find the second separator
					do
					{
						sepPos2 = strstr(sepPos1, ConvertToAscii(measure->separator.c_str()).c_str());
						if (sepPos2 == NULL)
						{
							// The separator wasn't found
							if (feof(file))
							{
								// End of file reached -> read the rest
								measure->value += ConvertToWide(sepPos1);
								break;
							}
							else
							{
								measure->value += ConvertToWide(sepPos1);

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
							measure->value += ConvertToWide(sepPos1);
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
		measure->value = measure->files[rand() % measure->files.size()];
	}

	return 0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	return measure->value.c_str();
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}