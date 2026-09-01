// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// Rainmeter.dll exports undecorated names, but 32-bit plugins reference the __stdcall functions
// by their decorated names, which only the compiler knows. Exporting the same functions out of
// this stub gives the linker enough to write the import library the plugin SDK ships. None of
// the code here ever runs: the DLL is thrown away and only the import library is kept.

#include <windows.h>

#define LIBRARY_EXPORTS
#include "../Library/Export.h"

LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures) { return nullptr; }

LPCWSTR __stdcall RmReadStringFromSection(void* rm, LPCWSTR section, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures) { return nullptr; }

double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue) { return 0.0; }

double __stdcall RmReadFormulaFromSection(void* rm, LPCWSTR section, LPCWSTR option, double defValue) { return 0.0; }

LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str) { return nullptr; }

LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath) { return nullptr; }

void __stdcall RmExecute(void* skin, LPCWSTR command) {}

void* __stdcall RmGet(void* rm, int type) { return nullptr; }

void __stdcall RmLog(void* rm, int level, LPCWSTR message) {}

void __cdecl RmLogF(void* rm, int level, LPCWSTR format, ...) {}

BOOL __cdecl LSLog(int level, LPCWSTR unused, LPCWSTR message) { return FALSE; }

LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue) { return nullptr; }

LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data) { return nullptr; }

// Exports.def lists these as well, and the linker needs something to attach them to, even though
// plugins never call them.
EXTERN_C int RainmeterMain(LPWSTR cmdLine) { return 0; }

EXTERN_C int SkinInstallerMain(LPWSTR cmdLine) { return 0; }
