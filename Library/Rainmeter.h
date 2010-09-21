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
#include <map>
#include <vector>
#include <string>
#include "Litestep.h"
#include "MeterWindow.h"
#include "TrayWindow.h"

#define MAX_LINE_LENGTH 4096

#define MAKE_VER(major, minor1, minor2) major * 1000000 + minor1 * 1000 + minor2

#define APPNAME L"Rainmeter"
#define APPVERSION L"1.3"
#ifdef _WIN64
#define APPBITS L"(64-bit)"
#else
#define APPBITS L"(32-bit)"
#endif
#define RAINMETER_VERSION MAKE_VER(1, 3, 0)

void RainmeterRefresh(HWND, const char* arg);
void RainmeterRefreshApp(HWND, const char* arg);
void RainmeterRedraw(HWND, const char* arg);
void RainmeterShow(HWND, const char* arg);
void RainmeterHide(HWND, const char* arg);
void RainmeterToggle(HWND, const char* arg);
void RainmeterShowFade(HWND, const char* arg);
void RainmeterHideFade(HWND, const char* arg);
void RainmeterToggleFade(HWND, const char* arg);
void RainmeterShowMeter(HWND, const char* arg);
void RainmeterHideMeter(HWND, const char* arg);
void RainmeterToggleMeter(HWND, const char* arg);
void RainmeterMoveMeter(HWND, const char* arg);
void RainmeterEnableMeasure(HWND, const char* arg);
void RainmeterDisableMeasure(HWND, const char* arg);
void RainmeterToggleMeasure(HWND, const char* arg);
void RainmeterActivateConfig(HWND, const char* arg);
void RainmeterDeactivateConfig(HWND, const char* arg);
void RainmeterToggleConfig(HWND, const char* arg);
void RainmeterMove(HWND, const char* arg);
void RainmeterZPos(HWND, const char* arg);
void RainmeterClickThrough(HWND, const char* arg);
void RainmeterDraggable(HWND, const char* arg);
void RainmeterSnapEdges(HWND, const char* arg);
void RainmeterKeepOnScreen(HWND, const char* arg);
void RainmeterSetTransparency(HWND, const char* arg);
void RainmeterSetVariable(HWND, const char* arg);

void RainmeterRefreshGroup(HWND, const char* arg);
void RainmeterRedrawGroup(HWND, const char* arg);
void RainmeterShowGroup(HWND, const char* arg);
void RainmeterHideGroup(HWND, const char* arg);
void RainmeterToggleGroup(HWND, const char* arg);
void RainmeterShowFadeGroup(HWND, const char* arg);
void RainmeterHideFadeGroup(HWND, const char* arg);
void RainmeterToggleFadeGroup(HWND, const char* arg);
void RainmeterShowMeterGroup(HWND, const char* arg);
void RainmeterHideMeterGroup(HWND, const char* arg);
void RainmeterToggleMeterGroup(HWND, const char* arg);
void RainmeterEnableMeasureGroup(HWND, const char* arg);
void RainmeterDisableMeasureGroup(HWND, const char* arg);
void RainmeterToggleMeasureGroup(HWND, const char* arg);
void RainmeterDeactivateConfigGroup(HWND, const char* arg);
void RainmeterZPosGroup(HWND, const char* arg);
void RainmeterClickThroughGroup(HWND, const char* arg);
void RainmeterDraggableGroup(HWND, const char* arg);
void RainmeterSnapEdgesGroup(HWND, const char* arg);
void RainmeterKeepOnScreenGroup(HWND, const char* arg);
void RainmeterSetTransparencyGroup(HWND, const char* arg);
void RainmeterSetVariableGroup(HWND, const char* arg);

void RainmeterLsHook(HWND, const char* arg);
void RainmeterAbout(HWND, const char* arg);
void RainmeterSkinMenu(HWND, const char* arg);
void RainmeterTrayMenu(HWND, const char* arg);
void RainmeterResetStats(HWND, const char* arg);
void RainmeterWriteKeyValue(HWND, const char* arg);
void RainmeterPluginBang(HWND, const char* arg);
void RainmeterQuit(HWND, const char* arg);

void BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs);
void BangGroupWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs);

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

	struct LOG_INFO
	{
		std::wstring type;
		std::wstring timestamp;
		std::wstring message;
	};


	CRainmeter();
	~CRainmeter();

	int Initialize(HWND Parent, HINSTANCE Instance, LPCSTR szPath);
	void Quit(HINSTANCE dllInst);

	CConfigParser* GetCurrentParser() { return m_CurrentParser; };
	void SetCurrentParser(CConfigParser* parser) { m_CurrentParser = parser; };

	CTrayWindow* GetTrayWindow() { return m_TrayWindow; };

	CMeterWindow* GetMeterWindow(const std::wstring& config);
	CMeterWindow* GetMeterWindow(HWND hwnd);
	void GetMeterWindowsByLoadOrder(std::multimap<int, CMeterWindow*>& windows, const std::wstring& group = L"");
	std::map<std::wstring, CMeterWindow*>& GetAllMeterWindows() { return m_Meters; };
	const std::vector<CONFIG>& GetAllConfigs() { return m_ConfigStrings; };
	const std::vector<std::wstring>& GetAllThemes() { return m_Themes; };

	void ActivateConfig(int configIndex, int iniIndex);
	bool DeactivateConfig(CMeterWindow* meterWindow, int configIndex);

	const std::wstring& GetPath() { return m_Path; };
	const std::wstring& GetIniFile() { return m_IniFile; };
	const std::wstring& GetLogFile() { return m_LogFile; };
	const std::wstring& GetSkinPath() { return m_SkinPath; };
	const std::wstring& GetPluginPath() { return m_PluginPath; };
	std::wstring GetSettingsPath() { return ExtractPath(m_IniFile); };

	const std::wstring& GetConfigEditor() { return m_ConfigEditor; };
	const std::wstring& GetLogViewer() { return m_LogViewer; };
	const std::wstring& GetStatsDate() { return m_StatsDate; };

	HINSTANCE GetInstance() { return m_Instance; };

	static void SetDummyLitestep(bool Dummy) { c_DummyLitestep = Dummy; };
	static bool GetDummyLitestep() { return c_DummyLitestep; };
	static void SetCommandLine(LPCTSTR CmdLine) { c_CmdLine = CmdLine;};
	static LPCTSTR GetCommandLine() { return c_CmdLine.c_str(); };
	static GlobalConfig& GetGlobalConfig() { return c_GlobalConfig; };

	static bool GetDebug() { return c_Debug; }

	void ReloadSettings();
	void SaveSettings();

	void UpdateStats();
	void ReadStats();
	void WriteStats(bool bForce);
	void ResetStats();

	BOOL GetDisableVersionCheck() { return m_DisableVersionCheck; };
	BOOL GetNewVersion() { return m_NewVersion; };
	void SetDisableVersionCheck(BOOL check) { m_DisableVersionCheck = check; };
	void SetNewVersion(BOOL NewVer) { m_NewVersion = NewVer; };

	bool GetLogging() { return m_Logging; }
	void StartLogging();
	void StopLogging();
	void DeleteLogFile();

	void AddAboutLogInfo(const LOG_INFO& logInfo);
	const std::list<LOG_INFO>& GetAboutLogData() { return m_LogData; }

	void SetDebug(bool debug);

	bool IsMenuActive() { return m_MenuActive; }
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

	void RefreshAll();

	void ClearDeleteLaterList();

	static std::vector<std::wstring> ParseString(LPCTSTR str);
	static std::wstring ExtractPath(const std::wstring& strFilePath);
	static void ExpandEnvironmentVariables(std::wstring& strPath);

private:
	void CreateMeterWindow(std::wstring path, std::wstring config, std::wstring iniFile);
	bool DeleteMeterWindow(CMeterWindow* meterWindow, bool bLater);
	void WriteActive(const std::wstring& config, int iniIndex);
	void ScanForConfigs(std::wstring& path);
	void ScanForThemes(std::wstring& path);
	void ReadGeneralSettings(std::wstring& path);
	void SetConfigOrder(int configIndex);
	int GetLoadOrder(const std::wstring& config);
	bool SetActiveConfig(std::wstring& skinName, std::wstring& skinIni);
	void UpdateDesktopWorkArea(bool reset);
	HMENU CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU configMenu);
	void ChangeSkinIndex(HMENU subMenu, int index);
	int ScanForConfigsRecursive(std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu, bool DontRecurse);
	HMENU CreateConfigMenu(std::vector<CONFIGMENU>& configMenuData);
	HMENU CreateThemeMenu();
	void CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow);
	void CreateDefaultConfigFile(std::wstring strFile);
	void SetLogging(bool logging);
	void TestSettingsFile(bool bDefaultIniLocation);
	void CheckSkinVersions();
	int CompareVersions(std::wstring strA, std::wstring strB);

	CTrayWindow* m_TrayWindow;

	std::vector<CONFIG> m_ConfigStrings;	    // All configs found in the given folder
	std::vector<CONFIGMENU> m_ConfigMenu;
	std::multimap<int, int> m_ConfigOrders;
	std::map<std::wstring, CMeterWindow*> m_Meters;			// The meter windows
	std::vector<std::wstring> m_Themes;

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

	BOOL m_DisableVersionCheck;
	BOOL m_NewVersion;
	
	bool m_DesktopWorkAreaChanged;
	bool m_DesktopWorkAreaType;			// If true, DesktopWorkArea is treated as "margin"
	std::map<UINT, RECT> m_DesktopWorkAreas;
	std::vector<RECT> m_OldDesktopWorkAreas;

	bool m_MenuActive;

	bool m_Logging;

	std::list<LOG_INFO> m_LogData;
	CRITICAL_SECTION m_CsLogData;

	std::wstring m_ConfigEditor;
	std::wstring m_LogViewer;

	CConfigParser* m_CurrentParser;

	HINSTANCE m_Instance;

	ULONG_PTR m_GDIplusToken;

	std::list<CMeterWindow*> m_DelayDeleteList;

	static bool c_DummyLitestep;	// true, if not a Litestep plugin
	static std::wstring c_CmdLine;	// The command line arguments
	static GlobalConfig c_GlobalConfig;
	static bool c_Debug;
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
	EXPORT_PLUGIN void ExecuteBang(LPCTSTR szBang);
}

#endif
