/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Util.h"
#include "Rainmeter.h"
#include "TrayIcon.h"
#include "UpdateCheck.h"
#include "../Version.h"

namespace {

void ShowError(WCHAR* description)
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

LPCWSTR Updater::c_UpdateURL = L"http://rainmeter.github.io/rainmeter/status.json";

Updater::Updater() : 
	m_Status(nullptr)
{
}

Updater::~Updater()
{
}

Updater& Updater::GetInstance()
{
	static Updater s_Updater;
	return s_Updater;
}

void Updater::CheckUpdate()
{
	_beginthread(CheckVersion, 0, this);
}

void Updater::CheckLanguage()
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
			LogDebug(L"  Status file: Invalid (may not have been downloaded)");
			LogDebug(L"------------------------------");
		}
		return;
	}

	bool obsolete = false;
	const auto lcid = (unsigned)GetRainmeter().GetResourceLCID();
	const auto lang = m_Status["language"];
	if (lang.is_null() || lang.empty() ||
		!lang.is_structured() || !lang.is_array())
	{
		if (debug)
		{
			LogDebug(L"  Status file: Invalid (possibly corrupt?)");
			LogDebug(L"------------------------------");
		}
		return;
	}

	for (auto& it = lang.cbegin(); it != lang.cend(); ++it)
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

void Updater::CheckVersion(void* pParam)
{
	auto updater = (Updater*)pParam;

	const bool debug = GetRainmeter().GetDebug();
	if (debug)
	{
		LogDebug(L"------------------------------");
		LogDebug(L"* Checking for updates:");
	}

	HINTERNET hRootHandle = InternetOpen(
		L"Rainmeter",
		INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr,
		nullptr,
		0);
	if (hRootHandle == nullptr)
	{
		if (debug) ShowError(L"Update error: InternetOpen failed");
		return;
	}

	HINTERNET hUrlDump = InternetOpenUrl(
		hRootHandle,
		updater->c_UpdateURL,
		nullptr,
		0,
		INTERNET_FLAG_RESYNCHRONIZE,
		0);
	if (hUrlDump)
	{
		// The |status.json| file should be UTF-8 (or ANSI) encoded, however, allocate the buffer
		// with 3 extra bytes for triple null termination in case the encoding changes.
		const int CHUNK_SIZE = 8192;
		DWORD bufferSize = CHUNK_SIZE;
		char* buffer = (char*)malloc(bufferSize + 3);
		DWORD dataSize = 0;

		do
		{
			DWORD readSize = 0;
			if (!InternetReadFile(hUrlDump, buffer + dataSize, bufferSize - dataSize, &readSize))
			{
				if (debug)
				{
					ShowError(L"Update error: InternetReadFile failed");
					LogDebug(L"------------------------------");
				}

				free(buffer);
				buffer = nullptr;
				InternetCloseHandle(hUrlDump);
				InternetCloseHandle(hRootHandle);
				return;
			}

			if (readSize == 0) break;

			dataSize += readSize;
			bufferSize += CHUNK_SIZE;
			buffer = (char*)realloc(buffer, bufferSize + 3);
		} while (true);

		buffer[dataSize] = 0;
		buffer[dataSize + 1] = 0;
		buffer[dataSize + 2] = 0;

		if (debug) LogDebug(L"  Downloading status file: Success");

		std::string data = buffer;

		free(buffer);
		buffer = nullptr;

		nlohmann::json status = nlohmann::json::parse(data, nullptr, false);
		if (!status.is_null() && !status.is_discarded())
		{
			if (debug) LogDebug(L"  Parsing status file: Success");

			const auto release = status["release"]["final"].get<std::string>();
			if (!release.empty())
			{
				const std::wstring tmpSz = StringUtil::Widen(release);
				LPCWSTR version = tmpSz.c_str();

				if (debug) LogDebugF(L"  Status file version: %s", version);

				const int availableVersion = ParseVersion(version);
				if (availableVersion > RAINMETER_VERSION ||
					(revision_beta && availableVersion == RAINMETER_VERSION))
				{
					GetRainmeter().SetNewVersion();

					WCHAR tmp[32];
					LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
					GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", tmp, _countof(tmp), dataFile);

					// Show tray notification only once per new version
					const int lastVersion = ParseVersion(tmp);
					if (availableVersion > lastVersion)
					{
						GetRainmeter().GetTrayIcon()->ShowUpdateNotification(version);
						WritePrivateProfileString(L"Rainmeter", L"LastCheck", version, dataFile);
					}
				}
			}
			updater->m_Status = std::move(status);
			updater->CheckLanguage();
		}
		else
		{
			if (debug) LogError(L"Update error: Status file parsing failed");
		}

		InternetCloseHandle(hUrlDump);
	}
	else
	{
		if (debug) ShowError(L"Update error: InternetOpenUrl failed");
	}

	InternetCloseHandle(hRootHandle);

	if (debug) LogDebug(L"------------------------------");
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
