// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Plugins/API/RainmeterAPI.h"

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue);

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data);

#ifdef LIBRARY_EXPORTS
void HandleExportSyncMessage(WPARAM wParam, LPARAM lParam);
#endif
