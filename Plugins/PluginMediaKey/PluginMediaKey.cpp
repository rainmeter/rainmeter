/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
