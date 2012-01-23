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

#ifndef __EXPORT_H__
#define __EXPORT_H__

#ifdef LIBRARY_EXPORTS
#define LIBRARY_DECLSPEC __declspec(dllexport)
#else
#define LIBRARY_DECLSPEC __declspec(dllimport)
#endif // LIBRARY_EXPORTS

#ifdef __cplusplus
#define LIBRARY_EXPORT extern "C" LIBRARY_DECLSPEC
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define LIBRARY_EXPORT LIBRARY_DECLSPEC
#define PLUGIN_EXPORT __declspec(dllexport)
#endif // __cplusplus

//
// Exported functions
//

#ifdef __cplusplus
LIBRARY_EXPORT LPCWSTR RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures = TRUE);
#else
LIBRARY_EXPORT LPCWSTR RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures);
#endif // __cplusplus

LIBRARY_EXPORT double RmReadFormula(void* rm, LPCWSTR option, double defValue);

LIBRARY_EXPORT LPCWSTR RmPathToAbsolute(void* rm, LPCWSTR relativePath);

LIBRARY_EXPORT void RmExecute(void* skin, LPCWSTR command);

LIBRARY_EXPORT void* RmGet(void* rm, int type);

enum RmGetType
{
	RMG_MEASURENAME  = 0,
	RMG_SKIN         = 1,
	RMG_SETTINGSFILE = 2
};

LIBRARY_EXPORT BOOL LSLog(int type, LPCWSTR unused, LPCWSTR message);

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue);

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data);

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
	LPCWSTR value = RmReadString(rm, option, L"", TRUE);
	return (*value) ? _wtoi(value) : defValue;
}

__inline double RmReadDouble(void* rm, LPCWSTR option, double defValue)
{
	LPCWSTR value = RmReadString(rm, option, L"", TRUE);
	return (*value) ? wcstod(value, NULL) : defValue;
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
