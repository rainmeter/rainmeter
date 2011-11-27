/*
  Copyright (C) 2011 Birunthan Mohanathas

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __RAINSTALLER_H__
#define __RAINSTALLER_H__

#define OBJECT_NAME		L"RainstallerObject_6D42B76464DA"
#define MB_ERROR		MB_OK | MB_TOPMOST | MB_ICONERROR

#ifdef _WIN64
	#define PLUGINS_ROOT	L"Plugins/64bit/"
#else
	#define PLUGINS_ROOT	L"Plugins/32bit/"
#endif

typedef BOOL (WINAPI* CHECKTOKENMEMBERSHIP)(HANDLE tokenHandle, PSID sidToCheck, PBOOL isMember);
typedef BOOL (WINAPI* GETFONTRESOURCEINFO)(LPCTSTR lpszFilename, LPDWORD cbBuffer, LPVOID lpBuffer, DWORD dwQueryType);

enum INSTTYPE
{
	INSTTYPE_ADMIN = 1,		// Installs plugins, addons, and fonts
	INSTTYPE_NOADMIN,		// Installs skins and themes
	INSTTYPE_FULL			// Install all components
};

enum TIMER
{
	TIMER_THREAD = 1,
	TIMER_PROCESS
};

struct RMSKIN_DATA
{
	INSTTYPE instType;
	HANDLE instHandle;
	int rootLen;
	bool mergeSkins;
	bool launchRainmeter;
	bool rainmeterFonts;

	std::wstring packageName;
	std::wstring packageAuthor;
	std::wstring packageVersion;
	std::wstring rmskinFile;
	std::wstring backupFolder;
	std::wstring iniPath;
	std::wstring skinsPath;
	std::wstring rainmeterPath;
	std::wstring addonsList;
	std::wstring pluginsList;
	std::wstring skinsList;
	std::wstring themesList;
	std::wstring fontsList;
	std::wstring loadTheme;
	std::wstring loadSkins;
	std::wstring keepVariables;
};

int Rainstaller(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK RunAsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitDialog(HWND hwnd);
void InitInstall(HWND hwnd);

bool ReadArchive();
bool ReadSettings(LPCTSTR filePath);
bool ExtractCurrentFile(unzFile& ufile, LPCTSTR fileName);
bool IsDefaultPlugin(LPCTSTR plugin);
bool IsDefaultAddon(LPCTSTR addon);

HANDLE CreateProcessElevated(HWND hwnd);
unsigned __stdcall CreateInstallThread(void* pParam);
unsigned __stdcall SetRunAsThread(void*);
bool InstallComponents(RMSKIN_DATA* data);
bool BackupComponent(const std::wstring& backupFolder, const std::wstring& list, const std::wstring& path);
void KeepVariables(const std::wstring& backupFolder, const std::wstring& skinsPath, const std::wstring& fileList);
void LaunchRainmeter();

int CompareVersions(const std::wstring& strA, const std::wstring& strB);
bool CopyFiles(const std::wstring& strFrom, const std::wstring& strTo, bool bMove);
std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
std::wstring GetDotNETVersion();
std::wstring GetFileVersion(const std::wstring& file);
std::wstring GetWindowsVersion();
BOOL IsCurrentProcessAdmin();
BOOL IsAboveVista();
BOOL IsActiveUAC();

#endif
