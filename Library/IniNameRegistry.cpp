// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "../Common/StringUtil.h"
#include "IniNameRegistry.h"
#include <cstdlib>
#include <limits>

namespace {

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

IniSectionID IniNameRegistry::InternSection(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? InternName(m_Sections, *normalized) : IniSectionID{};
}

IniOptionID IniNameRegistry::InternOption(std::wstring_view name)
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? InternName(m_Options, *normalized) : IniOptionID{};
}

std::optional<IniSectionID> IniNameRegistry::FindSection(std::wstring_view name) const
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? FindName(m_Sections, *normalized) : std::nullopt;
}

std::optional<IniOptionID> IniNameRegistry::FindOption(std::wstring_view name) const
{
	WCHAR buffer[256];
	const auto normalized = NormalizeName(name, buffer, _countof(buffer));
	return normalized ? FindName(m_Options, *normalized) : std::nullopt;
}

IniNameRegistry& GetIniNameRegistry()
{
	static IniNameRegistry registry;
	return registry;
}
