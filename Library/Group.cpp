/*
  Copyright (C) 2010 spx

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