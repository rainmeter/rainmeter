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

#define BUFFER_SIZE 4096


struct MeasureData
{
	std::wstring pathname;
	bool isUniqueRandom = false;
	std::wstring separator;
	std::vector<std::wstring> files;
	std::vector<std::wstring> usedFiles;
	std::vector<size_t> separators;
	std::vector<size_t> usedSeparators;
	std::wstring value;
};

void ReadFile(MeasureData* measure)
{
	BYTE buffer[BUFFER_SIZE + 2];
	buffer[BUFFER_SIZE] = 0;

	// Read the file
	FILE* file = _wfopen(measure->pathname.c_str(), L"rb");
	if (file)
	{
		// Check if the file is unicode or ascii
		fread(buffer, sizeof(WCHAR), 1, file);

		fseek(file, 0, SEEK_END);
		long size = ftell(file);

		if (size > 0)
		{
			fseek(file, 0, SEEK_SET);

			measure->value.clear();
			measure->separators.clear();

			if (0xFEFF == *(WCHAR*)buffer)
			{

				// It's unicode
				WCHAR* wBuffer = (WCHAR*)buffer;

				// Read until EOF
				do
				{
					size_t len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
					buffer[len] = 0;
					buffer[len + 1] = 0;

					std::wstring bufferString = std::wstring(wBuffer);
					const wchar_t* separatorSz = measure->separator.c_str();

					//Start of the file counts as a separator
					size_t sepPos = 0;
					do
					{
						measure->separators.push_back(sepPos);
						sepPos = bufferString.find(separatorSz, sepPos + 1);
					} while (sepPos != std::string::npos);

				} while (!feof(file));
			}
			else
			{
				// It's ascii
				char* aBuffer = (char*)buffer;

				const std::string separator = StringUtil::Narrow(measure->separator);
				const char* separatorSz = separator.c_str();

				// Read until EOF
				do
				{
					size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
					aBuffer[len] = 0;

					std::string bufferString = std::string(aBuffer);

					//Start of the file counts as a separator
					size_t sepPos = 0;
					do
					{
						measure->separators.push_back(sepPos);
						sepPos = bufferString.find(separatorSz, sepPos + 1);
					} while (sepPos != std::string::npos);

				} while (!feof(file));
			}
		}

		fclose(file);
	}
}

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
	}
	while (FindNextFile(hSearch, &fileData));

	FindClose(hSearch);
}


template <typename T>
T GetRandomNumber(T size)
{
	static std::mt19937 s_Engine((unsigned)time(nullptr));
	const std::uniform_int_distribution<T> distribution(0, size);
	return distribution(s_Engine);
}

void GetRandomLine(MeasureData* measure, size_t startPos)
{
	// Read the file from our selected pos
	FILE* file = _wfopen(measure->pathname.c_str(), L"rb");
	if (file)
	{
		BYTE buffer[BUFFER_SIZE + 2];
		buffer[BUFFER_SIZE] = 0;

		// Check if the file is unicode or ascii
		fread(buffer, sizeof(WCHAR), 1, file);

		fseek(file, 0, SEEK_END);
		long size = ftell(file);

		if (size > 0)
		{
			fseek(file, startPos, SEEK_SET);

			measure->value.clear();

			if (0xFEFF == *(WCHAR*)buffer)
			{
				// It's unicode
				WCHAR* wBuffer = (WCHAR*)buffer;

				const wchar_t* separatorSz = measure->separator.c_str();

				size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
				wBuffer[len] = 0;

				wchar_t* sepPos1 = wcsstr(wBuffer, separatorSz);
				wchar_t* sepPos2 = nullptr;

				//If separator is where we expect it
				if (wcscmp(wBuffer, sepPos1) == 0)
				{
					sepPos1 += measure->separator.size();
					do
					{
						sepPos2 = wcsstr(sepPos1, separatorSz);
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
								size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
								wBuffer[len] = 0;
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
					} while (sepPos2 == nullptr);
				}
				else
				{
					//If separator is not where we expect it then file has been modified and needs reindexed
					ReadFile(measure);
				}
			}
			else
			{
				// It's ascii
				char* aBuffer = (char*)buffer;

				const std::string separator = StringUtil::Narrow(measure->separator);
				const char* separatorSz = separator.c_str();

				size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
				aBuffer[len] = 0;

				char* sepPos1 = strstr(aBuffer, separatorSz);
				char* sepPos2 = nullptr;

				//If separator is where we expect it
				if (strcmp(aBuffer, sepPos1) == 0)
				{
					sepPos1 += separator.size();
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
					} while (sepPos2 == nullptr);
				}
				else
				{
					//If separator is not where we expect it then file has been modified and needs reindexed
					ReadFile(measure);
				}
			}
		}

		fclose(file);
	}
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

	measure->isUniqueRandom = RmReadInt(rm, L"UniqueRandom", 0) != 0;

	if (PathIsDirectory(measure->pathname.c_str()))
	{
		std::vector<std::wstring> fileFilters;
		LPCWSTR filter = RmReadString(rm, L"FileFilter", L"");
		if (*filter)
		{
			std::wstring ext = filter;

			size_t start = 0;
			size_t sepPos = ext.find(L';');
			while (sepPos != std::wstring::npos)
			{
				fileFilters.push_back(ext.substr(start, sepPos - start));
				start = sepPos + 1;
				sepPos = ext.find(L';', sepPos + 1);
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

		ReadFile(measure);
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	if (!measure->separators.empty())
	{
		// Select the filename
		size_t index = GetRandomNumber(measure->separators.size() - 1);
		size_t startPos = measure->separators[index];

		// Remove separator if using unique random
		if (measure->isUniqueRandom)
		{
			measure->separators.erase(measure->separators.begin() + index);
			measure->usedSeparators.push_back(startPos);

			// We are out of separators, swap separator indexes
			if (measure->separators.empty())
			{
				measure->separators = measure->usedSeparators;
				measure->usedSeparators.clear();
			}
		}

		GetRandomLine(measure, startPos);
	}
	else if(!measure->files.empty())
	{
		// Select the filename
		size_t index = GetRandomNumber(measure->files.size() - 1);

		measure->value = measure->files[index];

		if (measure->isUniqueRandom)
		{
			measure->files.erase(measure->files.begin() + index);
			measure->usedFiles.push_back(measure->value);

			//We are out of files, swap file indexes
			if (measure->files.empty())
			{
				measure->files = measure->usedFiles;
				measure->usedFiles.clear();
			}
		}
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
