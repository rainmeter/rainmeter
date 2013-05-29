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
#include "Logger.h"
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

struct GlobalOptions
{
	double netInSpeed;
	double netOutSpeed;
};

class CConfigParser;
class CTrayWindow;

class CRainmeter
{
public:
	struct SkinFolder 
	{
		std::wstring name;
		std::vector<std::wstring> files;
		UINT commandBase;
		int16_t active;
		int16_t level;

		SkinFolder() {}
		~SkinFolder() {}

		SkinFolder(SkinFolder&& r) :
			name(std::move(r.name)),
			files(std::move(r.files)),
			commandBase(r.commandBase),
			active(r.active),
			level(r.level)
		{
		}

		SkinFolder& operator=(SkinFolder&& r)
		{
			name = std::move(r.name);
			files = std::move(r.files);
			commandBase = r.commandBase;
			active = r.active;
			level = r.level;
			return *this;
		}
	};

	CRainmeter();
	~CRainmeter();

	int Initialize(LPCWSTR iniPath, LPCWSTR layout);
	bool IsAlreadyRunning();
	int MessagePump();

	void SetNetworkStatisticsTimer();

	CConfigParser* GetCurrentParser() { return m_CurrentParser; }
	void SetCurrentParser(CConfigParser* parser) { m_CurrentParser = parser; }

	CTrayWindow* GetTrayWindow() { return m_TrayWindow; }

	bool HasMeterWindow(const CMeterWindow* meterWindow) const;

	CMeterWindow* GetMeterWindow(const std::wstring& folderPath);
	CMeterWindow* GetMeterWindowByINI(const std::wstring& ini_searching);
	std::pair<int, int> GetMeterWindowIndex(const std::wstring& folderPath, const std::wstring& file);
	std::pair<int, int> GetMeterWindowIndex(CMeterWindow* meterWindow) { return GetMeterWindowIndex(meterWindow->GetFolderPath(), meterWindow->GetFileName()); }
	std::pair<int, int> GetMeterWindowIndex(UINT menuCommand);

	CMeterWindow* GetMeterWindow(HWND hwnd);
	void GetMeterWindowsByLoadOrder(std::multimap<int, CMeterWindow*>& windows, const std::wstring& group = std::wstring());
	std::map<std::wstring, CMeterWindow*>& GetAllMeterWindows() { return m_MeterWindows; }

	std::wstring GetFolderPath(int folderIndex);
	int FindSkinFolderIndex(const std::wstring& folderPath);

	const std::vector<SkinFolder>& GetFolders() { return m_SkinFolders; }
	const std::vector<std::wstring>& GetAllLayouts() { return m_Layouts; }

	void RemoveMeterWindow(CMeterWindow* meterWindow);
	void AddUnmanagedMeterWindow(CMeterWindow* meterWindow);
	void RemoveUnmanagedMeterWindow(CMeterWindow* meterWindow);

	void ActivateSkin(int folderIndex, int fileIndex);
	void DeactivateSkin(CMeterWindow* meterWindow, int folderIndex, bool save = true);
	void ToggleSkin(int folderIndex, int fileIndex);

	const std::wstring& GetPath() { return m_Path; }
	const std::wstring& GetIniFile() { return m_IniFile; }
	const std::wstring& GetDataFile() { return m_DataFile; }
	const std::wstring& GetSettingsPath() { return m_SettingsPath; }
	const std::wstring& GetSkinPath() { return m_SkinPath; }
	void SetSkinPath(const std::wstring& skinPath);
	std::wstring GetLayoutPath() { return m_SettingsPath + L"Layouts\\"; }
	std::wstring GetPluginPath() { return m_Path + L"Plugins\\"; }
	std::wstring GetUserPluginPath() { return m_SettingsPath + L"Plugins\\"; }
	std::wstring GetAddonPath() { return m_SettingsPath + L"Addons\\"; }

	bool HasUserPluginPath() { return (_wcsicmp(m_Path.c_str(), m_SettingsPath.c_str()) != 0); }

	std::wstring GetDefaultSkinPath() { return m_Path + L"Defaults\\Skins\\"; }
	std::wstring GetDefaultLayoutPath() { return m_Path + L"Defaults\\Layouts\\"; }
	std::wstring GetDefaultPluginPath() { return m_Path + L"Defaults\\Plugins\\"; }
	std::wstring GetDefaultAddonPath() { return m_Path + L"Defaults\\Addons\\"; }

	const std::wstring& GetDrive() { return m_Drive; }

	const std::wstring& GetSkinEditor() { return m_SkinEditor; }
	void SetSkinEditor(const std::wstring& path);
	const std::wstring& GetStatsDate() { return m_StatsDate; }

	HWND GetWindow() { return m_Window; }

	HINSTANCE GetInstance() { return m_Instance; }
	HINSTANCE GetResourceInstance() { return m_ResourceInstance; }
	LCID GetResourceLCID() { return m_ResourceLCID; }

	bool CanUseD2D() const { return m_UseD2D; }

	bool GetDebug() { return m_Debug; }

	GlobalOptions& GetGlobalOptions() { return m_GlobalOptions; }

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

	void ShowLogFile();

	bool GetDisableRDP() { return m_DisableRDP; }
	bool IsRedrawable() { return (!GetDisableRDP() || !GetSystemMetrics(SM_REMOTESESSION)); }

	bool GetDisableDragging() { return m_DisableDragging; }
	void SetDisableDragging(bool dragging);

	bool IsNormalStayDesktop() { return m_NormalStayDesktop; }

	void SetDebug(bool debug);

	int ShowMessage(HWND parent, const WCHAR* text, UINT type);

	bool IsMenuActive() { return m_MenuActive; }
	void ShowContextMenu(POINT pos, CMeterWindow* meterWindow);

	const std::wstring& GetTrayExecuteR() { return m_TrayExecuteR; }
	const std::wstring& GetTrayExecuteM() { return m_TrayExecuteM; }
	const std::wstring& GetTrayExecuteDR() { return m_TrayExecuteDR; }
	const std::wstring& GetTrayExecuteDM() { return m_TrayExecuteDM; }

	void ExecuteBang(const WCHAR* bang, std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow, bool multi = true);
	void DelayedExecuteCommand(const WCHAR* command);

	void RefreshAll();

	bool LoadLayout(const std::wstring& name);
	void PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace = true);

	static std::vector<std::wstring> ParseString(LPCTSTR str, CConfigParser* parser = NULL);
	static std::wstring ExtractPath(const std::wstring& strFilePath);
	static void ExpandEnvironmentVariables(std::wstring& strPath);

	friend class CDialogManage;

private:
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void BangWithArgs(BANGCOMMAND bang, std::vector<std::wstring>& args, size_t numOfArgs, CMeterWindow* meterWindow);
	void BangGroupWithArgs(BANGCOMMAND bang, std::vector<std::wstring>& args, size_t numOfArgs, CMeterWindow* meterWindow);
	void Bang_ActivateSkin(std::vector<std::wstring>& args);
	void Bang_DeactivateSkin(std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void Bang_ToggleSkin(std::vector<std::wstring>& args);
	void Bang_DeactivateSkinGroup(std::vector<std::wstring>& args);
	void Bang_LoadLayout(std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void Bang_SetClip(std::vector<std::wstring>& args);
	void Bang_SetWallpaper(std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void Bang_SkinMenu(std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void Bang_TrayMenu();
	void Bang_WriteKeyValue(std::vector<std::wstring>& args, CMeterWindow* meterWindow);
	void Bang_Log(std::vector<std::wstring>& args);

	void ActivateActiveSkins();
	void CreateMeterWindow(const std::wstring& folderPath, const std::wstring& file);
	void DeleteAllMeterWindows();
	void DeleteAllUnmanagedMeterWindows();
	void WriteActive(const std::wstring& folderPath, int fileIndex);
	void ScanForSkins();
	void ScanForLayouts();
	void ReadGeneralSettings(const std::wstring& iniFile);
	void SetLoadOrder(int folderIndex, int order);
	int GetLoadOrder(const std::wstring& folderPath);
	void UpdateDesktopWorkArea(bool reset);
	HMENU CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU menu);
	void ChangeSkinIndex(HMENU subMenu, int index);
	int ScanForSkinsRecursive(const std::wstring& path, std::wstring base, int index, UINT level);
	
	void CreateAllSkinsMenu(HMENU skinMenu) { CreateAllSkinsMenuRecursive(skinMenu, 0); }
	int CreateAllSkinsMenuRecursive(HMENU skinMenu, int index);

	void CreateLayoutMenu(HMENU layoutMenu);
	void CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow);
	void CreateOptionsFile();
	void CreateDataFile();
	void CreateComponentFolders(bool defaultIniLocation);
	void TestSettingsFile(bool bDefaultIniLocation);

	CTrayWindow* m_TrayWindow;

	std::vector<SkinFolder> m_SkinFolders;
	std::multimap<int, int> m_SkinOrders;
	std::map<std::wstring, CMeterWindow*> m_MeterWindows;
	std::list<CMeterWindow*> m_UnmanagedMeterWindows;
	std::vector<std::wstring> m_Layouts;

	std::wstring m_Path;
	std::wstring m_IniFile;
	std::wstring m_DataFile;
	std::wstring m_StatsFile;
	std::wstring m_SettingsPath;
	std::wstring m_SkinPath;

	std::wstring m_Drive;

	std::wstring m_StatsDate;

	std::wstring m_TrayExecuteR;
	std::wstring m_TrayExecuteM;
	std::wstring m_TrayExecuteDR;
	std::wstring m_TrayExecuteDM;

	bool m_UseD2D;

	bool m_Debug;

	bool m_DisableVersionCheck;
	bool m_NewVersion;

	bool m_DesktopWorkAreaChanged;
	bool m_DesktopWorkAreaType;
	std::map<UINT, RECT> m_DesktopWorkAreas;
	std::vector<RECT> m_OldDesktopWorkAreas;

	bool m_NormalStayDesktop;

	bool m_MenuActive;

	bool m_DisableRDP;

	bool m_DisableDragging;

	std::wstring m_SkinEditor;

	CConfigParser* m_CurrentParser;

	HWND m_Window;

	HANDLE m_Mutex;
	HINSTANCE m_Instance;
	HMODULE m_ResourceInstance;
	LCID m_ResourceLCID;

	ULONG_PTR m_GDIplusToken;

	GlobalOptions m_GlobalOptions;
};

#ifdef LIBRARY_EXPORTS
#define EXPORT_PLUGIN EXTERN_C
#else
#define EXPORT_PLUGIN EXTERN_C __declspec(dllimport)
#endif

EXPORT_PLUGIN int RainmeterMain(LPWSTR cmdLine);

#endif
