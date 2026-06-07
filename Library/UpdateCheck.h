/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __UPDATE_CHECK_H__
#define __UPDATE_CHECK_H__

#include "Net.h"
#include "inipp/inipp.h"

class Updater
{
public:
	static Updater& GetInstance();

	void CheckForUpdates(bool download);
	void CheckLanguageObsoleteStatus();

	static bool VerifyInstaller(const std::wstring& path, const std::wstring& fileName,
		const std::wstring& sha256, bool writeToDataFile);

private:
	Updater();
	~Updater();

	Updater(const Updater& other) = delete;
	Updater& operator=(Updater other) = delete;

	static void GetStatus(void* pParam);
	static bool DownloadStatusFile(std::string& data);
	static void CheckVersion(const inipp::Ini<char>& status, bool downloadNewVersion);

	static void StatusFetchResultCallback(const Net::Task* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode);
	static void InstallerFetchResultCallback(const Net::Task* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode);

	static bool VerifyInstallerHash(const BYTE* buffer, size_t size, const std::wstring& sha256);
	static bool VerifySignedInstaller(const std::wstring& file);

	Net::Task* m_FetchStatusTask;
	Net::Task* m_FetchInstallerTask;
	std::wstring m_InstallerPath;
	std::wstring m_InstallerFile;
	std::wstring m_InstallerHash;
	std::string m_ObsoleteLanguages;
	bool m_DownloadInstaller;

	static LPCWSTR s_UpdateURL;
	static LPCWSTR s_DownloadServer1;
	static LPCWSTR s_DownloadServer2;
};

// Convenience function.
inline Updater& GetUpdater() { return Updater::GetInstance(); }

#endif
