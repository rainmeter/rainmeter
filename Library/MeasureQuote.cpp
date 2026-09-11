// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureQuote.h"
#include "Rainmeter.h"
#include "../Common/FileUtil.h"
#include <random>

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
	m_FileText(),
	m_FileData(),
	m_StringValue()
{
}

MeasureQuote::~MeasureQuote()
{
}

void MeasureQuote::ReadOptions(ConfigParser::OptionReader& reader)
{
	Measure::ReadOptions(reader);

	const std::wstring oldPathName = m_PathName;
	reader.ReadString<"PathName">(m_PathName, L"");
	m_Skin->MakePathAbsolute(m_PathName);

	if (PathIsDirectory(m_PathName.c_str()))
	{
		std::vector<std::wstring> fileFilters;
		const std::wstring& filter = reader.ReadString<"FileFilter">(L"");
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
		bool subfolders = reader.ReadInt<"Subfolders">(1) == 1;
		ScanFolder(m_Files, fileFilters, subfolders, m_PathName);
	}
	else
	{
		reader.ReadString<"Separator">(m_Separator, L"\n");
		m_Files.clear();
	}

	if (m_PathName != oldPathName)
	{
		m_FileText.reset();
	}
}

void MeasureQuote::UpdateValue()
{
	m_Value = 0.0;

	if (m_Files.empty())
	{
		WIN32_FILE_ATTRIBUTE_DATA fileData;
		if (!GetFileAttributesEx(m_PathName.c_str(), GetFileExInfoStandard, &fileData))
		{
			m_FileText.reset();
			return;
		}

		const bool fileChanged = !m_FileText ||
			CompareFileTime(&fileData.ftLastWriteTime, &m_FileData.ftLastWriteTime) != 0 ||
			fileData.nFileSizeHigh != m_FileData.nFileSizeHigh ||
			fileData.nFileSizeLow != m_FileData.nFileSizeLow;
		if (fileChanged)
		{
			std::wstring text;
			if (!FileUtil::ReadTextFileWithAnsiFallback(m_PathName, text))
			{
				m_FileText.reset();
				return;
			}

			m_FileText = std::move(text);
			m_FileData = fileData;
		}

		if (!m_FileText->empty())
		{
			const size_t position = GetRandomNumber(m_FileText->size() - 1);
			const size_t separator = m_FileText->find(m_Separator, position);
			const size_t start = separator == std::wstring::npos ? 0 : separator + m_Separator.size();
			const size_t end = m_FileText->find(m_Separator, start);
			m_StringValue = m_FileText->substr(start, end - start);
		}
	}
	else
	{
		m_StringValue = m_Files[GetRandomNumber(m_Files.size() - 1)];
	}
}

std::optional<std::wstring_view> MeasureQuote::GetStringValue()
{
	return CheckSubstitute(m_StringValue);
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
