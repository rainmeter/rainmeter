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
#include "../Common/MenuTemplate.h"
#include "../Common/PathUtil.h"
#include "Rainmeter.h"
#include "TrayWindow.h"
#include "System.h"
#include "Error.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "MeasureNet.h"
#include "MeasureCPU.h"
#include "MeterString.h"
#include "resource.h"
#include "UpdateCheck.h"
#include "../Version.h"

using namespace Gdiplus;

enum TIMER
{
	TIMER_NETSTATS    = 1
};
enum INTERVAL
{
	INTERVAL_NETSTATS = 120000
};

/*
** Initializes Rainmeter.
**
*/
int RainmeterMain(LPWSTR cmdLine)
{
	// Avoid loading a dll from current directory
	SetDllDirectory(L"");

	const WCHAR* layout = nullptr;

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
			SendMessage(wnd, WM_COPYDATA, 0, (LPARAM)&cds);
			return 0;
		}

		// Disallow everything except !LoadLayout.
		if (_wcsnicmp(cmdLine, L"!LoadLayout ", 12) == 0)
		{
			layout = cmdLine + 12;  // Skip "!LoadLayout ".
		}
		else
		{
			return 1;
		}
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

	const WCHAR* iniFile = (*cmdLine && !layout) ? cmdLine : nullptr;

	auto& rainmeter = GetRainmeter();
	int ret = rainmeter.Initialize(iniFile, layout);
	if (ret == 0)
	{
		ret = rainmeter.MessagePump();
	}
	rainmeter.Finalize();

	return ret;
}

/*
** Constructor
**
*/
Rainmeter::Rainmeter() :
	m_TrayWindow(),
	m_UseD2D(false),
	m_Debug(false),
	m_DisableVersionCheck(false),
	m_NewVersion(false),
	m_DesktopWorkAreaChanged(false),
	m_DesktopWorkAreaType(false),
	m_NormalStayDesktop(true),
	m_MenuActive(false),
	m_DisableRDP(false),
	m_DisableDragging(false),
	m_CurrentParser(),
	m_Window(),
	m_Mutex(),
	m_Instance(),
	m_ResourceInstance(),
	m_ResourceLCID(),
	m_GDIplusToken(),
	m_GlobalOptions()
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	InitCommonControls();

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_GDIplusToken, &gdiplusStartupInput, nullptr);
}

/*
** Destructor
**
*/
Rainmeter::~Rainmeter()
{
	CoUninitialize();

	GdiplusShutdown(m_GDIplusToken);
}

Rainmeter& Rainmeter::GetInstance()
{
	static Rainmeter s_Rainmeter;
	return s_Rainmeter;
}

/*
** The main initialization function for the module.
**
*/
int Rainmeter::Initialize(LPCWSTR iniPath, LPCWSTR layout)
{
	m_Instance = GetModuleHandle(L"Rainmeter");

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	GetModuleFileName(m_Instance, buffer, MAX_LINE_LENGTH);

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(buffer, L'\\');
	m_Path.assign(buffer, pos ? pos - buffer + 1 : 0);
	m_Drive = PathUtil::GetVolume(m_Path);

	bool bDefaultIniLocation = false;
	if (iniPath)
	{
		// The command line defines the location of Rainmeter.ini (or whatever it calls it).
		std::wstring iniFile = iniPath;
		PathUtil::ExpandEnvironmentVariables(iniFile);

		if (iniFile.empty() || PathUtil::IsSeparator(iniFile[iniFile.length() - 1]))
		{
			iniFile += L"Rainmeter.ini";
		}
		else if (iniFile.length() <= 4 || _wcsicmp(iniFile.c_str() + (iniFile.length() - 4), L".ini") != 0)
		{
			iniFile += L"\\Rainmeter.ini";
		}

		if (!PathUtil::IsSeparator(iniFile[0]) && iniFile.find_first_of(L':') == std::wstring::npos)
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
			PathUtil::ExpandEnvironmentVariables(m_IniFile);
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
		nullptr,
		nullptr,
		m_Instance,
		nullptr);

	if (!m_Window) return 1;

	Logger& logger = GetLogger();
	const WCHAR* iniFile = m_IniFile.c_str();

	// Set file locations
	{
		m_SettingsPath = PathUtil::GetFolderFromFilePath(m_IniFile);

		size_t len = m_IniFile.length();
		if (len > 4 && _wcsicmp(iniFile + (len - 4), L".ini") == 0)
		{
			len -= 4;
		}

		std::wstring logFile(m_IniFile, 0, len);
		m_DataFile = m_StatsFile = logFile;
		logFile += L".log";
		m_StatsFile += L".stats";
		m_DataFile += L".data";

		logger.SetLogFilePath(logFile);
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
	System::RemoveFile(logger.GetLogFilePath());

	m_Debug = 0!=GetPrivateProfileInt(L"Rainmeter", L"Debug", 0, iniFile);

	const bool logging = GetPrivateProfileInt(L"Rainmeter", L"Logging", 0, iniFile) != 0;
	logger.SetLogToFile(logging);
	if (logging)
	{
		logger.StartLogFile();
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
			if (RegQueryValueEx(hKey, L"Language", nullptr, &type, (LPBYTE)buffer, (LPDWORD)&size) != ERROR_SUCCESS ||
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
		m_ResourceLCID = wcstoul(buffer, nullptr, 10);
		resource += buffer;
		resource += L".dll";

		m_ResourceInstance = LoadLibraryEx(resource.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	}
	if (!m_ResourceInstance)
	{
		// Try English
		resource = m_Path;
		resource += L"Languages\\1033.dll";
		m_ResourceInstance = LoadLibraryEx(resource.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		m_ResourceLCID = 1033;
		if (!m_ResourceInstance)
		{
			MessageBox(nullptr, L"Unable to load language library", APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
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
		PathUtil::ExpandEnvironmentVariables(m_SkinPath);
		PathUtil::AppendBacklashIfMissing(m_SkinPath);
	}
	else if (bDefaultIniLocation &&
		SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, buffer)))
	{
		// Use My Documents/Rainmeter/Skins
		m_SkinPath = buffer;
		m_SkinPath += L"\\Rainmeter\\";
		CreateDirectory(m_SkinPath.c_str(), nullptr);
		m_SkinPath += L"Skins\\";

		WritePrivateProfileString(L"Rainmeter", L"SkinPath", m_SkinPath.c_str(), iniFile);
	}
	else
	{
		m_SkinPath = m_Path + L"Skins\\";
	}

	// Create user skins, layouts, addons, and plugins folders if needed
	CreateComponentFolders(bDefaultIniLocation);

	delete [] buffer;
	buffer = nullptr;

	LogNoticeF(L"Path: %s", m_Path.c_str());
	LogNoticeF(L"IniFile: %s", iniFile);
	LogNoticeF(L"SkinPath: %s", m_SkinPath.c_str());

	// Test that the Rainmeter.ini file is writable
	TestSettingsFile(bDefaultIniLocation);

	System::Initialize(m_Instance);

	MeasureNet::InitializeStatic();
	MeasureCPU::InitializeStatic();
	MeterString::InitializeStatic();

	// Tray must exist before skins are read
	m_TrayWindow = new TrayWindow();
	m_TrayWindow->Initialize();

	ReloadSettings();

	if (m_SkinFolders.empty())
	{
		std::wstring error = GetFormattedString(ID_STR_NOAVAILABLESKINS, m_SkinPath.c_str());
		ShowMessage(nullptr, error.c_str(), MB_OK | MB_ICONERROR);
	}

	ResetStats();
	ReadStats();

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(false);
	}

	bool layoutLoaded = false;
	if (layout)
	{
		std::vector<std::wstring> args = CommandHandler::ParseString(layout);
		layoutLoaded = (args.size() == 1 && LoadLayout(args[0]));
	}

	if (!layoutLoaded)
	{
		ActivateActiveSkins();
	}

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

void Rainmeter::Finalize()
{
	KillTimer(m_Window, TIMER_NETSTATS);

	DeleteAllUnmanagedMeterWindows();
	DeleteAllMeterWindows();

	delete m_TrayWindow;

	System::Finalize();

	MeasureNet::UpdateIFTable();
	MeasureNet::UpdateStats();
	WriteStats(true);

	MeasureNet::FinalizeStatic();
	MeasureCPU::FinalizeStatic();
	MeterString::FinalizeStatic();

	// Change the work area back
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(true);
	}

	if (m_ResourceInstance) FreeLibrary(m_ResourceInstance);
	if (m_Mutex) ReleaseMutex(m_Mutex);
}

bool Rainmeter::IsAlreadyRunning()
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
	HMODULE cryptDll = System::RmLoadLibrary(L"cryptdll.dll");
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

			m_Mutex = CreateMutex(nullptr, FALSE, mutexName);
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				alreadyRunning = true;
				m_Mutex = nullptr;
			}
		}

		FreeLibrary(cryptDll);
	}

	return alreadyRunning;
}

int Rainmeter::MessagePump()
{
	MSG msg;
	BOOL ret;

	// Run the standard window message loop
	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if (ret == -1)
		{
			break;
		}

		if (!Dialog::HandleMessage(msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK Rainmeter::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
					GetRainmeter().DelayedExecuteCommand(data);
				}
			}
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_NETSTATS)
		{
			MeasureNet::UpdateIFTable();
			MeasureNet::UpdateStats();
			GetRainmeter().WriteStats(false);
		}
		break;

	case WM_RAINMETER_DELAYED_REFRESH_ALL:
		GetRainmeter().RefreshAll();
		break;

	case WM_RAINMETER_DELAYED_EXECUTE:
		if (lParam)
		{
			// Execute bang
			WCHAR* bang = (WCHAR*)lParam;
			GetRainmeter().ExecuteCommand(bang, nullptr);
			free(bang);  // _wcsdup()
		}
		break;

	case WM_RAINMETER_EXECUTE:
		if (GetRainmeter().HasMeterWindow((MeterWindow*)wParam))
		{
			GetRainmeter().ExecuteCommand((const WCHAR*)lParam, (MeterWindow*)wParam);
		}
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void Rainmeter::SetNetworkStatisticsTimer()
{
	static bool set = SetTimer(m_Window, TIMER_NETSTATS, INTERVAL_NETSTATS, nullptr);
}

void Rainmeter::CreateOptionsFile()
{
	CreateDirectory(m_SettingsPath.c_str(), nullptr);

	std::wstring defaultIni = GetDefaultLayoutPath();
	defaultIni += L"illustro default\\Rainmeter.ini";
	System::CopyFiles(defaultIni, m_IniFile);
}

void Rainmeter::CreateDataFile()
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
		HANDLE file = CreateFile(dataFile, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
		}
	}
}

void Rainmeter::CreateComponentFolders(bool defaultIniLocation)
{
	std::wstring path;

	if (CreateDirectory(m_SkinPath.c_str(), nullptr))
	{
		// Folder just created, so copy default skins there
		std::wstring from = GetDefaultSkinPath();
		from += L"*.*";
		System::CopyFiles(from, m_SkinPath);
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

	path = GetLayoutPath();
	if (_waccess(path.c_str(), 0) == -1)
	{
		std::wstring themesPath = m_SettingsPath + L"Themes";
		if (_waccess(themesPath.c_str(), 0) != -1)
		{
			// Migrate Themes into Layouts for backwards compatibility and rename
			// Rainmeter.thm to Rainmeter.ini and RainThemes.bmp to Wallpaper.bmp.
			MoveFile(themesPath.c_str(), path.c_str());

			path += L'*';  // For FindFirstFile.
			WIN32_FIND_DATA fd;
			HANDLE hFind = FindFirstFile(path.c_str(), &fd);
			path.pop_back();  // Remove '*'.

			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
						PathUtil::IsDotOrDotDot(fd.cFileName))
					{
						std::wstring layoutFolder = path + fd.cFileName;
						layoutFolder += L'\\';

						std::wstring file = layoutFolder + L"Rainmeter.thm";
						if (_waccess(file.c_str(), 0) != -1)
						{
							std::wstring newFile = layoutFolder + L"Rainmeter.ini";
							MoveFile(file.c_str(), newFile.c_str());
						}

						file = layoutFolder + L"RainThemes.bmp";
						if (_waccess(file.c_str(), 0) != -1)
						{
							std::wstring newFile = layoutFolder + L"Wallpaper.bmp";
							MoveFile(file.c_str(), newFile.c_str());
						}
					}
				}
				while (FindNextFile(hFind, &fd));

				FindClose(hFind);
			}
		}
		else
		{
			std::wstring from = GetDefaultLayoutPath();
			if (_waccess(from.c_str(), 0) != -1)
			{
				System::CopyFiles(from, m_SettingsPath);
			}
		}
	}
	else
	{
		path += L"Backup";
		if (_waccess(path.c_str(), 0) != -1)
		{
			std::wstring newPath = GetLayoutPath();
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
				System::CopyFiles(from, m_SettingsPath);
			}
		}

		path = GetAddonPath();
		if (_waccess(path.c_str(), 0) == -1)
		{
			std::wstring from = GetDefaultAddonPath();
			if (_waccess(from.c_str(), 0) != -1)
			{
				System::CopyFiles(from, m_SettingsPath);
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
			System::CopyFiles(from, path);

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

void Rainmeter::ReloadSettings()
{
	ScanForSkins();
	ScanForLayouts();
	ReadGeneralSettings(m_IniFile);
}

void Rainmeter::EditSettings()
{
	std::wstring file = L'"' + m_IniFile;
	file += L'"';
	CommandHandler::RunFile(m_SkinEditor.c_str(), file.c_str());
}

void Rainmeter::EditSkinFile(const std::wstring& name, const std::wstring& iniFile)
{
	std::wstring args = L'"' + m_SkinPath;
	args += name;
	args += L'\\';
	args += iniFile;
	args += L'"';
	CommandHandler::RunFile(m_SkinEditor.c_str(), args.c_str());
}

void Rainmeter::OpenSkinFolder(const std::wstring& name)
{
	std::wstring folderPath = m_SkinPath + name;
	CommandHandler::RunFile(folderPath.c_str());
}

void Rainmeter::ActivateActiveSkins()
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

void Rainmeter::ActivateSkin(int folderIndex, int fileIndex)
{
	if (folderIndex >= 0 && folderIndex < (int)m_SkinFolders.size() &&
		fileIndex >= 0 && fileIndex < (int)m_SkinFolders[folderIndex].files.size())
	{
		SkinFolder& skinFolder = m_SkinFolders[folderIndex];
		const std::wstring& file = skinFolder.files[fileIndex];
		const WCHAR* fileSz = file.c_str();

		std::wstring folderPath = GetFolderPath(folderIndex);

		// Verify that the skin is not already active
		std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.find(folderPath);
		if (iter != m_MeterWindows.end())
		{
			if (wcscmp(((*iter).second)->GetFileName().c_str(), fileSz) == 0)
			{
				LogWarningF(L"!ActivateConfig: \"%s\" already active", folderPath.c_str());
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
			ShowMessage(nullptr, message.c_str(), MB_OK | MB_ICONEXCLAMATION);
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

void Rainmeter::DeactivateSkin(MeterWindow* meterWindow, int folderIndex, bool save)
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

void Rainmeter::ToggleSkin(int folderIndex, int fileIndex)
{
	if (folderIndex >= 0 && folderIndex < (int)m_SkinFolders.size() &&
		fileIndex >= 0 && fileIndex < (int)m_SkinFolders[folderIndex].files.size())
	{
		if (m_SkinFolders[folderIndex].active == fileIndex + 1)
		{
			MeterWindow* meterWindow = GetRainmeter().GetMeterWindow(GetFolderPath(folderIndex));
			DeactivateSkin(meterWindow, folderIndex);
		}
		else
		{
			ActivateSkin(folderIndex, fileIndex);
		}
	}
}

void Rainmeter::SetSkinPath(const std::wstring& skinPath)
{
	WritePrivateProfileString(L"Rainmeter", L"SkinPath", skinPath.c_str(), m_IniFile.c_str());
}

void Rainmeter::SetSkinEditor(const std::wstring& path)
{
	if (!path.empty())
	{
		m_SkinEditor = path;
		WritePrivateProfileString(L"Rainmeter", L"ConfigEditor", path.c_str(), m_IniFile.c_str());
	}
}

void Rainmeter::WriteActive(const std::wstring& folderPath, int fileIndex)
{
	WCHAR buffer[32];
	_itow_s(fileIndex + 1, buffer, 10);
	WritePrivateProfileString(folderPath.c_str(), L"Active", buffer, m_IniFile.c_str());
}

void Rainmeter::CreateMeterWindow(const std::wstring& folderPath, const std::wstring& file)
{
	MeterWindow* mw = new MeterWindow(folderPath, file);

	// Note: May modify existing key
	m_MeterWindows[folderPath] = mw;

	mw->Initialize();

	DialogAbout::UpdateSkins();
	DialogManage::UpdateSkins(mw);
}

void Rainmeter::DeleteAllMeterWindows()
{
	auto it = m_MeterWindows.cbegin();
	while (it != m_MeterWindows.cend())
	{
		MeterWindow* mw = (*it).second;
		m_MeterWindows.erase(it++);  // Remove before deleting MeterWindow

		DialogManage::UpdateSkins(mw, true);
		delete mw;
	}

	m_MeterWindows.clear();
	DialogAbout::UpdateSkins();
}

void Rainmeter::DeleteAllUnmanagedMeterWindows()
{
	for (auto it = m_UnmanagedMeterWindows.cbegin(); it != m_UnmanagedMeterWindows.cend(); ++it)
	{
		delete (*it);
	}

	m_UnmanagedMeterWindows.clear();
}

/*
** Removes the skin from m_MeterWindows. The skin should delete itself.
**
*/
void Rainmeter::RemoveMeterWindow(MeterWindow* meterWindow)
{
	for (auto it = m_MeterWindows.cbegin(); it != m_MeterWindows.cend(); ++it)
	{
		if ((*it).second == meterWindow)
		{
			m_MeterWindows.erase(it);
			DialogManage::UpdateSkins(meterWindow, true);
			DialogAbout::UpdateSkins();
			break;
		}
	}
}

/*
** Adds the skin to m_UnmanagedMeterWindows. The skin should remove itself by calling RemoveUnmanagedMeterWindow().
**
*/
void Rainmeter::AddUnmanagedMeterWindow(MeterWindow* meterWindow)
{
	for (auto it = m_UnmanagedMeterWindows.cbegin(); it != m_UnmanagedMeterWindows.cend(); ++it)
	{
		if ((*it) == meterWindow)  // already added
		{
			return;
		}
	}

	m_UnmanagedMeterWindows.push_back(meterWindow);
}

void Rainmeter::RemoveUnmanagedMeterWindow(MeterWindow* meterWindow)
{
	for (auto it = m_UnmanagedMeterWindows.cbegin(); it != m_UnmanagedMeterWindows.cend(); ++it)
	{
		if ((*it) == meterWindow)
		{
			m_UnmanagedMeterWindows.erase(it);
			break;
		}
	}
}

bool Rainmeter::HasMeterWindow(const MeterWindow* meterWindow) const
{
	for (auto it = m_MeterWindows.begin(); it != m_MeterWindows.end(); ++it)
	{
		if ((*it).second == meterWindow)
		{
			return true;
		}
	}

	return false;
}

MeterWindow* Rainmeter::GetMeterWindow(const std::wstring& folderPath)
{
	const WCHAR* folderSz = folderPath.c_str();
	std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if (_wcsicmp((*iter).first.c_str(), folderSz) == 0)
		{
			return (*iter).second;
		}
	}

	return nullptr;
}

MeterWindow* Rainmeter::GetMeterWindowByINI(const std::wstring& ini_searching)
{
	if (_wcsnicmp(m_SkinPath.c_str(), ini_searching.c_str(), m_SkinPath.length()) == 0)
	{
		const std::wstring config_searching = ini_searching.substr(m_SkinPath.length());

		std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.begin();
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

	return nullptr;
}

std::pair<int, int> Rainmeter::GetMeterWindowIndex(const std::wstring& folderPath, const std::wstring& file)
{
	int index = FindSkinFolderIndex(folderPath);
	if (index != -1)
	{
		const SkinFolder& skinFolder = m_SkinFolders[index];

		const WCHAR* fileSz = file.c_str();
		for (size_t i = 0, isize = skinFolder.files.size(); i < isize; ++i)
		{
			if (_wcsicmp(skinFolder.files[i].c_str(), fileSz) == 0)
			{
				return std::make_pair(index, (int)i);
			}
		}
	}

	return std::make_pair(-1, -1);	// Error
}

std::pair<int, int> Rainmeter::GetMeterWindowIndex(UINT menuCommand)
{
	if (menuCommand >= ID_CONFIG_FIRST && menuCommand <= ID_CONFIG_LAST)
	{
		// Check which skin was selected
		for (size_t i = 0, isize = m_SkinFolders.size(); i < isize; ++i)
		{
			const SkinFolder& skinFolder = m_SkinFolders[i];
			if (menuCommand >= skinFolder.commandBase &&
				menuCommand < (skinFolder.commandBase + skinFolder.files.size()))
			{
				return std::make_pair((int)i, (int)(menuCommand - skinFolder.commandBase));
			}
		}
	}

	return std::make_pair(-1, -1);  // error;
}

MeterWindow* Rainmeter::GetMeterWindow(HWND hwnd)
{
	std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		if ((*iter).second->GetWindow() == hwnd)
		{
			return (*iter).second;
		}
	}

	return nullptr;
}

void Rainmeter::GetMeterWindowsByLoadOrder(std::multimap<int, MeterWindow*>& windows, const std::wstring& group)
{
	std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.begin();
	for (; iter != m_MeterWindows.end(); ++iter)
	{
		MeterWindow* mw = (*iter).second;
		if (mw && (group.empty() || mw->BelongsToGroup(group)))
		{
			windows.insert(std::pair<int, MeterWindow*>(GetLoadOrder((*iter).first), mw));
		}
	}
}

/*
** Returns the skin folder path relative to the skin folder (e.g. illustro\Clock).
**
*/
std::wstring Rainmeter::GetFolderPath(int folderIndex)
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

int Rainmeter::FindSkinFolderIndex(const std::wstring& folderPath)
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

void Rainmeter::SetLoadOrder(int folderIndex, int order)
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

int Rainmeter::GetLoadOrder(const std::wstring& folderPath)
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
void Rainmeter::ScanForSkins()
{
	m_SkinFolders.clear();
	m_SkinOrders.clear();

	ScanForSkinsRecursive(m_SkinPath, L"", 0, 0);
}

int Rainmeter::ScanForSkinsRecursive(const std::wstring& path, std::wstring base, int index, UINT level)
{
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile
	std::list<std::wstring> subfolders;

	// Find all .ini files and subfolders
	std::wstring filter = path + base;
	filter += L"\\*";

	hSearch = FindFirstFileEx(
		filter.c_str(),
		(Platform::IsAtLeastWin7()) ? FindExInfoBasic : FindExInfoStandard,
		&fileData,
		FindExSearchNameMatch,
		nullptr,
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
				if (!PathUtil::IsDotOrDotDot(fileData.cFileName) &&
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
** Scans the given folder for layouts
*/
void Rainmeter::ScanForLayouts()
{
	m_Layouts.clear();

	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	// Scan for folders
	std::wstring folders = GetLayoutPath();
	folders += L'*';

	hSearch = FindFirstFileEx(
		folders.c_str(),
		(Platform::IsAtLeastWin7()) ? FindExInfoBasic : FindExInfoStandard,
		&fileData,
		FindExSearchNameMatch,
		nullptr,
		0);

	if (hSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
				!PathUtil::IsDotOrDotDot(fileData.cFileName))
			{
				m_Layouts.push_back(fileData.cFileName);
			}
		}
		while (FindNextFile(hSearch, &fileData));

		FindClose(hSearch);
	}
}

void Rainmeter::ExecuteBang(const WCHAR* bang, std::vector<std::wstring>& args, MeterWindow* meterWindow)
{
	m_CommandHandler.ExecuteBang(bang, args, meterWindow);
}

/*
** Runs the given command or bang
**
*/
void Rainmeter::ExecuteCommand(const WCHAR* command, MeterWindow* meterWindow, bool multi)
{
	m_CommandHandler.ExecuteCommand(command, meterWindow, multi);
}

/*
** Executes command when current processing is done.
**
*/
void Rainmeter::DelayedExecuteCommand(const WCHAR* command)
{
	WCHAR* bang = _wcsdup(command);
	PostMessage(m_Window, WM_RAINMETER_DELAYED_EXECUTE, (WPARAM)nullptr, (LPARAM)bang);
}

/*
** Reads the general settings from the Rainmeter.ini file
**
*/
void Rainmeter::ReadGeneralSettings(const std::wstring& iniFile)
{
	WCHAR buffer[MAX_PATH];

	// Clear old settings
	m_DesktopWorkAreas.clear();

	ConfigParser parser;
	parser.Initialize(iniFile, nullptr, nullptr);

	m_UseD2D = 0!=parser.ReadInt(L"Rainmeter", L"UseD2D", 0);

	m_Debug = 0!=parser.ReadInt(L"Rainmeter", L"Debug", 0);
	
	// Read Logging settings
	Logger& logger = GetLogger();
	const bool logging = parser.ReadInt(L"Rainmeter", L"Logging", 0) != 0;
	logger.SetLogToFile(logging);
	if (logging)
	{
		logger.StartLogFile();
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

	if (m_Debug)
	{
		LogNoticeF(L"ConfigEditor: %s", m_SkinEditor.c_str());
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

	for (UINT i = 1, isize = System::GetMonitorCount(); i <= isize; ++i)
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
** Note: This function calls MeterWindow::Refresh() directly for synchronization. Be careful about crash.
**
*/
void Rainmeter::RefreshAll()
{
	// Read skins and settings
	ReloadSettings();

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		UpdateDesktopWorkArea(false);
	}

	// Make the sending order by using LoadOrder
	std::multimap<int, MeterWindow*> windows;
	GetMeterWindowsByLoadOrder(windows);

	// Prepare the helper window
	System::PrepareHelperWindow();

	// Refresh all
	std::multimap<int, MeterWindow*>::const_iterator iter = windows.begin();
	for ( ; iter != windows.end(); ++iter)
	{
		MeterWindow* mw = (*iter).second;
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

					ShowMessage(nullptr, error.c_str(), MB_OK | MB_ICONEXCLAMATION);
					continue;
				}
			}
			else
			{
				const WCHAR* skinFolderPath = mw->GetFolderPath().c_str();
				std::wstring error = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, skinFolderPath, L"");

				DeactivateSkin(mw, -2);  // -2 = Force deactivate

				ShowMessage(nullptr, error.c_str(), MB_OK | MB_ICONEXCLAMATION);
				continue;
			}

			mw->Refresh(false, true);
		}
	}

	DialogAbout::UpdateSkins();
	DialogManage::UpdateSkins(nullptr);
}

bool Rainmeter::LoadLayout(const std::wstring& name)
{
	// Replace Rainmeter.ini with layout
	std::wstring layout = GetLayoutPath();
	layout += name;
	std::wstring wallpaper = layout + L"\\Wallpaper.bmp";
	layout += L"\\Rainmeter.ini";

	if (_waccess(layout.c_str(), 0) == -1)
	{
		return false;
	}

	DeleteAllUnmanagedMeterWindows();
	DeleteAllMeterWindows();

	std::wstring backup = GetLayoutPath();
	backup += L"@Backup";
	CreateDirectory(backup.c_str(), nullptr);
	backup += L"\\Rainmeter.ini";

	bool backupLayout = (_wcsicmp(name.c_str(), L"@Backup") == 0);
	if (!backupLayout)
	{
		// Make a copy of current Rainmeter.ini
		System::CopyFiles(m_IniFile, backup);
	}

	System::CopyFiles(layout, m_IniFile);

	if (!backupLayout)
	{
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
		PreserveSetting(backup, L"UseD2D");

		// Set wallpaper if it exists
		if (_waccess(wallpaper.c_str(), 0) != -1)
		{
			SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)wallpaper.c_str(), SPIF_UPDATEINIFILE);
		}
	}

	ReloadSettings();

	// Create meter windows for active skins
	ActivateActiveSkins();

	return true;
}

void Rainmeter::PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace)
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
void Rainmeter::UpdateDesktopWorkArea(bool reset)
{
	bool changed = false;

	if (reset)
	{
		if (!m_OldDesktopWorkAreas.empty())
		{
			int i = 1;
			for (auto iter = m_OldDesktopWorkAreas.cbegin(); iter != m_OldDesktopWorkAreas.cend(); ++iter, ++i)
			{
				RECT r = (*iter);

				BOOL result = SystemParametersInfo(SPI_SETWORKAREA, 0, &r, 0);

				if (m_Debug)
				{
					std::wstring format = L"Resetting WorkArea@%i: L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)";
					if (!result)
					{
						format += L" => FAIL";
					}
					LogDebugF(format.c_str(), i, r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
			changed = true;
		}
	}
	else
	{
		const size_t numOfMonitors = System::GetMonitorCount();
		const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

		if (m_OldDesktopWorkAreas.empty())
		{
			// Store old work areas for changing them back
			for (size_t i = 0; i < numOfMonitors; ++i)
			{
				m_OldDesktopWorkAreas.push_back(monitors[i].work);
			}
		}

		if (m_Debug)
		{
			LogDebugF(L"DesktopWorkAreaType: %s", m_DesktopWorkAreaType ? L"Margin" : L"Default");
		}

		for (UINT i = 0; i <= numOfMonitors; ++i)
		{
			std::map<UINT, RECT>::const_iterator it = m_DesktopWorkAreas.find(i);
			if (it != m_DesktopWorkAreas.end())
			{
				RECT r = (*it).second;

				// Move rect to correct offset
				if (m_DesktopWorkAreaType)
				{
					RECT margin = r;
					r = (i == 0) ? monitors[monitorsInfo.primary - 1].screen : monitors[i - 1].screen;
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
					LogDebugF(format.c_str(), r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
		}
	}

	if (changed && System::GetWindow())
	{
		// Update System::MultiMonitorInfo for for work area variables
		SendMessageTimeout(System::GetWindow(), WM_SETTINGCHANGE, SPI_SETWORKAREA, 0, SMTO_ABORTIFHUNG, 1000, nullptr);
	}
}

/*
** Reads the statistics from the ini-file
**
*/
void Rainmeter::ReadStats()
{
	const WCHAR* statsFile = m_StatsFile.c_str();

	// If m_StatsFile doesn't exist, create it and copy the stats section from m_IniFile
	if (_waccess(statsFile, 0) == -1)
	{
		const WCHAR* iniFile = m_IniFile.c_str();
		WCHAR* tmpSz = new WCHAR[SHRT_MAX];	// Max size returned by GetPrivateProfileSection()

		if (GetPrivateProfileSection(L"Statistics", tmpSz, SHRT_MAX, iniFile) > 0)
		{
			WritePrivateProfileString(L"Statistics", nullptr, nullptr, iniFile);
		}
		else
		{
			tmpSz[0] = tmpSz[1] = L'\0';
		}
		WritePrivateProfileSection(L"Statistics", tmpSz, statsFile);

		delete [] tmpSz;
	}

	// Only Net measure has stats at the moment
	MeasureNet::ReadStats(m_StatsFile, m_StatsDate);
}

/*
** Writes the statistics to the ini-file. If bForce is false the stats are written only once per an appropriate interval.
**
*/
void Rainmeter::WriteStats(bool bForce)
{
	static ULONGLONG lastWrite = 0;

	ULONGLONG ticks = System::GetTickCount64();

	if (bForce || (lastWrite + INTERVAL_NETSTATS < ticks))
	{
		lastWrite = ticks;

		// Only Net measure has stats at the moment
		const WCHAR* statsFile = m_StatsFile.c_str();
		MeasureNet::WriteStats(statsFile, m_StatsDate);

		WritePrivateProfileString(nullptr, nullptr, nullptr, statsFile);
	}
}

/*
** Clears the statistics
**
*/
void Rainmeter::ResetStats()
{
	// Set the stats-date string
	tm* newtime;
	time_t long_time;
	time(&long_time);
	newtime = localtime(&long_time);
	m_StatsDate = _wasctime(newtime);
	m_StatsDate.erase(m_StatsDate.size() - 1);

	// Only Net measure has stats at the moment
	MeasureNet::ResetStats();
}

/*
** Wraps MessageBox(). Sets RTL flag if necessary.
**
*/
int Rainmeter::ShowMessage(HWND parent, const WCHAR* text, UINT type)
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
void Rainmeter::ShowContextMenu(POINT pos, MeterWindow* meterWindow)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_MANAGE, ID_STR_MANAGE),
		MENU_ITEM(IDM_ABOUT, ID_STR_ABOUT),
		MENU_ITEM(IDM_SHOW_HELP, ID_STR_HELP),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_SKINS,
			MENU_ITEM_GRAYED(0, ID_STR_NOSKINS),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_OPENSKINSFOLDER, ID_STR_OPENFOLDER),
			MENU_ITEM(IDM_DISABLEDRAG, ID_STR_DISABLEDRAGGING)),
		MENU_SUBMENU(ID_STR_THEMES,
			MENU_ITEM_GRAYED(0, ID_STR_NOTHEMES)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_EDITCONFIG, ID_STR_EDITSETTINGS),
		MENU_ITEM(IDM_REFRESH, ID_STR_REFRESHALL),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_LOGGING,
			MENU_ITEM(IDM_SHOWLOGFILE, ID_STR_SHOWLOGFILE),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_STARTLOG, ID_STR_STARTLOGGING),
			MENU_ITEM(IDM_STOPLOG, ID_STR_STOPLOGGING),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_DELETELOGFILE, ID_STR_DELETELOGFILE),
			MENU_ITEM(IDM_DEBUGLOG, ID_STR_DEBUGMODE)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_QUIT, ID_STR_EXIT)
	};

	if (!m_MenuActive && (!meterWindow || !meterWindow->IsClosing()))
	{
		m_MenuActive = true;

		// Show context menu, if no actions were executed
		HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
		if (menu)
		{
			SetMenuDefaultItem(menu, IDM_MANAGE, MF_BYCOMMAND);

			if (_waccess(GetLogger().GetLogFilePath().c_str(), 0) == -1)
			{
				EnableMenuItem(menu, IDM_SHOWLOGFILE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(menu, IDM_DELETELOGFILE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(menu, IDM_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				EnableMenuItem(
					menu,
					(GetLogger().IsLogToFile()) ? IDM_STARTLOG : IDM_STOPLOG,
					MF_BYCOMMAND | MF_GRAYED);
			}

			if (m_Debug)
			{
				CheckMenuItem(menu, IDM_DEBUGLOG, MF_BYCOMMAND | MF_CHECKED);
			}

			HMENU allSkinsMenu = GetSubMenu(menu, 4);
			if (allSkinsMenu)
			{
				if (!m_SkinFolders.empty())
				{
					DeleteMenu(allSkinsMenu, 0, MF_BYPOSITION);  // "No skins available" menuitem
					CreateAllSkinsMenu(allSkinsMenu);
				}

				if (m_DisableDragging)
				{
					CheckMenuItem(allSkinsMenu, IDM_DISABLEDRAG, MF_BYCOMMAND | MF_CHECKED);
				}
			}

			HMENU layoutMenu = GetSubMenu(menu, 5);
			if (layoutMenu)
			{
				if (!m_Layouts.empty())
				{
					DeleteMenu(layoutMenu, 0, MF_BYPOSITION);  // "No layouts available" menuitem
					CreateLayoutMenu(layoutMenu);
				}
			}

			if (meterWindow)
			{
				HMENU rainmeterMenu = menu;
				menu = CreateSkinMenu(meterWindow, 0, allSkinsMenu);

				InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)rainmeterMenu, L"Rainmeter");
				InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_SEPARATOR, 0, nullptr);
			}
			else
			{
				InsertMenu(menu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

				// Create a menu for all active skins
				int index = 0;
				std::map<std::wstring, MeterWindow*>::const_iterator iter = m_MeterWindows.begin();
				for (; iter != m_MeterWindows.end(); ++iter)
				{
					MeterWindow* mw = ((*iter).second);
					HMENU skinMenu = CreateSkinMenu(mw, index, allSkinsMenu);
					InsertMenu(menu, 12, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, mw->GetFolderPath().c_str());
					++index;
				}

				// Add update notification item
				if (m_NewVersion)
				{
					InsertMenu(menu, 0, MF_BYPOSITION, IDM_NEW_VERSION, GetString(ID_STR_UPDATEAVAILABLE));
					HiliteMenuItem(GetTrayWindow()->GetWindow(), menu, 0, MF_BYPOSITION | MF_HILITE);
					InsertMenu(menu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
				}
			}

			HWND hWnd = WindowFromPoint(pos);
			if (hWnd != nullptr)
			{
				MeterWindow* mw = GetMeterWindow(hWnd);
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
				DWORD foregroundThreadID = GetWindowThreadProcessId(hWndForeground, nullptr);
				DWORD currentThreadID = GetCurrentThreadId();
				AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
				SetForegroundWindow(hWnd);
				AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
			}

			// Show context menu
			TrackPopupMenu(
				menu,
				TPM_RIGHTBUTTON | TPM_LEFTALIGN | (*GetString(ID_STR_ISRTL) == L'1' ? TPM_LAYOUTRTL : 0),
				pos.x,
				pos.y,
				0,
				hWnd,
				nullptr);

			if (meterWindow)
			{
				DestroyMenu(menu);
			}
		}

		DestroyMenu(menu);

		m_MenuActive = false;
	}
}

int Rainmeter::CreateAllSkinsMenuRecursive(HMENU skinMenu, int index)
{
	int initialLevel = m_SkinFolders[index].level;
	int menuIndex = 0;

	const size_t max = GetRainmeter().m_SkinFolders.size();
	while (index < max)
	{
		const SkinFolder& skinFolder = GetRainmeter().m_SkinFolders[index];
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
				InsertMenu(subMenu, fileIndex, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
			}
		}

		++menuIndex;
		++index;
	}

	return index;
}

HMENU Rainmeter::CreateSkinMenu(MeterWindow* meterWindow, int index, HMENU menu)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_SKIN_OPENSKINSFOLDER, 0),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_VARIANTS,
			MENU_SEPARATOR()),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_SETTINGS,
			MENU_SUBMENU(ID_STR_POSITION,
				MENU_SUBMENU(ID_STR_DISPLAYMONITOR,
					MENU_ITEM(IDM_SKIN_MONITOR_PRIMARY, ID_STR_USEDEFAULTMONITOR),
					MENU_ITEM(ID_MONITOR_FIRST, ID_STR_VIRTUALSCREEN),
					MENU_SEPARATOR(),
					MENU_SEPARATOR(),
					MENU_ITEM(IDM_SKIN_MONITOR_AUTOSELECT, ID_STR_AUTOSELECTMONITOR)),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_VERYTOPMOST, ID_STR_STAYTOPMOST),
				MENU_ITEM(IDM_SKIN_TOPMOST, ID_STR_TOPMOST),
				MENU_ITEM(IDM_SKIN_NORMAL, ID_STR_NORMAL),
				MENU_ITEM(IDM_SKIN_BOTTOM, ID_STR_BOTTOM),
				MENU_ITEM(IDM_SKIN_ONDESKTOP, ID_STR_ONDESKTOP),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_FROMRIGHT, ID_STR_FROMRIGHT),
				MENU_ITEM(IDM_SKIN_FROMBOTTOM, ID_STR_FROMBOTTOM),
				MENU_ITEM(IDM_SKIN_XPERCENTAGE, ID_STR_XASPERCENTAGE),
				MENU_ITEM(IDM_SKIN_YPERCENTAGE, ID_STR_YASPERCENTAGE)),
			MENU_SUBMENU(ID_STR_TRANSPARENCY,
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_0, ID_STR_0PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_10, ID_STR_10PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_20, ID_STR_20PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_30, ID_STR_30PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_40, ID_STR_40PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_50, ID_STR_50PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_60, ID_STR_60PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_70, ID_STR_70PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_80, ID_STR_80PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_90, ID_STR_90PERCENT),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEIN, ID_STR_FADEIN),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEOUT, ID_STR_FADEOUT)),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_SKIN_HIDEONMOUSE, ID_STR_HIDEONMOUSEOVER),
			MENU_ITEM(IDM_SKIN_DRAGGABLE, ID_STR_DRAGGABLE),
			MENU_ITEM(IDM_SKIN_REMEMBERPOSITION, ID_STR_SAVEPOSITION),
			MENU_ITEM(IDM_SKIN_SNAPTOEDGES, ID_STR_SNAPTOEDGES),
			MENU_ITEM(IDM_SKIN_CLICKTHROUGH, ID_STR_CLICKTHROUGH),
			MENU_ITEM(IDM_SKIN_KEEPONSCREEN, ID_STR_KEEPONSCREEN)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_SKIN_MANAGESKIN, ID_STR_MANAGESKIN),
		MENU_ITEM(IDM_SKIN_EDITSKIN, ID_STR_EDITSKIN),
		MENU_ITEM(IDM_SKIN_REFRESH, ID_STR_REFRESHSKIN),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_CLOSESKIN, ID_STR_UNLOADSKIN)
	};

	HMENU skinMenu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
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

		// Add custom actions to the context menu
		std::wstring contextTitle = meterWindow->GetParser().ReadString(L"Rainmeter", L"ContextTitle", L"");
		if (!contextTitle.empty())
		{
			auto isTitleSeparator = [](const std::wstring& title)
			{
				return title.find_first_not_of(L'-') == std::wstring::npos;
			};

			std::wstring contextAction = meterWindow->GetParser().ReadString(L"Rainmeter", L"ContextAction", L"");
			if (!contextAction.empty() || isTitleSeparator(contextTitle))
			{
				std::vector<std::wstring> cTitles;
				WCHAR buffer[128];
				int i = 1;

				while (!contextTitle.empty() &&
					  (!contextAction.empty() || isTitleSeparator(contextTitle)) &&
					  (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i - 1) <= IDM_SKIN_CUSTOMCONTEXTMENU_LAST) // Set maximum context items in resource.h
				{
					// Trim long titles
					if (contextTitle.size() > 30)
					{
						contextTitle.replace(27, contextTitle.size() - 27, L"...");
					}

					cTitles.push_back(contextTitle);

					_snwprintf_s(buffer, _TRUNCATE, L"ContextTitle%i", ++i);
					contextTitle = meterWindow->GetParser().ReadString(L"Rainmeter", buffer, L"");
					_snwprintf_s(buffer, _TRUNCATE, L"ContextAction%i", i);
					contextAction = meterWindow->GetParser().ReadString(L"Rainmeter", buffer, L"");
				}

				// Build a sub-menu if more than three items
				size_t titleSize = cTitles.size();
				if (titleSize <= 3)
				{
					size_t position = 0;
					for (size_t i = 0; i < titleSize; ++i)
					{
						if (isTitleSeparator(cTitles[i]))
						{
							// Separators not allowed in main top-level menu
							--position;
						}
						else
						{
							InsertMenu(skinMenu, position + 1, MF_BYPOSITION | MF_STRING, (index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i), cTitles[i].c_str());
						}

						++position;
					}

					if (position != 0)
					{
						InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, L"Custom skin actions");
						InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
					}
				}
				else
				{
					HMENU customMenu = CreatePopupMenu();
					InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)customMenu, L"Custom skin actions");
				
					for (size_t i = 0; i < titleSize; ++i)
					{
						if (isTitleSeparator(cTitles[i]))
						{
							AppendMenu(customMenu, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
						}
						else
						{
							AppendMenu(customMenu, MF_BYPOSITION | MF_STRING, (index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i), cTitles[i].c_str());
						}
					}

					InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
				}
			}
		}
	}

	return skinMenu;
}

void Rainmeter::CreateLayoutMenu(HMENU layoutMenu)
{
	for (size_t i = 0, isize = m_Layouts.size(); i < isize; ++i)
	{
		InsertMenu(layoutMenu, i, MF_BYPOSITION, ID_THEME_FIRST + i, m_Layouts[i].c_str());
	}
}

void Rainmeter::CreateMonitorMenu(HMENU monitorMenu, MeterWindow* meterWindow)
{
	bool screenDefined = meterWindow->GetXScreenDefined();
	int screenIndex = meterWindow->GetXScreen();

	// for the "Specified monitor" (@n)
	const size_t numOfMonitors = System::GetMonitorCount();  // intentional
	const std::vector<MonitorInfo>& monitors = System::GetMultiMonitorInfo().monitors;

	int i = 1;
	for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++i)
	{
		WCHAR buffer[64];
		size_t len = _snwprintf_s(buffer, _TRUNCATE, L"@%i: ", i);

		std::wstring item(buffer, len);

		if ((*iter).monitorName.size() > 32)
		{
			item.append((*iter).monitorName, 0, 32);
			item += L"...";
		}
		else
		{
			item += (*iter).monitorName;
		}

		InsertMenu(monitorMenu,
			i + 2,
			MF_BYPOSITION | ((screenDefined && screenIndex == i) ? MF_CHECKED : MF_UNCHECKED) | ((!(*iter).active) ? MF_GRAYED : MF_ENABLED),
			ID_MONITOR_FIRST + i,
			item.c_str());
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

void Rainmeter::ChangeSkinIndex(HMENU menu, int index)
{
	if (index > 0)
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
				MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
				mii.fMask = MIIM_FTYPE | MIIM_ID;
				GetMenuItemInfo(menu, i, TRUE, &mii);
				if ((mii.fType & MFT_SEPARATOR) == 0)
				{
					mii.wID |= (index << 16);
					mii.fMask = MIIM_ID;
					SetMenuItemInfo(menu, i, TRUE, &mii);
				}
			}
		}
	}
}

void Rainmeter::ShowLogFile()
{
	std::wstring logFile = L'"' + GetLogger().GetLogFilePath();
	logFile += L'"';

	CommandHandler::RunFile(m_SkinEditor.c_str(), logFile.c_str());
}

void Rainmeter::SetDebug(bool debug)
{
	m_Debug = debug;
	WritePrivateProfileString(L"Rainmeter", L"Debug", debug ? L"1" : L"0", m_IniFile.c_str());
}

void Rainmeter::SetDisableDragging(bool dragging)
{
	m_DisableDragging = dragging;
	WritePrivateProfileString(L"Rainmeter", L"DisableDragging", dragging ? L"1" : L"0", m_IniFile.c_str());
}

void Rainmeter::SetDisableVersionCheck(bool check)
{
	m_DisableVersionCheck = check;
	WritePrivateProfileString(L"Rainmeter", L"DisableVersionCheck", check ? L"1" : L"0" , m_IniFile.c_str());
}

void Rainmeter::TestSettingsFile(bool bDefaultIniLocation)
{
	const WCHAR* iniFile = m_IniFile.c_str();
	if (!System::IsFileWritable(iniFile))
	{
		std::wstring error = GetString(ID_STR_SETTINGSNOTWRITABLE);

		if (!bDefaultIniLocation)
		{
			std::wstring strTarget = L"%APPDATA%\\Rainmeter\\";
			PathUtil::ExpandEnvironmentVariables(strTarget);

			error += GetFormattedString(ID_STR_SETTINGSMOVEFILE, iniFile, strTarget.c_str());
		}
		else
		{
			error += GetFormattedString(ID_STR_SETTINGSREADONLY, iniFile);
		}

		ShowMessage(nullptr, error.c_str(), MB_OK | MB_ICONERROR);
	}
}
