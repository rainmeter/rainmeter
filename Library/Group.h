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

#ifndef __GROUP_H__
#define __GROUP_H__

#include <string>
#include <unordered_set>

class __declspec(novtable) Group
{
public:
	virtual ~Group() {}

	bool BelongsToGroup(const std::wstring& group) const;

protected:
	Group() {}

	void InitializeGroup(const std::wstring& groups);

private:
	std::wstring& CreateGroup(std::wstring& str) const;
	std::wstring VerifyGroup(const std::wstring& str) const;

	std::unordered_set<std::wstring> m_Groups;
	std::wstring m_OldGroups;

};

#endif
