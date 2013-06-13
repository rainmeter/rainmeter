/*
  Copyright (C) 2004 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "TrayWindow.h"
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
					GetRainmeter().GetTrayWindow()->ShowUpdateNotification(version);
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
