/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __RAINMETER_H__
#define __RAINMETER_H__

#include <windows.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include "CommandHandler.h"
#include "ContextMenu.h"
#include "Logger.h"
#include "Skin.h"
#include "SkinRegistry.h"

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

class ConfigParser;
class TrayIcon;

class Rainmeter
{
public:
	static Rainmeter& GetInstance();

	int Initialize(LPCWSTR iniPath, LPCWSTR layout);
	void Finalize();

	bool IsAlreadyRunning();
	int MessagePump();

	void SetNetworkStatisticsTimer();

	ConfigParser* GetCurrentParser() { return m_CurrentParser; }
	void SetCurrentParser(ConfigParser* parser) { m_CurrentParser = parser; }

	TrayIcon* GetTrayIcon() { return m_TrayIcon; }

	bool HasSkin(const Skin* skin) const;

	Skin* GetSkin(std::wstring folderPath);
	Skin* GetSkinByINI(const std::wstring& ini_searching);

	Skin* GetSkin(HWND hwnd);
	void GetSkinsByLoadOrder(std::multimap<int, Skin*>& windows, const std::wstring& group = std::wstring());
	std::map<std::wstring, Skin*>& GetAllSkins() { return m_Skins; }

	const std::vector<std::wstring>& GetAllLayouts() { return m_Layouts; }

	void RemoveSkin(Skin* skin);
	void AddUnmanagedSkin(Skin* skin);
	void RemoveUnmanagedSkin(Skin* skin);

	bool ActivateSkin(const std::wstring& folderPath);
	bool ActivateSkin(const std::wstring& folderPath, const std::wstring& file);
	void ActivateSkin(int folderIndex, int fileIndex);
	void DeactivateSkin(Skin* skin, int folderIndex, bool save = true);
	void ToggleSkin(int folderIndex, int fileIndex);
	void ToggleSkinWithID(UINT id);

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

	HINSTANCE GetModuleInstance() { return m_Instance; }
	HINSTANCE GetResourceInstance() { return m_ResourceInstance; }
	LCID GetResourceLCID() { return m_ResourceLCID; }

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

	bool IsMenuActive() { return m_ContextMenu.IsMenuActive(); }
	void ShowContextMenu(POINT pos, Skin* skin) { return m_ContextMenu.ShowMenu(pos, skin); }
	void ShowSkinCustomContextMenu(POINT pos, Skin* skin) { return m_ContextMenu.ShowSkinCustomMenu(pos, skin); }

	const std::wstring& GetTrayExecuteR() { return m_TrayExecuteR; }
	const std::wstring& GetTrayExecuteM() { return m_TrayExecuteM; }
	const std::wstring& GetTrayExecuteDR() { return m_TrayExecuteDR; }
	const std::wstring& GetTrayExecuteDM() { return m_TrayExecuteDM; }

	void ExecuteBang(const WCHAR* bang, std::vector<std::wstring>& args, Skin* skin);
	void ExecuteCommand(const WCHAR* command, Skin* skin, bool multi = true);
	void DelayedExecuteCommand(const WCHAR* command, Skin* skin = nullptr);

	void RefreshAll();

	bool LoadLayout(const std::wstring& name);
	void PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace = true);

	bool IsSkinAFavorite(const std::wstring& folder, const std::wstring& filename);
	void UpdateFavorites(const std::wstring& folder, const std::wstring& file, bool favorite);

	Gdiplus::Color& GetDefaultSelectionColor() { return m_DefaultSelectedColor; }

	friend class CommandHandler;
	friend class ContextMenu;
	friend class DialogManage;
	friend class DialogNewSkin;

private:
	Rainmeter();
	~Rainmeter();

	Rainmeter(const Rainmeter& other) = delete;
	Rainmeter& operator=(Rainmeter other) = delete;

	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void ActivateActiveSkins();
	void CreateSkin(const std::wstring& folderPath, const std::wstring& file);
	void DeleteAllSkins();
	void DeleteAllUnmanagedSkins();
	void WriteActive(const std::wstring& folderPath, int fileIndex);
	void ScanForSkins();
	void ScanForLayouts();
	void ReadFavorites();
	void ReadGeneralSettings(const std::wstring& iniFile);
	void SetLoadOrder(int folderIndex, int order);
	int GetLoadOrder(const std::wstring& folderPath);
	void UpdateDesktopWorkArea(bool reset);

	void CreateOptionsFile();
	void CreateDataFile();
	void CreateComponentFolders(bool defaultIniLocation);
	void TestSettingsFile(bool bDefaultIniLocation);

	TrayIcon* m_TrayIcon;

	std::multimap<int, int> m_SkinOrders;
	std::map<std::wstring, Skin*> m_Skins;
	std::list<Skin*> m_UnmanagedSkins;
	std::vector<std::wstring> m_Layouts;
	std::vector<std::wstring> m_Favorites;

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

	bool m_Debug;

	bool m_DisableVersionCheck;
	bool m_NewVersion;

	bool m_DesktopWorkAreaChanged;
	bool m_DesktopWorkAreaType;
	std::map<UINT, RECT> m_DesktopWorkAreas;
	std::vector<RECT> m_OldDesktopWorkAreas;

	bool m_NormalStayDesktop;

	bool m_DisableRDP;

	bool m_DisableDragging;

	std::wstring m_SkinEditor;

	Gdiplus::Color m_DefaultSelectedColor;

	CommandHandler m_CommandHandler;
	ContextMenu m_ContextMenu;
	SkinRegistry m_SkinRegistry;

	ConfigParser* m_CurrentParser;

	HWND m_Window;

	HANDLE m_Mutex;
	HINSTANCE m_Instance;
	HMODULE m_ResourceInstance;
	LCID m_ResourceLCID;

	ULONG_PTR m_GDIplusToken;

	GlobalOptions m_GlobalOptions;
};

// Convenience function.
inline Rainmeter& GetRainmeter() { return Rainmeter::GetInstance(); }

#ifdef LIBRARY_EXPORTS
#define EXPORT_PLUGIN EXTERN_C
#else
#define EXPORT_PLUGIN EXTERN_C __declspec(dllimport)
#endif

EXPORT_PLUGIN int RainmeterMain(LPWSTR cmdLine);

#endif
