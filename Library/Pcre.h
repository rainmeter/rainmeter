/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_PCRE_H_
#define RM_LIBRARY_PCRE_H_

#include <string_view>

#include "pcre/config.h"
#include "pcre/pcre.h"

class Pcre
{
public:
	Pcre() : m_Pcre(nullptr), m_ErrorOffset(0), m_Offset(0) {}

	Pcre(const WCHAR* pattern, const char** error) : m_Pcre(nullptr), m_ErrorOffset(0), m_Offset(0)
	{
		Compile(pattern, error);
	}

	~Pcre() { Reset(); }

	Pcre(const Pcre&) = delete;
	Pcre& operator=(const Pcre&) = delete;

	void Compile(const WCHAR* pattern, const char** error)
	{
		Reset();
		m_Pcre = pcre16_compile((PCRE_SPTR16)pattern, 0, error, &m_ErrorOffset, nullptr);
	}

	void Reset()
	{
		pcre16_free(m_Pcre);
		m_Pcre = nullptr;
		m_ErrorOffset = 0;
		m_Offset = 0;
	}

	int Execute(std::wstring_view subject, int options, int* offsets, int offsetCount) const
	{
		return pcre16_exec(m_Pcre, nullptr, (PCRE_SPTR16)subject.data(), (int)subject.length(), m_Offset, options, offsets, offsetCount);
	}

	int GetErrorOffset() const { return m_ErrorOffset; }
	void SetOffset(int offset) { m_Offset = offset; }

	explicit operator bool() const { return m_Pcre != nullptr; }

private:
	pcre16* m_Pcre;
	int m_ErrorOffset;
	int m_Offset;
};

#endif
