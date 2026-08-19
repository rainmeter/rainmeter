// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include <string_view>

bool ConsumeGroupSelector(std::wstring_view& name);

class __declspec(novtable) Group
{
public:
	Group() {}
	virtual ~Group() {}

	Group(const Group& other) = delete;
	Group& operator=(Group other) = delete;

	void InitializeGroup(const std::wstring& groups);

	const ankerl::unordered_dense::set<std::wstring>& GetGroups() const { return m_Groups; }

	bool AddToGroup(const std::wstring& group);
	bool BelongsToGroup(std::wstring_view group) const;

private:
	std::wstring& CreateGroup(std::wstring& str) const;
	std::wstring VerifyGroup(std::wstring_view str) const;

	ankerl::unordered_dense::set<std::wstring> m_Groups;
	std::wstring m_OldGroups;
};
