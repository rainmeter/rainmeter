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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Group.h"
#include "ConfigParser.h"

/*
** CGroup
**
** The constructor
**
*/
CGroup::CGroup()
{
}

/*
** ~CGroup
**
** The destructor
**
*/
CGroup::~CGroup()
{
}

/*
** InitializeGroup
**
**
*/
void CGroup::InitializeGroup(const std::wstring& group)
{
	if (group != m_OldGroup)
	{
		m_OldGroup = group;
		m_Group.clear();

		std::vector<std::wstring> vGroup = CConfigParser::Tokenize(group, L"|");

		std::vector<std::wstring>::const_iterator iter = vGroup.begin();
		for ( ; iter != vGroup.end(); ++iter)
		{
			std::wstring group = CreateGroup(*iter);
			if (!group.empty())
			{
				m_Group.insert(group);
			}
		}
	}
}

/*
** BelongsToGroup
**
**
*/
bool CGroup::BelongsToGroup(const std::wstring& group)
{
	return (m_Group.find(CreateGroup(group)) != m_Group.end());
}

std::wstring CGroup::CreateGroup(const std::wstring& str)
{
	std::wstring strTmp = str;

	// Trim whitespace
	std::wstring::size_type pos = strTmp.find_last_not_of(L' ');
	if (pos != std::wstring::npos)
	{
		strTmp.erase(pos + 1);
		pos = strTmp.find_first_not_of(' ');
		if (pos != std::wstring::npos)
		{
			strTmp.erase(0, pos);
		}

		// Convert to lower
		std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);
	}
	else
	{
		strTmp.erase(strTmp.begin(), strTmp.end());
	}

	return strTmp;
}