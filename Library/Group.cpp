// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Group.h"
#include "../Common/StringParser.h"

bool ConsumeGroupSelector(std::wstring_view& name)
{
	constexpr std::wstring_view groupSelector = L"Group=";
	if (!name.starts_with(groupSelector)) return false;

	name.remove_prefix(groupSelector.length());
	return true;
}

void Group::InitializeGroup(const std::wstring& groups)
{
	if (wcscmp(groups.c_str(), m_OldGroups.c_str()) != 0)
	{
		m_OldGroups = groups;
		m_Groups.clear();

		if (!groups.empty())
		{
			StringParser::ForEachToken(groups, L'|', [&](std::wstring_view token)
			{
				std::wstring group(token);
				m_Groups.insert(CreateGroup(group));
			});
		}
	}
}

bool Group::AddToGroup(const std::wstring& group)
{
	if (!group.empty() && m_Groups.find(VerifyGroup(group)) == m_Groups.end())
	{
		if (!m_OldGroups.empty())
		{
			m_OldGroups.append(1, L'|');
		}

		m_OldGroups.append(group);

		StringParser::ForEachToken(group, L'|', [&](std::wstring_view token)
		{
			std::wstring newGroup(token);
			m_Groups.insert(m_Groups.end(), CreateGroup(newGroup));
		});

		return true;
	}

	return false;
}

bool Group::BelongsToGroup(std::wstring_view group) const
{
	enum class MatchMode
	{
		Single,
		Any,
		All
	};

	MatchMode mode = MatchMode::Single;
	StringParser parser(group);
	bool matches = false;
	bool first = true;
	while (true)
	{
		WCHAR delimiter;
		const auto token = parser.ConsumeUntilOrRest(L"&|", StringParser::SkipWhitespace, &delimiter);
		if (delimiter)
		{
			const MatchMode nextMode = delimiter == L'&' ? MatchMode::All : MatchMode::Any;
			if (mode != MatchMode::Single && mode != nextMode) return false;

			mode = nextMode;
		}

		const auto name = VerifyGroup(token);
		const bool belongs = !name.empty() && m_Groups.find(name) != m_Groups.end();
		if (first)
		{
			matches = belongs;
		}
		else
		{
			matches = mode == MatchMode::All ? matches && belongs : matches || belongs;
		}

		if (!delimiter) return matches;

		first = false;
	}
}

std::wstring& Group::CreateGroup(std::wstring& str) const
{
	_wcsupr(&str[0]);
	return str;
}

std::wstring Group::VerifyGroup(std::wstring_view str) const
{
	std::wstring strTmp;

	const auto pos = str.find_first_not_of(L" \t\r\n");
	if (pos != std::wstring_view::npos)
	{
		// Trim white-space
		strTmp.assign(str, pos, str.find_last_not_of(L" \t\r\n") - pos + 1);

		CreateGroup(strTmp);
	}

	return strTmp;
}
