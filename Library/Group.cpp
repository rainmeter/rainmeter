/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Group.h"
#include "ConfigParser.h"

void Group::InitializeGroup(const std::wstring& groups)
{
	if (wcscmp(groups.c_str(), m_OldGroups.c_str()) != 0)
	{
		m_OldGroups = groups;
		m_Groups.clear();

		if (!groups.empty())
		{
			std::vector<std::wstring> vGroups = ConfigParser::Tokenize(groups, L"|");
			for (auto iter = vGroups.begin(); iter != vGroups.end(); ++iter)
			{
				m_Groups.insert(CreateGroup(*iter));
			}
		}
	}
}

bool Group::AddToGroup(const std::wstring& group)
{
	if (!group.empty() && !BelongsToGroup(group))
	{
		if (!m_OldGroups.empty())
		{
			m_OldGroups.append(1, L'|');
		}

		m_OldGroups.append(group);

		std::vector<std::wstring> vGroups = ConfigParser::Tokenize(group, L"|");
		for (auto iter = vGroups.begin(); iter != vGroups.end(); ++iter)
		{
			m_Groups.insert(m_Groups.end(), CreateGroup(*iter));
		}

		return true;
	}

	return false;
}

bool Group::BelongsToGroup(const std::wstring& group) const
{
	return (m_Groups.find(VerifyGroup(group)) != m_Groups.end());
}

std::wstring& Group::CreateGroup(std::wstring& str) const
{
	_wcsupr(&str[0]);
	return str;
}

std::wstring Group::VerifyGroup(const std::wstring& str) const
{
	std::wstring strTmp;

	std::wstring::size_type pos = str.find_first_not_of(L" \t\r\n");
	if (pos != std::wstring::npos)
	{
		// Trim white-space
		strTmp.assign(str, pos, str.find_last_not_of(L" \t\r\n") - pos + 1);

		CreateGroup(strTmp);
	}

	return strTmp;
}
