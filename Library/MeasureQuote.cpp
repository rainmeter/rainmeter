/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureQuote.h"
#include "Rainmeter.h"
#include "../Common/StringUtil.h"
#include <random>

#define BUFFER_SIZE 4096

template <typename T>
T GetRandomNumber(T size)
{
	static std::mt19937 s_Engine((unsigned)time(nullptr));
	std::uniform_int_distribution<T> distribution(0, size);
	return distribution(s_Engine);
}

MeasureQuote::MeasureQuote(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_PathName(),
	m_Separator(),
	m_Files(),
	m_StringValue()
{
}

MeasureQuote::~MeasureQuote()
{
}

void MeasureQuote::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_PathName = parser.ReadString(section, L"PathName", L"");
	m_Skin->MakePathAbsolute(m_PathName);

	if (PathIsDirectory(m_PathName.c_str()))
	{
		std::vector<std::wstring> fileFilters;
		const std::wstring& filter = parser.ReadString(section, L"FileFilter", L"");
		if (!filter.empty())
		{
			size_t start = 0;
			size_t pos = filter.find(L';');
			while (pos != std::wstring::npos)
			{
				fileFilters.push_back(filter.substr(start, pos - start));
				start = pos + 1;
				pos = filter.find(L';', pos + 1);
			}
			fileFilters.push_back(filter.substr(start));
		}

		if (m_PathName[m_PathName.size() - 1] != L'\\')
		{
			m_PathName += L"\\";
		}

		m_Files.clear();
		bool subfolders = parser.ReadInt(section, L"Subfolders", 1) == 1;
		ScanFolder(m_Files, fileFilters, subfolders, m_PathName);
	}
	else
	{
		m_Separator = parser.ReadString(section, L"Separator", L"\n");
		m_Files.clear();
	}
}

void MeasureQuote::UpdateValue()
{
	m_Value = 0.0;

	if (m_Files.empty())
	{
		BYTE buffer[BUFFER_SIZE + 2];
		buffer[BUFFER_SIZE] = 0;

		FILE* file = _wfopen(m_PathName.c_str(), L"rb");
		if (file)
		{
			fread(buffer, sizeof(WCHAR), 1, file);

			fseek(file, 0, SEEK_END);
			long size = ftell(file);

			if (size > 0)
			{
				long pos = GetRandomNumber(size);

				fseek(file, (pos / 2) * 2, SEEK_SET);

				m_StringValue.clear();

				if (0xFEFF == *(WCHAR*)buffer)
				{
					WCHAR* wBuffer = (WCHAR*)buffer;

					WCHAR* sepPos1 = nullptr;
					WCHAR* sepPos2 = nullptr;
					do
					{
						size_t len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
						buffer[len] = 0;
						buffer[len + 1] = 0;

						sepPos1 = wcsstr(wBuffer, m_Separator.c_str());
						if (sepPos1 == nullptr)
						{
							if (feof(file))
							{
								fseek(file, 2, SEEK_SET);
								len = fread(buffer, sizeof(BYTE), BUFFER_SIZE, file);
								buffer[len] = 0;
								buffer[len + 1] = 0;
								sepPos1 = wBuffer;
							}
						}
						else
						{
							sepPos1 += m_Separator.size();
						}
					}
					while (sepPos1 == nullptr);

					do
					{
						sepPos2 = wcsstr(sepPos1, m_Separator.c_str());
						if (sepPos2 == nullptr)
						{
							if (feof(file))
							{
								m_StringValue += sepPos1;
								break;
							}
							else
							{
								m_StringValue += sepPos1;

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

							m_StringValue += sepPos1;
						}
					}
					while (sepPos2 == nullptr);
				}
				else
				{
					char* aBuffer = (char*)buffer;

					const std::string separator = StringUtil::Narrow(m_Separator);
					const char* separatorSz = separator.c_str();

					char* sepPos1 = nullptr;
					char* sepPos2 = nullptr;
					do
					{
						size_t len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
						aBuffer[len] = 0;

						sepPos1 = strstr(aBuffer, separatorSz);
						if (sepPos1 == nullptr)
						{
							if (feof(file))
							{
								fseek(file, 0, SEEK_SET);
								len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
								aBuffer[len] = 0;
								sepPos1 = aBuffer;
							}
						}
						else
						{
							sepPos1 += separator.size();
						}
					}
					while (sepPos1 == nullptr);

					do
					{
						sepPos2 = strstr(sepPos1, separatorSz);
						if (sepPos2 == nullptr)
						{
							if (feof(file))
							{
								m_StringValue += StringUtil::Widen(sepPos1);
								break;
							}
							else
							{
								m_StringValue += StringUtil::Widen(sepPos1);

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

							m_StringValue += StringUtil::Widen(sepPos1);
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
		m_StringValue = m_Files[GetRandomNumber(m_Files.size() - 1)];
	}
}

const WCHAR* MeasureQuote::GetStringValue()
{
	return CheckSubstitute(m_StringValue.c_str());
}

void MeasureQuote::ScanFolder(std::vector<std::wstring>& files, std::vector<std::wstring>& filters, bool subfolders, const std::wstring& path)
{
	WIN32_FIND_DATA fileData;
	std::wstring searchPath = path + L"*";

	HANDLE search = FindFirstFile(searchPath.c_str(), &fileData);
	if (search == INVALID_HANDLE_VALUE) return;

	do
	{
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (subfolders &&
				wcscmp(fileData.cFileName, L".") != 0 &&
				wcscmp(fileData.cFileName, L"..") != 0)
			{
				ScanFolder(files, filters, subfolders, path + fileData.cFileName + L"\\");
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
	while (FindNextFile(search, &fileData));

	FindClose(search);
}
