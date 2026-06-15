/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not included with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Language.h"

Language::Language() :
	m_LCID(),
	m_FileMapping(),
	m_Data(),
	m_ButtonWidth(0),
	m_LabelWidth(0),
	m_IsRTL(false)
{
}

Language::~Language()
{
	Unload();
}

std::vector<Language::Info> Language::GetAvailable(const std::wstring& directory)
{
	std::vector<Info> languages;

	WIN32_FIND_DATA fd;
	HANDLE search = FindFirstFile((directory + L"*.rmlang").c_str(), &fd);
	if (search == INVALID_HANDLE_VALUE) return languages;

	do
	{
		WCHAR* extension = wcsrchr(fd.cFileName, L'.');
		if (!extension || _wcsicmp(extension, L".rmlang") != 0) continue;

		Info info;
		info.lcid = wcstoul(fd.cFileName, nullptr, 10);
		if (info.lcid == 0) continue;

		WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
		if (LCIDToLocaleName(info.lcid, localeName, _countof(localeName), 0) == 0) continue;

		WCHAR name[MAX_PATH];
		if (GetLocaleInfoEx(localeName, LOCALE_SENGLISHLANGUAGENAME, name, _countof(name)) == 0) continue;
		info.englishName = name;

		if (GetLocaleInfoEx(localeName, LOCALE_SNATIVEDISPLAYNAME, name, _countof(name)) == 0) continue;
		info.nativeName = name;

		languages.push_back(info);
	}
	while (FindNextFile(search, &fd));

	FindClose(search);
	return languages;
}

const WCHAR* Language::GetString(UINT id) const
{
	auto iter = m_Strings.find(id);
	return iter != m_Strings.end() ? iter->second : L"";
}

bool Language::Load(const std::wstring& directory, const std::wstring& language)
{
	const LCID requestedLCID = wcstoul(language.c_str(), nullptr, 10);
	if (requestedLCID == 0) return false;

	HANDLE file = CreateFile((directory + language + L".rmlang").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart < 12 || fileSize.QuadPart > MAXDWORD)
	{
		CloseHandle(file);
		return false;
	}

	HANDLE fileMapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CloseHandle(file);
	if (!fileMapping) return false;

	const BYTE* data = static_cast<const BYTE*>(MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0));
	if (!data)
	{
		CloseHandle(fileMapping);
		return false;
	}

	auto fail = [data, fileMapping]() -> bool
	{
		UnmapViewOfFile(data);
		CloseHandle(fileMapping);
		return false;
	};

	const size_t dataSize = (size_t)fileSize.QuadPart;
	if (memcmp(data, "RMLANG", 6) != 0 || data[6] != 1 || data[7] > 1) return fail();

	auto readUInt32 = [data](size_t offset) -> uint32_t
	{
		uint32_t value;
		memcpy(&value, data + offset, sizeof(value));
		return value;
	};
	auto readUInt16 = [data](size_t offset) -> unsigned short
	{
		unsigned short value;
		memcpy(&value, data + offset, sizeof(value));
		return value;
	};

	std::unordered_map<UINT, const WCHAR*> strings;
	size_t offset = 12;
	while (offset < dataSize)
	{
		if (dataSize - offset < 8) return fail();
		const uint32_t id = readUInt32(offset);
		const uint32_t length = readUInt32(offset + 4);
		offset += 8;

		const size_t byteLength = (length + 1) * sizeof(WCHAR);
		if (byteLength > dataSize - offset || (offset % alignof(WCHAR)) != 0) return fail();
		if (data[offset + byteLength - sizeof(WCHAR)] != 0 || data[offset + byteLength - 1] != 0) return fail();

		const WCHAR* value = reinterpret_cast<const WCHAR*>(data + offset);
		if (!strings.emplace(id, value).second) return fail();

		offset += (size_t)byteLength;
	}

	Unload();

	m_FileMapping = fileMapping;
	m_Data = data;
	m_Strings.swap(strings);
	m_ButtonWidth = readUInt16(8);
	m_LabelWidth = readUInt16(10);
	m_IsRTL = data[7] == 1;
	m_LCID = requestedLCID;

	return true;
}

void Language::Unload()
{
	m_Strings.clear();

	if (m_Data) UnmapViewOfFile(m_Data);
	m_Data = nullptr;

	if (m_FileMapping) CloseHandle(m_FileMapping);
	m_FileMapping = nullptr;
}
