/* Copyright (C) 2023 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_VERSION_H_
#define RM_COMMON_VERSION_H_

#include <string>

namespace VersionHelper {

class Version
{
public:
	Version() : m_Version(), m_IsValid(false) { }
	~Version() { }

	Version(std::wstring v) : m_Version(std::move(v)), m_IsValid(false) { Validate(); }
	Version(const std::initializer_list<std::wstring> args) : m_Version(), m_IsValid(false) { for (const auto& arg : args) { Append(arg); } }

	void Set(std::wstring v) { m_Version = std::move(v); Validate(); }
	void Append(std::wstring v) { if (!m_Version.empty()) { m_Version += L'.'; } m_Version += v; Validate(); }

	std::wstring Get() { return m_Version; }

	bool IsValid() const { return m_IsValid; }

	bool operator <  (const Version& rhs) { return Compare(rhs.m_Version) == -1; }
	bool operator >  (const Version& rhs) { return Compare(rhs.m_Version) == +1; }
	bool operator <= (const Version& rhs) { return Compare(rhs.m_Version) != +1; }
	bool operator >= (const Version& rhs) { return Compare(rhs.m_Version) != -1; }
	bool operator == (const Version& rhs) { return Compare(rhs.m_Version) ==  0; }
	bool operator != (const Version& rhs) { return Compare(rhs.m_Version) !=  0; }

private:
	Version(const Version& other) = delete;
	Version& operator=(Version other) = delete;

	void Validate() { m_IsValid = (!m_Version.empty() && m_Version.find_first_not_of(L".0123456789") == std::wstring::npos); }

	int Compare(const std::wstring& version2) const
	{
		const size_t size1 = m_Version.size(), size2 = version2.size();
		size_t i = 0ULL, j = 0ULL;

		while (i < size1 || j < size2)
		{
			int m = 0, n = 0;

			while (i < size1 && m_Version[i] != L'.') { m = (m * 10) + (m_Version[i] - L'0'); ++i; }
			while (j < size2 && version2[j] != L'.') { n = (n * 10) + (version2[j] - L'0'); ++j; }

			if (m < n) return -1;
			if (m > n) return +1;

			++i;
			++j;
		}
		return 0;
	}

	std::wstring m_Version;
	bool m_IsValid;
};

}  // namespace VersionHelper

#endif
