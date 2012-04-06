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
#include "../Version.h"

extern CRainmeter* Rainmeter;

void CheckVersion(void* dummy)
{
	HINTERNET hRootHandle = InternetOpen(
		L"Rainmeter",
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);

	if (hRootHandle == NULL)
	{
		return;
	}

	HINTERNET hUrlDump = InternetOpenUrl(hRootHandle, L"http://rainmeter.github.com/rainmeter/release", NULL, NULL, INTERNET_FLAG_RESYNCHRONIZE, 0);
	if (hUrlDump)
	{
		DWORD dwSize;
		char buffer[16] = {0};	// 16 should be enough for the version number
		if (InternetReadFile(hUrlDump, (LPVOID)buffer, 15, &dwSize))
		{
			int version = atoi(buffer) * 1000000;
			char* pos = strchr(buffer, '.');
			if (pos)
			{
				++pos;	// Skip .
				version += atoi(pos) * 1000;

				pos = strchr(pos, '.');
				if (pos)
				{
					++pos;	// Skip .
					version += atoi(pos);
				}
			}

			if (version > RAINMETER_VERSION)
			{
				Rainmeter->SetNewVersion();
			}
		}
		InternetCloseHandle(hUrlDump);
	}

	InternetCloseHandle(hRootHandle);
}

void CheckUpdate()
{
	_beginthread(CheckVersion, 0, NULL);
}
