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

#ifndef __RAINMETER_H__
#define __RAINMETER_H__

#include <windows.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include "Litestep.h"
#include "MeterWindow.h"

#define MAX_LINE_LENGTH 4096

#define APPNAME L"Rainmeter"
#ifdef _WIN64
#define APPBITS L"64-bit"
#else
#define APPBITS L"32-bit"
#endif
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define APPDATE WIDEN(__DATE__)

#define RAINMETER_CLASS_NAME	L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME	L"Rainmeter control window"

#define WM_RAINMETER_DELAYED_REFRESH_ALL WM_APP + 0
#define WM_RAINMETER_DELAYED_EXECUTE     WM_APP + 1
#define WM_RAINMETER_EXECUTE             WM_APP + 2

struct GlobalConfig
{
	double netInSpeed;
	double netOutSpeed;
};

class CConfigParser;
class CTrayWindow;

class CRainmeter
{
public:
	struct CONFIG 
	{
		std::wstring config;
		std::vector<std::wstring> iniFiles;
		UINT commandBase;
		int active;

		CONFIG() {}
		~CONFIG() {}

		CONFIG(CONFIG&& r) :
			config(std::move(r.config)),
			iniFiles(std::move(r.iniFiles)),
			commandBase(r.commandBase),
			active(r.active)
		{
		}

		CONFIG& operator=(CONFIG&& r)
		{
			config = std::move(r.config);
			iniFiles = std::move(r.iniFiles);
			commandBase = r.commandBase;
			active = r.active;
			return *this;
		}
	};

	struct CONFIGMENU
	{
		std::wstring name;
		size_t index;
		std::vector<CONFIGMENU> children;

		CONFIGMENU() {}
		~CONFIGMENU() {}

		CONFIGMENU(CONFIGMENU&& r) :
			name(std::move(r.name)),
			index(r.index),
			children(std::move(r.children))
		{
		}

		CONFIGMENU& operator=(CONFIGMENU&& r)
		{
			name = std::move(r.name);
			index = r.index;
			children = std::move(r.children);
			return *this;
		}
	};

	struct LOG_INFO
	{
		int level;
		std::wstring timestamp;
		std::wstring message;
	};

	CRainmeter();
	~CRainmeter();

	int Initialize(LPCWSTR szPath);
	int MessagePump();

	void SetNetworkStatisticsTimer();

	CConfigParser* GetCurrentParser() { return m_CurrentParser; }
	void SetCurrentParser(CConfigParser* parser) { m_CurrentParser = parser; }

	CTrayWindow* GetTrayWindow() { return m_TrayWindow; }

	CMeterWindow* GetMeterWindow(const std::wstring& config);
	CMeterWindow* GetMeterWindowByINI(const std::wstring& ini_searching);
	std::pair<int, int> GetMeterWindowIndex(const std::wstring& config, const std::wstring& iniFile);
	std::pair<int, int> GetMeterWindowIndex(CMeterWindow* meterWindow) { return GetMeterWindowIndex(meterWindow->GetSkinName(), meterWindow->GetSkinIniFile()); }
	std::pair<int, int> GetMeterWindowIndex(UINT menuCommand);

	CMeterWindow* GetMeterWindow(HWND hwnd);
	void GetMeterWindowsByLoadOrder(std::multimap<int, CMeterWindow*>& windows, const std::wstring& group = std::wstring());
	std::map<std::wstring, CMeterWindow*>& GetAllMeterWindows() { return m_MeterWindows; }
	const std::vector<CONFIG>& GetAllConfigs() { return m_ConfigStrings; }
	const std::vector<std::wstring>& GetAllThemes() { return m_Themes; }

	void DeleteMeterWindow(CMeterWindow* meterWindow, bool force = false);

	void ActivateConfig(int configIndex, int iniIndex);
	void DeactivateConfig(CMeterWindow* meterWindow, int configIndex, bool save = true);
	void ToggleConfig(int configIndex, int iniIndex);

	const std::wstring& GetPath() { return m_Path; }
	const std::wstring& GetIniFile() { return m_IniFile; }
	const std::wstring& GetDataFile() { return m_DataFile; }
	const std::wstring& GetLogFile() { return m_LogFile; }
	const std::wstring& GetSkinPath() { return m_SkinPath; }
	const std::wstring& GetPluginPath() { return m_PluginPath; }
	const std::wstring& GetAddonPath() { return m_AddonPath; }
	std::wstring GetSettingsPath() { return ExtractPath(m_IniFile); }

	const std::wstring& GetDrive() { return m_Drive; }

	const std::wstring& GetConfigEditor() { return m_ConfigEditor; }
	const std::wstring& GetLogViewer() { return m_LogViewer; }
	const std::wstring& GetStatsDate() { return m_StatsDate; }

	HWND GetWindow() { return m_Window; }

	HINSTANCE GetInstance() { return m_Instance; }
	HINSTANCE GetResourceInstance() { return m_ResourceInstance; }
	LCID GetResourceLCID() { return m_ResourceLCID; }

	bool GetDebug() { return m_Debug; }

	GlobalConfig& GetGlobalConfig() { return m_GlobalConfig; }

	void ReloadSettings();
	void EditSettings();
	void EditSkinFile(const std::wstring& name, const std::wstring& iniFile);
	void OpenSkinFolder(const std::wstring& name = std::wstring());

	void UpdateStats();
	void ReadStats();
	void WriteStats(bool bForce);
	void ResetStats();

	bool GetDisableVersionCheck() { return m_DisableVersionCheck; }
	void SetDisableVersionCheck(bool check);
	bool GetNewVersion() { return m_NewVersion; }
	void SetNewVersion() { m_NewVersion = true; }

	bool GetLogging() { return m_Logging; }
	void StartLogging();
	void StopLogging();
	void ShowLogFile();
	void DeleteLogFile();

	bool GetDisableRDP() { return m_DisableRDP; }
	bool IsRedrawable() { return (!GetDisableRDP() || !GetSystemMetrics(SM_REMOTESESSION)); }

	bool GetDisableDragging() { return m_DisableDragging; }
	void SetDisableDragging(bool dragging);

	bool IsNormalStayDesktop() { return m_NormalStayDesktop; }

	void AddAboutLogInfo(int level, LPCWSTR time, LPCWSTR message);
	const std::list<LOG_INFO>& GetAboutLogData() { return m_LogData; }

	void SetDebug(bool debug);

	bool IsMenuActive() { return m_MenuActive; }
	void ShowContextMenu(POINT pos, CMeterWindow* meterWindow);

	const std::wstring& GetTrayExecuteR() { return m_TrayExecuteR; }
	const std::wstring& GetTrayExecuteM() { return m_TrayExecuteM; }
	const std::wstring& GetTrayExecuteDR() { return m_TrayExecuteDR; }
	const std::wstring& GetTrayExecuteDM() { return m_TrayExecuteDM; }

	void ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow);
	void DelayedExecuteCommand(const WCHAR* command);

	void RefreshAll();

	void LoadTheme(const std::wstring& name);
	void PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace = true);

	static std::vector<std::wstring> ParseString(LPCTSTR str);
	static std::wstring ExtractPath(const std::wstring& strFilePath);
	static void ExpandEnvironmentVariables(std::wstring& strPath);

	friend class CDialogManage;

private:
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs, CMeterWindow* meterWindow);
	void BangGroupWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs, CMeterWindow* meterWindow);
	void Bang_ActivateConfig(const WCHAR* arg);
	void Bang_DeactivateConfig(const WCHAR* arg, CMeterWindow* meterWindow);
	void Bang_ToggleConfig(const WCHAR* arg);
	void Bang_DeactivateConfigGroup(const WCHAR* arg);
	void Bang_SetClip(const WCHAR* arg);
	void Bang_SetWallpaper(const WCHAR* arg);
	void Bang_SkinMenu(const WCHAR* arg, CMeterWindow* meterWindow);
	void Bang_TrayMenu();
	void Bang_WriteKeyValue(const WCHAR* arg, CMeterWindow* meterWindow);
	void Bang_Log(const WCHAR* arg);

	void ExecuteBang(const std::wstring& name, std::wstring& arg, CMeterWindow* meterWindow);

	void ActivateActiveConfigs();
	void CreateMeterWindow(const std::wstring& config, const std::wstring& iniFile);
	void WriteActive(const std::wstring& config, int iniIndex);
	void ScanForConfigs(const std::wstring& path);
	void ScanForThemes(const std::wstring& path);
	void ReadGeneralSettings(const std::wstring& iniFile);
	void SetLoadOrder(int configIndex, int order);
	int GetLoadOrder(const std::wstring& config);
	void UpdateDesktopWorkArea(bool reset);
	HMENU CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU configMenu);
	void ChangeSkinIndex(HMENU subMenu, int index);
	int ScanForConfigsRecursive(const std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu, bool DontRecurse);
	HMENU CreateConfigMenu(HMENU configMenu, const std::vector<CONFIGMENU>& configMenuData);
	void CreateThemeMenu(HMENU themeMenu);
	void CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow);
	void CreateDefaultConfigFile();
	void CreateDataFile();
	void SetLogging(bool logging);
	void TestSettingsFile(bool bDefaultIniLocation);

	CTrayWindow* m_TrayWindow;

	std::vector<CONFIG> m_ConfigStrings;				// All configs found in the given folder
	std::vector<CONFIGMENU> m_ConfigMenu;
	std::multimap<int, int> m_ConfigOrders;
	std::map<std::wstring, CMeterWindow*> m_MeterWindows;
	std::vector<std::wstring> m_Themes;

	std::wstring m_Path;
	std::wstring m_IniFile;
	std::wstring m_DataFile;
	std::wstring m_StatsFile;
	std::wstring m_LogFile;
	std::wstring m_SkinPath;
	std::wstring m_PluginPath;
	std::wstring m_AddonPath;

	std::wstring m_Drive;

	std::wstring m_StatsDate;					// The date when stats gathering started

	std::wstring m_TrayExecuteR;
	std::wstring m_TrayExecuteM;
	std::wstring m_TrayExecuteDR;
	std::wstring m_TrayExecuteDM;

	bool m_Debug;

	bool m_DisableVersionCheck;
	bool m_NewVersion;

	bool m_DesktopWorkAreaChanged;
	bool m_DesktopWorkAreaType;			// If true, DesktopWorkArea is treated as "margin"
	std::map<UINT, RECT> m_DesktopWorkAreas;
	std::vector<RECT> m_OldDesktopWorkAreas;

	bool m_NormalStayDesktop;

	bool m_MenuActive;

	bool m_DisableRDP;

	bool m_DisableDragging;

	bool m_Logging;

	std::list<LOG_INFO> m_LogData;

	std::wstring m_ConfigEditor;
	std::wstring m_LogViewer;

	CConfigParser* m_CurrentParser;

	HWND m_Window;

	HINSTANCE m_Instance;
	HMODULE m_ResourceInstance;
	LCID m_ResourceLCID;

	ULONG_PTR m_GDIplusToken;

	GlobalConfig m_GlobalConfig;
};

#ifdef LIBRARY_EXPORTS
#define EXPORT_PLUGIN EXTERN_C
#else
#define EXPORT_PLUGIN EXTERN_C __declspec(dllimport)
#endif

EXPORT_PLUGIN int RainmeterMain(LPWSTR cmdLine);

#endif
