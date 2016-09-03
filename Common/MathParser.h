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

namespace MathParser
{
	typedef bool (*GetValueFunc)(const WCHAR* str, int len, double* value, void* context);

	const WCHAR* Check(const WCHAR* formula);
	const WCHAR* CheckedParse(const WCHAR* formula, double* result);
	const WCHAR* Parse(
		const WCHAR* formula, double* result,
		GetValueFunc getValue = nullptr, void* getValueContext = nullptr);

	bool IsDelimiter(WCHAR ch);
};

#endif
