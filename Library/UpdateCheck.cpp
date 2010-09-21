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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Litestep.h"
#include "Rainmeter.h"

extern CRainmeter* Rainmeter;

void CheckVersion(void* dummy)
{
	int version = 0;

	HINTERNET hRootHandle = InternetOpen(L"Rainmeter",
									INTERNET_OPEN_TYPE_PRECONFIG,
									NULL,
									NULL,
									0);
	if (hRootHandle == NULL)
	{
		DebugLog(L"CheckUpdate: InternetOpen failed.");
		return;
	}

	HINTERNET hUrlDump = InternetOpenUrl(hRootHandle, L"http://rainmeter.googlecode.com/svn/version", NULL, NULL, INTERNET_FLAG_RESYNCHRONIZE, 0);
	if (hUrlDump)
	{
		DWORD dwSize;
		char buffer[16] = {0};	// 16 should be enough for the version number
		if (InternetReadFile(hUrlDump, (LPVOID)buffer, 15, &dwSize))
		{
			std::string verStr = buffer;
			size_t pos = verStr.find('.');
			if (pos != std::wstring::npos)
			{
				std::string verMajor = verStr.substr(0, pos);
				std::string verMinor = verStr.substr(pos + 1);

				version = atoi(verMajor.c_str()) * 1000000;

				pos = verMinor.find('.');
				if (pos != std::wstring::npos)
				{
					std::string verMinor1 = verMinor.substr(0, pos);
					std::string verMinor2 = verMinor.substr(pos + 1);

					version += atoi(verMinor1.c_str()) * 1000;
					version += atoi(verMinor2.c_str());
				}
				else
				{
					version += atoi(verMinor.c_str()) * 1000;
				}
			}
				
			if (version > RAINMETER_VERSION)
			{
				Rainmeter->SetNewVersion(TRUE);
			}
			else
			{
				Rainmeter->SetNewVersion(FALSE);
				DebugLog(L"CheckUpdate: No new version available.");
			}
		}
		else
		{
			DebugLog(L"CheckUpdate: InternetReadFile failed.");
		}
		InternetCloseHandle(hUrlDump);
	}
	else
	{
		DebugLog(L"CheckUpdate: InternetOpenUrl failed.");
	}

	InternetCloseHandle(hRootHandle);
}

void CheckUpdate()
{
	_beginthread(CheckVersion, 0, NULL );
}
