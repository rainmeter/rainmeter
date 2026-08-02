// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <cstdint>
#include <string>

class Platform
{
public:
	static Platform& GetInstance();

	bool Is64Bit() { return m_Is64Bit; }

	std::wstring GetName() { return m_Name; }
	std::wstring GetFriendlyName() { return m_FriendlyName; }
	std::wstring GetReleaseID() { return m_DisplayVersion; }  // Can be empty
	std::wstring GetRawVersion() { return m_RawVersion; }  // ex. 10.0.10240
	uint32_t GetBuildNumber();
	std::wstring GetProductName() { return m_ProductName; }
	std::wstring GetUserLanguage() { return m_UserLanguage; }

private:
	Platform();
	~Platform();

	Platform(const Platform& other) = delete;
	Platform& operator=(Platform other) = delete;

	void Initialize();

	bool m_Is64Bit;

	std::wstring m_Name;
	std::wstring m_FriendlyName;
	std::wstring m_DisplayVersion;
	std::wstring m_RawVersion;
	std::wstring m_ProductName;
	std::wstring m_UserLanguage;
};

// Convenience function.
inline Platform& GetPlatform() { return Platform::GetInstance(); }
