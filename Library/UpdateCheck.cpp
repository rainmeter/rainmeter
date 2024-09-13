/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/FileUtil.h"
#include "../Common/Platform.h"
#include "../Common/StringUtil.h"
#include "../Common/Version.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayIcon.h"
#include "UpdateCheck.h"
#include "Util.h"
#include "../Version.h"

#include <SoftPub.h>
#include <bcrypt.h>
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0L)

namespace {

// To test a local |status.json| file, set |LOCAL_STATUS_FILE| to your local |status.json| file.
// Example: L"C:\\Rainmeter\\status.json"
// Remember to set this back to an empty string before committing any changes to this file!!
std::wstring LOCAL_STATUS_FILE = L"";

}  // namespace

bool Updater::s_IsInDebugMode = false;
LPCWSTR Updater::s_UpdateURL = L"https://version.rainmeter.net/rainmeter/status.json";
LPCWSTR Updater::s_DownloadServer1 = L"https://github.com/rainmeter/rainmeter/";
LPCWSTR Updater::s_DownloadServer2 = L"https://builds.rainmeter.net/";

Updater::Updater() : 
	m_Status(nullptr),
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

	s_IsInDebugMode = GetRainmeter().GetDebug();
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
	_beginthread(GetStatus, 0, this);
}

void Updater::GetLanguageStatus()
{
	LogIfInDebugMode(L"------------------------------");
	LogIfInDebugMode(L"* Checking language status:");

	if (m_Status.is_null() || m_Status.is_discarded())
	{
		LogError(L">>Status file: Invalid (may not have been downloaded)");
		LogIfInDebugMode(L"------------------------------");
		return;
	}

	bool obsolete = false;
	const auto lcid = (unsigned)GetRainmeter().GetResourceLCID();
	const auto& lang = m_Status["language"];
	if (lang.is_null() || lang.empty() ||
		!lang.is_structured() || !lang.is_array())
	{
		LogError(L">>Status file: Invalid (possibly corrupt?)");
		LogIfInDebugMode(L"------------------------------");
		return;
	}

	for (auto& it = lang.cbegin(); it != lang.cend(); ++it)
	{
		const auto& id = it.value()["id"];
		if (id.is_number_unsigned() && id.get<unsigned>() == lcid)
		{
			LogIfInDebugModeF(L"  Language ID found: %u", lcid);

			const auto& obs = it.value()["obsolete"];
			if (obs.is_boolean())
			{
				obsolete = obs.get<bool>();

				LogIfInDebugModeF(L"  Language status: %s", obsolete ? L"Obsolete" : L"Current");
			}
			break;
		}
	}

	GetRainmeter().SetLanguageStatus(obsolete);
	LogIfInDebugMode(L"------------------------------");
}

void Updater::GetStatus(void* pParam)
{
	auto updater = (Updater*)pParam;

	LogIfInDebugMode(L"------------------------------");
	LogIfInDebugMode(L"* Checking for status updates:");

	// Download the status file |status.json|
	std::string data;
	if (!DownloadStatusFile(data) || data.empty())
	{
		if (s_IsInDebugMode) ShowInternetError(L">>Status file: Download failed");
		return;
	}

	LogIfInDebugMode(L"  Downloading status file: Success!");

	json status = json::parse(data, nullptr, false);
	if (status.is_null() || status.is_discarded())
	{
		LogError(L">>Status file: Invalid status file");
		return;
	}

	LogIfInDebugMode(L"  Parsing status file: Success!");

	CheckVersion(status, updater->m_DownloadInstaller);
	updater->m_Status = std::move(status);
	updater->GetLanguageStatus();
}

bool Updater::DownloadStatusFile(std::string& data)
{
	LPCWSTR url = Updater::s_UpdateURL;
	if (_wcsnicmp(url, L"file://", 7) == 0)  // Local file
	{
		WCHAR path[MAX_PATH] = { 0 };
		DWORD pathLength = _countof(path);
		HRESULT hr = PathCreateFromUrl(url, path, &pathLength, 0UL);
		if (FAILED(hr))
		{
			return false;
		}

		size_t fileSize = 0ULL;
		BYTE* buffer = FileUtil::ReadFullFile(path, &fileSize).release();

		data = (char*)buffer;
		free(buffer);
		buffer = nullptr;
		return true;
	}

	HINTERNET hRootHandle = InternetOpen(L"Rainmeter", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0UL);
	if (!hRootHandle) return false;

	HINTERNET hUrlDump = InternetOpenUrl(hRootHandle, url, nullptr, 0UL, INTERNET_FLAG_RESYNCHRONIZE, 0UL);
	if (!hUrlDump)
	{
		InternetCloseHandle(hRootHandle);
		return false;
	}

	// The |status.json| file should be UTF-8 (or ANSI) encoded, however, allocate the buffer
	// with 3 extra bytes for triple null termination in case the encoding changes.
	const DWORD CHUNK_SIZE = 8192UL;
	DWORD bufferSize = CHUNK_SIZE;
	BYTE* buffer = (BYTE*)malloc(bufferSize + 3UL);
	DWORD dataSize = 0UL;

	// Read the data.
	do
	{
		DWORD readSize = 0UL;
		if (!InternetReadFile(hUrlDump, buffer + dataSize, bufferSize - dataSize, &readSize))
		{
			free(buffer);
			buffer = nullptr;
			InternetCloseHandle(hUrlDump);
			InternetCloseHandle(hRootHandle);
			return false;
		}
		else if (readSize == 0UL)
		{
			// All data read.
			break;
		}

		dataSize += readSize;

		bufferSize += CHUNK_SIZE;

		BYTE* oldBuffer = buffer;
		if ((buffer = (BYTE*)realloc(buffer, bufferSize + 3UL)) == nullptr)
		{
			free(oldBuffer);  // In case realloc fails
			oldBuffer = nullptr;
			InternetCloseHandle(hUrlDump);
			InternetCloseHandle(hRootHandle);
			return false;
		}

	} while (true);

	InternetCloseHandle(hUrlDump);
	InternetCloseHandle(hRootHandle);

	// Triple null terminate the buffer.
	buffer[dataSize] = 0;
	buffer[dataSize + 1] = 0;
	buffer[dataSize + 2] = 0;

	data = (char*)buffer;
	free(buffer);
	buffer = nullptr;
	return true;
}

void Updater::CheckVersion(json& status, bool downloadNewVersion)
{
	std::wstring buffer;

	auto getStatusValue = [&buffer](json& key) -> bool
	{
		buffer.clear();
		if (!key.empty())
		{
			std::string value = key.get<std::string>();
			if (!value.empty())
			{
				buffer = StringUtil::Widen(value);
				return true;
			}
		}
		return false;
	};

	// Get "release version" from the status file
	if (!getStatusValue(status["release"]["version"]))
	{
		buffer = StringUtil::Widen(status["release"].dump());
		LogErrorF(L">>Status File: Parsing error: release: %s", buffer.c_str());
		return;
	}

	VersionHelper::Version availableVersion = { buffer };
	if (!availableVersion.IsValid())
	{
		LogErrorF(L">>Status File: Invalid \"version\": %s", buffer.c_str());
		return;
	}

	LogIfInDebugModeF(L"  Status file version: %s", buffer.c_str());

	// Get "Rainmeter version"
	VersionHelper::Version rainmeterVersion = { APPVERSION };
	if (!rainmeterVersion.IsValid())
	{
		LogErrorF(L">>Status File: Invalid Rainmeter version: %s", APPVERSION);  // Probably never reach this
		return;
	}

	if (availableVersion > rainmeterVersion)
	{
		const bool isDevBuild = []()
		{
			if (!LOCAL_STATUS_FILE.empty()) return true;
			return RAINMETER_VERSION == 0;
			return false;
		} ();

		// Get "minimum Windows version" from the status file
		if (!getStatusValue(status["release"]["minimum_windows"]["version"]))
		{
			buffer = StringUtil::Widen(status["release"]["minimum_windows"].dump());
			LogErrorF(L">>Status File: Error parsing \"version\": minimum_windows: %s", buffer.c_str());
			return;
		}

		VersionHelper::Version statusWinVer = { buffer };
		if (!statusWinVer.IsValid())
		{
			LogErrorF(L">>Status File: Invalid \"minimum_windows\" version: %s", buffer.c_str());  // Probably never reach this
			return;
		}

		// Get the user's Windows version
		VersionHelper::Version systemWinVer = { GetPlatform().GetRawVersion() };
		if (!systemWinVer.IsValid())
		{
			LogErrorF(L">>Status File: Invalid system version: %s", systemWinVer.Get().c_str());
			return;
		}

		// Check if the new Rainmeter version requires a Windows version that is newer than the user's installed version
		if (statusWinVer > systemWinVer)
		{
			if (!getStatusValue(status["release"]["minimum_windows"]["name"]))
			{
				buffer = StringUtil::Widen(status["release"]["minimum_windows"].dump());
				LogErrorF(L">>Status File: Error parsing \"name\": minimum_windows: %s", buffer.c_str());

				buffer = L"Windows build";  // For error message below
			}

			LogNoticeF(
				L"* A new version of Rainmeter is available, however, your system does not meet the minimum required Windows version."
				L" Rainmeter %s requires \"%s (%s)\" or higher.",
				availableVersion.Get().c_str(),
				buffer.c_str(),
				statusWinVer.Get().c_str());
			return;
		}

		if (!isDevBuild)
		{
			LogNotice(L"* New Rainmeter version available!");
			GetRainmeter().SetNewVersion();
		}

		const bool downloadedNewVersion = [&]()
		{
			if (isDevBuild) return false;
			if (downloadNewVersion) return DownloadNewVersion(status);
			return false;
		} ();

		WCHAR tmp[32] = { 0 };
		LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
		GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", tmp, _countof(tmp), dataFile);

		// Show tray notification only once per new version
		VersionHelper::Version lastVersion = { tmp };
		if (!lastVersion.IsValid())
		{
			lastVersion.Set(L"0");  // "LastCheck" probably doesn't exist yet.
		}

		buffer = availableVersion.Get();
		if (availableVersion > lastVersion)
		{
			WritePrivateProfileString(L"Rainmeter", L"LastCheck", buffer.c_str(), dataFile);
			if (!isDevBuild && !downloadedNewVersion)
			{
				GetRainmeter().GetTrayIcon()->ShowUpdateNotification(buffer.c_str());
			}
		}

		if (!isDevBuild && downloadedNewVersion)
		{
			GetRainmeter().GetTrayIcon()->ShowInstallUpdateNotification(buffer.c_str());
		}
	}
}

bool Updater::DownloadNewVersion(json& status)
{
	std::string download_url;
	if (!status["release"]["download_url"].empty())
	{
		download_url = status["release"]["download_url"].get<std::string>();
	}
	if (download_url.empty())
	{
		LogError(L">>Status file: Parsing \"download_url\" failed");
		return false;
	}

	std::string download_sha256;
	if (!status["release"]["download_sha256"].empty())
	{
		download_sha256 = status["release"]["download_sha256"].get<std::string>();
	}
	if (download_sha256.empty())
	{
		LogError(L">>Status file: Parsing \"download_sha256\" failed");
		return false;
	}

	const std::wstring url = StringUtil::Widen(download_url);
	const std::wstring sha = StringUtil::Widen(download_sha256);
	std::wstring path = GetRainmeter().GetSettingsPath();
	path += L"Updates\\";

	// Check to see if installer download location is correct
	if (_wcsnicmp(url.c_str(), s_DownloadServer1, wcslen(s_DownloadServer1)) != 0 &&
		_wcsnicmp(url.c_str(), s_DownloadServer2, wcslen(s_DownloadServer2)) != 0)
	{
		LogErrorF(L">>Status file: Invalid \"download_url\": %s", url.c_str());
		return false;
	}

	std::wstring filename = url;
	std::wstring::size_type pos = filename.rfind(L'/');
	if (pos == std::wstring::npos)
	{
		LogError(L">>Status file: Invalid \"download_url\"");
		return false;
	}
	filename = filename.substr(pos + 1);
	if (_wcsnicmp(filename.c_str(), L"Rainmeter", 9) != 0)
	{
		LogErrorF(L">>Status file: Invalid installer name: %s", filename.c_str());
		return false;
	}

	const std::wstring fullPath = path + filename;
	if (PathFileExists(fullPath.c_str()))
	{
		if (VerifyInstaller(path, filename, sha, true))
		{
			GetRainmeter().SetDownloadedNewVersion();
			return true;
		}
		DeleteFile(fullPath.c_str());
	}

	CreateDirectory(path.c_str(), nullptr);

	// Download the installer
	HRESULT resultCoInitialize = CoInitialize(nullptr);
	auto cleanup = [&](bool ret) -> bool
	{
		if (SUCCEEDED(resultCoInitialize))
		{
			CoUninitialize();
		}
		return ret;
	};

	HRESULT result = URLDownloadToFile(nullptr, url.c_str(), fullPath.c_str(), 0UL, nullptr);
	if (result != S_OK)
	{
		LogErrorF(L">>New installer download failed (res=0x%08X, COM=0x%08X): %s",
			result, resultCoInitialize, url.c_str());
		return cleanup(false);
	}

	LogIfInDebugMode(L"  Downloading new installer: Success!");

	if (VerifyInstaller(path, filename, sha, true))
	{
		GetRainmeter().SetDownloadedNewVersion();
		return cleanup(true);
	}

	// Installer not verified, remove file and |Updates| folder
	DeleteFile(fullPath.c_str());
	System::RemoveFolder(path);

	return cleanup(false);
}

bool Updater::VerifyInstaller(const std::wstring& path, const std::wstring& filename, const std::wstring& sha256, bool writeToDataFile)
{
	const std::wstring fullpath = path + filename;
	if (!PathFileExists(fullpath.c_str()))
	{
		LogError(L">>Verify installer: Installer file does not exist");
		return false;
	}

	// Dump installer contents into byte array
	size_t fileSize = 0ULL;
	BYTE* buffer = FileUtil::ReadFullFile(fullpath, &fileSize).release();

	NTSTATUS status = 0L;
	BCRYPT_ALG_HANDLE provider = nullptr;
	BCRYPT_HASH_HANDLE hashHandle = nullptr;
	PBYTE hash = nullptr;
	DWORD hashLength = 0UL;
	DWORD resultLength = 0UL;

	auto cleanup = [&](LPCWSTR func, bool ret) -> bool
	{
		free(buffer);
		buffer = nullptr;
		if (!ret && func) LogErrorF(L">>Verify installer error (%s): 0x%08x (%lu)", func, status, status);
		if (hash) HeapFree(GetProcessHeap(), 0UL, hash);
		if (hashHandle) BCryptDestroyHash(hashHandle);
		if (provider) BCryptCloseAlgorithmProvider(provider, 0UL);
		return ret;
	};

	status = BCryptOpenAlgorithmProvider(&provider, BCRYPT_SHA256_ALGORITHM, nullptr, 0UL);
	if (!NT_SUCCESS(status)) return cleanup(L"OpenProvider", false);

	status = BCryptGetProperty(provider, BCRYPT_HASH_LENGTH, (PBYTE)&hashLength, sizeof(hashLength), &resultLength, 0UL);
	if (!NT_SUCCESS(status)) return cleanup(L"GetProperty", false);

	hash = (PBYTE)HeapAlloc(GetProcessHeap(), 0UL, hashLength);
	if (!hash)
	{
		status = STATUS_NO_MEMORY;
		return cleanup(L"No Memory", false);
	}

	status = BCryptCreateHash(provider, &hashHandle, nullptr, 0UL, nullptr, 0UL, 0UL);
	if (!NT_SUCCESS(status)) return cleanup(L"CreateHash", false);

	status = BCryptHashData(hashHandle, buffer, (ULONG)fileSize, 0UL);
	if (!NT_SUCCESS(status)) return cleanup(L"HashData", false);

	status = BCryptFinishHash(hashHandle, hash, hashLength, 0UL);
	if (!NT_SUCCESS(status)) return cleanup(L"FinishHash", false);

	// Convert the hash to a hex string
	std::wstring hashStr;
	WCHAR hashChar[3] = { 0 };  // 2 chars + null terminator
	for (DWORD i = 0UL; i < hashLength; ++i)
	{
		_snwprintf_s(hashChar, _countof(hashChar), L"%02hhX", hash[i]);
		hashStr += hashChar;
	}

	// Make sure hash string has the right length for the algorithm.
	// |hashLength| should be 32. Hex string should be 64.
	if (hashStr.length() != (size_t)(hashLength * 2))
	{
		LogErrorF(L">>Verify installer error: Invalid hash: %s (%llu)",
			hashStr.empty() ? L"[empty]" : hashStr.c_str(), hashStr.length());
		return false;
	}

	// Clean up bcrypt info
	cleanup(nullptr, true);

	// Verify hash and installer signature
	bool isHashVerified = _wcsicmp(sha256.c_str(), hashStr.c_str()) == 0;
	if (isHashVerified)
	{
		LogIfInDebugMode(L"  Verifying installer integrity: Success!");
	}
	else
	{
		LogError(L">>Verify installer error: Hashes do not match!");
		LogErrorF(L">>>Status file SHA256 hash:    %s", sha256.c_str());
		LogErrorF(L">>>Installer file SHA256 hash: %s", hashStr.c_str());
		return false;
	}

	bool isVerified = isHashVerified && VerifySignedInstaller(fullpath);
	if (isVerified)
	{
		if (writeToDataFile)
		{
			LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
			WritePrivateProfileString(L"Rainmeter", L"InstallerName", filename.c_str(), dataFile);
			WritePrivateProfileString(L"Rainmeter", L"InstallerSha256", sha256.c_str(), dataFile);
		}

		LogIfInDebugModeF(L"  Installer location: %s", fullpath.c_str());
	}

	return isVerified;
}

bool Updater::VerifySignedInstaller(const std::wstring& file)
{
	bool isSuccessful = false;

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

	LONG lStatus = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guid, &data);
	if (lStatus == ERROR_SUCCESS)
	{
		isSuccessful = true;
		LogIfInDebugMode(L"  Verifying installer signature: Success!");
	}
	else if (s_IsInDebugMode)
	{
		switch (lStatus)
		{
		case TRUST_E_NOSIGNATURE:          ShowWinTrustError(L">>Installer signature is missing or invalid"); break;
		case TRUST_E_EXPLICIT_DISTRUST:    ShowWinTrustError(L">>Installer signature explicitly disallowed"); break;
		case CRYPT_E_SECURITY_SETTINGS:    ShowWinTrustError(L">>Installer signature trusted, but admin policy has disabled user trust"); break;

		// It is unclear if the following cases will ever be returned based on the options specified in |data| (WINTRUST_DATA)
		case TRUST_E_PROVIDER_UNKNOWN:     ShowWinTrustError(L">>Installer signature provider error"); break;
		case TRUST_E_ACTION_UNKNOWN:       ShowWinTrustError(L">>Installer signature verification action unknown"); break;
		case TRUST_E_SUBJECT_FORM_UNKNOWN: ShowWinTrustError(L">>Installer signature provider specified form is unknown"); break;

		// It is unclear if this error will be returned based on |data.dwUIChoice = WTD_UI_NONE|.
		// Also, due to |CVE-2013-3900|, this error might happen on verification actions without the registry key(s):
		//   32/64-bit Windows:   [HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\Wintrust\Config] "EnableCertPaddingCheck"="1"
		//   64-bit Windows only: [HKEY_LOCAL_MACHINE\Software\Wow6432Node\Microsoft\Cryptography\Wintrust\Config] "EnableCertPaddingCheck"="1"
		// >> These keys are OPT-IN on all Windows version from XP+ <<
		// WinTrustVerify:            https://learn.microsoft.com/en-us/windows/win32/api/wintrust/nf-wintrust-winverifytrust#return-value
		// Original advisory:         https://learn.microsoft.com/en-us/security-updates/SecurityAdvisories/2014/2915720
		// Original bulletin:         https://learn.microsoft.com/en-us/security-updates/SecurityBulletins/2013/ms13-098
		// CVE reissue update (2022): https://msrc.microsoft.com/update-guide/vulnerability/CVE-2013-3900
		case TRUST_E_SUBJECT_NOT_TRUSTED: ShowWinTrustError(L">>Installer signature present, but not trusted"); break;  // User clicked "No" in the popup UI

		// Default errors provided by the policy provider
		default: ShowWinTrustError(L">>Installer signature verification error"); break;
		}
	}

	// Release state data
	data.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guid, &data);

	return isSuccessful;
}

void Updater::ShowInternetError(WCHAR* description)
{
	DWORD dwErr = GetLastError();
	if (dwErr == ERROR_INTERNET_EXTENDED_ERROR)
	{
		WCHAR szBuffer[1024] = { 0 };
		DWORD dwError = 0UL, dwLen = _countof(szBuffer);
		LPCWSTR error = L"Unknown Error";

		if (InternetGetLastResponseInfo(&dwError, szBuffer, &dwLen) == TRUE)
		{
			error = szBuffer;
			dwErr = dwError;
		}

		LogErrorF(L"%s: %s (ErrorCode=0x%X)", description, error, dwErr);
		return;
	}

	ShowError(description, dwErr, GetModuleHandle(L"wininet"));
}

void Updater::ShowWinTrustError(WCHAR* description)
{
	ShowError(description, GetLastError(), GetModuleHandle(L"Wintrust"));
}

void Updater::ShowError(WCHAR* description, DWORD dwErr, HMODULE module)
{
	LPVOID lpMsgBuf = nullptr;

	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD res = FormatMessage(flags, module, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0UL, nullptr);
	if (res == 0UL || res == ERROR_RESOURCE_NOT_FOUND)
	{
		// Try again if the message isn't found in the module (or if there is no message table)
		flags ^= FORMAT_MESSAGE_FROM_HMODULE;
		res = FormatMessage(flags, nullptr, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0UL, nullptr);
	}

	LPCWSTR error = lpMsgBuf && res > 0UL ? (WCHAR*)lpMsgBuf : L"Unknown error";
	LogErrorF(L"%s: %s (ErrorCode=0x%X)", description, error, dwErr);

	if (lpMsgBuf) LocalFree(lpMsgBuf);
}

void Updater::LogIfInDebugMode(LPCWSTR message)
{
	if (s_IsInDebugMode)
	{
		LogDebug(message);
	}
}

void Updater::LogIfInDebugModeF(LPCWSTR format, ...)
{
	if (s_IsInDebugMode)
	{
		va_list args;
		va_start(args, format);
		GetLogger().LogVF(Logger::Level::Debug, L"", format, args);
		va_end(args);
	}
}
