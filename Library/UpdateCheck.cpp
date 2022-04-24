/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/FileUtil.h"
#include "../Common/StringUtil.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayIcon.h"
#include "UpdateCheck.h"
#include "Util.h"
#include "../Version.h"

#include <sstream>
#include <iomanip>

#include <bcrypt.h>
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0L)

namespace {

// To test a local |status.json| file, set |LOCAL_STATUS_FILE| to your local |status.json| file.
// Example: L"C:\\Rainmeter\\status.json"
// Remember to set this back to an empty string before committing any changes to this file!!
std::wstring LOCAL_STATUS_FILE = L"";

void ShowError(LPCWCHAR description)
{
	DWORD dwErr = GetLastError();
	if (dwErr == ERROR_INTERNET_EXTENDED_ERROR)
	{
		WCHAR szBuffer[1024];
		DWORD dwError, dwLen = 1024;
		LPCWSTR error = L"Unknown Error";
		if (InternetGetLastResponseInfo(&dwError, szBuffer, &dwLen))
		{
			error = szBuffer;
			dwErr = dwError;
		}

		LogErrorF(L"(%s) %s (ErrorCode=%i)", description, error, dwErr);
	}
	else
	{
		LPVOID lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS |
			FORMAT_MESSAGE_MAX_WIDTH_MASK,
			GetModuleHandle(L"wininet"),
			dwErr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			nullptr);

		LPCWSTR error = lpMsgBuf ? (WCHAR*)lpMsgBuf : L"Unknown error";
		LogErrorF(L"(%s) %s (ErrorCode=%i)", description, error, dwErr);

		if (lpMsgBuf) LocalFree(lpMsgBuf);
	}
}

}  // namespace

LPCWSTR Updater::s_UpdateURL = L"http://rainmeter.github.io/rainmeter/status.json";

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
	const bool debug = GetRainmeter().GetDebug();
	if (debug)
	{
		LogDebug(L"------------------------------");
		LogDebug(L"* Checking language status:");
	}

	if (m_Status.is_null() || m_Status.is_discarded())
	{
		if (debug)
		{
			LogError(L">>Status file: Invalid (may not have been downloaded)");
			LogDebug(L"------------------------------");
		}
		return;
	}

	bool obsolete = false;
	const auto lcid = (unsigned)GetRainmeter().GetResourceLCID();
	const auto& lang = m_Status["language"];
	if (lang.is_null() || lang.empty() ||
		!lang.is_structured() || !lang.is_array())
	{
		if (debug)
		{
			LogDebug(L">>Status file: Invalid (possibly corrupt?)");
			LogDebug(L"------------------------------");
		}
		return;
	}

	for (auto it = lang.cbegin(); it != lang.cend(); ++it)
	{
		const auto& id = it.value()["id"];
		if (id.is_number_unsigned() && id.get<unsigned>() == lcid)
		{
			if (debug) LogDebugF(L"  Language ID found: %u", lcid);

			const auto& obs = it.value()["obsolete"];
			if (obs.is_boolean())
			{
				obsolete = obs.get<bool>();

				if (debug) LogDebugF(L"  Language status: %s", obsolete ? L"Obsolete" : L"Current");
			}
			break;
		}
	}

	GetRainmeter().SetLanguageStatus(obsolete);
	if (debug) LogDebug(L"------------------------------");
}

void Updater::GetStatus(void* pParam)
{
	auto updater = (Updater*)pParam;

	const bool debug = GetRainmeter().GetDebug();
	if (debug)
	{
		LogDebug(L"------------------------------");
		LogDebug(L"* Checking for status updates:");
	}

	// Download the status file |status.json|
	std::string data;
	if (!DownloadStatusFile(data) || data.empty())
	{
		if (debug) ShowError(L">>Status file: Download failed");
		return;
	}

	if (debug) LogDebug(L"  Downloading status file: Success!");

	json status = json::parse(data, nullptr, false);
	if (status.is_null() || status.is_discarded())
	{
		if (debug) LogError(L">>Status file: Invalid status file");
		return;
	}

	if (debug) LogDebug(L"  Parsing status file: Success!");

	CheckVersion(status, updater->m_DownloadInstaller);
	updater->m_Status = std::move(status);
	updater->GetLanguageStatus();
}

bool Updater::DownloadStatusFile(std::string& data)
{
	LPCWSTR url = Updater::s_UpdateURL;
	if (_wcsnicmp(url, L"file://", 7) == 0)  // Local file
	{
		WCHAR path[MAX_PATH];
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
	BYTE* buffer = (BYTE*)malloc(bufferSize + 3);
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
		buffer = (BYTE*)realloc(buffer, bufferSize + 3);

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
	const bool debug = GetRainmeter().GetDebug();

	std::string release;
	if (!status["release"]["version"].empty())
	{
		release = status["release"]["version"].get<std::string>();
	}
	if (release.empty())
	{
		if (debug) LogError(L">>Status file: Parsing \"version\" failed");
		return;
	}

	const std::wstring tmpSz = StringUtil::Widen(release);
	LPCWSTR version = tmpSz.c_str();

	if (debug) LogDebugF(L"  Status file version: %s", version);

	const int availableVersion = ParseVersion(version);
	if (availableVersion > RAINMETER_VERSION)
	{
		const bool isDevBuild = []()
		{
			if (!LOCAL_STATUS_FILE.empty()) return false;
			return RAINMETER_VERSION == 0;
		} ();

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

		WCHAR tmp[32];
		LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
		GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", tmp, _countof(tmp), dataFile);

		// Show tray notification only once per new version
		const int lastVersion = ParseVersion(tmp);
		if (availableVersion > lastVersion)
		{
			WritePrivateProfileString(L"Rainmeter", L"LastCheck", version, dataFile);
			if (!isDevBuild && !downloadedNewVersion)
			{
				GetRainmeter().GetTrayIcon()->ShowUpdateNotification(version);
			}
		}

		if (!isDevBuild && downloadedNewVersion)
		{
			GetRainmeter().GetTrayIcon()->ShowInstallUpdateNotification(version);
		}
	}
}

int Updater::ParseVersion(LPCWSTR str)
{
	int version = _wtoi(str) * 1000000;
	const WCHAR* pos = wcschr(str, L'.');
	if (pos)
	{
		++pos;	// Skip .
		version += _wtoi(pos) * 1000;

		pos = wcschr(pos, '.');
		if (pos)
		{
			++pos;	// Skip .
			version += _wtoi(pos);
		}
	}
	return version;
}

bool Updater::DownloadNewVersion(json& status)
{
	const bool debug = GetRainmeter().GetDebug();

	std::string download_url;
	if (!status["release"]["download_url"].empty())
	{
		download_url = status["release"]["download_url"].get<std::string>();
	}
	if (download_url.empty())
	{
		if (debug) LogError(L">>Status file: Parsing \"download_url\" failed");
		return false;
	}

	std::string download_sha256;
	if (!status["release"]["download_sha256"].empty())
	{
		download_sha256 = status["release"]["download_sha256"].get<std::string>();
	}
	if (download_sha256.empty())
	{
		if (debug) LogError(L">>Status file: Parsing \"download_sha256\" failed");
		return false;
	}

	const std::wstring url = StringUtil::Widen(download_url);
	const std::wstring sha = StringUtil::Widen(download_sha256);
	std::wstring path = GetRainmeter().GetSettingsPath();
	path += L"Updates\\";

	std::wstring filename = url;
	std::wstring::size_type pos = filename.rfind(L'/');
	if (pos == std::wstring::npos)
	{
		if (debug) LogError(L">>Status file: Invalid \"download_url\"");
		return false;
	}
	filename = filename.substr(pos + 1);

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

	if (VerifyInstaller(path, filename, sha, true))
	{
		if (debug)
		{
			LogDebug(L"  Downloading new installer: Success!");
			LogDebug(L"  Verifying installer integrity: Success!");
			LogDebugF(L"  Installer location: %s", fullPath.c_str());
		}
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
	const bool debug = GetRainmeter().GetDebug();

	const std::wstring fullpath = path + filename;
	if (!PathFileExists(fullpath.c_str()))
	{
		if (debug) LogErrorF(L">>Verify installer: Installer file does not exist");
		return false;
	}

	// Dump installer contents into byte array
	size_t fileSize = 0;
	BYTE * buffer = FileUtil::ReadFullFile(fullpath, &fileSize).release();

	NTSTATUS status;
	BCRYPT_ALG_HANDLE provider = nullptr;
	BCRYPT_HASH_HANDLE hashHandle = nullptr;
	PBYTE hash = nullptr;
	DWORD hashLength = 0UL;
	DWORD resultLength = 0UL;

	auto cleanup = [&](LPCWSTR func, bool ret) -> bool
	{
		free(buffer);
		buffer = nullptr;
		if (!ret && func && debug) LogErrorF(L">>Verify installer error (%s): 0x%08x (%lu)", func, status, status);
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
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (DWORD i = 0UL; i < hashLength; ++i)
	{
		ss << std::setw(2) << static_cast<int>(hash[i]);
	}
	std::wstring hashStr = StringUtil::Widen(ss.str());
	cleanup(nullptr, true);

	bool isVerified = _wcsicmp(sha256.c_str(), hashStr.c_str()) == 0;
	if (isVerified && writeToDataFile)
	{
		LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
		WritePrivateProfileString(L"Rainmeter", L"InstallerName", filename.c_str(), dataFile);
		WritePrivateProfileString(L"Rainmeter", L"InstallerSha256", sha256.c_str(), dataFile);
	}

	if (!isVerified && debug)
	{
		LogError(L">>Verify installer error: Hashes do not match!");
		LogErrorF(L">>>Status file SHA256 hash:    %s", sha256.c_str());
		LogErrorF(L">>>Installer file SHA256 hash: %s", hashStr.c_str());
	}

	return isVerified;
}
