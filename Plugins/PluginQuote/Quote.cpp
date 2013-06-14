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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <string>
#include <vector>
#include <time.h>
#include <shlwapi.h>
#include "../API/RainmeterAPI.h"
#include "../../Common/StringUtil.h"

#define BUFFER_SIZE 4096

struct MeasureData
{
	std::wstring pathname;
	std::wstring separator;
	std::vector<std::wstring> files;
	std::wstring value;
};

void ScanFolder(std::vector<std::wstring>& files, std::vector<std::wstring>& filters, bool bSubfolders, const std::wstring& path)
{
	// Get folder listing
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	std::wstring searchPath = path + L"*";

	hSearch = FindFirstFile(searchPath.c_str(), &fileData);
	if (hSearch == INVALID_HANDLE_VALUE) return;    // No more files found

	do
	{
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

	FindClose(hSearch);
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
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

	srand((unsigned)time(nullptr));
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
					WCHAR* sepPos1 = nullptr;
					WCHAR* sepPos2 = nullptr;
					do
					{
						size_t len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
						buffer[len] = 0;
						buffer[len + 1] = 0;

						sepPos1 = wcsstr(wBuffer, measure->separator.c_str());
						if (sepPos1 == nullptr)
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
					while (sepPos1 == nullptr);

					// Find the second separator
					do
					{
						sepPos2 = wcsstr(sepPos1, measure->separator.c_str());
						if (sepPos2 == nullptr)
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
					while (sepPos2 == nullptr);
				}
				else
				{
					// It's ascii
					char* aBuffer = (char*)buffer;

					const std::string separator = StringUtil::Narrow(measure->separator);
					const char* separatorSz = separator.c_str();

					// Read until we find the first separator
					char* sepPos1 = nullptr;
					char* sepPos2 = nullptr;
					do
					{
						size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
						aBuffer[len] = 0;

						sepPos1 = strstr(aBuffer, separatorSz);
						if (sepPos1 == nullptr)
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
							sepPos1 += separator.size();
						}
					}
					while (sepPos1 == nullptr);

					// Find the second separator
					do
					{
						sepPos2 = strstr(sepPos1, separatorSz);
						if (sepPos2 == nullptr)
						{
							// The separator wasn't found
							if (feof(file))
							{
								// End of file reached -> read the rest
								measure->value += StringUtil::Widen(sepPos1);
								break;
							}
							else
							{
								measure->value += StringUtil::Widen(sepPos1);

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
							measure->value += StringUtil::Widen(sepPos1);
						}
					}
					while (sepPos2 == nullptr);
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