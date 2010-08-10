/*
  Copyright (C) 2010 poiru

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

#include <windows.h>
#include <string>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

void SendKey(WORD key);

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	return 0;
}

void SendKey(WORD key)
{
	KEYBDINPUT kbi;
	kbi.wVk = key; // Provide your own
	kbi.wScan = 0;
	kbi.dwFlags = 0;  // See docs for flags (mm keys may need Extended key flag)
	kbi.time = 0;
	kbi.dwExtraInfo = (ULONG_PTR) GetMessageExtraInfo();

	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki   = kbi;

	SendInput(1, &input, sizeof(INPUT));
}

void ExecuteBang(LPCTSTR args, UINT id)
{
	std::wstring wholeBang = args;

	size_t pos = wholeBang.find(' ');
	if (_wcsicmp(wholeBang.c_str(), L"NextTrack") == 0)
	{
		SendKey(VK_MEDIA_NEXT_TRACK);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"PrevTrack") == 0)
	{
		SendKey(VK_MEDIA_PREV_TRACK);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"Stop") == 0)
	{
		SendKey(VK_MEDIA_STOP);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"PlayPause") == 0)
	{
		SendKey(VK_MEDIA_PLAY_PAUSE);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"VolumeMute") == 0)
	{
		SendKey(VK_VOLUME_MUTE);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"VolumeDown") == 0)
	{
		SendKey(VK_VOLUME_DOWN);
	}
	else if (_wcsicmp(wholeBang.c_str(), L"VolumeUp") == 0)
	{
		SendKey(VK_VOLUME_UP);
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"MediaKeyPlugin: Unknown bang!");
	}
}

UINT GetPluginVersion()
{
	return 1000;
}

LPCTSTR GetPluginAuthor()
{
	return L"poiru";
}