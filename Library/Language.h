// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <string>
#include <vector>

class Language
{
public:
	struct Info
	{
		std::wstring englishName;
		std::wstring nativeName;
		LCID lcid;
	};

	Language();
	~Language();

	Language(const Language&) = delete;
	Language& operator=(const Language&) = delete;

	bool Load(const std::wstring& directory, const std::wstring& language);
	bool IsLoaded() const { return m_Data != nullptr; }

	LCID GetLCID() const { return m_LCID; }

	const WCHAR* GetString(UINT id) const;
	unsigned short GetButtonWidth() const { return m_ButtonWidth; }
	unsigned short GetLabelWidth() const { return m_LabelWidth; }
	bool IsRTL() const { return m_IsRTL; }

	static std::vector<Info> GetAvailable(const std::wstring& directory);

private:
	void Unload();

	LCID m_LCID;
	HANDLE m_FileMapping;
	const BYTE* m_Data;
	ankerl::unordered_dense::map<UINT, const WCHAR*> m_Strings;
	unsigned short m_ButtonWidth;
	unsigned short m_LabelWidth;
	bool m_IsRTL;
};
