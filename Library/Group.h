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

#ifndef __GROUP_H__
#define __GROUP_H__

#include <string>
#include <set>

class CGroup
{
public:
	bool BelongsToGroup(const std::wstring& group);

protected:
	CGroup() {}
	virtual ~CGroup() {}

	void InitializeGroup(const std::wstring& groups);

private:
	std::wstring CreateGroup(const std::wstring& str);

	std::set<std::wstring> m_Groups;
	std::wstring m_OldGroups;

};

#endif
