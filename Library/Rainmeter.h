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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __RAINMETER_H__
#define __RAINMETER_H__

#include <windows.h>
#include "Litestep.h"
#include "MeterWindow.h"
#include "TrayWindow.h"
#include <map>
#include <vector>
#include <string>

#define MAX_LINE_LENGTH 4096

#define MAKE_VER(major, minor) major * 1000 + minor

#define APPNAME L"Rainmeter"
#define APPVERSION L"0.14.1"
#ifdef _WIN64
#define APPBITS L"(64-bit)"
#else
#define APPBITS L"(32-bit)"
#endif
#define RAINMETER_VERSION MAKE_VER(14, 1)

enum PLATFORM
{
	PLATFORM_9X,
	PLATFORM_NT4,
	PLATFORM_2K,
	PLATFORM_XP
};

void RainmeterActivateConfig(HWND, const char* arg);
void RainmeterDeactivateConfig(HWND, const char* arg);
void RainmeterToggleConfig(HWND, const char* arg);
void RainmeterRefresh(HWND, const char* arg);
void RainmeterRedraw(HWND, const char* arg);
void RainmeterToggleMeasure(HWND, const char* arg);
void RainmeterEnableMeasure(HWND, const char* arg);
void RainmeterDisableMeasure(HWND, const char* arg);
void RainmeterToggleMeter(HWND, const char* arg);
void RainmeterShowMeter(HWND, const char* arg);
void RainmeterHideMeter(HWND, const char* arg);
void RainmeterShow(HWND, const char* arg);
void RainmeterHide(HWND, const char* arg);
void RainmeterToggle(HWND, const char* arg);
void RainmeterMove(HWND, const char* arg);
void RainmeterZPos(HWND, const char* arg);
void RainmeterLsHook(HWND, const char* arg);
void RainmeterAbout(HWND, const char* arg);
void RainmeterResetStats(HWND, const char* arg);
void RainmeterMoveMeter(HWND, const char* arg);
void RainmeterPluginBang(HWND, const char* arg);

void BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs);

struct GlobalConfig
{
	double netInSpeed;
	double netOutSpeed;
};

class CRainmeter
{
public:
	struct CONFIG 
	{
		std::wstring path;
		std::wstring config;
		std::vector<std::wstring> iniFiles;
		std::vector<UINT> commands;
		int active;
	};

	struct CONFIGMENU
	{
		std::wstring name;
		size_t index;
		std::vector<CONFIGMENU> children;
	};


	CRainmeter();
	~CRainmeter();

	int Initialize(HWND Parent, HINSTANCE Instance, LPCSTR szPath);
	void Quit(HINSTANCE dllInst);

	CConfigParser* GetCurrentParser() { return m_CurrentParser; };
	void SetCurrentParser(CConfigParser* parser) { m_CurrentParser = parser; };

	CTrayWindow* GetTrayWindow() { return m_TrayWindow; };

	CMeterWindow* GetMeterWindow(const std::wstring& config);
	std::map<std::wstring, CMeterWindow*>& GetAllMeterWindows() { return m_Meters; };
	const std::vector<CONFIG>& GetAllConfigs() { return m_ConfigStrings; };

	void ActivateConfig(int configIndex, int iniIndex);
	bool DeactivateConfig(CMeterWindow* meterWindow, int configIndex);

	const std::wstring& GetPath() { return m_Path; };
	const std::wstring& GetIniFile() { return m_IniFile; };
	const std::wstring& GetLogFile() { return m_LogFile; };
	const std::wstring& GetSkinPath() { return m_SkinPath; };
	const std::wstring& GetPluginPath() { return m_PluginPath; };
	std::wstring GetSettingsPath() { return ExtractPath(m_IniFile); };

	const std::wstring& GetConfigEditor() { return m_ConfigEditor; };
	const std::wstring& GetStatsDate() { return m_StatsDate; };

	HINSTANCE GetInstance() { return m_Instance; };

	static void SetDummyLitestep(bool Dummy) { c_DummyLitestep = Dummy; };
	static bool GetDummyLitestep() { return c_DummyLitestep; };
	static void SetCommandLine(LPCTSTR CmdLine) { c_CmdLine = CmdLine;};
	static LPCTSTR GetCommandLine() { return c_CmdLine.c_str(); };
	static GlobalConfig& GetGlobalConfig() { return c_GlobalConfig; };

	void ReloadSettings();
	void SaveSettings();

	void UpdateStats();
	void ReadStats();
	void WriteStats(bool bForce);
	void ResetStats();

	BOOL GetCheckUpdate() { return m_CheckUpdate; };
	void SetCheckUpdate(BOOL check) { m_CheckUpdate = check; };

	void ShowContextMenu(POINT pos, CMeterWindow* meterWindow);

	std::wstring GetTrayExecuteL() { return m_TrayExecuteL; };
	std::wstring GetTrayExecuteR() { return m_TrayExecuteR; };
	std::wstring GetTrayExecuteM() { return m_TrayExecuteM; };
	std::wstring GetTrayExecuteDR() { return m_TrayExecuteDR; };
	std::wstring GetTrayExecuteDL() { return m_TrayExecuteDL; };
	std::wstring GetTrayExecuteDM() { return m_TrayExecuteDM; };

	BOOL ExecuteBang(const std::wstring& bang, const std::wstring& arg, CMeterWindow* meterWindow);
	std::wstring ParseCommand(const WCHAR* command, CMeterWindow* meterWindow);
	void ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow);

	static PLATFORM IsNT();
	static std::wstring ExtractPath(const std::wstring& strFilePath);
	static std::wstring ExpandEnvironmentVariables(const std::wstring strPath);

private:
	void CreateMeterWindow(std::wstring path, std::wstring config, std::wstring iniFile);
	bool DeleteMeterWindow(CMeterWindow* meterWindow);
	void ScanForConfigs(std::wstring& path);
	void ReadGeneralSettings(std::wstring& path);
	bool SetActiveConfig(std::wstring& skinName, std::wstring& skinIni);
	void Refresh(const WCHAR* arg);
	HMENU CreateSkinMenu(CMeterWindow* meterWindow, int index);
	void ChangeSkinIndex(HMENU subMenu, int index);
	int ScanForConfigsRecursive(std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu);
	HMENU CreateConfigMenu(std::vector<CONFIGMENU>& configMenuData);
	void CreateDefaultConfigFile(std::wstring strFile);
	void TestSettingsFile(bool bDefaultIniLocation);
	void CopyFiles(std::wstring strFrom, std::wstring strTo);

	CTrayWindow* m_TrayWindow;

	std::vector<CONFIG> m_ConfigStrings;	    // All configs found in the given folder
	std::vector<CONFIGMENU> m_ConfigMenu;
	std::map<std::wstring, CMeterWindow*> m_Meters;			// The meter windows

	std::wstring m_Path;			    // Path to the main folder
	std::wstring m_IniFile;				// The main ini file
	std::wstring m_LogFile;				// The log file
	std::wstring m_SkinPath;			// Path to the folder where the skins are
	std::wstring m_PluginPath;			// Path to the folder where the plugins are

	std::wstring m_StatsDate;					// The date when stats gathering started

	std::wstring m_TrayExecuteL;
	std::wstring m_TrayExecuteR;
	std::wstring m_TrayExecuteM;
	std::wstring m_TrayExecuteDL;
	std::wstring m_TrayExecuteDR;
	std::wstring m_TrayExecuteDM;

	BOOL m_CheckUpdate;

	BOOL m_DesktopWorkAreaChanged;
	RECT m_DesktopWorkArea;

	std::wstring m_ConfigEditor;

	CConfigParser* m_CurrentParser;

	HINSTANCE m_Instance;

	ULONG_PTR m_GDIplusToken;

	static bool c_DummyLitestep;	// true, if not a Litestep plugin
	static std::wstring c_CmdLine;	// The command line arguments
	static GlobalConfig c_GlobalConfig;
};

#ifdef LIBRARY_EXPORTS
#define EXPORT_PLUGIN __declspec(dllexport)
#else
#define EXPORT_PLUGIN __declspec(dllimport)
#endif

extern "C"
{
	EXPORT_PLUGIN int initModuleEx(HWND ParentWnd, HINSTANCE dllInst, LPCSTR szPath);
	EXPORT_PLUGIN void quitModule(HINSTANCE dllInst);
	EXPORT_PLUGIN void Initialize(bool DummyLS, LPCTSTR CmdLine);
}

#endif
