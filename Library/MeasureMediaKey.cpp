// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureMediaKey.h"
#include "Logger.h"
#include "System.h"

MeasureMediaKey::MeasureMediaKey(Skin* skin, const WCHAR* name) : Measure(skin, name)
{
}

MeasureMediaKey::~MeasureMediaKey()
{
}

void MeasureMediaKey::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();
	if (_wcsicmp(args, L"NextTrack") == 0)
	{
		System::SendKey(VK_MEDIA_NEXT_TRACK);
	}
	else if (_wcsicmp(args, L"PrevTrack") == 0)
	{
		System::SendKey(VK_MEDIA_PREV_TRACK);
	}
	else if (_wcsicmp(args, L"Stop") == 0)
	{
		System::SendKey(VK_MEDIA_STOP);
	}
	else if (_wcsicmp(args, L"PlayPause") == 0)
	{
		System::SendKey(VK_MEDIA_PLAY_PAUSE);
	}
	else if (_wcsicmp(args, L"VolumeMute") == 0)
	{
		System::SendKey(VK_VOLUME_MUTE);
	}
	else if (_wcsicmp(args, L"VolumeDown") == 0)
	{
		System::SendKey(VK_VOLUME_DOWN);
	}
	else if (_wcsicmp(args, L"VolumeUp") == 0)
	{
		System::SendKey(VK_VOLUME_UP);
	}
	else
	{
		LogErrorF(this, L"Unknown command: %s", args);
	}
}
