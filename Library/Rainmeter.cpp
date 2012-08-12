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
	if (cmdLine[0] == L'!' || cmdLine[0] == L'[')
	{
		HWND wnd = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);
		if (wnd)
		{
			// Deliver bang to existing Rainmeter instance
			COPYDATASTRUCT cds;
			cds.dwData = 1;
			cds.cbData = (DWORD)((wcslen(cmdLine) + 1) * sizeof(WCHAR));
			cds.lpData = (PVOID)cmdLine;
			SendMessage(wnd, WM_COPYDATA, NULL, (LPARAM)&cds);
			return 0;
		}

		return 1;
	}
	else if (cmdLine[0] == L'"')
	{
		// Strip quotes
		++cmdLine;
		WCHAR* pos = wcsrchr(cmdLine, L'"');
		if (pos)
		{
			*pos = L'\0';
		}
	}

	// Avoid loading a dll from current directory
	SetDllDirectory(L"");

	int ret = 1;

	Rainmeter = new CRainmeter;
	ret = Rainmeter->Initialize(cmdLine);
	if (ret == 0)
	{
		ret = Rainmeter->MessagePump();
	}

	delete Rainmeter;
	Rainmeter = NULL;

	return ret;
}

/*
** Splits the given string into substrings
**
*/
std::vector<std::wstring> CRainmeter::ParseString(LPCTSTR str, CConfigParser* parser)
{
	std::vector<std::wstring> result;

	if (str)
	{
		std::wstring arg = str;

		// Split the argument between first space.
		// Or if string is in quotes, the after the second quote.

		auto addResult = [&](std::wstring& string, bool stripQuotes)
		{
			if (stripQuotes)
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
			}

			if (parser)
			{
				parser->ReplaceMeasures(string);
			}

			result.push_back(string);
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

				addResult(newStr, extra == 1);
			}
			else  // quote or space not found
			{
				addResult(arg, extra == 1);
				arg.clear();
				break;
			}
		}

		if (!arg.empty() && result.empty())
		{
			addResult(arg, true);
		}
	}

	return result;
}

/*
** Parses Bang args
**
*/
void CRainmeter::BangWithArgs(BANGCOMMAND bang, std::vector<std::wstring>& args, size_t numOfArgs, CMeterWindow* meterWindow)
{
	const size_t argsCount = args.size();

	if (argsCount >= numOfArgs)
	{
		if (argsCount == numOfArgs && meterWindow)
		{
			meterWindow->RunBang(bang, args);
		}
		else
		{
			// Use the specified window instead of meterWindow parameter
			if (argsCount > numOfArgs)
			{
				const std::wstring& folderPath = args[numOfArgs];
				if (!folderPath.empty() && (folderPath.length() != 1 || folderPath[0] != L'*'))
				{
					CMeterWindow* meterWindow = GetMeterWindow(folderPath);
					if (meterWindow)
					{
						meterWindow->RunBang(bang, args);
					}
					else
					{
						LogWithArgs(LOG_ERROR,  L"Bang: Skin \"%s\" not found", folderPath.c_str());
					}
					return;
				}
			}

			// No skin defined -> apply to all.
			std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
			for (; iter != m_MeterWindows.end(); ++iter)
			{
				((*iter).second)->RunBang(bang, args);
			}
		}
	}
	else
	{
		// For backwards compatibility
		if (bang == BANG_COMMANDMEASURE && argsCount >= 1)
		{
			std::wstring& firstArg = args[0];
			std::wstring::size_type pos = firstArg.find_first_of(L' ');
			if (pos != std::wstring::npos)
			{
				std::wstring newArg = firstArg.substr(0, pos);
				firstArg.erase(0, pos + 1);
				args.insert(args.begin(), newArg);

				Log(LOG_WARNING, L"!CommandMeasure: Two parameters required, only one given");
				BangWithArgs(bang, args, numOfArgs, meterWindow);
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
void CRainmeter::BangGroupWithArgs(BANGCOMMAND bang, std::vector<std::wstring>& args, size_t numOfArgs, CMeterWindow* meterWindow)
{
	if (args.size() > numOfArgs)
	{
		std::multimap<int, CMeterWindow*> windows;
		GetMeterWindowsByLoadOrder(windows, args[numOfArgs]);

		args.resize(numOfArgs);	// Remove extra parameters (including group)

		std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
		for (; iter != windows.end(); ++iter)
		{
			BangWithArgs(bang, args, numOfArgs, (*iter).second);
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
void CRainmeter::Bang_ActivateSkin(std::vector<std::wstring>& args)
{
	if (args.size() == 1)
	{
		int index = FindSkinFolderIndex(args[0]);
		if (index != -1)
		{
			const SkinFolder& skinFolder = m_SkinFolders[index];
			if (!(skinFolder.active == 1 && skinFolder.files.size() == 1))
			{
				// Activate the next index.
				ActivateSkin(index, (skinFolder.active < skinFolder.files.size()) ? skinFolder.active : 0);
			}
			return;
		}
	}
	else if (args.size() > 1)
	{
		std::pair<int, int> indexes = GetMeterWindowIndex(args[0], args[1]);
		if (indexes.first != -1 && indexes.second != -1)
		{
			ActivateSkin(indexes.first, indexes.second);
			return;
		}
	}

	Log(LOG_ERROR, L"!ActivateConfig: Invalid parameters");
}

/*
** !DeactivateConfig bang
**
*/
void CRainmeter::Bang_DeactivateSkin(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	if (!args.empty())
	{
		meterWindow = GetMeterWindow(args[0]);
		if (!meterWindow)
		{
			LogWithArgs(LOG_WARNING, L"!DeactivateConfig: \"%s\" not active", args[0].c_str());
			return;
		}
	}

	if (meterWindow)
	{
		DeactivateSkin(meterWindow, -1);
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
void CRainmeter::Bang_ToggleSkin(std::vector<std::wstring>& args)
{
	if (args.size() >= 2)
	{
		CMeterWindow* mw = GetMeterWindow(args[0]);
		if (mw)
		{
			DeactivateSkin(mw, -1);
			return;
		}

		// If the skin wasn't active, activate it
		Bang_ActivateSkin(args);
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
void CRainmeter::Bang_DeactivateSkinGroup(std::vector<std::wstring>& args)
{
	if (!args.empty())
	{
		std::multimap<int, CMeterWindow*> windows;
		GetMeterWindowsByLoadOrder(windows, args[0]);

		std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
		for (; iter != windows.end(); ++iter)
		{
			DeactivateSkin((*iter).second, -1);
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
void CRainmeter::Bang_SetClip(std::vector<std::wstring>& args)
{
	if (!args.empty())
	{
		CSystem::SetClipboardText(args[0]);
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
void CRainmeter::Bang_SetWallpaper(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	const size_t argsSize = args.size();
	if (argsSize >= 1 && argsSize <= 2)
	{
		std::wstring& file = args[0];
		const std::wstring& style = (argsSize == 2) ? args[1] : L"";

		if (meterWindow)
		{
			meterWindow->MakePathAbsolute(file);
		}

		CSystem::SetWallpaper(file, style);
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
void CRainmeter::Bang_SkinMenu(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	if (!args.empty())
	{
		meterWindow = GetMeterWindow(args[0]);
		if (!meterWindow)
		{
			LogWithArgs(LOG_WARNING, L"!SkinMenu: \"%s\" not active", args[0].c_str());
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
void CRainmeter::Bang_WriteKeyValue(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	if (args.size() == 3 && meterWindow)
	{
		// Add the skin file path to the args
		args.push_back(meterWindow->GetFilePath());
	}
	else if (args.size() < 4)
	{
		Log(LOG_ERROR, L"!WriteKeyValue: Invalid parameters");
		return;
	}

	std::wstring& strIniFile = args[3];
	if (meterWindow)
	{
		meterWindow->MakePathAbsolute(strIniFile);
	}

	const WCHAR* iniFile = strIniFile.c_str();

	if (strIniFile.find(L"..\\") != std::wstring::npos || strIniFile.find(L"../") != std::wstring::npos)
	{
		LogWithArgs(LOG_ERROR, L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	if (_wcsnicmp(iniFile, m_SkinPath.c_str(), m_SkinPath.size()) != 0 &&
		_wcsnicmp(iniFile, m_SettingsPath.c_str(), m_SettingsPath.size()) != 0)
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
	const WCHAR* section = args[0].c_str();
	const WCHAR* key = args[1].c_str();
	const std::wstring& strValue = args[2];

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

/*
** !Log bang
**
*/
void CRainmeter::Bang_Log(std::vector<std::wstring>& args)
{
	if (!args.empty())
	{
		int level = LOG_NOTICE;

		if (args.size() > 1)
		{
			const WCHAR* type = args[1].c_str();
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

		Log(level, args[0].c_str());
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
	m_Mutex(),
	m_Instance(),
	m_ResourceInstance(),
	m_ResourceLCID(),
	m_GDIplusToken(),
	m_GlobalOptions()
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
	if (m_Mutex) ReleaseMutex(m_Mutex);

	CoUninitialize();

	GdiplusShutdown(m_GDIplusToken);
}

/*
** The main initialization function for the module.
**
*/
int CRainmeter::Initialize(LPCWSTR iniPath)
{
	InitalizeLitestep();

	m_Instance = GetModuleHandle(L"Rainmeter");

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	GetModuleFileName(m_Instance, buffer, MAX_LINE_LENGTH);

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(buffer, L'\\');
	m_Path.assign(buffer, pos ? pos - buffer + 1 : 0);

	bool bDefaultIniLocation = false;
	if (*iniPath)
	{
		// The command line defines the location of Rainmeter.ini (or whatever it calls it).
		std::wstring iniFile = iniPath;
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
		}
	}

	if (IsAlreadyRunning())
	{
		// Instance already running with same .ini file
		return 1;
	}

	WNDCLASS wc = {0};
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.hInstance = m_Instance;
	wc.lpszClassName = RAINMETER_CLASS_NAME;
	ATOM className = RegisterClass(&wc);

	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		MAKEINTATOM(className),
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

	const WCHAR* iniFile = m_IniFile.c_str();

	// Set file locations
	{
		m_SettingsPath = ExtractPath(m_IniFile);

		size_t len = m_IniFile.length();
		if (len > 4 && _wcsicmp(iniFile + (len - 4), L".ini") == 0)
		{
			len -= 4;
		}

		m_LogFile.assign(m_IniFile, 0, len);
		m_DataFile = m_StatsFile = m_LogFile;
		m_LogFile += L".log";
		m_StatsFile += L".stats";
		m_DataFile += L".data";
	}

	// Create a default Rainmeter.ini file if needed
	if (_waccess(iniFile, 0) == -1)
	{
		CreateOptionsFile();
	}

	bool dataFileCreated = false;
	if (_waccess(m_DataFile.c_str(), 0) == -1)
	{
		dataFileCreated = true;
		CreateDataFile();
	}

	// Reset log file
	CSystem::RemoveFile(m_LogFile);

	m_Debug = 0!=GetPrivateProfileInt(L"Rainmeter", L"Debug", 0, iniFile);
	m_Logging = 0!=GetPrivateProfileInt(L"Rainmeter", L"Logging", 0, iniFile);

	if (m_Logging)
	{
		StartLogging();
	}

	// Determine the language resource to load
	std::wstring resource = m_Path + L"Languages\\";
	if (GetPrivateProfileString(L"Rainmeter", L"Language", L"", buffer, MAX_LINE_LENGTH, iniFile) == 0)
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
			MessageBox(NULL, L"Unable to load language library", APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
			return 1;
		}
	}

	// Get skin folder path
	size_t len = GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, iniFile);
	if (len > 0 &&
		_waccess(buffer, 0) != -1)	// Temporary fix
	{
		// Try Rainmeter.ini first
		m_SkinPath.assign(buffer, len);
		ExpandEnvironmentVariables(m_SkinPath);

		if (!m_SkinPath.empty() && !CSystem::IsPathSeparator(m_SkinPath[m_SkinPath.length() - 1]))
		{
			m_SkinPath += L'\\';
		}
	}
	else if (bDefaultIniLocation &&
		SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, buffer)))
	{
		// Use My Documents/Rainmeter/Skins
		m_SkinPath = buffer;
		m_SkinPath += L"\\Rainmeter\\";
		CreateDirectory(m_SkinPath.c_str(), NULL);
		m_SkinPath += L"Skins\\";

		WritePrivateProfileString(L"Rainmeter", L"SkinPath", m_SkinPath.c_str(), iniFile);
	}
	else
	{
		m_SkinPath = m_Path + L"Skins\\";
	}

	// Create user skins, themes, addons, and plugins folders if needed
	CreateComponentFolders(bDefaultIniLocation);

	delete [] buffer;
	buffer = NULL;

	LogWithArgs(LOG_NOTICE, L"Path: %s", m_Path.c_str());
	LogWithArgs(LOG_NOTICE, L"IniFile: %s", iniFile);
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

	// Tray must exist before skins are read
	m_TrayWindow = new CTrayWindow();
	m_TrayWindow->Initialize();

	ReloadSettings();

	if (m_SkinFolders.empty())
	{
		std::wstring error = GetFormattedString(ID_STR_NOAVAILABLESKINS, m_SkinPath.c_str());
		ShowMessage(NULL, error.c_str(), MB_OK | MB_ICONERROR);
	}

	ResetStats();
	ReadStats();

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(false);
	}

	// Create meter windows for active skins
	ActivateActiveSkins();

	if (dataFileCreated)
	{
		m_TrayWindow->ShowWelcomeNotification();
	}
	else if (!m_DisableVersionCheck)
	{
		CheckUpdate();
	}

	return 0;	// All is OK
}

bool CRainmeter::IsAlreadyRunning()
{
	typedef struct
	{
		ULONG i[2];
		ULONG buf[4];
		unsigned char in[64];
		unsigned char digest[16];
	} MD5_CTX;

	typedef void (WINAPI * FPMD5INIT)(MD5_CTX* context);
	typedef void (WINAPI * FPMD5UPDATE)(MD5_CTX* context, const unsigned char* input, unsigned int inlen);
	typedef void (WINAPI * FPMD5FINAL)(MD5_CTX* context);

	bool alreadyRunning = false;

	// Create MD5 digest from command line
	HMODULE cryptDll = CSystem::RmLoadLibrary(L"cryptdll.dll");
	if (cryptDll)
	{
		FPMD5INIT MD5Init = (FPMD5INIT)GetProcAddress(cryptDll, "MD5Init");
		FPMD5UPDATE MD5Update = (FPMD5UPDATE)GetProcAddress(cryptDll, "MD5Update");
		FPMD5FINAL MD5Final = (FPMD5FINAL)GetProcAddress(cryptDll, "MD5Final");
		if (MD5Init && MD5Update && MD5Final)
		{
			std::wstring data = m_IniFile;
			_wcsupr(&data[0]);

			MD5_CTX ctx = {0};
			MD5Init(&ctx);
			MD5Update(&ctx, (LPBYTE)&data[0], data.length() * sizeof(WCHAR));
			MD5Final(&ctx);
			FreeLibrary(cryptDll);

			// Convert MD5 digest to mutex string (e.g. "Rainmeter0123456789abcdef0123456789abcdef")
			const WCHAR hexChars[] = L"0123456789abcdef";
			WCHAR mutexName[64] = L"Rainmeter";
			WCHAR* pos = mutexName + (_countof(L"Rainmeter") - 1);
			for (size_t i = 0; i < 16; ++i)
			{
				*(pos++) = hexChars[ctx.digest[i] >> 4];
				*(pos++) = hexChars[ctx.digest[i] & 0xF];
			}
			*pos = L'\0';

			m_Mutex = CreateMutex(NULL, FALSE, mutexName);
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				alreadyRunning = true;
				m_Mutex = NULL;
			}
		}

		FreeLibrary(cryptDll);
	}

	return alreadyRunning;
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

void CRainmeter::CreateOptionsFile()
{
	CreateDirectory(m_SettingsPath.c_str(), NULL);

	std::wstring defaultIni = GetDefaultThemePath();
	defaultIni += L"illustro default\\Rainmeter.thm";
	CSystem::CopyFiles(defaultIni, m_IniFile);
}

void CRainmeter::CreateDataFile()
{
	std::wstring tmpSz = m_SettingsPath + L"Plugins.ini";

	const WCHAR* pluginsFile = tmpSz.c_str();
	const WCHAR* dataFile = m_DataFile.c_str();

	if (_waccess(pluginsFile, 0) == 0)
	{
		MoveFile(pluginsFile, dataFile);
	}
	else
	{
		// Create empty file
		HANDLE file = CreateFile(dataFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
		}
	}
}

void CRainmeter::CreateComponentFolders(bool defaultIniLocation)
{
	std::wstring path;

	if (CreateDirectory(m_SkinPath.c_str(), NULL))
	{
		// Folder just created, so copy default skins there
		std::wstring from = GetDefaultSkinPath();
		from += L"*.*";
		CSystem::CopyFiles(from, m_SkinPath);
	}
	else
	{
		path = m_SkinPath;
		path += L"Backup";
		if (_waccess(path.c_str(), 0) != -1)
		{
			std::wstring newPath = m_SkinPath + L"@Backup";
			MoveFile(path.c_str(), newPath.c_str());
		}
	}

	path = GetThemePath();
	if (_waccess(path.c_str(), 0) == -1)
	{
		std::wstring from = GetDefaultThemePath();
		if (_waccess(from.c_str(), 0) != -1)
		{
			CSystem::CopyFiles(from, m_SettingsPath);
		}
	}
	else
	{
		path += L"Backup";
		if (_waccess(path.c_str(), 0) != -1)
		{
			std::wstring newPath = GetThemePath();
			newPath += L"@Backup";
			MoveFile(path.c_str(), newPath.c_str());
		}
	}

	if (defaultIniLocation)
	{
		path = GetUserPluginPath();
		if (_waccess(path.c_str(), 0) == -1)
		{
			std::wstring from = GetDefaultPluginPath();
			if (_waccess(from.c_str(), 0) != -1)
			{
				CSystem::CopyFiles(from, m_SettingsPath);
			}
		}

		path = GetAddonPath();
		if (_waccess(path.c_str(), 0) == -1)
		{
			std::wstring from = GetDefaultAddonPath();
			if (_waccess(from.c_str(), 0) != -1)
			{
				CSystem::CopyFiles(from, m_SettingsPath);
			}
		}

		path = m_SettingsPath;
		path += L"Rainmeter.exe";
		const WCHAR* pathSz = path.c_str();
		if (_waccess(pathSz, 0) == -1)
		{
			// Create a hidden stub Rainmeter.exe into SettingsPath for old addon
			// using relative path to Rainmeter.exe
			std::wstring from = m_Path + L"Rainmeter.exe";
			CSystem::CopyFiles(from, path);

			// Get rid of all resources from the stub executable
			HANDLE stub = BeginUpdateResource(pathSz, TRUE);

			// Add the manifest of Rainmeter.dll to the stub
			HRSRC manifest = FindResource(m_Instance, MAKEINTRESOURCE(2), RT_MANIFEST);
			DWORD manifestSize = SizeofResource(m_Instance, manifest);
			HGLOBAL	manifestLoad = LoadResource(m_Instance, manifest);
			void* manifestLoadData = LockResource(manifestLoad);
			if (manifestLoadData)
			{
				LANGID langID = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
				UpdateResource(stub, RT_MANIFEST, MAKEINTRESOURCE(1), langID, manifestLoadData, manifestSize);
			}

			EndUpdateResource(stub, FALSE);
			SetFileAttributes(pathSz, FILE_ATTRIBUTE_HIDDEN);
		}
	}
}

void CRainmeter::ReloadSettings()
{
	ScanForSkins();
	ScanForThemes();
	ReadGeneralSettings(m_IniFile);
}

void CRainmeter::EditSettings()
{
	RunFile(m_SkinEditor.c_str(), m_IniFile.c_str());
}

void CRainmeter::EditSkinFile(const std::wstring& name, const std::wstring& iniFile)
{
	std::wstring args = m_SkinPath + name;
	args += L'\\';
	args += iniFile;
	bool writable = CSystem::IsFileWritable(args.c_str());

	// Execute as admin if in protected location
	RunFile(m_SkinEditor.c_str(), args.c_str(), !writable);
}

void CRainmeter::OpenSkinFolder(const std::wstring& name)
{
	std::wstring folderPath = m_SkinPath + name;
	RunFile(folderPath.c_str());
}

void CRainmeter::ActivateActiveSkins()
{
	std::multimap<int, int>::const_iterator iter = m_SkinOrders.begin();
	for ( ; iter != m_SkinOrders.end(); ++iter)
	{
		const SkinFolder& skinFolder = m_SkinFolders[(*iter).second];
		if (skinFolder.active > 0 && skinFolder.active <= (int)skinFolder.files.size())
		{
			ActivateSkin((*iter).second, skinFolder.active - 1);
		}
	}
}

void CRainmeter::ActivateSkin(int folderIndex, int fileIndex)
{
	if (folderIndex >= 0 && folderIndex < (int)m_SkinFolders.size() &&
		fileIndex >= 0 && fileIndex < (int)m_SkinFolders[folderIndex].files.size())
	{
		SkinFolder& skinFolder = m_SkinFolders[folderIndex];
		const std::wstring& file = skinFolder.files[fileIndex];
		const WCHAR* fileSz = file.c_str();

		std::wstring folderPath = GetFolderPath(folderIndex);

		// Verify that the skin is not already active
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.find(folderPath);
		if (iter != m_MeterWindows.end())
		{
			if (wcscmp(((*iter).second)->GetFileName().c_str(), fileSz) == 0)
			{
				LogWithArgs(LOG_WARNING, L"!ActivateConfig: \"%s\" already active", folderPath.c_str());
				return;
			}
			else
			{
				// Deactivate the existing skin
				DeactivateSkin((*iter).second, folderIndex);
			}
		}

		// Verify whether the ini-file exists
		std::wstring skinIniPath = m_SkinPath + folderPath;
		skinIniPath += L'\\';
		skinIniPath += file;

		if (_waccess(skinIniPath.c_str(), 0) == -1)
		{
			std::wstring message = GetFormattedString(ID_STR_UNABLETOACTIVATESKIN, folderPath.c_str(), fileSz);
			ShowMessage(NULL, message.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return;
		}

		if (skinFolder.active != fileIndex + 1)
		{
			// Write only if changed.
			skinFolder.active = fileIndex + 1;
			WriteActive(folderPath, fileIndex);
		}

		CreateMeterWindow(folderPath, file);
	}
}

void CRainmeter::DeactivateSkin(CMeterWindow* meterWindow, int folderIndex, bool save)
{
	if (folderIndex >= 0 && folderIndex < (int)m_SkinFolders.size())
	{
		m_SkinFolders[folderIndex].active = 0;	// Deactivate the skin
	}
	else if (folderIndex == -1 && meterWindow)
	{
		folderIndex = FindSkinFolderIndex(meterWindow->GetFolderPath());
		if (folderIndex != -1)
		{
			m_SkinFolders[folderIndex].active = 0;
		}
	}

	if (meterWindow)
	{
		if (save)
		{
			// Disable the skin in the ini-file
			WriteActive(meterWindow->GetFolderPath(), -1);
		}

		meterWindow->Deactivate();
	}
}

void CRainmeter::ToggleSkin(int folderIndex, int fileIndex)
{
	if (folderIndex >= 0 && folderIndex < (int)m_SkinFolders.size() &&
		fileIndex >= 0 && fileIndex < (int)m_SkinFolders[folderIndex].files.size())
	{
		if (m_SkinFolders[folderIndex].active == fileIndex + 1)
		{
			CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(GetFolderPath(folderIndex));
			DeactivateSkin(meterWindow, folderIndex);
		}
		else
		{
			ActivateSkin(folderIndex, fileIndex);
		}
	}
}

void CRainmeter::WriteActive(const std::wstring& folderPath, int fileIndex)
{
	WCHAR buffer[32];
	_itow_s(fileIndex + 1, buffer, 10);
	WritePrivateProfileString(folderPath.c_str(), L"Active", buffer, m_IniFile.c_str());
}

void CRainmeter::CreateMeterWindow(const std::wstring& folderPath, const std::wstring& file)
{
	CMeterWindow* mw = new CMeterWindow(folderPath, file);

	// Note: May modify existing key
	m_MeterWindows[folderPath] = mw;

	mw->Initialize();

	CDialogAbout::UpdateSkins();
	CDialogManage::UpdateSkins(mw);
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

CMeterWindow* CRainmeter::GetMeterWindow(const std::wstring& folderPath)
{
	const WCHAR* folderSz = folderPath.c_str();
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if (_wcsicmp((*iter).first.c_str(), folderSz) == 0)
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
			std::wstring config_current = (*iter).second->GetFolderPath() + L'\\';
			config_current += (*iter).second->GetFileName();

			if (_wcsicmp(config_current.c_str(), config_searching.c_str()) == 0)
			{
				return (*iter).second;
			}
		}
	}

	return NULL;
}

std::pair<int, int> CRainmeter::GetMeterWindowIndex(const std::wstring& folderPath, const std::wstring& file)
{
	int index = FindSkinFolderIndex(folderPath);
	if (index != -1)
	{
		const SkinFolder& skinFolder = m_SkinFolders[index];

		const WCHAR* fileSz = file.c_str();
		for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
		{
			if (_wcsicmp(skinFolder.files[i].c_str(), fileSz) == 0)
			{
				return std::make_pair(index, i);
			}
		}
	}

	return std::make_pair(-1, -1);	// Error
}

std::pair<int, int> CRainmeter::GetMeterWindowIndex(UINT menuCommand)
{
	std::pair<int, int> indexes;

	if (menuCommand >= ID_CONFIG_FIRST && menuCommand <= ID_CONFIG_LAST)
	{
		// Check which skin was selected
		for (size_t i = 0, isize = m_SkinFolders.size(); i < isize; ++i)
		{
			const SkinFolder& skinFolder = m_SkinFolders[i];
			if (menuCommand >= skinFolder.commandBase &&
				menuCommand < (skinFolder.commandBase + skinFolder.files.size()))
			{
				indexes = std::make_pair(i, menuCommand - skinFolder.commandBase);
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

/*
** Returns the skin folder path relative to the skin folder (e.g. illustro\Clock).
**
*/
std::wstring CRainmeter::GetFolderPath(int folderIndex)
{
	const SkinFolder& skinFolder = m_SkinFolders[folderIndex];
	std::wstring path = skinFolder.name;
	for (int i = skinFolder.level - 1, index = folderIndex; i >= 1; --i)
	{
		while (m_SkinFolders[index].level != i)
		{
			--index;
		}

		path.insert(0, L"\\");
		path.insert(0, m_SkinFolders[index].name);
	}
	return path;
}

int CRainmeter::FindSkinFolderIndex(const std::wstring& folderPath)
{
	if (!folderPath.empty())
	{
		const WCHAR* path = folderPath.c_str();
		int len = 0;
		while (path[len] && path[len] != L'\\') ++len;

		int level = 1;
		for (int i = 0, isize = (int)m_SkinFolders.size(); i < isize; ++i)
		{
			const SkinFolder& skinFolder = m_SkinFolders[i];
			if (skinFolder.level == level)
			{
				if (skinFolder.name.length() == len && _wcsnicmp(skinFolder.name.c_str(), path, len) == 0)
				{
					path += len;
					if (*path)
					{
						++path;	// Skip backslash
						len = 0;
						while (path[len] && path[len] != L'\\') ++len;
					}
					else
					{
						// Match found
						return i;
					}

					++level;
				}
			}
			else if (skinFolder.level < level)
			{
				break;
			}
		}
	}

	return -1;
}

void CRainmeter::SetLoadOrder(int folderIndex, int order)
{
	std::multimap<int, int>::iterator iter = m_SkinOrders.begin();
	for ( ; iter != m_SkinOrders.end(); ++iter)
	{
		if ((*iter).second == folderIndex)  // already exists
		{
			if ((*iter).first != order)
			{
				m_SkinOrders.erase(iter);
				break;
			}
			else
			{
				return;
			}
		}
	}

	m_SkinOrders.insert(std::pair<int, int>(order, folderIndex));
}

int CRainmeter::GetLoadOrder(const std::wstring& folderPath)
{
	int index = FindSkinFolderIndex(folderPath);
	if (index != -1)
	{
		std::multimap<int, int>::const_iterator iter = m_SkinOrders.begin();
		for ( ; iter != m_SkinOrders.end(); ++iter)
		{
			if ((*iter).second == index)
			{
				return (*iter).first;
			}
		}
	}

	// LoadOrder not specified
	return 0;
}

/*
** Scans all the subfolders and locates the ini-files.
*/
void CRainmeter::ScanForSkins()
{
	m_SkinFolders.clear();
	m_SkinOrders.clear();

	ScanForSkinsRecursive(m_SkinPath, L"", 0, 0);
}

int CRainmeter::ScanForSkinsRecursive(const std::wstring& path, std::wstring base, int index, UINT level)
{
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile
	std::list<std::wstring> subfolders;

	// Find all .ini files and subfolders
	std::wstring filter = path + base;
	filter += L"\\*";

	hSearch = FindFirstFileEx(
		filter.c_str(),
		(CSystem::GetOSPlatform() >= OSPLATFORM_7) ? FindExInfoBasic : FindExInfoStandard,
		&fileData,
		FindExSearchNameMatch,
		NULL,
		0);

	bool foundFiles = false;
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		SkinFolder skinFolder;
		skinFolder.commandBase = ID_CONFIG_FIRST + index;
		skinFolder.active = 0;
		skinFolder.level = level;

		do
		{
			const std::wstring filename = fileData.cFileName;

			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wcscmp(L".", fileData.cFileName) != 0 &&
					wcscmp(L"..", fileData.cFileName) != 0 &&
					!(level == 0 && wcscmp(L"@Backup", fileData.cFileName) == 0) &&
					!(level == 0 && wcscmp(L"Backup", fileData.cFileName) == 0) &&
					!(level == 1 && wcscmp(L"@Resources", fileData.cFileName) == 0))
				{
					subfolders.push_back(filename);
				}
			}
			else if (level != 0)
			{
				// Check whether the extension is ".ini"
				size_t filenameLen = filename.size();
				if (filenameLen >= 4 && _wcsicmp(fileData.cFileName + (filenameLen - 4), L".ini") == 0)
				{
					foundFiles = true;
					skinFolder.files.push_back(filename);
					++index;
				}
			}
		}
		while (FindNextFile(hSearch, &fileData));

		FindClose(hSearch);

		if (level > 0 && (foundFiles || !subfolders.empty()))
		{
			if (level == 1)
			{
				skinFolder.name = base;
			}
			else
			{
				std::wstring::size_type pos = base.rfind(L'\\') + 1;
				skinFolder.name.assign(base, pos, base.length() - pos);
			}

			m_SkinFolders.push_back(std::move(skinFolder));
		}
	}

	if (level != 0)
	{
		base += L'\\';
	}

	if (!subfolders.empty())
	{
		bool popFolder = !foundFiles;

		std::list<std::wstring>::const_iterator iter = subfolders.begin();
		for ( ; iter != subfolders.end(); ++iter)
		{
			int newIndex = ScanForSkinsRecursive(path, base + (*iter), index, level + 1);
			if (newIndex != index)
			{
				popFolder = false;
			}

			index = newIndex;
		}

		if (popFolder)
		{
			m_SkinFolders.pop_back();
		}
	}

	return index;
}

/*
** Scans the given folder for themes
*/
void CRainmeter::ScanForThemes()
{
	m_Themes.clear();

	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	// Scan for folders
	std::wstring folders = GetThemePath();
	folders += L'*';

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

void CRainmeter::ExecuteBang(const WCHAR* bang, std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
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
		Bang_ActivateSkin(args);
	}
	else if (_wcsicmp(bang, L"DeactivateConfig") == 0)
	{
		Bang_DeactivateSkin(args, meterWindow);
	}
	else if (_wcsicmp(bang, L"ToggleConfig") == 0)
	{
		Bang_ToggleSkin(args);
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
		Bang_DeactivateSkinGroup(args);
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
		Bang_SetWallpaper(args, meterWindow);
	}
	else if (_wcsicmp(bang, L"About") == 0)
	{
		CDialogAbout::Open(args.empty() ? L"" : args[0].c_str());
	}
	else if (_wcsicmp(bang, L"Manage") == 0)
	{
		CDialogManage::Open(args.empty() ? L"" : args[0].c_str());
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
	else if (_wcsicmp(bang, L"LsBoxHook") == 0)
	{
		// Deprecated.
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"Invalid bang: !%s", bang);
	}
}

/*
** Runs the given command or bang
**
*/
void CRainmeter::ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow, bool multi)
{
	if (command[0] == L'!')	// Bang
	{
		++command;	// Skip "!"

		if (_wcsnicmp(L"Execute", command, 7) == 0)
		{
			command += 7;
			command = wcschr(command, L'[');
			if (!command) return;
		}
		else
		{
			if (_wcsnicmp(command, L"Rainmeter", 9) == 0)
			{
				// Skip "Rainmeter" for backwards compatibility
				command += 9;
			}

			std::wstring bang;
			std::vector<std::wstring> args;

			// Find the first space
			const WCHAR* pos = wcschr(command, L' ');
			if (pos)
			{
				bang.assign(command, 0, pos - command);
				args = ParseString(pos + 1, meterWindow ? &meterWindow->GetParser() : NULL);
			}
			else
			{
				bang = command;
			}

			ExecuteBang(bang.c_str(), args, meterWindow);
			return;
		}
	}

	if (multi && command[0] == L'[')	// Multi-bang
	{
		std::wstring bangs = command;
		std::wstring::size_type start = std::wstring::npos;
		int count = 0;
		for (size_t i = 0, isize = bangs.size(); i < isize; ++i)
		{
			if (bangs[i] == L'[')
			{
				if (count == 0)
				{
					start = i;
				}
				++count;
			}
			else if (bangs[i] == L']')
			{
				--count;

				if (count == 0 && start != std::wstring::npos)
				{
					// Change ] to NULL
					bangs[i] = L'\0';

					// Skip whitespace
					start = bangs.find_first_not_of(L" \t\r\n", start + 1, 4);

					ExecuteCommand(bangs.c_str() + start, meterWindow, false);
				}
			}
			else if (bangs[i] == L'"' && isize > (i + 2) && bangs[i + 1] == L'"' && bangs[i + 2] == L'"')
			{
				i += 3;

				std::wstring::size_type pos = bangs.find(L"\"\"\"", i);
				if (pos != std::wstring::npos)
				{
					i = pos + 2;	// Skip "", loop will skip last "
				}
			}
		}
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
		std::wstring tmpSz = command;
		if (meterWindow)
		{
			meterWindow->GetParser().ReplaceMeasures(tmpSz);
		}
		RunCommand(tmpSz);
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
	parser.Initialize(iniFile, NULL, NULL);

	// Read Logging settings
	m_Logging = 0!=parser.ReadInt(L"Rainmeter", L"Logging", 0);
	m_Debug = 0!=parser.ReadInt(L"Rainmeter", L"Debug", 0);

	if (m_Logging)
	{
		StartLogging();
	}

	if (m_TrayWindow)
	{
		m_TrayWindow->ReadOptions(parser);
	}

	m_GlobalOptions.netInSpeed = parser.ReadFloat(L"Rainmeter", L"NetInSpeed", 0.0);
	m_GlobalOptions.netOutSpeed = parser.ReadFloat(L"Rainmeter", L"NetOutSpeed", 0.0);

	m_DisableDragging = 0!=parser.ReadInt(L"Rainmeter", L"DisableDragging", 0);
	m_DisableRDP = 0!=parser.ReadInt(L"Rainmeter", L"DisableRDP", 0);

	m_SkinEditor = parser.ReadString(L"Rainmeter", L"ConfigEditor", L"");
	if (m_SkinEditor.empty())
	{
		// Get the program path associated with .ini files
		DWORD cchOut = MAX_PATH;
		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".ini", L"open", buffer, &cchOut);
		m_SkinEditor = (SUCCEEDED(hr) && cchOut > 0) ? buffer : L"Notepad";
	}

	m_LogViewer = parser.ReadString(L"Rainmeter", L"LogViewer", L"");
	if (m_LogViewer.empty())
	{
		// Get the program path associated with .log files
		DWORD cchOut = MAX_PATH;
		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".log", L"open", buffer, &cchOut);
		m_LogViewer = (SUCCEEDED(hr) && cchOut > 0) ? buffer : L"Notepad";
	}

	if (m_Debug)
	{
		LogWithArgs(LOG_NOTICE, L"ConfigEditor: %s", m_SkinEditor.c_str());
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

	for (auto iter = parser.GetSections().cbegin(); iter != parser.GetSections().end(); ++iter)
	{
		const WCHAR* section = (*iter).c_str();

		if (wcscmp(section, L"Rainmeter") == 0 ||
			wcscmp(section, L"TrayMeasure") == 0)
		{
			continue;
		}

		int index = FindSkinFolderIndex(*iter);
		if (index == -1)
		{
			continue;
		}

		SkinFolder& skinFolder = m_SkinFolders[index];

		// Make sure there is a ini file available
		int active = parser.ReadInt(section, L"Active", 0);
		if (active > 0 && active <= (int)skinFolder.files.size())
		{
			skinFolder.active = active;
		}

		int order = parser.ReadInt(section, L"LoadOrder", 0);
		SetLoadOrder(index, order);
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
			int index = FindSkinFolderIndex(mw->GetFolderPath());
			if (index != -1)
			{
				SkinFolder& skinFolder = m_SkinFolders[index];

				const WCHAR* skinIniFile = mw->GetFileName().c_str();

				bool found = false;
				for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
				{
					if (_wcsicmp(skinIniFile, skinFolder.files[i].c_str()) == 0)
					{
						found = true;
						if (skinFolder.active != i + 1)
						{
							// Switch to new ini-file order
							skinFolder.active = i + 1;
							WriteActive(mw->GetFolderPath(), i);
						}
						break;
					}
				}

				if (!found)
				{
					const WCHAR* skinFolderPath = mw->GetFolderPath().c_str();
					std::wstring error = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, skinFolderPath, skinIniFile);

					DeactivateSkin(mw, index);

					ShowMessage(NULL, error.c_str(), MB_OK | MB_ICONEXCLAMATION);
					continue;
				}
			}
			else
			{
				const WCHAR* skinFolderPath = mw->GetFolderPath().c_str();
				std::wstring error = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, skinFolderPath, L"");

				DeactivateSkin(mw, -2);  // -2 = Force deactivate

				ShowMessage(NULL, error.c_str(), MB_OK | MB_ICONEXCLAMATION);
				continue;
			}

			mw->Refresh(false, true);
		}
	}

	CDialogAbout::UpdateSkins();
	CDialogManage::UpdateSkins(NULL);
}

void CRainmeter::LoadTheme(const std::wstring& name)
{
	// Delete all meter windows
	DeleteMeterWindow(NULL);

	std::wstring backup = GetThemePath();
	backup += L"@Backup";
	CreateDirectory(backup.c_str(), NULL);
	backup += L"\\Rainmeter.thm";

	if (_wcsicmp(name.c_str(), L"@Backup") == 0)
	{
		// Just load the backup
		CSystem::CopyFiles(backup, m_IniFile);
	}
	else
	{
		// Make a copy of current Rainmeter.ini
		CSystem::CopyFiles(m_IniFile, backup);

		// Replace Rainmeter.ini with theme
		std::wstring theme = GetThemePath();
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

	// Create meter windows for active skins
	ActivateActiveSkins();
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
		const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

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
		// Update CSystem::MultiMonitorInfo for for work area variables
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
	CMeasureNet::ReadStats(m_StatsFile, m_StatsDate);
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
	tm* newtime;
	time_t long_time;
	time(&long_time);
	newtime = localtime(&long_time);
	m_StatsDate = _wasctime(newtime);
	m_StatsDate.erase(m_StatsDate.size() - 1);

	// Only Net measure has stats at the moment
	CMeasureNet::ResetStats();
}

/*
** Wraps MessageBox(). Sets RTL flag if necessary.
**
*/
int CRainmeter::ShowMessage(HWND parent, const WCHAR* text, UINT type)
{
	type |= MB_TOPMOST;

	if (*GetString(ID_STR_ISRTL) == L'1')
	{
		type |= MB_RTLREADING;
	}

	return MessageBox(parent, text, APPNAME, type);
};

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

				HMENU skinMenu = GetSubMenu(subMenu, 4);
				if (skinMenu)
				{
					if (!m_SkinFolders.empty())
					{
						DeleteMenu(skinMenu, 0, MF_BYPOSITION);  // "No skins available" menuitem
						CreateAllSkinsMenu(skinMenu);
					}

					if (m_DisableDragging)
					{
						CheckMenuItem(skinMenu, IDM_DISABLEDRAG, MF_BYCOMMAND | MF_CHECKED);
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
					subMenu = CreateSkinMenu(meterWindow, 0, skinMenu);

					WCHAR buffer[256];
					GetMenuString(menu, 0, buffer, 256, MF_BYPOSITION);
					InsertMenu(subMenu, 11, MF_BYPOSITION | MF_POPUP, (UINT_PTR)rainmeterMenu, buffer);
					InsertMenu(subMenu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
				}
				else
				{
					InsertMenu(subMenu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

					// Create a menu for all active skins
					int index = 0;
					std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_MeterWindows.begin();
					for (; iter != m_MeterWindows.end(); ++iter)
					{
						CMeterWindow* mw = ((*iter).second);
						HMENU menu = CreateSkinMenu(mw, index, skinMenu);
						InsertMenu(subMenu, 12, MF_BYPOSITION | MF_POPUP, (UINT_PTR)menu, mw->GetFolderPath().c_str());
						++index;
					}

					// Add update notification item
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
					TPM_RIGHTBUTTON | TPM_LEFTALIGN | (*GetString(ID_STR_ISRTL) == L'1' ? TPM_LAYOUTRTL : 0),
					pos.x,
					pos.y,
					0,
					hWnd,
					NULL);

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

int CRainmeter::CreateAllSkinsMenuRecursive(HMENU skinMenu, int index)
{
	int initialLevel = m_SkinFolders[index].level;
	int menuIndex = 0;

	const size_t max = Rainmeter->m_SkinFolders.size();
	while (index < max)
	{
		const SkinFolder& skinFolder = Rainmeter->m_SkinFolders[index];
		if (skinFolder.level != initialLevel)
		{
			return index - 1;
		}

		HMENU subMenu = CreatePopupMenu();

		// Add current folder
		InsertMenu(skinMenu, menuIndex, MF_POPUP | MF_BYPOSITION, (UINT_PTR)subMenu, skinFolder.name.c_str());

		// Add subfolders
		const bool hasSubfolder = (index + 1) < max && m_SkinFolders[index + 1].level == initialLevel + 1;
		if (hasSubfolder)
		{
			index = CreateAllSkinsMenuRecursive(subMenu, index + 1);
		}

		// Add files
		{
			int fileIndex = 0;
			int fileCount = (int)skinFolder.files.size();
			for ( ; fileIndex < fileCount; ++fileIndex)
			{
				InsertMenu(subMenu, fileIndex, MF_STRING | MF_BYPOSITION, skinFolder.commandBase + fileIndex, skinFolder.files[fileIndex].c_str());
			}

			if (skinFolder.active)
			{
				UINT checkPos = skinFolder.active - 1;
				CheckMenuRadioItem(subMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);
			}

			if (hasSubfolder && fileIndex != 0)
			{
				InsertMenu(subMenu, fileIndex, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			}
		}

		++menuIndex;
		++index;
	}

	return index;
}

HMENU CRainmeter::CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU menu)
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
				UINT checkPos = IDM_SKIN_NORMAL - (UINT)meterWindow->GetWindowZPosition();
				CheckMenuRadioItem(posMenu, checkPos, checkPos, checkPos, MF_BYCOMMAND);

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
			HMENU alphaMenu = GetSubMenu(settingsMenu, 1);
			if (alphaMenu)
			{
				UINT checkPos = (UINT)(10 - meterWindow->GetAlphaValue() / 25.5);
				checkPos = min(9, checkPos);
				checkPos = max(0, checkPos);
				CheckMenuRadioItem(alphaMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);

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

			// Tick the settings
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
		const std::wstring& skinName = meterWindow->GetFolderPath();
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
			const SkinFolder& skinFolder = m_SkinFolders[FindSkinFolderIndex(skinName)];
			for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
			{
				InsertMenu(variantsMenu, i, MF_BYPOSITION, skinFolder.commandBase + i, skinFolder.files[i].c_str());
			}

			if (skinFolder.active)
			{
				UINT checkPos = skinFolder.active - 1;
				CheckMenuRadioItem(variantsMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);
			}
		}

		// Add skin root menu
		int itemCount = GetMenuItemCount(menu);
		if (itemCount > 0)
		{
			std::wstring root = meterWindow->GetFolderPath();
			std::wstring::size_type pos = root.find_first_of(L'\\');
			if (pos != std::wstring::npos)
			{
				root.erase(pos);
			}

			for (int i = 0; i < itemCount; ++i)
			{
				UINT state = GetMenuState(menu, i, MF_BYPOSITION);
				if (state == 0xFFFFFFFF || (state & MF_POPUP) == 0) break;

				WCHAR buffer[MAX_PATH];
				if (GetMenuString(menu, i, buffer, MAX_PATH, MF_BYPOSITION))
				{
					if (_wcsicmp(root.c_str(), buffer) == 0)
					{
						HMENU skinRootMenu = GetSubMenu(menu, i);
						if (skinRootMenu)
						{
							InsertMenu(skinMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinRootMenu, root.c_str());
						}
						break;
					}
				}
			}
		}
	}

	return skinMenu;
}

void CRainmeter::CreateThemeMenu(HMENU themeMenu)
{
	for (size_t i = 0, isize = m_Themes.size(); i < isize; ++i)
	{
		InsertMenu(themeMenu, i, MF_BYPOSITION, ID_THEME_FIRST + i, m_Themes[i].c_str());
	}
}

void CRainmeter::CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow)
{
	bool screenDefined = meterWindow->GetXScreenDefined();
	int screenIndex = meterWindow->GetXScreen();

	// for the "Specified monitor" (@n)
	if (CSystem::GetMonitorCount() > 0)
	{
		const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

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
	const WCHAR* logFile = m_LogFile.c_str();
	if (_waccess(logFile, 0) == -1)
	{
		// Create log file
		HANDLE file = CreateFile(logFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			SetLogging(true);
		}
		else
		{
			// Disable logging
			SetLogging(false);
	
			std::wstring text = GetFormattedString(ID_STR_LOGFILECREATEFAIL, logFile);
			ShowMessage(NULL, text.c_str(), MB_OK | MB_ICONERROR);
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
	const WCHAR* logFile = m_LogFile.c_str();
	if (_waccess(logFile, 0) != -1)
	{
		RunFile(m_LogViewer.c_str(), logFile);
	}
}

void CRainmeter::DeleteLogFile()
{
	// Check if the file exists
	const WCHAR* logFile = m_LogFile.c_str();
	if (_waccess(logFile, 0) != -1)
	{
		std::wstring text = GetFormattedString(ID_STR_LOGFILEDELETE, logFile);
		int res = ShowMessage(NULL, text.c_str(), MB_YESNO | MB_ICONQUESTION);
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
	LogInfo logInfo = {level, time, message};
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
	const WCHAR* iniFile = m_IniFile.c_str();
	if (!CSystem::IsFileWritable(iniFile))
	{
		std::wstring error = GetString(ID_STR_SETTINGSNOTWRITABLE);

		if (!bDefaultIniLocation)
		{
			std::wstring strTarget = L"%APPDATA%\\Rainmeter\\";
			ExpandEnvironmentVariables(strTarget);

			error += GetFormattedString(ID_STR_SETTINGSMOVEFILE, iniFile, strTarget.c_str());
		}
		else
		{
			error += GetFormattedString(ID_STR_SETTINGSREADONLY, iniFile);
		}

		ShowMessage(NULL, error.c_str(), MB_OK | MB_ICONERROR);
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
