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
#define SANITY_CHECK 2

//Measure options
//*****************************************************************//
//Note to future maintainers: if you add another option you need to//
//add proper checks in the == operator in order for DyanmicVariable//
//and setOption to work correctly with it, you have been warned    //
//*****************************************************************//
struct Options
{
	//General measure settings
	std::wstring pathname;
	bool isDirectory;
	bool isUniqueRandom;

	//Settings that only matter when path is directory
	std::wstring fileFilter;
	bool includeSubfolders;

	//Settings that only matter when path is a file
	std::wstring separator;
};

//Equality check for measure options, for checking options for important changes see note above
//Optimally organized to ensure best performance and unneeded checks
bool operator==(Options& lhs, Options& rhs)
{
	//Check if basic general options are the same
	if (lhs.isDirectory == rhs.isDirectory && lhs.isUniqueRandom == rhs.isUniqueRandom)
	{
		//If it is a directory we only need to check certain options for changes
		if (lhs.isDirectory)
		{
			if (lhs.includeSubfolders == rhs.includeSubfolders && lhs.fileFilter.compare(rhs.fileFilter) == 0 
				&& lhs.pathname.compare(rhs.pathname) == 0)
			{
				//Relavant options are the same
				return true;
			}
		}
		//If it is not a directory we only need to check file name and seperator
		else
		{
			if (lhs.separator.compare(rhs.separator) == 0 
				&& lhs.pathname.compare(rhs.pathname) == 0)
			{
				//Relavant options are the same
				return true;
			}
				
		}
	}
	return false;
}
bool operator!=(Options& lhs, Options& rhs)
{
	return !(lhs == rhs);
}

//Data and options for the measure
struct MeasureData
{
	Options* options;

	//Data that only matters when path is directory
	std::vector<std::wstring> files;
	std::vector<std::wstring> usedFiles;

	//Data that only matters when path is a file
	std::vector<size_t> separators;
	std::vector<size_t> usedSeparators;
	size_t fileSize;

	//String value of the measure (Number value is always 0)
	std::wstring value;
};

// Generates a random number from 0 (inclusive) to size (not inclusive)
template <typename T>
T GetRandomNumber(T size)
{
	static std::mt19937 s_Engine((unsigned)time(nullptr));
	const std::uniform_int_distribution<T> distribution(0, size-1);
	return distribution(s_Engine);
}

//Scan a folder for a list of all files, will recurse for each subfolder if subfolders is on
//Note: Does not yet watch for file updates to see if new files have been added or removed
void ScanFolder(std::vector<std::wstring>& files, std::vector<std::wstring>& usedFiles, std::vector<std::wstring>& filters, bool bSubfolders, const std::wstring& path)
{
	//Clear file list before going any further since we will be rebuilding them
	files.clear();
	usedFiles.clear();

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
				ScanFolder(files, usedFiles, filters, bSubfolders, path + fileData.cFileName + L"\\");
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

//Get random file from the list of files with option to prevent duplicates till a full cycle is done
HRESULT GetRandomFile(MeasureData* measure)
{
	// Select the filename
	size_t index = GetRandomNumber(measure->files.size());

	measure->value = measure->files[index].c_str();

	//If unique random is on swap the selected file to the list of used files
	if (measure->options->isUniqueRandom)
	{
		measure->files.erase(measure->files.begin() + index);
		measure->usedFiles.push_back(measure->value);

		//We are out of files that have not been used, swap file lists
		if (measure->files.empty())
		{
			measure->files = measure->usedFiles;
			measure->usedFiles.clear();
		}
	}

	return S_OK;
}

//Scan a file for all the seperators and make a list of them
void ScanFile(MeasureData* measure)
{
	//Clear seperator list before going any further since we will be rebuilding them
	measure->separators.clear();
	measure->usedSeparators.clear();

	BYTE buffer[BUFFER_SIZE + 2];
	buffer[BUFFER_SIZE] = 0;

	// Read the file
	FILE* file = _wfopen(measure->options->pathname.c_str(), L"rb");
	if (file)
	{
		// Check if the file is unicode or ascii
		fread(buffer, sizeof(WCHAR), 1, file);

		fseek(file, 0, SEEK_END);
		measure->fileSize = ftell(file);

		if (measure->fileSize > 0)
		{
			fseek(file, 0, SEEK_SET);

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
					const wchar_t* separatorSz = measure->options->separator.c_str();

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

				const std::string separator = StringUtil::Narrow(measure->options->separator);
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

//Get a random line from the file with option to prevent duplicates till a full cycle is done
//Returns E_ABORT if file had to be rebuilt and call was abort and needs to be run again
HRESULT GetRandomLine(MeasureData* measure)
{
	// Select the string
	size_t index = GetRandomNumber(measure->separators.size());
	size_t startPos = measure->separators[index];
	
	// Remove separator if using unique random
	if (measure->options->isUniqueRandom)
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

	// Read the file from our selected position
	FILE* file = _wfopen(measure->options->pathname.c_str(), L"rb");
	if (file)
	{
		std::wstring tempstr;

		BYTE buffer[BUFFER_SIZE + 2];
		buffer[BUFFER_SIZE] = 0;

		// Check if the file is unicode or ascii
		fread(buffer, sizeof(WCHAR), 1, file);

		fseek(file, 0, SEEK_END);
		long size = ftell(file);

		// File size does not match size when last indexed
		if (measure->fileSize != size)
		{
			// Reindex and abort
			fclose(file);
			ScanFile(measure);
			return E_ABORT;
		}
		else if (size > 0)
		{
			fseek(file, (long)startPos, SEEK_SET);
			if (0xFEFF == *(WCHAR*)buffer)
			{
				// It's unicode
				WCHAR* wBuffer = (WCHAR*)buffer;

				const wchar_t* separatorSz = measure->options->separator.c_str();

				size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
				wBuffer[len] = 0;

				//If at start do not find first separator else set start pos to first separator
				wchar_t* sepPos1 = startPos == 0 ? wBuffer : wcsstr(wBuffer, separatorSz);
				wchar_t* sepPos2 = nullptr;

				//If separator is where we expect it or this is the first line of the file
				if (wcscmp(wBuffer, sepPos1) == 0 || startPos == 0)
				{
					if (startPos != 0)
					{
						sepPos1 += measure->options->separator.size();
					}

					do
					{
						sepPos2 = wcsstr(sepPos1, separatorSz);
						if (sepPos2 == nullptr)
						{
							// The separator wasn't found
							if (feof(file))
							{
								// End of file reached -> read the rest
								tempstr += sepPos1;
								break;
							}
							else
							{
								tempstr += sepPos1;

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
							tempstr += sepPos1;
						}
					} while (sepPos2 == nullptr);
				}
				else
				{
					//If separator is not where we expect it then file has been modified and needs reindexed and we need to abort
					fclose(file);
					ScanFile(measure);
					return E_ABORT;
				}
			}
			else
			{
				// It's ascii
				char* aBuffer = (char*)buffer;

				const std::string separator = StringUtil::Narrow(measure->options->separator);
				const char* separatorSz = separator.c_str();

				size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
				aBuffer[len] = 0;

				//If at start do not find first separator else set start pos to first separator
				char* sepPos1 = startPos == 0 ? aBuffer : strstr(aBuffer, separatorSz);
				char* sepPos2 = nullptr;

				//If separator is where we expect it or this is the first line of the file
				if (strcmp(aBuffer, sepPos1) == 0 || startPos == 0)
				{
					if (startPos != 0)
					{
						sepPos1 += measure->options->separator.size();
					}

					do
					{
						sepPos2 = strstr(sepPos1, separatorSz);
						if (sepPos2 == nullptr)
						{
							// The separator wasn't found
							if (feof(file))
							{
								// End of file reached -> read the rest
								tempstr += StringUtil::Widen(sepPos1);
								break;
							}
							else
							{
								tempstr += StringUtil::Widen(sepPos1);

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
							tempstr += StringUtil::Widen(sepPos1);
						}
					} while (sepPos2 == nullptr);
				}
				else
				{
					//If separator is not where we expect it then file has been modified and needs reindexed and we need to abort
					fclose(file);
					ScanFile(measure);
					return E_ABORT;
				}
			}
		}

		fclose(file);
		measure->value = tempstr.c_str();
		return S_OK;
	}
	return E_ACCESSDENIED;
}

//Perform setup for the measure,  
void Setup(MeasureData* measure)
{
	if (measure->options->isDirectory)
	{
		std::vector<std::wstring> fileFilters;
		if (measure->options->fileFilter.length() > std::wstring::npos)
		{
			std::wstring ext = measure->options->fileFilter;

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

		if (measure->options->pathname[measure->options->pathname.size() - 1] != L'\\')
		{
			measure->options->pathname += L"\\";
		}

		// Scan files
		ScanFolder(measure->files, measure->usedFiles, fileFilters, measure->options->includeSubfolders, measure->options->pathname);
	}
	else
	{
		measure->separators.clear();
		measure->usedSeparators.clear();
		ScanFile(measure);
	}
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
	measure->options = new Options;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;
	Options* temp = new Options;

	//General options
	temp->pathname = RmReadPath(rm, L"PathName", L"");
	temp->isDirectory = PathIsDirectory(temp->pathname.c_str());
	temp->isUniqueRandom = RmReadInt(rm, L"UniqueRandom", 0) == 1;

	//Options that only matter if it is a directory
	temp->fileFilter = RmReadString(rm, L"FileFilter", L"");
	temp->includeSubfolders = RmReadInt(rm, L"Subfolders", 1) == 1;

	//Options that only matter if it is a file
	temp->separator = RmReadString(rm, L"Separator", L"\n");

	//Check if options have changed since last setup, if they have set new options
	if (*measure->options != *temp)
	{
		measure->options = temp;
		Setup(measure);
	}
	else
	{
		//Temp was not needed so deallocate it
		delete temp;
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	if (measure->options->isDirectory)
	{
		GetRandomFile(measure);
	}
	else
	{
		int i = 0;
		HRESULT hr;
		do
		{
			hr = GetRandomLine(measure);
			i++;
		//If GetRandomLine was aborted we will need to run it again till we have reach max we want to try on one go
		} while (hr == E_ABORT && i < SANITY_CHECK);
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
	delete measure->options;
	delete measure;
}
