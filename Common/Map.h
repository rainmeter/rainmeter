// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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

using StringSet = ankerl::unordered_dense::set<std::wstring, StringHash, std::equal_to<>>;
