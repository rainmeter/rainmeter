/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not included with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_LANGUAGE_H_
#define RM_LIBRARY_LANGUAGE_H_

#include <windows.h>
#include <string>
#include <unordered_map>
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
	std::unordered_map<UINT, const WCHAR*> m_Strings;
	unsigned short m_ButtonWidth;
	unsigned short m_LabelWidth;
	bool m_IsRTL;
};

#endif
