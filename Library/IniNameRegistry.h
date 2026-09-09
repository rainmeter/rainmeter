// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Common/Map.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

struct IniSectionID
{
	uint32_t value = 0;

	bool IsValid() const { return value != 0; }
	bool operator==(const IniSectionID&) const = default;
};

struct IniOptionID
{
	uint32_t value = 0;

	bool IsValid() const { return value != 0; }
	bool operator==(const IniOptionID&) const = default;
};

using IniValueID = uint64_t;

constexpr IniValueID MakeIniValueID(IniSectionID section, IniOptionID option)
{
	return (static_cast<IniValueID>(section.value) << 32) | option.value;
}

template <size_t N>
struct FixedWString
{
	wchar_t value[N];

	constexpr FixedWString(const wchar_t (&string)[N]) : value{}
	{
		for (size_t i = 0; i < N; ++i)
		{
			value[i] = string[i];
		}
	}

	constexpr FixedWString(const char (&string)[N]) : value{}
	{
		for (size_t i = 0; i < N; ++i)
		{
			value[i] = static_cast<wchar_t>(string[i]);
		}
	}

	constexpr std::wstring_view View() const { return std::wstring_view(value, N - 1); }
};

namespace std {

template <>
struct hash<IniSectionID>
{
	size_t operator()(IniSectionID id) const noexcept { return hash<uint32_t>{}(id.value); }
};

template <>
struct hash<IniOptionID>
{
	size_t operator()(IniOptionID id) const noexcept { return hash<uint32_t>{}(id.value); }
};

}  // namespace std

namespace IniNameRegistry {

IniSectionID InternSection(std::wstring_view name);
IniOptionID InternOption(std::wstring_view name);

// These overloads cache the ID because the name is fixed at compile time.
template<FixedWString Name>
IniSectionID InternSection()
{
	static const IniSectionID id = InternSection(Name.View());
	return id;
}

template<FixedWString Name>
IniOptionID InternOption()
{
	static const IniOptionID id = InternOption(Name.View());
	return id;
}

std::optional<IniSectionID> FindSection(std::wstring_view name);
std::optional<IniOptionID> FindOption(std::wstring_view name);
const std::wstring& GetOptionName(IniOptionID id);

}  // namespace IniNameRegistry
