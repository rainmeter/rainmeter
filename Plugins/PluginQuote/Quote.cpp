/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <string>
#include <vector>
#include <time.h>
#include <shlwapi.h>
#include <random>
#include "../API/RainmeterAPI.h"
#include "../../Common/StringUtil.h"
#include "../../Common/FileUtil.h"

#define BUFFER_SIZE 4096

template <typename T>
T GetRandomNumber(T size)
{
	static std::mt19937 s_Engine((unsigned)time(nullptr));
	const std::uniform_int_distribution<T> distribution(0, size);
	return distribution(s_Engine);
}

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
				for (size_t i = 0; i < filters.size(); ++i)
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
	} while (FindNextFile(hSearch, &fileData));

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
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->files.empty())
	{

		// Read the file
		size_t size = 0;
		auto buffer = FileUtil::ReadFullFile(measure->pathname, &size);
		if (buffer && size > 0)
		{
			if (size > 0)
			{
				// Go to a random place
				long pos = GetRandomNumber(size);

				measure->value.clear();

				if (0xFEFF == *(WCHAR*)buffer.get())
				{
					// It's unicode
					WCHAR* wBuffer = (WCHAR*)buffer.get() + (pos / 2);

					// Read until we find the first separator
					WCHAR* sepPos1 = nullptr;
					WCHAR* sepPos2 = nullptr;

					bool fromStart = false;
					do
					{
						sepPos1 = wcsstr(wBuffer, measure->separator.c_str());
						if (sepPos1 == nullptr)
						{
							// The separator wasn't found
							if (!fromStart)
							{
								// End of file reached -> read from start
								sepPos1 = (WCHAR*)buffer.get();
								fromStart = true;
							}
							// else continue reading
						}
						else
						{
							sepPos1 += measure->separator.size();
						}
					} while (sepPos1 == nullptr);

					// Find the second separator

					sepPos2 = wcsstr(sepPos1, measure->separator.c_str());
					if (sepPos2 == nullptr)
					{
						// End of file reached -> read the rest
						measure->value += sepPos1;
					}
					else
					{
						if (sepPos2)
						{
							*sepPos2 = 0;
						}
						measure->value += sepPos1;
					}
				}
				else
				{
					// It's ascii
					char* aBuffer = (char*)buffer.get() + pos;

					const std::string separator = StringUtil::Narrow(measure->separator);
					const char* separatorSz = separator.c_str();

					// Read until we find the first separator
					char* sepPos1 = nullptr;
					char* sepPos2 = nullptr;

					bool fromStart = false;
					do
					{
						sepPos1 = strstr(aBuffer, separatorSz);
						if (sepPos1 == nullptr)
						{
							// The separator wasn't found
							if (!fromStart)
							{
								// End of file reached -> read from start
								sepPos1 = (char*)buffer.get();
								fromStart = true;
							}
							// else continue reading
						}
						else
						{
							sepPos1 += separator.size();
						}
					} while (sepPos1 == nullptr);

					// Find the second separator

					sepPos2 = strstr(sepPos1, separatorSz);
					if (sepPos2 == nullptr)
					{
						// End of file reached -> read the rest
						measure->value += StringUtil::Widen(sepPos1);
					}
					else
					{
						if (sepPos2)
						{
							*sepPos2 = 0;
						}
						measure->value += StringUtil::Widen(sepPos1);
					}
				}
			}

		}
		//free(buffer);
	}
	else
	{
		// Select the filename
		measure->value = measure->files[GetRandomNumber(measure->files.size() - 1)];
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
