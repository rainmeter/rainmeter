// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include "zip.h"
#include "unzip.h"

#define MAX_LINE_LENGTH		4096
#define MB_ERROR			MB_OK | MB_TOPMOST | MB_ICONERROR

struct GlobalData
{
	std::wstring programPath;
	std::wstring settingsPath;
	std::wstring skinsPath;
	std::wstring iniFile;
};

HINSTANCE GetInstanceHandle();

bool CloseRainmeterIfActive();

bool IsRunning(const WCHAR* name, HANDLE* hMutex);
