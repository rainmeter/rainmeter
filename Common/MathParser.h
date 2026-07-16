/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#ifndef RM_COMMON_MATHPARSER_H_
#define RM_COMMON_MATHPARSER_H_

#include <Windows.h>

class MathParser
{
public:
	typedef bool (*GetValueFunc)(const WCHAR* str, int len, double* value, void* context);

	MathParser(GetValueFunc getValue = nullptr, void* getValueContext = nullptr);

	const WCHAR* Check(const WCHAR* formula) const;
	const WCHAR* CheckedParse(const WCHAR* formula, double* result) const;
	const WCHAR* Parse(const WCHAR* formula, double* result) const;

	bool IsDelimiter(WCHAR ch) const;

private:
	GetValueFunc m_GetValue;
	void* m_GetValueContext;
};

#endif
