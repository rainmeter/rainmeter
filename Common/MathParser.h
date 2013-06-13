/*
  Copyright (C) 2011 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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