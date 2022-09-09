/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_PLATFORM_H_
#define RM_COMMON_PLATFORM_H_

#include <string>

class Platform
{
public:
	static Platform& GetInstance();

	bool Is64BitWindows() { return m_Is64BitWindows; }

	std::wstring GetName() { return m_Name; }
	std::wstring GetFriendlyName() { return m_FriendlyName; }
	std::wstring GetReleaseID() { return m_DisplayVersion; }  // Can be empty
	std::wstring GetBuildNumber() { return m_BuildNumber; }
	std::wstring GetProductName() { return m_ProductName; }
	std::wstring GetUserLanguage() { return m_UserLanguage; }

private:
	Platform();
	~Platform();

	Platform(const Platform& other) = delete;
	Platform& operator=(Platform other) = delete;

	void Initialize();

	bool m_Is64BitWindows;

	std::wstring m_Name;
	std::wstring m_FriendlyName;
	std::wstring m_DisplayVersion;
	std::wstring m_BuildNumber;
	std::wstring m_ProductName;
	std::wstring m_UserLanguage;
};

// Convenience function.
inline Platform& GetPlatform() { return Platform::GetInstance(); }

/*
namespace Platform {

LPCWSTR GetPlatformName();
std::wstring GetPlatformReleaseID();
std::wstring GetPlatformFriendlyName();
std::wstring GetPlatformUserLanguage();
bool Is64BitWindows();

}  // namespace Platform
*/

#endif
