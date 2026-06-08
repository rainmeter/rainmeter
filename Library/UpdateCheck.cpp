/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/FileUtil.h"
#include "../Common/ParseUtil.h"
#include "../Common/StringUtil.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayIcon.h"
#include "UpdateCheck.h"
#include "Util.h"
#include "../Version.h"

#include "inipp/inipp.h"

#include <SoftPub.h>
#include <bcrypt.h>

namespace {

// To test a local release.ini file, set to e.g. L"C:\\Rainmeter\\release.ini"
std::wstring LOCAL_STATUS_FILE = L"";

HINTERNET GetInternetHandle()
{
	// This will be leaked on quit, but that's not a problem.
	static HINTERNET s_InternetHandle = InternetOpen(L"Rainmeter", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	return s_InternetHandle;
}

}  // namespace

LPCWSTR Updater::s_UpdateURL = L"https://version.rainmeter.net/rainmeter/v5.ini";
LPCWSTR Updater::s_DownloadServer1 = L"https://github.com/rainmeter/rainmeter/";
LPCWSTR Updater::s_DownloadServer2 = L"https://builds.rainmeter.net/";

Updater::Updater() :
	m_FetchStatusTask(nullptr),
	m_FetchInstallerTask(nullptr),
	m_DownloadInstaller(true)
{
	if (!LOCAL_STATUS_FILE.empty())
	{
		if (_wcsnicmp(LOCAL_STATUS_FILE.c_str(), L"file://", 7) != 0)
		{
			LOCAL_STATUS_FILE.insert(0, L"file://");
		}
		s_UpdateURL = LOCAL_STATUS_FILE.c_str();
	}
}

Updater::~Updater()
{
}

Updater& Updater::GetInstance()
{
	static Updater s_Updater;
	return s_Updater;
}

void Updater::CheckForUpdates(bool download)
{
	m_DownloadInstaller = download;

	if (!m_FetchStatusTask && !m_FetchInstallerTask)
	{
		m_FetchStatusTask = Net::FetchTask::Create((void*)this, s_UpdateURL, {}, GetInternetHandle(), INTERNET_FLAG_RESYNCHRONIZE, StatusFetchResultCallback);
	}
}

void Updater::CheckLanguageObsoleteStatus()
{
	if (m_ObsoleteLanguages.empty()) return;

	bool obsolete = false;
	const auto lcid = (unsigned)GetRainmeter().GetResourceLCID();

	auto obsoleteLanguages = ParseUtil::Tokenize(StringUtil::Widen(m_ObsoleteLanguages), L",");
	for (const auto& idString : obsoleteLanguages)
	{
		if (ParseUtil::ParseUInt(idString.c_str(), UINT32_MAX) == lcid)
		{
			obsolete = true;
			break;
		}
	}

	if (GetRainmeter().GetDebug())
	{
		WCHAR language[LOCALE_NAME_MAX_LENGTH] = { 0 };
		GetLocaleInfo(GetRainmeter().GetResourceLCID(), LOCALE_SENGLISHLANGUAGENAME, language, _countof(language));
		LogDebugF(L"Language status: %s (%s)", obsolete ? L"Obsolete" : L"Current", language);
	}

	GetRainmeter().SetLanguageStatus(obsolete);
}

void Updater::StatusFetchResultCallback(const Net::Task* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode)
{
	auto updater = (Updater*)requestor;
	if (updater->m_FetchStatusTask != fetchTask) return;
	updater->m_FetchStatusTask = nullptr;

	std::stringstream stream(std::string((char*)data, dataSize));
	inipp::Ini<char> ini;
	ini.parse(stream);

	std::string version, downloadUrl, downloadHash;
	const auto& section = ini.sections["Release"];
	if (!inipp::get_value(section, "Version", version) ||
			!inipp::get_value(section, "URL", downloadUrl) ||
			!inipp::get_value(section, "Hash", downloadHash) ||
			!inipp::get_value(section, "ObsoleteLanguages", updater->m_ObsoleteLanguages))
	{
		LogErrorF(L"Invalid update .ini");
		return;
	}

	const std::wstring url = StringUtil::Widen(downloadUrl);
	if (wcsncmp(url.c_str(), s_DownloadServer1, wcslen(s_DownloadServer1)) != 0 &&
			wcsncmp(url.c_str(), s_DownloadServer2, wcslen(s_DownloadServer2)) != 0)
	{
		LogErrorF(L"Invalid update .ini");
		return;
	}

	updater->m_AvailableVersion.Set(StringUtil::Widen(version));
	if (!updater->m_AvailableVersion.IsValid())
	{
		LogErrorF(L"Invalid update .ini");
		return;
	}

	VersionHelper::Version rainmeterVersion(APPVERSION);
	if (!rainmeterVersion.IsValid() || updater->m_AvailableVersion <= rainmeterVersion)
	{
		return;
	}

	const bool isDevBuild = !LOCAL_STATUS_FILE.empty() || RAINMETER_VERSION == 0;
	if (isDevBuild) return;

	LogNoticeF(L"New Rainmeter version %s available", updater->m_AvailableVersion.Get().c_str());
	GetRainmeter().SetNewVersion();

	std::wstring::size_type pos = url.rfind(L'/');
	if (pos == std::wstring::npos) return;

	std::wstring fileName = url.substr(pos + 1);

	const std::wstring path = GetRainmeter().GetSettingsPath() + L"Updates\\";
	const std::wstring fullPath = path + fileName;

	const std::wstring fileHash = StringUtil::Widen(downloadHash);

	if (PathFileExists(fullPath.c_str()))
	{
		if (VerifyInstaller(path, fileName, fileHash, true))
		{
			GetRainmeter().SetDownloadedNewVersion();
			return;
		}
		DeleteFile(fullPath.c_str());
	}

	if (updater->m_DownloadInstaller)
	{
		updater->m_FetchInstallerTask = Net::FetchTask::Create((void*)updater, url, {}, GetInternetHandle(), INTERNET_FLAG_RESYNCHRONIZE, InstallerFetchResultCallback);
		if (updater->m_FetchInstallerTask)
		{
			updater->m_InstallerPath = path;
			updater->m_InstallerFile = fileName;
			updater->m_InstallerHash = fileHash;
		}
	}
}

void Updater::InstallerFetchResultCallback(const Net::Task* fetchTask, void* requestor, BYTE* data, DWORD dataSize, DWORD errorCode)
{
	auto updater = (Updater*)requestor;
	if (updater->m_FetchInstallerTask != fetchTask) return;
	updater->m_FetchInstallerTask = nullptr;

	const std::wstring path(std::move(updater->m_InstallerPath));
	const std::wstring fileName(std::move(updater->m_InstallerFile));
	const std::wstring fileHash(std::move(updater->m_InstallerHash));
	const std::wstring fullPath = path + fileName;

	auto cleanup = [&]()
	{
		GetRainmeter().GetTrayIcon()->ShowUpdateNotification(updater->m_AvailableVersion.Get().c_str());

		DeleteFile(fullPath.c_str());
		System::RemoveFolder(path);
	};

	if (errorCode != ERROR_SUCCESS || !data)
	{
		LogErrorF(L"Installer download failed (ErrorCode=0x%08X): %s", errorCode, fullPath.c_str());
		cleanup();
		return;
	}

	if (!VerifyInstallerHash(data, dataSize, fileHash))
	{
		cleanup();
		return;
	}

	CreateDirectory(path.c_str(), nullptr);

	HANDLE file = CreateFile(fullPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		LogErrorF(L"Installer file creation failed: %s", fullPath.c_str());
		cleanup();
		return;
	}

	DWORD bytesWritten = 0UL;
	const BOOL writeSucceeded = WriteFile(file, data, dataSize, &bytesWritten, nullptr);
	CloseHandle(file);

	if (!writeSucceeded || bytesWritten != dataSize)
	{
		LogErrorF(L"Installer file write failed: %s", fullPath.c_str());
		cleanup();
		return;
	}

	if (!VerifySignedInstaller(fullPath))
	{
		cleanup();
		return;
	}

	LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
	WritePrivateProfileString(L"Rainmeter", L"InstallerName", fileName.c_str(), dataFile);
	WritePrivateProfileString(L"Rainmeter", L"InstallerSha256", fileHash.c_str(), dataFile);

	// Show tray notification only once per new version
	WCHAR buffer[32] = { 0 };
	GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", buffer, _countof(buffer), dataFile);
	VersionHelper::Version lastVersion(buffer);
	if (!lastVersion.IsValid()) lastVersion.Set(L"0");

	if (updater->m_AvailableVersion > lastVersion)
	{
		const auto* availableVersionString = updater->m_AvailableVersion.Get().c_str();
		WritePrivateProfileString(L"Rainmeter", L"LastCheck", availableVersionString, dataFile);
		GetRainmeter().GetTrayIcon()->ShowInstallUpdateNotification(availableVersionString);
	}

	updater->CheckLanguageObsoleteStatus();
	GetRainmeter().SetDownloadedNewVersion();
}

bool Updater::VerifyInstallerHash(const BYTE* buffer, size_t size, const std::wstring& sha256)
{
	NTSTATUS status = 0L;
	BCRYPT_ALG_HANDLE provider = nullptr;
	BCRYPT_HASH_HANDLE hashHandle = nullptr;
	PBYTE hash = nullptr;
	DWORD hashLength = 0UL;
	DWORD resultLength = 0UL;

	auto cleanup = [&](LPCWSTR func, bool ret) -> bool
	{
		if (!ret && func) LogErrorF(L"Verify installer error (%s): 0x%08x (%lu)", func, status, status);
		if (hash) HeapFree(GetProcessHeap(), 0UL, hash);
		if (hashHandle) BCryptDestroyHash(hashHandle);
		if (provider) BCryptCloseAlgorithmProvider(provider, 0UL);
		return ret;
	};

	status = BCryptOpenAlgorithmProvider(&provider, BCRYPT_SHA256_ALGORITHM, nullptr, 0UL);
	if (status != 0) return cleanup(L"OpenProvider", false);

	status = BCryptGetProperty(provider, BCRYPT_HASH_LENGTH, (PBYTE)&hashLength, sizeof(hashLength), &resultLength, 0UL);
	if (status != 0) return cleanup(L"GetProperty", false);

	hash = (PBYTE)HeapAlloc(GetProcessHeap(), 0UL, hashLength);
	if (!hash)
	{
		status = STATUS_NO_MEMORY;
		return cleanup(L"No Memory", false);
	}

	status = BCryptCreateHash(provider, &hashHandle, nullptr, 0UL, nullptr, 0UL, 0UL);
	if (status != 0) return cleanup(L"CreateHash", false);

	status = BCryptHashData(hashHandle, (PUCHAR)buffer, (ULONG)size, 0UL);
	if (status != 0) return cleanup(L"HashData", false);

	status = BCryptFinishHash(hashHandle, hash, hashLength, 0UL);
	if (status != 0) return cleanup(L"FinishHash", false);

	std::wstring hashStr;
	WCHAR hashChar[3] = { 0 };  // 2 chars + null terminator
	for (DWORD i = 0UL; i < hashLength; ++i)
	{
		_snwprintf_s(hashChar, _countof(hashChar), L"%02hhX", hash[i]);
		hashStr += hashChar;
	}

	if (_wcsicmp(sha256.c_str(), hashStr.c_str()) != 0)
	{
		LogErrorF(L"Installer hash mismatch: %s != %s", sha256.c_str(), hashStr.c_str());
		return cleanup(nullptr, false);
	}

	return cleanup(nullptr, true);
}

bool Updater::VerifyInstaller(const std::wstring& path, const std::wstring& fileName, const std::wstring& sha256, bool writeToDataFile)
{
	const std::wstring fullPath = path + fileName;
	size_t fileSize = 0ULL;
	std::unique_ptr<BYTE[]> buffer = FileUtil::ReadFullFile(fullPath, &fileSize);
	if (!buffer) return false;

	if (!VerifyInstallerHash(buffer.get(), fileSize, sha256))
	{
		return false;
	}

	const bool isVerified = VerifySignedInstaller(fullPath);
	if (isVerified && writeToDataFile)
	{
		LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
		WritePrivateProfileString(L"Rainmeter", L"InstallerName", fileName.c_str(), dataFile);
		WritePrivateProfileString(L"Rainmeter", L"InstallerSha256", sha256.c_str(), dataFile);
	}

	return isVerified;
}

bool Updater::VerifySignedInstaller(const std::wstring& file)
{
	WINTRUST_FILE_INFO fileData = { 0 };
	fileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	fileData.pcwszFilePath = file.c_str();
	fileData.hFile = nullptr;
	fileData.pgKnownSubject = nullptr;

	GUID guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	WINTRUST_DATA data = { 0 };
	data.cbStruct = sizeof(WINTRUST_DATA);
	data.pPolicyCallbackData = nullptr;
	data.pSIPClientData = nullptr;
	data.dwUIChoice = WTD_UI_NONE;
	data.fdwRevocationChecks = WTD_REVOKE_NONE;
	data.dwUnionChoice = WTD_CHOICE_FILE;
	data.pFile = &fileData;
	data.dwStateAction = WTD_STATEACTION_VERIFY;
	data.hWVTStateData = nullptr;
	data.dwProvFlags = WTD_SAFER_FLAG;  // See WinTrust.h WinVerifyTrust() notes
	data.pwszURLReference = nullptr;
	data.dwUIContext = WTD_UICONTEXT_EXECUTE;

	const LONG lStatus = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guid, &data);
	const bool isSuccessful = lStatus == ERROR_SUCCESS;
	if (!isSuccessful)
	{
		LogErrorF(L"Verifying installer failed with WinVerifyTrust error: %ld", lStatus);
	}

	// Release state data
	data.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guid, &data);

	return isSuccessful;
}
