/*
  Copyright (C) 2001 Kimmo Pekkola

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

#include "StdAfx.h"
#include "Rainmeter.h"
#include "TrayWindow.h"
#include "System.h"
#include "Error.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "MeasureNet.h"
#include "MeterString.h"
#include "resource.h"
#include "UpdateCheck.h"
#include "../Version.h"

#include "DisableThreadLibraryCalls.h"	// contains DllMain entry point

using namespace Gdiplus;

enum TIMER
{
	TIMER_NETSTATS    = 1
};
enum INTERVAL
{
	INTERVAL_NETSTATS = 120000
};

CRainmeter* Rainmeter; // The module

/*
** Initializes Rainmeter.
**
*/
int RainmeterMain(LPWSTR cmdLine)
{
	HWND wnd = NULL;
	while (wnd = FindWindowEx(NULL, wnd, RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME))
	{
		COPYDATASTRUCT cds;

		if (cmdLine[0] == L'!')
		{
			// Deliver bang to existing Rainmeter instance
			cds.dwData = 1;
			cds.cbData = (DWORD)((wcslen(cmdLine) + 1) * sizeof(WCHAR));
			cds.lpData = (PVOID)cmdLine;
			SendMessage(wnd, WM_COPYDATA, NULL, (LPARAM)&cds);

			return 0;
		}
		else
		{
			const WCHAR* fullCmdLine = GetCommandLine();

			COPYDATASTRUCT cds;
			cds.dwData = SIZE_MAX;
			cds.cbData = (DWORD)((wcslen(fullCmdLine) + 1) * sizeof(WCHAR));
			cds.lpData = (PVOID)fullCmdLine;

			if (SendMessage(wnd, WM_COPYDATA, NULL, (LPARAM)&cds) == SIZE_MAX)
			{
				// An instance of Rainmeter with same command-line arguments already exists
				return 1;
			}
		}
	}

	if (cmdLine[0] == L'!' &&
		_wcsicmp(L"!RainmeterQuit", cmdLine) != 0 &&
		_wcsicmp(L"!Quit", cmdLine) != 0)
	{
		MessageBox(NULL, L"Unable to send bang: Rainmeter is not running.", L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONERROR);
		return 1;
	}

	// Avoid loading a dll from current directory
	SetDllDirectory(L"");
	
	int ret = 1;
	Rainmeter = new CRainmeter;
	if (Rainmeter)
	{
		try
		{
			ret = Rainmeter->Initialize(cmdLine);
		}
		catch (CError& error)
		{
			MessageBox(NULL, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
		}

		if (ret == 0)
		{
			ret = Rainmeter->MessagePump();
		}

		delete Rainmeter;
		Rainmeter = NULL;
	}

	return ret;
}

/*
** Splits the given string into substrings
**
*/
std::vector<std::wstring> CRainmeter::ParseString(LPCTSTR str)
{
	std::vector<std::wstring> result;

	if (str)
	{
		std::wstring arg = str;

		// Split the argument between first space.
		// Or if string is in quotes, the after the second quote.

		auto stripQuotes = [&](std::wstring& string)
		{
			size_t pos = 0;
			do
			{
				pos = string.find(L'"', pos);
				if (pos != std::wstring::npos)
				{
					string.erase(pos, 1);
				}
			}
			while (pos != std::wstring::npos);
		};

		size_t pos;
		std::wstring newStr;
		while ((pos = arg.find_first_not_of(L' ')) != std::wstring::npos)
		{
			size_t extra = 1;
			if (arg[pos] == L'"')
			{
				if (arg.size() > (pos + 2) &&
					arg[pos + 1] == L'"' && arg[pos + 2] == L'"')
				{
					// Eat found quotes and finding ending """
					arg.erase(0, pos + 3);

					extra = 4;
					if ((pos = arg.find(L"\"\"\" ")) == std::wstring::npos)
					{
						extra = 3;
						pos = arg.rfind(L"\"\"\"");  // search backward
					}
				}
				else
				{
					// Eat found quote and find ending quote 
					arg.erase(0, pos + 1);
					pos = arg.find_first_of(L'"');
				}
			}
			else
			{
				if (pos > 0)
				{
					// Eat everything until non-space (and non-quote) char
					arg.erase(0, pos);
				}

				// Find the second quote
				pos = arg.find_first_of(L' ');
			}

			if (pos != std::wstring::npos)
			{
				newStr.assign(arg, 0, pos);
				arg.erase(0, pos + extra);

				if (extra == 1) stripQuotes(newStr);
				result.push_back(newStr);
			}
			else  // quote or space not found
			{
				newStr = arg;
				arg.clear();

				if (extra == 1) stripQuotes(newStr);
				result.push_back(newStr);
				break;
			}
		}

		if (!arg.empty() && result.empty())
		{
			stripQuotes(arg);
			result.push_back(arg);
		}
	}

	return result;
}

/*
** Parses Bang args
**
*/
void CRainmeter::BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs, CMeterWindow* meterWindow)
{
	std::vector<std::wstring> subStrings = ParseString(arg);
	const size_t subStringsSize = subStrings.size();

	if (subStringsSize >= numOfArgs)
	{
		if (subStringsSize == numOfArgs && meterWindow)
		{
			meterWindow->RunBang(bang, subStrings);
		}
		else
		{
			// Use the specified window instead of meterWindow parameter
			if (subStringsSize > numOfArgs)
			{
				const std::wstring& config = subStrings[numOfArgs];
				if (!config.empty() && (config.length() != 1 || config[0] != L'*'))
				{
					CMeterWindow* meterWindow = GetMeterWindow(config);
					if (meterWindow)
					{
						meterWindow->RunBang(bang, subStrings);
					}
					else
					{
						LogWithArgs(LOG_ERROR,  L"Bang: Config \"%s\" not found", config.c_str());
					}
					return;
				}
			}

			// No config defined -> apply to all.
			std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
			for (; iter != m_MeterWindows.end(); ++iter)
			{
				((*iter).second)->RunBang(bang, subStrings);
			}
		}
	}
	else
	{
		// For backwards compatibility
		if (bang == BANG_COMMANDMEASURE && subStringsSize >= 1)
		{
			std::wstring tmpSz = arg;
			std::wstring::size_type pos = tmpSz.find_first_of(L' ');
			if (pos != std::wstring::npos)
			{
				tmpSz.replace(pos, 1, L"\" \"");
				BangWithArgs(bang, tmpSz.c_str(), numOfArgs, meterWindow);
				Log(LOG_WARNING, L"!CommandMeasure: Two parameters required, only one given");
				return;
			}
		}

		Log(LOG_ERROR, L"Bang: Incorrect number of arguments");
	}
}

/*
** Parses Bang args for Group
**
*/
void CRainmeter::BangGroupWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs, CMeterWindow* meterWindow)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (subStrings.size() > numOfArgs)
	{
		std::multimap<int, CMeterWindow*> windows;
		GetMeterWindowsByLoadOrder(windows, subStrings[numOfArgs]);

		std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
		for (; iter != windows.end(); ++iter)
		{
			std::wstring argument(1, L'"');
			for (size_t i = 0; i < numOfArgs; ++i)
			{
				argument += subStrings[i];
				argument += L"\" \"";
			}
			argument += (*iter).second->GetSkinName();
			argument += L'"';
			BangWithArgs(bang, argument.c_str(), numOfArgs, meterWindow);
		}
	}
	else
	{
		Log(LOG_ERROR, L"BangGroup: Incorrect number of arguments");
	}
}

/*
** !ActivateConfig bang
**
*/
void CRainmeter::Bang_ActivateConfig(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (subStrings.size() > 1)
	{
		std::pair<int, int> indexes = GetMeterWindowIndex(subStrings[0], subStrings[1]);
		if (indexes.first != -1 && indexes.second != -1)
		{
			ActivateConfig(indexes.first, indexes.second);
			return;
		}
		LogWithArgs(LOG_ERROR, L"!ActivateConfig: \"%s\\%s\" not found", subStrings[0].c_str(), subStrings[1].c_str());
	}
	else
	{
		// If we got this far, something went wrong
		Log(LOG_ERROR, L"!ActivateConfig: Invalid parameters");
	}
}

/*
** !DeactivateConfig bang
**
*/
void CRainmeter::Bang_DeactivateConfig(const WCHAR* arg, CMeterWindow* meterWindow)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (!subStrings.empty())
	{
		meterWindow = GetMeterWindow(subStrings[0]);
		if (!meterWindow)
		{
			LogWithArgs(LOG_WARNING, L"!DeactivateConfig: \"%s\" not active", subStrings[0].c_str());
			return;
		}
	}

	if (meterWindow)
	{
		DeactivateConfig(meterWindow, -1);
	}
	else
	{
		Log(LOG_ERROR, L"!DeactivateConfig: Invalid parameters");
	}
}

/*
** !ToggleConfig bang
**
*/
void CRainmeter::Bang_ToggleConfig(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (subStrings.size() >= 2)
	{
		CMeterWindow* mw = GetMeterWindow(subStrings[0]);
		if (mw)
		{
			DeactivateConfig(mw, -1);
			return;
		}

		// If the config wasn't active, activate it
		Bang_ActivateConfig(arg);
	}
	else
	{
		Log(LOG_ERROR, L"!ToggleConfig: Invalid parameters");
	}
}

/*
** !DeactivateConfigGroup bang
**
*/
void CRainmeter::Bang_DeactivateConfigGroup(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (!subStrings.empty())
	{
		std::multimap<int, CMeterWindow*> windows;
		GetMeterWindowsByLoadOrder(windows, subStrings[0]);

		std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
		for (; iter != windows.end(); ++iter)
		{
			DeactivateConfig((*iter).second, -1);
		}
	}
	else
	{
		Log(LOG_ERROR, L"!DeactivateConfigGroup: Invalid parameters");
	}
}

/*
** !SetClip bang
**
*/
void CRainmeter::Bang_SetClip(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (!subStrings.empty())
	{
		CSystem::SetClipboardText(subStrings[0]);
	}
	else
	{
		Log(LOG_ERROR, L"!SetClip: Invalid parameter");
	}
}

/*
** !SetWallpaper bang
**
*/
void CRainmeter::Bang_SetWallpaper(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (subStrings.size() == 1)
	{
		CSystem::SetWallpaper(subStrings[0], L"");
	}
	else if (subStrings.size() == 2)
	{
		CSystem::SetWallpaper(subStrings[0], subStrings[1]);
	}
	else
	{
		Log(LOG_ERROR, L"!SetWallpaper: Invalid parameters");
	}
}

/*
** !SkinMenu bang
**
*/
void CRainmeter::Bang_SkinMenu(const WCHAR* arg, CMeterWindow* meterWindow)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (!subStrings.empty())
	{
		meterWindow = GetMeterWindow(subStrings[0]);
		if (!meterWindow)
		{
			LogWithArgs(LOG_WARNING, L"!SkinMenu: \"%s\" not active", subStrings[0].c_str());
			return;
		}
	}

	if (meterWindow)
	{
		POINT pos;
		GetCursorPos(&pos);
		ShowContextMenu(pos, meterWindow);
	}
	else
	{
		Log(LOG_ERROR, L"!SkinMenu: Invalid parameter");
	}
}

/*
** !TrayMenu bang
**
*/
void CRainmeter::Bang_TrayMenu()
{
	POINT pos;
	GetCursorPos(&pos);
	ShowContextMenu(pos, NULL);
}

/*
** !WriteKeyValue bang
**
*/
void CRainmeter::Bang_WriteKeyValue(const WCHAR* arg, CMeterWindow* meterWindow)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (subStrings.size() < 4)
	{
		if (!meterWindow) return;

		// Add the config filepath to the args
		std::wstring path = m_SkinPath + meterWindow->GetSkinName();
		path += L'\\';
		path += meterWindow->GetSkinIniFile();
		subStrings.push_back(std::move(path));
	}

	if (subStrings.size() > 3)
	{
		const std::wstring& strIniFile = subStrings[3];
		const WCHAR* iniFile = strIniFile.c_str();

		if (strIniFile.find(L"..\\") != std::wstring::npos || strIniFile.find(L"../") != std::wstring::npos)
		{
			LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Illegal path: %s", iniFile);
			return;
		}

		const std::wstring& skinPath = GetSkinPath();
		const std::wstring settingsPath = GetSettingsPath();

		if (_wcsnicmp(iniFile, skinPath.c_str(), skinPath.size()) != 0 &&
			_wcsnicmp(iniFile, settingsPath.c_str(), settingsPath.size()) != 0)
		{
			LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Illegal path: %s", iniFile);
			return;
		}

		// Verify whether the file exists
		if (_waccess(iniFile, 0) == -1)
		{
			LogWithArgs(LOG_ERROR, L"!WriteKeyValue: File not found: %s", iniFile);
			return;
		}

		// Verify whether the file is read-only
		DWORD attr = GetFileAttributes(iniFile);
		if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
		{
			LogWithArgs(LOG_WARNING, L"!WriteKeyValue: File is read-only: %s", iniFile);
			return;
		}

		// Avoid "IniFileMapping"
		CSystem::UpdateIniFileMappingList();
		std::wstring strIniWrite = CSystem::GetTemporaryFile(strIniFile);
		if (strIniWrite.size() == 1 && strIniWrite[0] == L'?')  // error occurred
		{
			return;
		}

		bool temporary = !strIniWrite.empty();

		if (temporary)
		{
			if (GetDebug()) LogWithArgs(LOG_DEBUG, L"!WriteKeyValue: Writing to: %s (Temp: %s)", iniFile, strIniWrite.c_str());
		}
		else
		{
			if (GetDebug()) LogWithArgs(LOG_DEBUG, L"!WriteKeyValue: Writing to: %s", iniFile);
			strIniWrite = strIniFile;
		}

		const WCHAR* iniWrite = strIniWrite.c_str();
		const WCHAR* section = subStrings[0].c_str();
		const WCHAR* key = subStrings[1].c_str();
		const std::wstring& strValue = subStrings[2];

		bool formula = false;
		BOOL write = 0;

		if (meterWindow)
		{
			double value;
			formula = meterWindow->GetParser().ParseFormula(strValue, &value);

			// Formula read fine
			if (formula)
			{
				WCHAR buffer[256];
				int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", value);
				CMeasure::RemoveTrailingZero(buffer, len);

				write = WritePrivateProfileString(section, key, buffer, iniWrite);
			}
		}

		if (!formula)
		{
			write = WritePrivateProfileString(section, key, strValue.c_str(), iniWrite);
		}

		if (temporary)
		{
			if (write != 0)
			{
				WritePrivateProfileString(NULL, NULL, NULL, iniWrite);  // FLUSH

				// Copy the file back
				if (!CSystem::CopyFiles(strIniWrite, strIniFile))
				{
					LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Failed to copy temporary file to original filepath: %s (Temp: %s)", iniFile, iniWrite);
				}
			}
			else  // failed
			{
				LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Failed to write to: %s (Temp: %s)", iniFile, iniWrite);
			}

			// Remove a temporary file
			CSystem::RemoveFile(strIniWrite);
		}
		else
		{
			if (write == 0)  // failed
			{
				LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Failed to write to: %s", iniFile);
			}
		}
	}
	else
	{
		Log(LOG_ERROR, L"!WriteKeyValue: Invalid parameters");
	}
}

/*
** !Log bang
**
*/
void CRainmeter::Bang_Log(const WCHAR* arg)
{
	std::vector<std::wstring> subStrings = ParseString(arg);

	if (!subStrings.empty())
	{
		int level = LOG_NOTICE;

		if (subStrings.size() > 1)
		{
			const WCHAR* type = subStrings[1].c_str();
			if (_wcsicmp(type, L"ERROR") == 0)
			{
				level = LOG_ERROR;
			}
			else if (_wcsicmp(type, L"WARNING") == 0)
			{
				level = LOG_WARNING;
			}
			else if (_wcsicmp(type, L"DEBUG") == 0)
			{
				level = LOG_DEBUG;
			}
			else if (_wcsicmp(type, L"NOTICE") != 0)
			{
				Log(LOG_ERROR, L"!Log: Invalid type");
				return;
			}
		}

		Log(level, subStrings[0].c_str());
	}
}


// -----------------------------------------------------------------------------------------------
//
//                                The class starts here
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor
**
*/
CRainmeter::CRainmeter() :
	m_TrayWindow(),
	m_Debug(false),
	m_DisableVersionCheck(false),
	m_NewVersion(false),
	m_DesktopWorkAreaChanged(false),
	m_DesktopWorkAreaType(false),
	m_NormalStayDesktop(true),
	m_MenuActive(false),
	m_DisableRDP(false),
	m_DisableDragging(false),
	m_Logging(false),
	m_CurrentParser(),
	m_Window(),
	m_Instance(),
	m_ResourceInstance(),
	m_ResourceLCID(),
	m_GDIplusToken(),
	m_GlobalConfig()
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	InitCommonControls();

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_GDIplusToken, &gdiplusStartupInput, NULL);
}

/*
** Destructor
**
*/
CRainmeter::~CRainmeter()
{
	KillTimer(m_Window, TIMER_NETSTATS);

	DeleteMeterWindow(NULL);

	delete m_TrayWindow;

	CSystem::Finalize();

	CMeasureNet::UpdateIFTable();
	CMeasureNet::UpdateStats();
	WriteStats(true);

	CMeasureNet::FinalizeNewApi();

	CMeterString::FreeFontCache();

	// Change the work area back
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(true);
	}

	FinalizeLitestep();

	if (m_ResourceInstance) FreeLibrary(m_ResourceInstance);

	CoUninitialize();

	GdiplusShutdown(m_GDIplusToken);
}

/*
** The main initialization function for the module.
**
*/
int CRainmeter::Initialize(LPCWSTR szPath)
{
	int result = 0;

	m_Instance = GetModuleHandle(L"Rainmeter");

	WNDCLASS wc = {0};
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.hInstance = m_Instance;
	wc.lpszClassName = RAINMETER_CLASS_NAME;
	RegisterClass(&wc);

	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		RAINMETER_CLASS_NAME,
		RAINMETER_WINDOW_NAME,
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		m_Instance,
		NULL);

	if (!m_Window) return 1;

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	GetModuleFileName(m_Instance, buffer, MAX_LINE_LENGTH);

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(buffer, L'\\');

	m_Path.assign(buffer, pos ? pos - buffer + 1 : 0);

	InitalizeLitestep();

	bool bDefaultIniLocation = false;

	if (*szPath)
	{
		// The command line defines the location of Rainmeter.ini (or whatever it calls it).
		std::wstring iniFile = szPath;
		if (iniFile[0] == L'"')
		{
			if (iniFile.length() == 1)
			{
				iniFile.clear();
			}
			else if (iniFile[iniFile.length() - 1] == L'"')
			{
				iniFile.assign(iniFile, 1, iniFile.length() - 2);
			}
		}

		ExpandEnvironmentVariables(iniFile);

		if (iniFile.empty() || CSystem::IsPathSeparator(iniFile[iniFile.length() - 1]))
		{
			iniFile += L"Rainmeter.ini";
		}
		else if (iniFile.length() <= 4 || _wcsicmp(iniFile.c_str() + (iniFile.length() - 4), L".ini") != 0)
		{
			iniFile += L"\\Rainmeter.ini";
		}

		if (!CSystem::IsPathSeparator(iniFile[0]) && iniFile.find_first_of(L':') == std::wstring::npos)
		{
			// Make absolute path
			iniFile.insert(0, m_Path);
		}

		m_IniFile = iniFile;

		// If the ini file doesn't exist, create a default Rainmeter.ini file.
		if (_waccess(m_IniFile.c_str(), 0) == -1)
		{
			CreateDefaultConfigFile();
		}
		bDefaultIniLocation = true;
	}
	else
	{
		m_IniFile = m_Path;
		m_IniFile += L"Rainmeter.ini";

		// If the ini file doesn't exist in the program folder store it to the %APPDATA% instead so that things work better in Vista/Win7
		if (_waccess(m_IniFile.c_str(), 0) == -1)
		{
			m_IniFile = L"%APPDATA%\\Rainmeter\\Rainmeter.ini";
			ExpandEnvironmentVariables(m_IniFile);
			bDefaultIniLocation = true;

			// If the ini file doesn't exist in the %APPDATA% either, create a default Rainmeter.ini file.
			if (_waccess(m_IniFile.c_str(), 0) == -1)
			{
				CreateDefaultConfigFile();
			}
		}
	}

	// Set the log file and stats file location
	m_LogFile = m_StatsFile = m_IniFile;
	size_t len = m_LogFile.length();
	if (len > 4 && _wcsicmp(m_LogFile.c_str() + (len - 4), L".ini") == 0)
	{
		m_LogFile.replace(len - 4, 4, L".log");
		m_StatsFile.replace(len - 4, 4, L".stats");
	}
	else
	{
		m_LogFile += L".log";	// Append the extension so that we don't accidentally overwrite the ini file
		m_StatsFile += L".stats";
	}

	// Read Logging settings beforehand
	m_Logging = 0!=GetPrivateProfileInt(L"Rainmeter", L"Logging", 0, m_IniFile.c_str());
	m_Debug = 0!=GetPrivateProfileInt(L"Rainmeter", L"Debug", 0, m_IniFile.c_str());

	// Determine the language resource to load
	std::wstring resource = m_Path + L"Languages\\";
	if (GetPrivateProfileString(L"Rainmeter", L"Language", L"", buffer, MAX_LINE_LENGTH, m_IniFile.c_str()) == 0)
	{
		// Use whatever the user selected for the installer
		DWORD size = MAX_LINE_LENGTH;
		HKEY hKey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Rainmeter", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
		{
			DWORD type = 0;
			if (RegQueryValueEx(hKey, L"Language", NULL, &type, (LPBYTE)buffer, (LPDWORD)&size) != ERROR_SUCCESS ||
				type != REG_SZ)
			{
				buffer[0] = L'\0';
			}
			RegCloseKey(hKey);
		}
	}
	if (buffer[0] != L'\0')
	{
		// Try selected language
		m_ResourceLCID = wcstoul(buffer, NULL, 10);
		resource += buffer;
		resource += L".dll";

		m_ResourceInstance = LoadLibraryEx(resource.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	}
	if (!m_ResourceInstance)
	{
		// Try English
		resource = m_Path;
		resource += L"Languages\\1033.dll";
		m_ResourceInstance = LoadLibraryEx(resource.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		m_ResourceLCID = 1033;
		if (!m_ResourceInstance)
		{
			throw CError(L"Unable to load language library");
		}
	}

	// Reset log file
	CSystem::RemoveFile(m_LogFile);

	if (m_Logging)
	{
		StartLogging();
	}

	m_PluginPath = m_AddonPath = m_SkinPath = m_Path;
	m_PluginPath += L"Plugins\\";
	m_AddonPath += L"Addons\\";
	m_SkinPath += L"Skins\\";

	// Read the skin folder from the ini file
	len = GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, m_IniFile.c_str());
	if (len > 0)
	{
		m_SkinPath.assign(buffer, len);
		ExpandEnvironmentVariables(m_SkinPath);

		if (!m_SkinPath.empty())
		{
			if (!CSystem::IsPathSeparator(m_SkinPath[m_SkinPath.length() - 1]))
			{
				m_SkinPath += L'\\';
			}
		}
	}
	else if (bDefaultIniLocation)
	{
		// If the skin path is not defined in the Rainmeter.ini file use My Documents/Rainmeter/Skins
		buffer[0] = L'\0';
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, buffer);
		if (SUCCEEDED(hr))
		{
			// Make the folders if they don't exist yet
			m_SkinPath = buffer;
			m_SkinPath += L"\\Rainmeter";
			CreateDirectory(m_SkinPath.c_str(), NULL);
			m_SkinPath += L"\\Skins\\";
			DWORD result = CreateDirectory(m_SkinPath.c_str(), NULL);
			if (result != 0)
			{
				// The folder was created successfully which means that it wasn't available yet.
				// Copy the default skin to the Skins folder
				std::wstring strFrom(m_Path + L"Skins\\*.*");
				std::wstring strTo(m_SkinPath);
				CSystem::CopyFiles(strFrom, strTo);

				// Copy also the themes to the %APPDATA%
				strFrom = m_Path;
				strFrom += L"Themes\\*.*";
				strTo = GetSettingsPath();
				strTo += L"Themes\\";
				CreateDirectory(strTo.c_str(), NULL);
				CSystem::CopyFiles(strFrom, strTo);
			}
		}
		else
		{
			Log(LOG_WARNING, L"Documents folder not found");
		}

		WritePrivateProfileString(L"Rainmeter", L"SkinPath", m_SkinPath.c_str(), m_IniFile.c_str());
	}

	delete [] buffer;
	buffer = NULL;

	LogWithArgs(LOG_NOTICE, L"Path: %s", m_Path.c_str());
	LogWithArgs(LOG_NOTICE, L"IniFile: %s", m_IniFile.c_str());
	LogWithArgs(LOG_NOTICE, L"SkinPath: %s", m_SkinPath.c_str());

	// Extract volume path from program path
	// E.g.:
	//  "C:\path\" to "C:"
	//  "\\server\share\" to "\\server\share"
	//  "\\server\C:\path\" to "\\server\C:"
	std::wstring::size_type loc;
	if ((loc = m_Path.find_first_of(L':')) != std::wstring::npos)
	{
		m_Drive.assign(m_Path, 0, loc + 1);
	}
	else if (CSystem::IsUNCPath(m_Path))
	{
		if ((loc = m_Path.find_first_of(L"\\/", 2)) != std::wstring::npos)
		{
			std::wstring::size_type loc2;
			if ((loc2 = m_Path.find_first_of(L"\\/", loc + 1)) != std::wstring::npos || loc != (m_Path.length() - 1))
			{
				loc = loc2;
			}
		}
		m_Drive.assign(m_Path, 0, loc);
	}

	// Test that the Rainmeter.ini file is writable
	TestSettingsFile(bDefaultIniLocation);

	CSystem::Initialize(m_Instance);
	CMeasureNet::InitializeNewApi();

	if (m_Debug)
	{
		Log(LOG_DEBUG, L"Enumerating font families...");
		CMeterString::EnumerateInstalledFontFamilies();
	}

	// Tray must exist before configs are read
	m_TrayWindow = new CTrayWindow(m_Instance);

	ReloadSettings();

	if (m_ConfigStrings.empty())
	{
		std::wstring error = GetFormattedString(ID_STR_NOAVAILABLESKINS, m_SkinPath.c_str());
		MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
	}

	WritePrivateProfileString(L"Rainmeter", L"CheckUpdate", NULL , m_IniFile.c_str());

	ResetStats();
	ReadStats();

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(false);
	}

	// Create meter windows for active configs
	ActivateActiveConfigs();

	if (!m_DisableVersionCheck)
	{
		CheckUpdate();
	}

	return result;	// All is OK
}

int CRainmeter::MessagePump()
{
	MSG msg;
	BOOL ret;

	HACCEL hAccel = LoadAccelerators(m_Instance, MAKEINTRESOURCE(IDR_DIALOG_ACCELERATORS));

	// Run the standard window message loop
	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (ret == -1)
		{
			break;
		}
		else if (!CDialog::GetActiveDialogWindow() ||
			!TranslateAccelerator(CDialog::GetActiveTabWindow(), hAccel, &msg) ||
			!IsDialogMessage(CDialog::GetActiveDialogWindow(), &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK CRainmeter::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COPYDATA:
		{
			COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
			if (cds)
			{
				const WCHAR* data = (const WCHAR*)cds->lpData;
				if (cds->dwData == 1 && (cds->cbData > 0))
				{
					Rainmeter->DelayedExecuteCommand(data);
				}
				else if (cds->dwData == SIZE_MAX && _wcsicmp(GetCommandLine(), data) == 0)
				{
					return SIZE_MAX;
				}
			}
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_NETSTATS)
		{
			CMeasureNet::UpdateIFTable();
			CMeasureNet::UpdateStats();
			Rainmeter->WriteStats(false);
		}
		break;

	case WM_RAINMETER_DELAYED_REFRESH_ALL:
		Rainmeter->RefreshAll();
		break;

	case WM_RAINMETER_DELAYED_EXECUTE:
		if (lParam)
		{
			// Execute bang
			WCHAR* bang = (WCHAR*)lParam;
			Rainmeter->ExecuteCommand(bang, NULL);
			free(bang);  // _wcsdup()
		}
		break;

	case WM_RAINMETER_EXECUTE:
		Rainmeter->ExecuteCommand((const WCHAR*)lParam, (CMeterWindow*)wParam);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void CRainmeter::SetNetworkStatisticsTimer()
{
	static bool set = SetTimer(m_Window, TIMER_NETSTATS, INTERVAL_NETSTATS, NULL);
}

/*
** Creates the default Rainmeter.ini file with illustro\System enabled.
**
*/
void CRainmeter::CreateDefaultConfigFile()
{
	size_t pos = m_IniFile.find_last_of(L'\\');
	if (pos != std::wstring::npos)
	{
		std::wstring strPath(m_IniFile, 0, pos);
		CreateDirectory(strPath.c_str(), NULL);
	}

	std::wstring defaultIni = GetPath() + L"Themes\\illustro default\\Rainmeter.thm";
	if (_waccess(defaultIni.c_str(), 0) == -1)
	{
		WritePrivateProfileString(L"Rainmeter", L"\r\n[illustro\\System]\r\nActive", L"1", m_IniFile.c_str());
	}
	else
	{
		CSystem::CopyFiles(defaultIni, m_IniFile);
	}
}

void CRainmeter::ReloadSettings()
{
	ScanForConfigs(m_SkinPath);
	ScanForThemes(GetSettingsPath() + L"Themes");
	ReadGeneralSettings(m_IniFile);
}

void CRainmeter::EditSettings()
{
	std::wstring command = m_ConfigEditor + L" \"";
	command += m_IniFile;
	command += L'"';
	RunCommand(m_Window, command.c_str(), SW_SHOWNORMAL);
}

void CRainmeter::EditSkinFile(const std::wstring& name, const std::wstring& iniFile)
{
	std::wstring command = m_SkinPath + name;
	command += L'\\';
	command += iniFile;
	bool writable = CSystem::IsFileWritable(command.c_str());

	command.insert(0, L" \"");
	command.insert(0, m_ConfigEditor);
	command += L'"';

	// Execute as admin if in protected location
	RunCommand(m_Window, command.c_str(), SW_SHOWNORMAL, !writable);
}

void CRainmeter::OpenSkinFolder(const std::wstring& name)
{
	std::wstring command = L'"' + m_SkinPath;
	if (!name.empty()) command += name;
	command += L'"';
	RunCommand(m_Window, command.c_str(), SW_SHOWNORMAL);
}

void CRainmeter::ActivateActiveConfigs()
{
	std::multimap<int, int>::const_iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		const CONFIG& configS = m_ConfigStrings[(*iter).second];
		if (configS.active > 0 && configS.active <= (int)configS.iniFiles.size())
		{
			ActivateConfig((*iter).second, configS.active - 1);
		}
	}
}

void CRainmeter::ActivateConfig(int configIndex, int iniIndex)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size() &&
		iniIndex >= 0 && iniIndex < (int)m_ConfigStrings[configIndex].iniFiles.size())
	{
		const std::wstring& skinIniFile = m_ConfigStrings[configIndex].iniFiles[iniIndex];
		const std::wstring& skinConfig = m_ConfigStrings[configIndex].config;

		// Verify that the config is not already active
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.find(skinConfig);
		if (iter != m_MeterWindows.end())
		{
			if (((*iter).second)->GetSkinIniFile() == skinIniFile)
			{
				LogWithArgs(LOG_WARNING, L"!ActivateConfig: \"%s\" already active", skinConfig.c_str());
				return;
			}
			else
			{
				// Deactivate the existing config
				DeactivateConfig((*iter).second, configIndex);
			}
		}

		// Verify whether the ini-file exists
		std::wstring skinIniPath = m_SkinPath + skinConfig;
		skinIniPath += L'\\';
		skinIniPath += skinIniFile;

		if (_waccess(skinIniPath.c_str(), 0) == -1)
		{
			std::wstring message = GetFormattedString(ID_STR_UNABLETOACTIVATESKIN, skinConfig.c_str(), skinIniFile.c_str());
			MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			return;
		}

		m_ConfigStrings[configIndex].active = iniIndex + 1;
		WriteActive(skinConfig, iniIndex);

		CreateMeterWindow(skinConfig, skinIniFile);
	}
}

void CRainmeter::DeactivateConfig(CMeterWindow* meterWindow, int configIndex, bool save)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size())
	{
		m_ConfigStrings[configIndex].active = 0;	// Deactivate the config
	}
	else if (configIndex == -1 && meterWindow)
	{
		// Deactivate the config by using the meter window's config name
		const WCHAR* skinConfig = meterWindow->GetSkinName().c_str();
		for (size_t i = 0, isize = m_ConfigStrings.size(); i < isize; ++i)
		{
			if (_wcsicmp(skinConfig, m_ConfigStrings[i].config.c_str()) == 0)
			{
				m_ConfigStrings[i].active = 0;
				break;
			}
		}
	}

	if (meterWindow)
	{
		if (save)
		{
			// Disable the config in the ini-file
			WriteActive(meterWindow->GetSkinName(), -1);
		}

		meterWindow->Deactivate();
	}
}

void CRainmeter::ToggleConfig(int configIndex, int iniIndex)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size() &&
		iniIndex >= 0 && iniIndex < (int)m_ConfigStrings[configIndex].iniFiles.size())
	{
		if (m_ConfigStrings[configIndex].active == iniIndex + 1)
		{
			CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(m_ConfigStrings[configIndex].config);
			DeactivateConfig(meterWindow, configIndex);
		}
		else
		{
			ActivateConfig(configIndex, iniIndex);
		}
	}
}

void CRainmeter::WriteActive(const std::wstring& config, int iniIndex)
{
	WCHAR buffer[32];
	_itow_s(iniIndex + 1, buffer, 10);
	WritePrivateProfileString(config.c_str(), L"Active", buffer, m_IniFile.c_str());
}

void CRainmeter::CreateMeterWindow(const std::wstring& config, const std::wstring& iniFile)
{
	CMeterWindow* mw = new CMeterWindow(config, iniFile);

	if (mw)
	{
		m_MeterWindows[config] = mw;

		try
		{
			mw->Initialize();

			CDialogAbout::UpdateSkins();
			CDialogManage::UpdateSkins(mw);
		}
		catch (CError& error)
		{
			DeactivateConfig(mw, -1);
			LogError(error);
		}
	}
}

void CRainmeter::DeleteMeterWindow(CMeterWindow* meterWindow, bool force)
{
	std::map<std::wstring, CMeterWindow*>::iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if (meterWindow == NULL)
		{
			// Delete all meter windows
			CDialogManage::UpdateSkins((*iter).second, true);
			delete (*iter).second;
		}
		else if ((*iter).second == meterWindow)
		{
			m_MeterWindows.erase(iter);
			force = true;
			break;
		}
	}

	if (meterWindow == NULL)
	{
		m_MeterWindows.clear();
	}
	else if (force)
	{
		CDialogManage::UpdateSkins(meterWindow, true);
		delete meterWindow;
	}

	CDialogAbout::UpdateSkins();
}

CMeterWindow* CRainmeter::GetMeterWindow(const std::wstring& config)
{
	const WCHAR* configName = config.c_str();
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if (_wcsicmp((*iter).first.c_str(), configName) == 0)
		{
			return (*iter).second;
		}
	}

	return NULL;
}

CMeterWindow* CRainmeter::GetMeterWindowByINI(const std::wstring& ini_searching)
{
	if (_wcsnicmp(m_SkinPath.c_str(), ini_searching.c_str(), m_SkinPath.length()) == 0)
	{
		const std::wstring config_searching = ini_searching.substr(m_SkinPath.length());

		std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
		for (; iter != m_MeterWindows.end(); ++iter)
		{
			std::wstring config_current = (*iter).second->GetSkinName() + L'\\';
			config_current += (*iter).second->GetSkinIniFile();

			if (_wcsicmp(config_current.c_str(), config_searching.c_str()) == 0)
			{
				return (*iter).second;
			}
		}
	}

	return NULL;
}

std::pair<int, int> CRainmeter::GetMeterWindowIndex(const std::wstring& config, const std::wstring& iniFile)
{
	const WCHAR* configName = config.c_str();
	std::pair<int, int> indexes;

	for (int i = 0, isize = (int)m_ConfigStrings.size(); i < isize; ++i)
	{
		const CONFIG& configS = m_ConfigStrings[i];
		if (_wcsicmp(configS.config.c_str(), configName) == 0)
		{
			const WCHAR* iniFileName = iniFile.c_str();
			for (int j = 0, jsize = (int)configS.iniFiles.size(); j < jsize; ++j)
			{
				if (_wcsicmp(configS.iniFiles[j].c_str(), iniFileName) == 0)
				{
					indexes = std::make_pair(i, j);
					return indexes;
				}
			}
		}
	}

	indexes = std::make_pair(-1, -1);  // error
	return indexes;
}

std::pair<int, int> CRainmeter::GetMeterWindowIndex(UINT menuCommand)
{
	std::pair<int, int> indexes;

	if (menuCommand >= ID_CONFIG_FIRST && menuCommand <= ID_CONFIG_LAST)
	{
		// Check which config was selected
		for (size_t i = 0, isize = m_ConfigStrings.size(); i < isize; ++i)
		{
			const CONFIG& configS = m_ConfigStrings[i];
			if (menuCommand >= configS.commandBase &&
				menuCommand < (configS.commandBase + configS.iniFiles.size()))
			{
				indexes = std::make_pair(i, menuCommand - configS.commandBase);
				return indexes;
			}
		}
	}

	indexes = std::make_pair(-1, -1);  // error
	return indexes;
}

CMeterWindow* CRainmeter::GetMeterWindow(HWND hwnd)
{
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if ((*iter).second->GetWindow() == hwnd)
		{
			return (*iter).second;
		}
	}

	return NULL;
}

void CRainmeter::GetMeterWindowsByLoadOrder(std::multimap<int, CMeterWindow*>& windows, const std::wstring& group)
{
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		CMeterWindow* mw = (*iter).second;
		if (mw && (group.empty() || mw->BelongsToGroup(group)))
		{
			windows.insert(std::pair<int, CMeterWindow*>(GetLoadOrder((*iter).first), mw));
		}
	}
}

void CRainmeter::SetLoadOrder(int configIndex, int order)
{
	std::multimap<int, int>::iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		if ((*iter).second == configIndex)  // already exists
		{
			if ((*iter).first != order)
			{
				m_ConfigOrders.erase(iter);
				break;
			}
			else
			{
				return;
			}
		}
	}

	m_ConfigOrders.insert(std::pair<int, int>(order, configIndex));
}

int CRainmeter::GetLoadOrder(const std::wstring& config)
{
	const WCHAR* configName = config.c_str();
	std::multimap<int, int>::const_iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		if (wcscmp(m_ConfigStrings[(*iter).second].config.c_str(), configName) == 0)
		{
			return (*iter).first;
		}
	}

	// LoadOrder not specified
	return 0;
}

/*
** Scans all the subfolders and locates the ini-files.
*/
void CRainmeter::ScanForConfigs(const std::wstring& path)
{
	m_ConfigStrings.clear();
	m_ConfigMenu.clear();
	m_ConfigOrders.clear();

	ScanForConfigsRecursive(path, L"", 0, m_ConfigMenu, false);
}

int CRainmeter::ScanForConfigsRecursive(const std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu, bool DontRecurse)
{
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile
	std::list<std::wstring> folders;
	const bool first = base.empty();

	// Scan all .ini files and folders from the subfolder
	std::wstring filter = path + base;
	filter += L"\\*";

	hSearch = FindFirstFileEx(
		filter.c_str(),
		(CSystem::GetOSPlatform() >= OSPLATFORM_7) ? FindExInfoBasic : FindExInfoStandard,
		&fileData,
		FindExSearchNameMatch,
		NULL,
		0);

	if (hSearch != INVALID_HANDLE_VALUE)
	{
		CONFIG config;
		config.config = base;
		config.commandBase = ID_CONFIG_FIRST + index;
		config.active = 0;

		do
		{
			const std::wstring filename = fileData.cFileName;

			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wcscmp(L".", fileData.cFileName) != 0 &&
					wcscmp(L"..", fileData.cFileName) != 0 &&
					!(first && wcscmp(L"Backup", fileData.cFileName) == 0))		// Skip the backup folder
				{
					folders.push_back(filename);
				}
			}
			else if (!first)
			{
				// Check whether the extension is ".ini"
				size_t filenameLen = filename.size();
				if (filenameLen >= 4 && _wcsicmp(fileData.cFileName + (filenameLen - 4), L".ini") == 0)
				{
					CONFIGMENU menuItem;
					menuItem.name = filename;
					menuItem.index = m_ConfigStrings.size();
					menu.push_back(std::move(menuItem));

					config.iniFiles.push_back(filename);
					++index;
				}
			}
		}
		while (FindNextFile(hSearch, &fileData));

		FindClose(hSearch);

		if (!config.iniFiles.empty())
		{
			m_ConfigStrings.push_back(std::move(config));
		}
	}

	if (!first)
	{
		base += L'\\';
	}

	menu.reserve(menu.size() + folders.size());

	std::list<std::wstring>::const_iterator iter = folders.begin();
	for ( ; iter != folders.end(); ++iter)
	{
		CONFIGMENU menuItem;
		menuItem.name = (*iter);
		menuItem.index = -1;
		menu.push_back(std::move(menuItem));

		if (!DontRecurse)
		{
			std::vector<CONFIGMENU>::iterator iter2 = menu.end() - 1;
			index = ScanForConfigsRecursive(path, base + (*iter), index, (*iter2).children, false);

			// Remove menu item if it has no child
			if ((*iter2).children.empty())
			{
				menu.erase(iter2);
			}
		}
	}

	return index;
}

/*
** Scans the given folder for themes
*/
void CRainmeter::ScanForThemes(const std::wstring& path)
{
	m_Themes.clear();

	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	// Scan for folders
	std::wstring folders = path + L"\\*";

	hSearch = FindFirstFileEx(
		folders.c_str(),
		(CSystem::GetOSPlatform() >= OSPLATFORM_7) ? FindExInfoBasic : FindExInfoStandard,
		&fileData,
		FindExSearchNameMatch,
		NULL,
		0);

	if (hSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
				wcscmp(L".", fileData.cFileName) != 0 &&
				wcscmp(L"..", fileData.cFileName) != 0)
			{
				m_Themes.push_back(fileData.cFileName);
			}
		}
		while (FindNextFile(hSearch, &fileData));

		FindClose(hSearch);
	}
}

void CRainmeter::ExecuteBang(const std::wstring& name, std::wstring& arg, CMeterWindow* meterWindow)
{
	const WCHAR* bang = name.c_str();
	const WCHAR* args = arg.c_str();

	if (_wcsnicmp(bang, L"Rainmeter", 9) == 0)
	{
		// Skip "Rainmeter"
		bang += 9;
	}

	if (_wcsicmp(bang, L"Refresh") == 0)
	{
		BangWithArgs(BANG_REFRESH, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"RefreshApp") == 0)
	{
		// Refresh needs to be delayed since it crashes if done during Update()
		PostMessage(m_Window, WM_RAINMETER_DELAYED_REFRESH_ALL, (WPARAM)NULL, (LPARAM)NULL);
	}
	else if (_wcsicmp(bang, L"Redraw") == 0)
	{
		BangWithArgs(BANG_REDRAW, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"Update") == 0)
	{
		BangWithArgs(BANG_UPDATE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"Hide") == 0)
	{
		BangWithArgs(BANG_HIDE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"Show") == 0)
	{
		BangWithArgs(BANG_SHOW, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"Toggle") == 0)
	{
		BangWithArgs(BANG_TOGGLE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideFade") == 0)
	{
		BangWithArgs(BANG_HIDEFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowFade") == 0)
	{
		BangWithArgs(BANG_SHOWFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleFade") == 0)
	{
		BangWithArgs(BANG_TOGGLEFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideMeter") == 0)
	{
		BangWithArgs(BANG_HIDEMETER, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowMeter") == 0)
	{
		BangWithArgs(BANG_SHOWMETER, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleMeter") == 0)
	{
		BangWithArgs(BANG_TOGGLEMETER, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"MoveMeter") == 0)
	{
		BangWithArgs(BANG_MOVEMETER, args, 3, meterWindow);
	}
	else if (_wcsicmp(bang, L"UpdateMeter") == 0)
	{
		BangWithArgs(BANG_UPDATEMETER, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"DisableMeasure") == 0)
	{
		BangWithArgs(BANG_DISABLEMEASURE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"EnableMeasure") == 0)
	{
		BangWithArgs(BANG_ENABLEMEASURE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleMeasure") == 0)
	{
		BangWithArgs(BANG_TOGGLEMEASURE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"UpdateMeasure") == 0)
	{
		BangWithArgs(BANG_UPDATEMEASURE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"CommandMeasure") == 0)
	{
		BangWithArgs(BANG_COMMANDMEASURE, args, 2, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowBlur") == 0)
	{
		BangWithArgs(BANG_SHOWBLUR, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideBlur") == 0)
	{
		BangWithArgs(BANG_HIDEBLUR, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleBlur") == 0)
	{
		BangWithArgs(BANG_TOGGLEBLUR, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"AddBlur") == 0)
	{
		BangWithArgs(BANG_ADDBLUR, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"RemoveBlur") == 0)
	{
		BangWithArgs(BANG_REMOVEBLUR, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ActivateConfig") == 0)
	{
		Bang_ActivateConfig(args);
	}
	else if (_wcsicmp(bang, L"DeactivateConfig") == 0)
	{
		Bang_DeactivateConfig(args, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleConfig") == 0)
	{
		Bang_ToggleConfig(args);
	}
	else if (_wcsicmp(bang, L"Move") == 0)
	{
		BangWithArgs(BANG_MOVE, args, 2, meterWindow);
	}
	else if (_wcsicmp(bang, L"ZPos") == 0 || _wcsicmp(bang, L"ChangeZPos") == 0)	// For backwards compatibility
	{
		BangWithArgs(BANG_ZPOS, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ClickThrough") == 0)
	{
		BangWithArgs(BANG_CLICKTHROUGH, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"Draggable") == 0)
	{
		BangWithArgs(BANG_DRAGGABLE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SnapEdges") == 0)
	{
		BangWithArgs(BANG_SNAPEDGES, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"KeepOnScreen") == 0)
	{
		BangWithArgs(BANG_KEEPONSCREEN, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetTransparency") == 0)
	{
		BangWithArgs(BANG_SETTRANSPARENCY, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetVariable") == 0)
	{
		BangWithArgs(BANG_SETVARIABLE, args, 2, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetOption") == 0)
	{
		BangWithArgs(BANG_SETOPTION, args, 3, meterWindow);
	}
	else if (_wcsicmp(bang, L"RefreshGroup") == 0)
	{
		BangGroupWithArgs(BANG_REFRESH, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"UpdateGroup") == 0)
	{
		BangGroupWithArgs(BANG_UPDATE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"RedrawGroup") == 0)
	{
		BangGroupWithArgs(BANG_REDRAW, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideGroup") == 0)
	{
		BangGroupWithArgs(BANG_HIDE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowGroup") == 0)
	{
		BangGroupWithArgs(BANG_SHOW, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleGroup") == 0)
	{
		BangGroupWithArgs(BANG_TOGGLE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideFadeGroup") == 0)
	{
		BangGroupWithArgs(BANG_HIDEFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowFadeGroup") == 0)
	{
		BangGroupWithArgs(BANG_SHOWFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleFadeGroup") == 0)
	{
		BangGroupWithArgs(BANG_TOGGLEFADE, args, 0, meterWindow);
	}
	else if (_wcsicmp(bang, L"HideMeterGroup") == 0)
	{
		BangWithArgs(BANG_HIDEMETERGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ShowMeterGroup") == 0)
	{
		BangWithArgs(BANG_SHOWMETERGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleMeterGroup") == 0)
	{
		BangWithArgs(BANG_TOGGLEMETERGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"UpdateMeterGroup") == 0)
	{
		BangWithArgs(BANG_UPDATEMETERGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"DisableMeasureGroup") == 0)
	{
		BangWithArgs(BANG_DISABLEMEASUREGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"EnableMeasureGroup") == 0)
	{
		BangWithArgs(BANG_ENABLEMEASUREGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleMeasureGroup") == 0)
	{
		BangWithArgs(BANG_TOGGLEMEASUREGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"UpdateMeasureGroup") == 0)
	{
		BangWithArgs(BANG_UPDATEMEASUREGROUP, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"DeactivateConfigGroup") == 0)
	{
		Bang_DeactivateConfigGroup(args);
	}
	else if (_wcsicmp(bang, L"ZPosGroup") == 0)
	{
		BangGroupWithArgs(BANG_ZPOS, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"ClickThroughGroup") == 0)
	{
		BangGroupWithArgs(BANG_CLICKTHROUGH, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"DraggableGroup") == 0)
	{
		BangGroupWithArgs(BANG_DRAGGABLE, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SnapEdgesGroup") == 0)
	{
		BangGroupWithArgs(BANG_SNAPEDGES, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"KeepOnScreenGroup") == 0)
	{
		BangGroupWithArgs(BANG_KEEPONSCREEN, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetTransparencyGroup") == 0)
	{
		BangGroupWithArgs(BANG_SETTRANSPARENCY, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetVariableGroup") == 0)
	{
		BangGroupWithArgs(BANG_SETVARIABLE, args, 2, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetOptionGroup") == 0)
	{
		BangWithArgs(BANG_SETOPTIONGROUP, args, 3, meterWindow);
	}
	else if (_wcsicmp(bang, L"WriteKeyValue") == 0)
	{
		Bang_WriteKeyValue(args, meterWindow);
	}
	else if (_wcsicmp(bang, L"PluginBang") == 0)
	{
		BangWithArgs(BANG_PLUGIN, args, 1, meterWindow);
	}
	else if (_wcsicmp(bang, L"SetClip") == 0)
	{
		Bang_SetClip(args);
	}
	else if (_wcsicmp(bang, L"SetWallpaper") == 0)
	{
		Bang_SetWallpaper(args);
	}
	else if (_wcsicmp(bang, L"About") == 0)
	{
		CDialogAbout::Open(args);
	}
	else if (_wcsicmp(bang, L"Manage") == 0)
	{
		CDialogManage::Open(args);
	}
	else if (_wcsicmp(bang, L"SkinMenu") == 0)
	{
		Bang_SkinMenu(args, meterWindow);
	}
	else if (_wcsicmp(bang, L"TrayMenu") == 0)
	{
		Bang_TrayMenu();
	}
	else if (_wcsicmp(bang, L"ResetStats") == 0)
	{
		ResetStats();
	}
	else if (_wcsicmp(bang, L"Log") == 0)
	{
		Bang_Log(args);
	}
	else if (_wcsicmp(bang, L"Quit") == 0)
	{
		// Quit needs to be delayed since it crashes if done during Update()
		PostMessage(GetTrayWindow()->GetWindow(), WM_COMMAND, MAKEWPARAM(IDM_QUIT, 0), (LPARAM)NULL);
	}
	else if (_wcsicmp(name.c_str(), L"Execute") == 0)
	{
		// Special case for multibang execution
		std::wstring::size_type start = std::wstring::npos;
		int count = 0;
		for (size_t i = 0, isize = arg.size(); i < isize; ++i)
		{
			if (args[i] == L'[')
			{
				if (count == 0)
				{
					start = i;
				}
				++count;
			}
			else if (args[i] == L']')
			{
				--count;

				if (count == 0 && start != std::wstring::npos)
				{
					// Change ] to NULL
					arg[i] = L'\0';

					// Skip whitespace
					start = arg.find_first_not_of(L" \t\r\n", start + 1, 4);

					ExecuteCommand(arg.c_str() + start, meterWindow);
				}
			}
			else if (args[i] == L'"' && isize > (i + 2) && args[i + 1] == L'"' && args[i + 2] == L'"')
			{
				i += 3;

				std::wstring::size_type pos = arg.find(L"\"\"\"", i);
				if (pos != std::wstring::npos)
				{
					i = pos + 2;	// Skip "", loop will skip last "
				}
			}
		}
	}
	else if (_wcsicmp(bang, L"LsBoxHook") == 0)
	{
		// Deprecated.
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"Invalid bang: %s", name.c_str());
	}
}

/*
** Runs the given command or bang
**
*/
void CRainmeter::ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow)
{
	if (command[0] == L'!') // Bang
	{
		++command;	// Skip "!"
		std::wstring bang, arg;

		// Find the first space
		const WCHAR* pos = wcschr(command, L' ');
		if (pos)
		{
			bang.assign(command, 0, pos - command);
			arg.assign(pos + 1);
		}
		else
		{
			bang = command;
		}

		if (meterWindow && _wcsnicmp(L"Execute", command, 7) != 0)
		{
			meterWindow->GetParser().ReplaceMeasures(arg);
		}

		ExecuteBang(bang, arg, meterWindow);
	}
	else
	{
		// Check for built-ins
		if (_wcsnicmp(L"PLAY", command, 4) == 0)
		{
			if (command[4] == L' ' ||                      // PLAY
				_wcsnicmp(L"LOOP ", &command[4], 5) == 0)  // PLAYLOOP
			{
				command += 4;	// Skip PLAY

				DWORD flags = SND_FILENAME | SND_ASYNC;

				if (command[0] != L' ')
				{
					flags |= SND_LOOP | SND_NODEFAULT;
					command += 4;	// Skip LOOP
				}

				++command;	// Skip the space
				if (command[0] != L'\0')
				{
					std::wstring sound = command;

					// Strip the quotes
					std::wstring::size_type len = sound.length();
					if (len >= 2 && sound[0] == L'"' && sound[len - 1] == L'"')
					{
						len -= 2;
						sound.assign(sound, 1, len);
					}

					if (meterWindow)
					{
						meterWindow->GetParser().ReplaceMeasures(sound);
						meterWindow->MakePathAbsolute(sound);
					}

					PlaySound(sound.c_str(), NULL, flags);
				}
				return;
			}
			else if (_wcsnicmp(L"STOP", &command[4], 4) == 0)  // PLAYSTOP
			{
				PlaySound(NULL, NULL, SND_PURGE);
				return;
			}
		}

		// Run command
		if (meterWindow)
		{
			std::wstring tmpSz = command;
			meterWindow->GetParser().ReplaceMeasures(tmpSz);
			RunCommand(NULL, tmpSz.c_str(), SW_SHOWNORMAL);
		}
		else
		{
			RunCommand(NULL, command, SW_SHOWNORMAL);
		}
	}
}

/*
** Executes command when current processing is done.
**
*/
void CRainmeter::DelayedExecuteCommand(const WCHAR* command)
{
	WCHAR* bang = _wcsdup(command);
	PostMessage(m_Window, WM_RAINMETER_DELAYED_EXECUTE, (WPARAM)NULL, (LPARAM)bang);
}

/*
** Reads the general settings from the Rainmeter.ini file
**
*/
void CRainmeter::ReadGeneralSettings(const std::wstring& iniFile)
{
	WCHAR buffer[MAX_PATH];

	// Clear old settings
	m_DesktopWorkAreas.clear();

	CConfigParser parser;
	parser.Initialize(iniFile.c_str(), this);

	// Read Logging settings
	m_Logging = 0!=parser.ReadInt(L"Rainmeter", L"Logging", 0);
	m_Debug = 0!=parser.ReadInt(L"Rainmeter", L"Debug", 0);

	if (m_Logging)
	{
		StartLogging();
	}

	if (m_TrayWindow)
	{
		m_TrayWindow->ReadConfig(parser);
	}

	m_GlobalConfig.netInSpeed = parser.ReadFloat(L"Rainmeter", L"NetInSpeed", 0.0);
	m_GlobalConfig.netOutSpeed = parser.ReadFloat(L"Rainmeter", L"NetOutSpeed", 0.0);

	m_DisableDragging = 0!=parser.ReadInt(L"Rainmeter", L"DisableDragging", 0);
	m_DisableRDP = 0!=parser.ReadInt(L"Rainmeter", L"DisableRDP", 0);

	m_ConfigEditor = parser.ReadString(L"Rainmeter", L"ConfigEditor", L"");
	if (m_ConfigEditor.empty())
	{
		// Get the program path associated with .ini files
		DWORD cchOut = MAX_PATH;
		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".ini", L"open", buffer, &cchOut);
		m_ConfigEditor = (SUCCEEDED(hr) && cchOut > 0) ? buffer : L"Notepad";
	}
	if (!m_ConfigEditor.empty() && m_ConfigEditor[0] != L'"')
	{
		m_ConfigEditor.insert(0, 1, L'"');
		m_ConfigEditor += L'"';
	}

	m_LogViewer = parser.ReadString(L"Rainmeter", L"LogViewer", L"");
	if (m_LogViewer.empty())
	{
		// Get the program path associated with .log files
		DWORD cchOut = MAX_PATH;
		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".log", L"open", buffer, &cchOut);
		m_LogViewer = (SUCCEEDED(hr) && cchOut > 0) ? buffer : L"Notepad";
	}
	if (!m_LogViewer.empty() && m_LogViewer[0] != L'"')
	{
		m_LogViewer.insert(0, 1, L'"');
		m_LogViewer += L'"';
	}

	if (m_Debug)
	{
		LogWithArgs(LOG_NOTICE, L"ConfigEditor: %s", m_ConfigEditor.c_str());
		LogWithArgs(LOG_NOTICE, L"LogViewer: %s", m_LogViewer.c_str());
	}

	m_TrayExecuteR = parser.ReadString(L"Rainmeter", L"TrayExecuteR", L"", false);
	m_TrayExecuteM = parser.ReadString(L"Rainmeter", L"TrayExecuteM", L"", false);
	m_TrayExecuteDR = parser.ReadString(L"Rainmeter", L"TrayExecuteDR", L"", false);
	m_TrayExecuteDM = parser.ReadString(L"Rainmeter", L"TrayExecuteDM", L"", false);

	m_DisableVersionCheck = 0!=parser.ReadInt(L"Rainmeter", L"DisableVersionCheck", 0);

	const std::wstring& area = parser.ReadString(L"Rainmeter", L"DesktopWorkArea", L"");
	if (!area.empty())
	{
		m_DesktopWorkAreas[0] = parser.ParseRECT(area.c_str());
		m_DesktopWorkAreaChanged = true;
	}

	for (UINT i = 1; i <= CSystem::GetMonitorCount(); ++i)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"DesktopWorkArea@%i", i);
		const std::wstring& area = parser.ReadString(L"Rainmeter", buffer, L"");
		if (!area.empty())
		{
			m_DesktopWorkAreas[i] = parser.ParseRECT(area.c_str());
			m_DesktopWorkAreaChanged = true;
		}
	}

	m_DesktopWorkAreaType = 0!=parser.ReadInt(L"Rainmeter", L"DesktopWorkAreaType", 0);

	m_NormalStayDesktop = 0!=parser.ReadInt(L"Rainmeter", L"NormalStayDesktop", 1);

	for (int i = 0, isize = (int)m_ConfigStrings.size(); i < isize; ++i)
	{
		CONFIG& configS = m_ConfigStrings[i];
		int active = parser.ReadInt(configS.config.c_str(), L"Active", 0);

		// Make sure there is a ini file available
		if (active > 0 && active <= (int)configS.iniFiles.size())
		{
			configS.active = active;
		}

		int order = parser.ReadInt(configS.config.c_str(), L"LoadOrder", 0);
		SetLoadOrder(i, order);
	}
}

/*
** Refreshes all active meter windows.
** Note: This function calls CMeterWindow::Refresh() directly for synchronization. Be careful about crash.
**
*/
void CRainmeter::RefreshAll()
{
	// Read skins and settings
	ReloadSettings();

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(false);
	}

	// Make the sending order by using LoadOrder
	std::multimap<int, CMeterWindow*> windows;
	GetMeterWindowsByLoadOrder(windows);

	// Prepare the helper window
	CSystem::PrepareHelperWindow();

	// Refresh all
	std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
	for ( ; iter != windows.end(); ++iter)
	{
		CMeterWindow* mw = (*iter).second;
		if (mw)
		{
			// Verify whether the cached information is valid
			int found = 0;
			const WCHAR* skinConfig = mw->GetSkinName().c_str();
			for (int i = 0, isize = (int)m_ConfigStrings.size(); i < isize; ++i)
			{
				CONFIG& configS = m_ConfigStrings[i];
				if (_wcsicmp(skinConfig, configS.config.c_str()) == 0)
				{
					found = 1;
					const WCHAR* skinIniFile = mw->GetSkinIniFile().c_str();
					for (int j = 0, jsize = (int)configS.iniFiles.size(); j < jsize; ++j)
					{
						if (_wcsicmp(skinIniFile, configS.iniFiles[j].c_str()) == 0)
						{
							found = 2;
							if (configS.active != j + 1)
							{
								// Switch to new ini-file order
								configS.active = j + 1;
								WriteActive(mw->GetSkinName(), j);
							}
							break;
						}
					}

					if (found == 1)  // Not found in ini-files
					{
						DeactivateConfig(mw, i);

						std::wstring error = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, skinConfig, skinIniFile);
						MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
					}
					break;
				}
			}

			if (found != 2)
			{
				if (found == 0)  // Not found in configs
				{
					DeactivateConfig(mw, -2);  // -2 = Deactivate the config forcibly

					std::wstring error = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, skinConfig, L"");
					MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}
				continue;
			}

			try
			{
				mw->Refresh(false, true);
			}
			catch (CError& error)
			{
				LogError(error);
			}
		}
	}

	CDialogAbout::UpdateSkins();
	CDialogManage::UpdateSkins(NULL);
}

void CRainmeter::LoadTheme(const std::wstring& name)
{
	// Delete all meter windows
	DeleteMeterWindow(NULL);

	std::wstring backup = GetSettingsPath() + L"Themes\\Backup";
	CreateDirectory(backup.c_str(), NULL);
	backup += L"\\Rainmeter.thm";

	if (_wcsicmp(name.c_str(), L"Backup") == 0)
	{
		// Just load the backup
		CSystem::CopyFiles(backup, m_IniFile);
	}
	else
	{
		// Make a copy of current Rainmeter.ini
		CSystem::CopyFiles(m_IniFile, backup);

		// Replace Rainmeter.ini with theme
		std::wstring theme = GetSettingsPath() + L"Themes\\";
		theme += name;
		std::wstring wallpaper = theme + L"\\RainThemes.bmp";
		theme += L"\\Rainmeter.thm";
		CSystem::CopyFiles(theme, GetIniFile());

		PreserveSetting(backup, L"SkinPath");
		PreserveSetting(backup, L"ConfigEditor");
		PreserveSetting(backup, L"LogViewer");
		PreserveSetting(backup, L"Logging");
		PreserveSetting(backup, L"DisableVersionCheck");
		PreserveSetting(backup, L"Language");
		PreserveSetting(backup, L"NormalStayDesktop");
		PreserveSetting(backup, L"TrayExecuteM", false);
		PreserveSetting(backup, L"TrayExecuteR", false);
		PreserveSetting(backup, L"TrayExecuteDM", false);
		PreserveSetting(backup, L"TrayExecuteDR", false);

		// Set wallpaper if it exists
		if (_waccess(wallpaper.c_str(), 0) != -1)
		{
			SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)wallpaper.c_str(), SPIF_UPDATEINIFILE);
		}
	}

	ReloadSettings();

	// Create meter windows for active configs
	ActivateActiveConfigs();
}

void CRainmeter::PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace)
{
	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];

	if ((replace || GetPrivateProfileString(L"Rainmeter", key, L"", buffer, 4, m_IniFile.c_str()) == 0) &&
		GetPrivateProfileString(L"Rainmeter", key, L"", buffer, MAX_LINE_LENGTH, from.c_str()) > 0)
	{
		WritePrivateProfileString(L"Rainmeter", key, buffer, m_IniFile.c_str());
	}

	delete [] buffer;
}

/*
** Applies given DesktopWorkArea and DesktopWorkArea@n.
**
*/
void CRainmeter::UpdateDesktopWorkArea(bool reset)
{
	bool changed = false;

	if (reset)
	{
		if (!m_OldDesktopWorkAreas.empty())
		{
			for (size_t i = 0, isize = m_OldDesktopWorkAreas.size(); i < isize; ++i)
			{
				RECT r = m_OldDesktopWorkAreas[i];

				BOOL result = SystemParametersInfo(SPI_SETWORKAREA, 0, &r, 0);

				if (m_Debug)
				{
					std::wstring format = L"Resetting WorkArea@%i: L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)";
					if (!result)
					{
						format += L" => FAIL";
					}
					LogWithArgs(LOG_DEBUG, format.c_str(), (int)i + 1, r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
			changed = true;
		}
	}
	else
	{
		const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		if (m_OldDesktopWorkAreas.empty())
		{
			// Store old work areas for changing them back
			for (size_t i = 0; i < CSystem::GetMonitorCount(); ++i)
			{
				m_OldDesktopWorkAreas.push_back(monitors[i].work);
			}
		}

		if (m_Debug)
		{
			LogWithArgs(LOG_DEBUG, L"DesktopWorkAreaType: %s", m_DesktopWorkAreaType ? L"Margin" : L"Default");
		}

		for (UINT i = 0; i <= CSystem::GetMonitorCount(); ++i)
		{
			std::map<UINT, RECT>::const_iterator it = m_DesktopWorkAreas.find(i);
			if (it != m_DesktopWorkAreas.end())
			{
				RECT r = it->second;

				// Move rect to correct offset
				if (m_DesktopWorkAreaType)
				{
					RECT margin = r;
					r = (i == 0) ? monitors[multimonInfo.primary - 1].screen : monitors[i - 1].screen;
					r.left += margin.left;
					r.top += margin.top;
					r.right -= margin.right;
					r.bottom -= margin.bottom;
				}
				else
				{
					if (i != 0)
					{
						const RECT screenRect = monitors[i - 1].screen;
						r.left += screenRect.left;
						r.top += screenRect.top;
						r.right += screenRect.left;
						r.bottom += screenRect.top;
					}
				}

				BOOL result = SystemParametersInfo(SPI_SETWORKAREA, 0, &r, 0);
				if (result)
				{
					changed = true;
				}

				if (m_Debug)
				{
					std::wstring format = L"Applying DesktopWorkArea";
					if (i != 0)
					{
						WCHAR buffer[64];
						size_t len = _snwprintf_s(buffer, _TRUNCATE, L"@%i", i);
						format.append(buffer, len);
					}
					format += L": L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)";
					if (!result)
					{
						format += L" => FAIL";
					}
					LogWithArgs(LOG_DEBUG, format.c_str(), r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
		}
	}

	if (changed && CSystem::GetWindow())
	{
		// Update CSystem::MULTIMONITOR_INFO for for work area variables
		SendMessageTimeout(CSystem::GetWindow(), WM_SETTINGCHANGE, SPI_SETWORKAREA, 0, SMTO_ABORTIFHUNG, 1000, NULL);
	}
}

/*
** Reads the statistics from the ini-file
**
*/
void CRainmeter::ReadStats()
{
	const WCHAR* statsFile = m_StatsFile.c_str();

	// If m_StatsFile doesn't exist, create it and copy the stats section from m_IniFile
	if (_waccess(statsFile, 0) == -1)
	{
		const WCHAR* iniFile = m_IniFile.c_str();
		WCHAR* tmpSz = new WCHAR[SHRT_MAX];	// Max size returned by GetPrivateProfileSection()

		if (GetPrivateProfileSection(L"Statistics", tmpSz, SHRT_MAX, iniFile) > 0)
		{
			WritePrivateProfileString(L"Statistics", NULL, NULL, iniFile);
		}
		else
		{
			tmpSz[0] = tmpSz[1] = L'\0';
		}
		WritePrivateProfileSection(L"Statistics", tmpSz, statsFile);

		delete [] tmpSz;
	}

	// Only Net measure has stats at the moment
	CMeasureNet::ReadStats(statsFile, m_StatsDate);
}

/*
** Writes the statistics to the ini-file. If bForce is false the stats are written only once per an appropriate interval.
**
*/
void CRainmeter::WriteStats(bool bForce)
{
	static ULONGLONG lastWrite = 0;

	ULONGLONG ticks = CSystem::GetTickCount64();

	if (bForce || (lastWrite + INTERVAL_NETSTATS < ticks))
	{
		lastWrite = ticks;

		// Only Net measure has stats at the moment
		const WCHAR* statsFile = m_StatsFile.c_str();
		CMeasureNet::WriteStats(statsFile, m_StatsDate);

		WritePrivateProfileString(NULL, NULL, NULL, statsFile);
	}
}

/*
** Clears the statistics
**
*/
void CRainmeter::ResetStats()
{
	// Set the stats-date string
	struct tm* newtime;
	time_t long_time;
	time(&long_time);
	newtime = localtime(&long_time);
	m_StatsDate = _wasctime(newtime);
	m_StatsDate.erase(m_StatsDate.size() - 1);

	// Only Net measure has stats at the moment
	CMeasureNet::ResetStats();
}

/*
** Opens the context menu in given coordinates.
**
*/
void CRainmeter::ShowContextMenu(POINT pos, CMeterWindow* meterWindow)
{
	if (!m_MenuActive)
	{
		m_MenuActive = true;

		// Show context menu, if no actions were executed
		HMENU menu = LoadMenu(m_ResourceInstance, MAKEINTRESOURCE(IDR_CONTEXT_MENU));

		if (menu)
		{
			HMENU subMenu = GetSubMenu(menu, 0);
			if (subMenu)
			{
				SetMenuDefaultItem(subMenu, IDM_MANAGE, MF_BYCOMMAND);

				if (_waccess(m_LogFile.c_str(), 0) == -1)
				{
					EnableMenuItem(subMenu, IDM_SHOWLOGFILE, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(subMenu, IDM_DELETELOGFILE, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(subMenu, IDM_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
				}
				else
				{
					EnableMenuItem(subMenu, (m_Logging) ? IDM_STARTLOG : IDM_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
				}

				if (m_Debug)
				{
					CheckMenuItem(subMenu, IDM_DEBUGLOG, MF_BYCOMMAND | MF_CHECKED);
				}

				HMENU configMenu = GetSubMenu(subMenu, 4);
				if (configMenu)
				{
					if (!m_ConfigMenu.empty())
					{
						DeleteMenu(configMenu, 0, MF_BYPOSITION);  // "No skins available" menuitem
						CreateConfigMenu(configMenu, m_ConfigMenu);
					}

					if (m_DisableDragging)
					{
						CheckMenuItem(configMenu, IDM_DISABLEDRAG, MF_BYCOMMAND | MF_CHECKED);
					}
				}

				HMENU themeMenu = GetSubMenu(subMenu, 5);
				if (themeMenu)
				{
					if (!m_Themes.empty())
					{
						DeleteMenu(themeMenu, 0, MF_BYPOSITION);  // "No themes available" menuitem
						CreateThemeMenu(themeMenu);
					}
				}

				if (meterWindow)
				{
					HMENU rainmeterMenu = subMenu;
					subMenu = CreateSkinMenu(meterWindow, 0, configMenu);

					WCHAR buffer[256];
					GetMenuString(menu, 0, buffer, 256, MF_BYPOSITION);
					InsertMenu(subMenu, 11, MF_BYPOSITION | MF_POPUP, (UINT_PTR)rainmeterMenu, buffer);
					InsertMenu(subMenu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
				}
				else
				{
					InsertMenu(subMenu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

					// Create a menu for all active configs
					int index = 0;
					std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
					for (; iter != m_MeterWindows.end(); ++iter)
					{
						CMeterWindow* mw = ((*iter).second);
						HMENU skinMenu = CreateSkinMenu(mw, index, configMenu);
						InsertMenu(subMenu, 12, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, mw->GetSkinName().c_str());
						++index;
					}

					// Put Update notifications in the Tray menu
					if (m_NewVersion)
					{
						InsertMenu(subMenu, 0, MF_BYPOSITION, IDM_NEW_VERSION, GetString(ID_STR_UPDATEAVAILABLE));
						HiliteMenuItem(GetTrayWindow()->GetWindow(), subMenu, 0, MF_BYPOSITION | MF_HILITE);
						InsertMenu(subMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
					}
				}

				HWND hWnd = WindowFromPoint(pos);
				if (hWnd != NULL)
				{
					CMeterWindow* mw = GetMeterWindow(hWnd);
					if (mw)
					{
						// Cancel the mouse event beforehand
						mw->SetMouseLeaveEvent(true);
					}
				}

				// Set the window to foreground
				hWnd = meterWindow ? meterWindow->GetWindow() : m_TrayWindow->GetWindow();
				HWND hWndForeground = GetForegroundWindow();
				if (hWndForeground != hWnd)
				{
					DWORD foregroundThreadID = GetWindowThreadProcessId(hWndForeground, NULL);
					DWORD currentThreadID = GetCurrentThreadId();
					AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
					SetForegroundWindow(hWnd);
					AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
				}

				// Show context menu
				TrackPopupMenu(
				  subMenu,
				  TPM_RIGHTBUTTON | TPM_LEFTALIGN,
				  pos.x,
				  pos.y,
				  0,
				  hWnd,
				  NULL
				);

				if (meterWindow)
				{
					DestroyMenu(subMenu);
				}
			}

			DestroyMenu(menu);
		}

		m_MenuActive = false;
	}
}

HMENU CRainmeter::CreateConfigMenu(HMENU configMenu, const std::vector<CONFIGMENU>& configMenuData)
{
	if (!configMenuData.empty())
	{
		if (!configMenu)
		{
			configMenu = CreatePopupMenu();
		}

		bool separator = false;
		for (int i = 0, j = 0, isize = (int)configMenuData.size(); i < isize; ++i)
		{
			const CONFIGMENU& configMenuS = configMenuData[i];
			if (configMenuS.index == -1)
			{
				HMENU submenu = CreateConfigMenu(NULL, configMenuS.children);
				if (submenu)
				{
					if (separator)
					{
						// Insert a separator
						InsertMenu(configMenu, i, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
						++j;
						separator = false;
					}
					InsertMenu(configMenu, i + j, MF_BYPOSITION | MF_POPUP, (UINT_PTR)submenu, configMenuS.name.c_str());
				}
			}
			else
			{
				const CONFIG& configS = m_ConfigStrings[configMenuS.index];
				InsertMenu(configMenu, i, MF_BYPOSITION | ((configS.active == i + 1) ? MF_CHECKED : MF_UNCHECKED), configS.commandBase + i, configMenuS.name.c_str());
				separator = true;
			}
		}

		return configMenu;
	}

	return NULL;
}

void CRainmeter::CreateThemeMenu(HMENU themeMenu)
{
	for (size_t i = 0, isize = m_Themes.size(); i < isize; ++i)
	{
		InsertMenu(themeMenu, i, MF_BYPOSITION, ID_THEME_FIRST + i, m_Themes[i].c_str());
	}
}

HMENU CRainmeter::CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU configMenu)
{
	HMENU skinMenu = LoadMenu(m_ResourceInstance, MAKEINTRESOURCE(IDR_SKIN_MENU));

	if (skinMenu)
	{
		HMENU subSkinMenu = GetSubMenu(skinMenu, 0);
		RemoveMenu(skinMenu, 0, MF_BYPOSITION);
		DestroyMenu(skinMenu);
		skinMenu = subSkinMenu;
	}

	if (skinMenu)
	{
		// Tick the position
		HMENU settingsMenu = GetSubMenu(skinMenu, 4);
		if (settingsMenu)
		{
			HMENU posMenu = GetSubMenu(settingsMenu, 0);
			if (posMenu)
			{
				switch (meterWindow->GetWindowZPosition())
				{
				case ZPOSITION_ONDESKTOP:
					CheckMenuItem(posMenu, IDM_SKIN_ONDESKTOP, MF_BYCOMMAND | MF_CHECKED);
					break;

				case ZPOSITION_ONBOTTOM:
					CheckMenuItem(posMenu, IDM_SKIN_BOTTOM, MF_BYCOMMAND | MF_CHECKED);
					break;

				case ZPOSITION_ONTOP:
					CheckMenuItem(posMenu, IDM_SKIN_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
					break;

				case ZPOSITION_ONTOPMOST:
					CheckMenuItem(posMenu, IDM_SKIN_VERYTOPMOST, MF_BYCOMMAND | MF_CHECKED);
					break;

				default:
					CheckMenuItem(posMenu, IDM_SKIN_NORMAL, MF_BYCOMMAND | MF_CHECKED);
				}

				if (meterWindow->GetXFromRight()) CheckMenuItem(posMenu, IDM_SKIN_FROMRIGHT, MF_BYCOMMAND | MF_CHECKED);
				if (meterWindow->GetYFromBottom()) CheckMenuItem(posMenu, IDM_SKIN_FROMBOTTOM, MF_BYCOMMAND | MF_CHECKED);
				if (meterWindow->GetXPercentage()) CheckMenuItem(posMenu, IDM_SKIN_XPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);
				if (meterWindow->GetYPercentage()) CheckMenuItem(posMenu, IDM_SKIN_YPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);

				HMENU monitorMenu = GetSubMenu(posMenu, 0);
				if (monitorMenu)
				{
					CreateMonitorMenu(monitorMenu, meterWindow);
				}
			}

			// Tick the transparency
			if (!meterWindow->GetNativeTransparency())
			{
				EnableMenuItem(settingsMenu, 1, MF_BYPOSITION | MF_GRAYED);  // "Transparency" menu
				EnableMenuItem(settingsMenu, IDM_SKIN_CLICKTHROUGH, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				HMENU alphaMenu = GetSubMenu(settingsMenu, 1);
				if (alphaMenu)
				{
					int value = (int)(10 - meterWindow->GetAlphaValue() / 25.5);
					value = min(9, value);
					value = max(0, value);
					CheckMenuItem(alphaMenu, value, MF_BYPOSITION | MF_CHECKED);

					switch (meterWindow->GetWindowHide())
					{
					case HIDEMODE_FADEIN:
						CheckMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_CHECKED);
						EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
						break;

					case HIDEMODE_FADEOUT:
						CheckMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_CHECKED);
						EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
						break;

					case HIDEMODE_HIDE:
						EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
						EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
						break;
					}
				}
			}

			// Tick the configs
			switch (meterWindow->GetWindowHide())
			{
			case HIDEMODE_HIDE:
				CheckMenuItem(settingsMenu, IDM_SKIN_HIDEONMOUSE, MF_BYCOMMAND | MF_CHECKED);
				break;

			case HIDEMODE_FADEIN:
			case HIDEMODE_FADEOUT:
				EnableMenuItem(settingsMenu, IDM_SKIN_HIDEONMOUSE, MF_BYCOMMAND | MF_GRAYED);
				break;
			}

			if (meterWindow->GetSnapEdges())
			{
				CheckMenuItem(settingsMenu, IDM_SKIN_SNAPTOEDGES, MF_BYCOMMAND | MF_CHECKED);
			}

			if (meterWindow->GetSavePosition())
			{
				CheckMenuItem(settingsMenu, IDM_SKIN_REMEMBERPOSITION, MF_BYCOMMAND | MF_CHECKED);
			}

			if (m_DisableDragging)
			{
				EnableMenuItem(settingsMenu, IDM_SKIN_DRAGGABLE, MF_BYCOMMAND | MF_GRAYED);
			}
			else if (meterWindow->GetWindowDraggable())
			{
				CheckMenuItem(settingsMenu, IDM_SKIN_DRAGGABLE, MF_BYCOMMAND | MF_CHECKED);
			}

			if (meterWindow->GetClickThrough())
			{
				CheckMenuItem(settingsMenu, IDM_SKIN_CLICKTHROUGH, MF_BYCOMMAND | MF_CHECKED);
			}

			if (meterWindow->GetKeepOnScreen())
			{
				CheckMenuItem(settingsMenu, IDM_SKIN_KEEPONSCREEN, MF_BYCOMMAND | MF_CHECKED);
			}
		}

		// Add the name of the Skin to the menu
		const std::wstring& skinName = meterWindow->GetSkinName();
		ModifyMenu(skinMenu, IDM_SKIN_OPENSKINSFOLDER, MF_BYCOMMAND, IDM_SKIN_OPENSKINSFOLDER, skinName.c_str());
		SetMenuDefaultItem(skinMenu, IDM_SKIN_OPENSKINSFOLDER, FALSE);

		// Remove dummy menuitem from the variants menu
		HMENU variantsMenu = GetSubMenu(skinMenu, 2);
		if (variantsMenu)
		{
			DeleteMenu(variantsMenu, 0, MF_BYPOSITION);
		}

		// Give the menuitem the unique id that depends on the skin
		ChangeSkinIndex(skinMenu, index);

		// Add the variants menu
		if (variantsMenu)
		{
			const WCHAR* skin = skinName.c_str();
			for (int i = 0, isize = (int)m_ConfigStrings.size(); i < isize; ++i)
			{
				const CONFIG& configS = m_ConfigStrings[i];
				if (_wcsicmp(configS.config.c_str(), skin) == 0)
				{
					for (int j = 0, jsize = (int)configS.iniFiles.size(); j < jsize; ++j)
					{
						InsertMenu(variantsMenu, j, MF_BYPOSITION | ((configS.active == j + 1) ? MF_CHECKED : MF_UNCHECKED), configS.commandBase + j, configS.iniFiles[j].c_str());
					}
					break;
				}
			}
		}

		// Add config's root menu
		int itemCount = GetMenuItemCount(configMenu);
		if (itemCount > 0)
		{
			std::wstring root = meterWindow->GetSkinName();
			std::wstring::size_type pos = root.find_first_of(L'\\');
			if (pos != std::wstring::npos)
			{
				root.erase(pos);
			}

			for (int i = 0; i < itemCount; ++i)
			{
				UINT state = GetMenuState(configMenu, i, MF_BYPOSITION);
				if (state == 0xFFFFFFFF || (state & MF_POPUP) == 0) break;

				WCHAR buffer[MAX_PATH];
				if (GetMenuString(configMenu, i, buffer, MAX_PATH, MF_BYPOSITION))
				{
					if (_wcsicmp(root.c_str(), buffer) == 0)
					{
						HMENU configRootMenu = GetSubMenu(configMenu, i);
						if (configRootMenu)
						{
							InsertMenu(skinMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)configRootMenu, root.c_str());
						}
						break;
					}
				}
			}
		}
	}

	return skinMenu;
}

void CRainmeter::CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow)
{
	bool screenDefined = meterWindow->GetXScreenDefined();
	int screenIndex = meterWindow->GetXScreen();

	// for the "Specified monitor" (@n)
	if (CSystem::GetMonitorCount() > 0)
	{
		const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		for (int i = 0, isize = (int)monitors.size(); i < isize; ++i)
		{
			WCHAR buffer[64];
			size_t len = _snwprintf_s(buffer, _TRUNCATE, L"@%i: ", i + 1);

			std::wstring item(buffer, len);

			if (monitors[i].monitorName.size() > 32)
			{
				item.append(monitors[i].monitorName, 0, 32);
				item += L"...";
			}
			else
			{
				item += monitors[i].monitorName;
			}

			InsertMenu(monitorMenu,
				i + 3,
				MF_BYPOSITION | ((screenDefined && screenIndex == i + 1) ? MF_CHECKED : MF_UNCHECKED) | ((!monitors[i].active) ? MF_GRAYED : MF_ENABLED),
				ID_MONITOR_FIRST + i + 1,
				item.c_str());
		}
	}

	// Tick the configs
	if (!screenDefined)
	{
		CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_PRIMARY, MF_BYCOMMAND | MF_CHECKED);
	}

	if (screenDefined && screenIndex == 0)
	{
		CheckMenuItem(monitorMenu, ID_MONITOR_FIRST, MF_BYCOMMAND | MF_CHECKED);
	}

	if (meterWindow->GetAutoSelectScreen())
	{
		CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_AUTOSELECT, MF_BYCOMMAND | MF_CHECKED);
	}
}

void CRainmeter::ChangeSkinIndex(HMENU menu, int index)
{
	int count = GetMenuItemCount(menu);

	for (int i = 0; i < count; ++i)
	{
		HMENU subMenu = GetSubMenu(menu, i);
		if (subMenu)
		{
			ChangeSkinIndex(subMenu, index);
		}
		else
		{
			WCHAR buffer[256];
			GetMenuString(menu, i, buffer, 256, MF_BYPOSITION);
			UINT id = GetMenuItemID(menu, i);
			UINT flags = GetMenuState(menu, i, MF_BYPOSITION);
			ModifyMenu(menu, i, MF_BYPOSITION | flags, id | (index << 16), buffer);
		}
	}
}

void CRainmeter::StartLogging()
{
	// Check if the file exists
	if (_waccess(m_LogFile.c_str(), 0) == -1)
	{
		// Create log file
		HANDLE file = CreateFile(m_LogFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			SetLogging(true);

			// std::wstring text = GetFormattedString(ID_STR_LOGFILECREATED, m_LogFile.c_str());
			// MessageBox(NULL, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		}
		else
		{
			// Disable logging
			SetLogging(false);
	
			std::wstring text = GetFormattedString(ID_STR_LOGFILECREATEFAIL, m_LogFile.c_str());
			MessageBox(NULL, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
		}
	}
	else
	{
		SetLogging(true);
	}
}

void CRainmeter::StopLogging()
{
	SetLogging(false);
}

void CRainmeter::ShowLogFile()
{
	// Check if the file exists
	if (_waccess(m_LogFile.c_str(), 0) != -1)
	{
		std::wstring command = m_LogViewer + L" ";
		command += m_LogFile;
		RunCommand(m_Window, command.c_str(), SW_SHOWNORMAL);
	}
}

void CRainmeter::DeleteLogFile()
{
	// Check if the file exists
	if (_waccess(m_LogFile.c_str(), 0) != -1)
	{
		std::wstring text = GetFormattedString(ID_STR_LOGFILEDELETE, m_LogFile.c_str());
		int res = MessageBox(NULL, text.c_str(), APPNAME, MB_YESNO | MB_TOPMOST | MB_ICONQUESTION);
		if (res == IDYES)
		{
			// Disable logging
			SetLogging(false);
			CSystem::RemoveFile(m_LogFile);
		}
	}
}

void CRainmeter::AddAboutLogInfo(int level, LPCWSTR time, LPCWSTR message)
{
	// Store 20 last items
	LOG_INFO logInfo = {level, time, message};
	m_LogData.push_back(logInfo);
	if (m_LogData.size() > 20)
	{
		m_LogData.pop_front();
	}

	CDialogAbout::AddLogItem(level, time, message);
}

void CRainmeter::SetLogging(bool logging)
{
	m_Logging = logging;
	WritePrivateProfileString(L"Rainmeter", L"Logging", logging ? L"1" : L"0", m_IniFile.c_str());
}

void CRainmeter::SetDebug(bool debug)
{
	m_Debug = debug;
	WritePrivateProfileString(L"Rainmeter", L"Debug", debug ? L"1" : L"0", m_IniFile.c_str());
}

void CRainmeter::SetDisableDragging(bool dragging)
{
	m_DisableDragging = dragging;
	WritePrivateProfileString(L"Rainmeter", L"DisableDragging", dragging ? L"1" : L"0", m_IniFile.c_str());
}

void CRainmeter::SetDisableVersionCheck(bool check)
{
	m_DisableVersionCheck = check;
	WritePrivateProfileString(L"Rainmeter", L"DisableVersionCheck", check ? L"1" : L"0" , m_IniFile.c_str());
}

void CRainmeter::TestSettingsFile(bool bDefaultIniLocation)
{
	if (!CSystem::IsFileWritable(m_IniFile.c_str()))
	{
		std::wstring error = GetString(ID_STR_SETTINGSNOTWRITABLE);

		if (!bDefaultIniLocation)
		{
			std::wstring strTarget = L"%APPDATA%\\Rainmeter\\";
			ExpandEnvironmentVariables(strTarget);

			error += GetFormattedString(ID_STR_SETTINGSMOVEFILE, m_IniFile.c_str(), strTarget.c_str());
		}
		else
		{
			error += GetFormattedString(ID_STR_SETTINGSREADONLY, m_IniFile.c_str());
		}

		MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_ICONERROR);
	}
}

std::wstring CRainmeter::ExtractPath(const std::wstring& strFilePath)
{
	std::wstring::size_type pos = strFilePath.find_last_of(L"\\/");
	if (pos != std::wstring::npos)
	{
		return strFilePath.substr(0, pos + 1);
	}
	return L".\\";
}

void CRainmeter::ExpandEnvironmentVariables(std::wstring& strPath)
{
	std::wstring::size_type pos;

	if ((pos = strPath.find(L'%')) != std::wstring::npos &&
		strPath.find(L'%', pos + 2) != std::wstring::npos)
	{
		DWORD bufSize = 4096;
		WCHAR* buffer = new WCHAR[bufSize];	// lets hope the buffer is large enough...

		// %APPDATA% is a special case
		pos = strPath.find(L"%APPDATA%", pos);
		if (pos != std::wstring::npos)
		{
			HRESULT hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
			if (SUCCEEDED(hr))
			{
				size_t len = wcslen(buffer);
				do
				{
					strPath.replace(pos, 9, buffer, len);
				}
				while ((pos = strPath.find(L"%APPDATA%", pos + len)) != std::wstring::npos);
			}
		}

		if ((pos = strPath.find(L'%')) != std::wstring::npos &&
			strPath.find(L'%', pos + 2) != std::wstring::npos)
		{
			// Expand the environment variables
			do
			{
				DWORD ret = ExpandEnvironmentStrings(strPath.c_str(), buffer, bufSize);
				if (ret == 0)  // Error
				{
					LogWithArgs(LOG_WARNING, L"Unable to expand environment strings in: %s", strPath.c_str());
					break;
				}
				if (ret <= bufSize)  // Fits in the buffer
				{
					strPath.assign(buffer, ret - 1);
					break;
				}

				delete [] buffer;
				bufSize = ret;
				buffer = new WCHAR[bufSize];
			}
			while (true);
		}

		delete [] buffer;
	}
}
