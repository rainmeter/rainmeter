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
/*
  $Header: /home/cvsroot/Rainmeter/Library/UpdateCheck.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: UpdateCheck.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.1  2004/08/13 15:44:02  rainy
  Initial version.

*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include "Litestep.h"
#include "Rainmeter.h"
#include <string>
#include <Wininet.h>
#include <process.h>

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

	HINTERNET hUrlDump = InternetOpenUrl(hRootHandle, L"http://www.ipi.fi/~rainy/Rainmeter/version", NULL, NULL, INTERNET_FLAG_RESYNCHRONIZE, 0);
	if (hUrlDump)
	{
		DWORD dwSize;
		char buffer[16];	// 16 should be enough for the version number
		buffer[0] = 0;
		if (InternetReadFile(hUrlDump, (LPVOID)buffer, 16, &dwSize))
		{
			// Add a null terminator to the end of the buffer (just in case...). 
			std::string verStr = buffer;
			size_t pos = verStr.find('.');
			if (pos != std::wstring::npos)
			{
				std::string verStr1 = verStr.substr(pos + 1);
				version = atoi(verStr1.c_str()) * 1000;
				pos = verStr.find('.', pos + 1);
				if (pos != std::wstring::npos)
				{
					std::string verStr1 = verStr.substr(pos + 1);
					version += atoi(verStr1.c_str());
				}
			}

			if (version > RAINMETER_VERSION)
			{
				if (IDYES == MessageBox(NULL, L"A new version of Rainmeter is available at http://www.iki.fi/rainy.\nDo you want to go there now?", APPNAME, MB_YESNO | MB_ICONQUESTION))
				{
					LSExecute(NULL, L"http://www.iki.fi/rainy/Rainmeter.html", SW_SHOWNORMAL);
				}
			}
			else
			{
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
