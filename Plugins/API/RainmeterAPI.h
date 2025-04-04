/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

/// <summary>
/// Retrieves the option defined in the skin file
/// </summary>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="option">Option name to be read from skin</param>
/// <param name="defValue">Default value for the option if it is not found or invalid</param>
/// <param name="replaceMeasures">If true, replaces section variables in the returned string</param>
/// <returns>Returns the option value as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	LPCWSTR value = RmReadString(rm, L"Value", L"DefaultValue");
/// 	LPCWSTR action = RmReadString(rm, L"Action", L"", FALSE);  // [MeasureNames] will be parsed/replaced when the action is executed with RmExecute
/// }
/// </code>
/// </example>
#ifdef __cplusplus
LIBRARY_EXPORT LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures = TRUE);
#else
LIBRARY_EXPORT LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures);
#endif // __cplusplus

/// <summary>
/// Parses any formulas in the option (use RmReadDouble/RmReadInt instead)
/// </summary>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="option">Option name to be read from skin</param>
/// <param name="defValue">Default value for the option if it is not found, invalid, or a formula could not be parsed</param>
/// <returns>Returns the option value as an double</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	double value = RmReadFormula(rm, L"Value", 20);
/// }
/// </code>
/// </example>
LIBRARY_EXPORT double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue);

/// <summary>
/// Returns a string, replacing any variables (or section variables) within the inputted string
/// </summary>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="str">String with unresolved variables</param>
/// <returns>Returns a string replacing any variables in the 'str'</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT double Update(void* data)
/// {
/// 	Measure* measure = (Measure*)data;
/// 	LPCWSTR myVar = RmReplaceVariables(measure->rm, L"#MyVar#");  // 'measure->rm' stored previously in the Initialize function
/// 	if (_wcsicmp(myVar, L"SOMETHING") == 0) { return 1.0; }
/// 	return 0.0;
/// }
/// </code>
/// </example>
LIBRARY_EXPORT LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str);

/// <summary>
/// Converts a relative path to a absolute path (use RmReadPath where appropriate)
/// </summary>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="relativePath">String of path to be converted</param>
/// <returns>Returns the absolute path of the relativePath value as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	std::wstring somePath = L"..\\SomeFolder";
/// 	LPCWSTR path = RmPathToAbsolute(rm, somePath.c_str());
/// }
/// </code>
/// </example>
LIBRARY_EXPORT LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath);

/// <summary>
/// Executes a command
/// </summary>
/// <param name="skin">Pointer to current skin (See RmGetSkin)</param>
/// <param name="command">Bang to execute</param>
/// <returns>No return type</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT double Update(void* data)
/// {
/// 	Measure* measure = (Measure*)data;
/// 	RmExecute(measure->skin, L"!SetVariable SomeVar 10");  // 'measure->skin' stored previously in the Initialize function
/// 	return 0.0;
/// }
/// </code>
/// </example>
LIBRARY_EXPORT void __stdcall RmExecute(void* skin, LPCWSTR command);

/// <summary>
/// Retrieves data from the measure or skin (use the helper functions instead)
/// </summary>
/// <remarks>Call RmGet() in the Initialize function and store the results for later use</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="type">Type of information to retrieve (see RmGetType)</param>
/// <returns>Returns a pointer to an object which is determined by the 'type'</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
/// 	measure->hwnd = RmGet(rm, RMG_SKINWINDOWHANDLE);  // 'measure->hwnd' defined as HWND in class scope
/// }
/// </code>
/// </example>
LIBRARY_EXPORT void* __stdcall RmGet(void* rm, int type);

enum RmGetType
{
	RMG_MEASURENAME      = 0,
	RMG_SKIN             = 1,
	RMG_SETTINGSFILE     = 2,
	RMG_SKINNAME         = 3,
	RMG_SKINWINDOWHANDLE = 4
};

/// <summary>
/// Sends a message to the Rainmeter log with source
/// </summary>
/// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="type">Log type (LOG_ERROR, LOG_WARNING, LOG_NOTICE, or LOG_DEBUG)</param>
/// <param name="message">Message to be logged</param>
/// <returns>No return type</returns>
/// <example>
/// <code>
/// RmLog(rm, LOG_NOTICE, L"I am a 'notice' log message with a source");
/// </code>
/// </example>
LIBRARY_EXPORT void __stdcall RmLog(void* rm, int level, LPCWSTR message);

/// <summary>
/// Sends a formatted message to the Rainmeter log
/// </summary>
/// <remarks>LOG_DEBUG messages are logged only when Rainmeter is in debug mode</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="level">Log level (LOG_ERROR, LOG_WARNING, LOG_NOTICE, or LOG_DEBUG)</param>
/// <param name="format">Formatted message to be logged, follows printf syntax</param>
/// <param name="...">Comma separated list of args referenced in the formatted message</param>
/// <returns>No return type</returns>
/// <example>
/// <code>
/// std::wstring notice = L"notice";
/// RmLogF(rm, LOG_NOTICE, L"I am a '%s' log message with a source", notice.c_str());
/// </code>
/// </example>
LIBRARY_EXPORT void __cdecl RmLogF(void* rm, int level, LPCWSTR format, ...);

/// <summary>
/// DEPRECATED: Use RmLog. Sends a message to the Rainmeter log.
/// </summary>
LIBRARY_EXPORT BOOL __cdecl LSLog(int level, LPCWSTR unused, LPCWSTR message);

//
// Wrapper functions
//

#ifndef LIBRARY_EXPORTS
/// <summary>
/// Retrieves the option defined in the skin file and converts a relative path to a absolute path
/// </summary>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="option">Option name to be read from skin</param>
/// <param name="defValue">Default value for the option if it is not found or invalid</param>
/// <returns>Returns the absolute path of the option value as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	LPCWSTR path = RmReadPath(rm, L"MyPath", L"C:\\");
/// }
/// </code>
/// </example>
__inline LPCWSTR RmReadPath(void* rm, LPCWSTR option, LPCWSTR defValue)
{
	LPCWSTR relativePath = RmReadString(rm, option, defValue, TRUE);
	return RmPathToAbsolute(rm, relativePath);
}

/// <summary>
/// Retrieves the option defined in the skin file and converts it to an integer
/// </summary>
/// <remarks>If the option is a formula, the returned value will be the result of the parsed formula</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="option">Option name to be read from skin</param>
/// <param name="defValue">Default value for the option if it is not found, invalid, or a formula could not be parsed</param>
/// <returns>Returns the option value as an integer</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	int value = RmReadInt(rm, L"Value", 20);
/// }
/// </code>
/// </example>
__inline int RmReadInt(void* rm, LPCWSTR option, int defValue)
{
	return (int)RmReadFormula(rm, option, defValue);
}

/// <summary>
/// Retrieves the option defined in the skin file and converts it to a double
/// </summary>
/// <remarks>If the option is a formula, the returned value will be the result of the parsed formula</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <param name="option">Option name to read from skin</param>
/// <param name="defValue">Default value for the option if it is not found, invalid, or a formula could not be parsed</param>
/// <returns>Returns the option value as a double</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
/// {
/// 	double value = RmReadDouble(rm, L"Value", 20.0);
/// }
/// </code>
/// </example>
__inline double RmReadDouble(void* rm, LPCWSTR option, double defValue)
{
	return RmReadFormula(rm, option, defValue);
}

/// <summary>
/// Retrieves the name of the measure
/// </summary>
/// <remarks>Call RmGetMeasureName() in the Initialize function and store the results for later use</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <returns>Returns the current measure name as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
/// 	measure->myName = RmGetMeasureName(rm);  // 'measure->myName' defined as a string (LPCWSTR) in class scope
/// }
/// </code>
/// </example>
__inline LPCWSTR RmGetMeasureName(void* rm)
{
	return (LPCWSTR)RmGet(rm, RMG_MEASURENAME);
}

/// <summary>
/// Retrieves a path to the Rainmeter data file (Rainmeter.data).
/// </summary>
/// <remarks>Call GetSettingsFile() in the Initialize function and store the results for later use</remarks>
/// <returns>Returns the path and filename of the Rainmeter data file as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
///		if (rmDataFile == nullptr) { rmDataFile = RmGetSettingsFile(); }  // 'rmDataFile' defined as a string (LPCWSTR) in global scope
/// }
/// </code>
/// </example>
__inline LPCWSTR RmGetSettingsFile()
{
	return (LPCWSTR)RmGet(NULL, RMG_SETTINGSFILE);
}

/// <summary>
/// Retrieves an internal pointer to the current skin
/// </summary>
/// <remarks>Call GetSkin() in the Initialize function and store the results for later use</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <returns>Returns a pointer to the current skin</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
/// 	measure->mySkin = RmGetSkin(rm);  // 'measure->mySkin' defined as a 'void*' in class scope
/// }
/// </code>
/// </example>
__inline void* RmGetSkin(void* rm)
{
	return (void*)RmGet(rm, RMG_SKIN);
}

/// <summary>
/// Retrieves full path and name of the skin
/// </summary>
/// <remarks>Call GetSkinName() in the Initialize function and store the results for later use</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <returns>Returns the path and filename of the skin as a string (LPCWSTR)</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
/// 	skinName = RmGetSkinName(rm); }  // 'measure->skinName' defined as a string (LPCWSTR) in class scope
/// }
/// </code>
/// </example>
__inline LPCWSTR RmGetSkinName(void* rm)
{
	return (LPCWSTR)RmGet(rm, RMG_SKINNAME);
}

/// <summary>
/// Returns a pointer to the handle of the skin window (HWND)
/// </summary>
/// <remarks>Call GetSkinWindow() in the Initialize function and store the results for later use</remarks>
/// <param name="rm">Pointer to the plugin measure</param>
/// <returns>Returns a handle to the skin window as a HWND</returns>
/// <example>
/// <code>
/// PLUGIN_EXPORT void Initialize(void** data, void* rm)
/// {
/// 	Measure* measure = new Measure;
/// 	*data = measure;
/// 	measure->skinWindow = RmGetSkinWindow(rm); }  // 'measure->skinWindow' defined as HWND in class scope
/// }
/// </code>
/// </example>
__inline HWND RmGetSkinWindow(void* rm)
{
	return (HWND)RmGet(rm, RMG_SKINWINDOWHANDLE);
}

/// <summary>
/// DEPRECATED: Use RmLog(rm, type, message). Sends a message to the Rainmeter log.
/// </summary>
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
