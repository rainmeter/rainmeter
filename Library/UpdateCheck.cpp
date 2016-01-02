/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Util.h"
#include "Rainmeter.h"
#include "TrayIcon.h"
#include "../Version.h"

void CheckVersion(void* dummy)
{
	HINTERNET hRootHandle = InternetOpen(
		L"Rainmeter",
		INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr,
		nullptr,
		0);

	if (hRootHandle == nullptr)
	{
		return;
	}

	HINTERNET hUrlDump = InternetOpenUrl(
		hRootHandle, L"http://rainmeter.github.io/rainmeter/release", nullptr, 0, INTERNET_FLAG_RESYNCHRONIZE, 0);
	if (hUrlDump)
	{
		DWORD dwSize;
		char urlData[16] = {0};
		if (InternetReadFile(hUrlDump, (LPVOID)urlData, sizeof(urlData) - 1, &dwSize))
		{
			auto parseVersion = [](const WCHAR* str)->int
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
			};

			std::wstring tmpSz = StringUtil::Widen(urlData);
			const WCHAR* version = tmpSz.c_str();

			int availableVersion = parseVersion(version);

			// 4.0 will require Windows 7 SP1.
			if (availableVersion > 4000000 && !IsWindows7SP1OrGreater())
			{
				return;
			}

			if (availableVersion > RAINMETER_VERSION ||
				(revision_beta && availableVersion == RAINMETER_VERSION))
			{
				GetRainmeter().SetNewVersion();

				WCHAR buffer[32];
				const WCHAR* dataFile = GetRainmeter().GetDataFile().c_str();
				GetPrivateProfileString(L"Rainmeter", L"LastCheck", L"0", buffer, _countof(buffer), dataFile);

				// Show tray notification only once per new version
				int lastVersion = parseVersion(buffer);
				if (availableVersion > lastVersion)
				{
					GetRainmeter().GetTrayIcon()->ShowUpdateNotification(version);
					WritePrivateProfileString(L"Rainmeter", L"LastCheck", version, dataFile);
				}
			}
		}
		InternetCloseHandle(hUrlDump);
	}

	InternetCloseHandle(hRootHandle);
}

void CheckUpdate()
{
	_beginthread(CheckVersion, 0, nullptr);
}
