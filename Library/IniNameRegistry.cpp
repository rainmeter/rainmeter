// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "../Common/StringUtil.h"
#include "IniNameRegistry.h"
#include <cstdlib>
#include <limits>

namespace {

StringMap<IniSectionID> g_Sections;
StringMap<IniOptionID> g_Options;

template <typename ID>
ID InternName(StringMap<ID>& names, std::wstring_view name)
{
	auto iter = names.find(name);
	if (iter != names.end()) return iter->second;

	if (names.size() >= std::numeric_limits<uint32_t>::max()) std::abort();

	const ID id{ static_cast<uint32_t>(names.size() + 1) };
	names.emplace(name, id);
	return id;
}

template <typename ID>
std::optional<ID> FindName(const StringMap<ID>& names, std::wstring_view name)
{
	auto iter = names.find(name);
	return iter != names.end() ? std::optional<ID>(iter->second) : std::nullopt;
}

std::optional<std::wstring_view> NormalizeName(std::wstring_view name, WCHAR* buffer, size_t bufferCount)
{
	if (!StringUtil::ToUpperCase(name, buffer, bufferCount)) return std::nullopt;
	return std::wstring_view(buffer, name.length());
}

}  // namespace

namespace IniNameRegistry {

IniSectionID InternSection(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? InternName(g_Sections, *normalized) : IniSectionID{};
}

IniOptionID InternOption(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? InternName(g_Options, *normalized) : IniOptionID{};
}

std::optional<IniSectionID> FindSection(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? FindName(g_Sections, *normalized) : std::nullopt;
}

std::optional<IniOptionID> FindOption(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? FindName(g_Options, *normalized) : std::nullopt;
}

const std::wstring& GetOptionName(IniOptionID id)
{
	for (const auto& option : g_Options)
	{
		if (option.second == id) return option.first;
	}

	static const std::wstring empty;
	return empty;
}

}  // namespace IniNameRegistry
