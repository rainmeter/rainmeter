/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __UPDATE_CHECK_H__
#define __UPDATE_CHECK_H__

#define JSON_NOEXCEPTION

#include "json/json.hpp"

using nlohmann::json;

class Updater
{
public:
	static Updater& GetInstance();

	void CheckForUpdates(bool download);
	void GetLanguageStatus();

	static bool VerifyInstaller(const std::wstring& path, const std::wstring& filename,
		const std::wstring& sha256, bool writeToDataFile);

private:
	Updater();
	~Updater();

	Updater(const Updater& other) = delete;
	Updater& operator=(Updater other) = delete;

	static void GetStatus(void* pParam);
	static bool DownloadStatusFile(std::string& data);
	static void CheckVersion(json& status, bool downloadNewVersion);
	static int ParseVersion(LPCWSTR str);
	static bool DownloadNewVersion(json& status);

	json m_Status;
	bool m_DownloadInstaller;

	static LPCWSTR s_UpdateURL;
};

// Convenience function.
inline Updater& GetUpdater() { return Updater::GetInstance(); }

#endif
