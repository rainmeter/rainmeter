/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __GROUP_H__
#define __GROUP_H__

#include <string>
#include <unordered_set>

class __declspec(novtable) Group
{
public:
	Group() {}
	virtual ~Group() {}

	Group(const Group& other) = delete;
	Group& operator=(Group other) = delete;

	void InitializeGroup(const std::wstring& groups);

	const std::unordered_set<std::wstring>& GetGroups() const { return m_Groups; }

	bool AddToGroup(const std::wstring& group);
	bool BelongsToGroup(const std::wstring& group) const;

private:
	std::wstring& CreateGroup(std::wstring& str) const;
	std::wstring VerifyGroup(const std::wstring& str) const;

	std::unordered_set<std::wstring> m_Groups;
	std::wstring m_OldGroups;

};

#endif
