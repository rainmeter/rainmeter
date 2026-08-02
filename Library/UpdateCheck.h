// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Net.h"
#include "../Common/Version.h"

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

	static void StatusFetchResultCallback(const Net::FetchTask* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode);
	static void InstallerFetchResultCallback(const Net::FetchTask* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode);

	static bool VerifyInstallerHash(const BYTE* buffer, size_t size, const std::wstring& sha256);
	static bool VerifySignedInstaller(const std::wstring& file);

	Net::FetchTask* m_FetchStatusTask;
	Net::FetchTask* m_FetchInstallerTask;

	std::wstring m_InstallerPath;
	std::wstring m_InstallerFile;
	std::wstring m_InstallerHash;
	std::string m_ObsoleteLanguages;
	VersionHelper::Version m_AvailableVersion;

	bool m_DownloadInstaller;

	static LPCWSTR s_UpdateURL;
	static LPCWSTR s_DownloadServer1;
	static LPCWSTR s_DownloadServer2;
};

// Convenience function.
inline Updater& GetUpdater() { return Updater::GetInstance(); }
