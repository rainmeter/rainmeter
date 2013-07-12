/*
  Copyright (C) 2011 Kimmo Pekkola, Birunthan Mohanathas

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

#ifndef __RAINMETERAPI_H__
#define __RAINMETERAPI_H__

#ifdef LIBRARY_EXPORTS
#define LIBRARY_EXPORT EXTERN_C
#else
#define LIBRARY_EXPORT EXTERN_C __declspec(dllimport)
#endif // LIBRARY_EXPORTS

#define PLUGIN_EXPORT EXTERN_C __declspec(dllexport)

//
// Exported functions
//

#ifdef __cplusplus
LIBRARY_EXPORT LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures = TRUE);
#else
LIBRARY_EXPORT LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures);
#endif // __cplusplus

LIBRARY_EXPORT double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue);

LIBRARY_EXPORT LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str);

LIBRARY_EXPORT LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath);

LIBRARY_EXPORT void __stdcall RmExecute(void* skin, LPCWSTR command);

LIBRARY_EXPORT void* __stdcall RmGet(void* rm, int type);

enum RmGetType
{
	RMG_MEASURENAME      = 0,
	RMG_SKIN             = 1,
	RMG_SETTINGSFILE     = 2,
	RMG_SKINNAME         = 3,
	RMG_SKINWINDOWHANDLE = 4
};

LIBRARY_EXPORT BOOL LSLog(int type, LPCWSTR unused, LPCWSTR message);

//
// Wrapper functions
//

#ifndef LIBRARY_EXPORTS
__inline LPCWSTR RmReadPath(void* rm, LPCWSTR option, LPCWSTR defValue)
{
	LPCWSTR relativePath = RmReadString(rm, option, defValue, TRUE);
	return RmPathToAbsolute(rm, relativePath);
}

__inline int RmReadInt(void* rm, LPCWSTR option, int defValue)
{
	return (int)RmReadFormula(rm, option, defValue);
}

__inline double RmReadDouble(void* rm, LPCWSTR option, double defValue)
{
	return RmReadFormula(rm, option, defValue);
}

__inline LPCWSTR RmGetMeasureName(void* rm)
{
	return (LPCWSTR)RmGet(rm, RMG_MEASURENAME);
}

__inline LPCWSTR RmGetSettingsFile()
{
	return (LPCWSTR)RmGet(NULL, RMG_SETTINGSFILE);
}

__inline void* RmGetSkin(void* rm)
{
	return (void*)RmGet(rm, RMG_SKIN);
}

__inline LPCWSTR RmGetSkinName(void* rm)
{
	return (LPCWSTR)RmGet(rm, RMG_SKINNAME);
}

__inline HWND RmGetSkinWindow(void* rm)
{
	return (HWND)RmGet(rm, RMG_SKINWINDOWHANDLE);
}

__inline void RmLog(int level, LPCWSTR message)
{
	LSLog(level, NULL, message);
}

enum LOGLEVEL
{
	LOG_ERROR   = 1,
	LOG_WARNING = 2,
	LOG_NOTICE  = 3,
	LOG_DEBUG   = 4
};
#endif // LIBRARY_EXPORTS

#endif
