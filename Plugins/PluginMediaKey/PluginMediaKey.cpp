/*
  Copyright (C) 2010 Birunthan Mohanathas (www.poiru.net)

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

#include <windows.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions

void SendKey(WORD key)
{
	KEYBDINPUT kbi;
	kbi.wVk = key;
	kbi.wScan = 0;
	kbi.dwFlags = 0;
	kbi.time = 0;
	kbi.dwExtraInfo = (ULONG_PTR)GetMessageExtraInfo();

	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki   = kbi;

	SendInput(1, &input, sizeof(INPUT));
}

PLUGIN_EXPORT void ExecuteBang(LPCTSTR args, UINT id)
{
	if (_wcsicmp(args, L"NextTrack") == 0)
	{
		SendKey(VK_MEDIA_NEXT_TRACK);
	}
	else if (_wcsicmp(args, L"PrevTrack") == 0)
	{
		SendKey(VK_MEDIA_PREV_TRACK);
	}
	else if (_wcsicmp(args, L"Stop") == 0)
	{
		SendKey(VK_MEDIA_STOP);
	}
	else if (_wcsicmp(args, L"PlayPause") == 0)
	{
		SendKey(VK_MEDIA_PLAY_PAUSE);
	}
	else if (_wcsicmp(args, L"VolumeMute") == 0)
	{
		SendKey(VK_VOLUME_MUTE);
	}
	else if (_wcsicmp(args, L"VolumeDown") == 0)
	{
		SendKey(VK_VOLUME_DOWN);
	}
	else if (_wcsicmp(args, L"VolumeUp") == 0)
	{
		SendKey(VK_VOLUME_UP);
	}
	else
	{
		RmLog(LOG_WARNING, L"MediaKey.dll: Unknown bang");
	}
}
