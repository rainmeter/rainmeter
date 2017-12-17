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

LPCWSTR Updater::c_UpdateURL = L"http://rainmeter.github.io/rainmeter/status.json";

Updater::Updater() : 
	m_Status()
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
	if (m_Status.is_discarded()) return;

	auto lang = m_Status["language"];
	if (lang.empty()) return;

	bool obsolete = false;

	auto id = (unsigned)GetRainmeter().GetResourceLCID();
	size_t pos = 0;
	while (!lang[pos].empty())
	{
		if (!lang[pos]["id"].empty() &&
			lang[pos]["id"] == id &&
			!lang[pos]["obsolete"].empty())
		{
			obsolete = lang[pos]["obsolete"];
			break;
		}
		++pos;
	}

	GetRainmeter().SetLanguageStatus(obsolete);
}

void Updater::CheckVersion(void* pParam)
{
	auto updater = (Updater*)pParam;

	HINTERNET hRootHandle = InternetOpen(
		L"Rainmeter",
		INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr,
		nullptr,
		0);
	if (hRootHandle == nullptr)	return;

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
				break;
			}

			if (readSize == 0) break;

			dataSize += readSize;
			bufferSize += CHUNK_SIZE;
			buffer = (char*)realloc(buffer, bufferSize + 3);
		} while (true);

		buffer[dataSize] = 0;
		buffer[dataSize + 1] = 0;
		buffer[dataSize + 2] = 0;

		std::string data = buffer;

		free(buffer);
		buffer = nullptr;

		nlohmann::json status = nlohmann::json::parse(data, nullptr);
		if (!status.is_discarded())
		{
			auto release = status["release"]["final"];
			if (!release.empty())
			{
				std::wstring tmpSz = StringUtil::Widen(release);
				LPCWSTR version = tmpSz.c_str();

				int availableVersion = ParseVersion(version);
				if (availableVersion > RAINMETER_VERSION ||
					(revision_beta && availableVersion == RAINMETER_VERSION))
				{
					GetRainmeter().SetNewVersion();

					WCHAR tmp[32];
					LPCWSTR dataFile = GetRainmeter().GetDataFile().c_str();
					GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", tmp, _countof(tmp), dataFile);

					// Show tray notification only once per new version
					int lastVersion = ParseVersion(tmp);
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
		InternetCloseHandle(hUrlDump);
	}

	InternetCloseHandle(hRootHandle);
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
