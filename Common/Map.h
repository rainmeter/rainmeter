/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_MAP_H_
#define RM_COMMON_MAP_H_

#include <functional>
#include <string>
#include <string_view>

#include "ankerl/unordered_dense.h"

struct StringHash
{
	using is_transparent = void;

	size_t operator()(const std::wstring_view& str) const noexcept
	{
		return std::hash<std::wstring_view>{}(str);
	}
};

template <typename ValueType>
using StringMap = ankerl::unordered_dense::map<std::wstring, ValueType, StringHash, std::equal_to<>>;

#endif
