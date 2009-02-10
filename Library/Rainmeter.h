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
/*
  $Header: /home/cvsroot/Rainmeter/Library/Rainmeter.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Rainmeter.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.19  2004/08/13 15:48:34  rainy
  no message

  Revision 1.18  2004/07/11 17:19:57  rainy
  Whole skin folder is recursively scanned.

  Revision 1.17  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.16  2004/03/13 16:20:41  rainy
  Improved multiconfig support

  Revision 1.15  2003/12/05 15:50:10  Rainy
  Multi-instance changes.

  Revision 1.14  2003/02/10 18:10:48  rainy
  Added support for the new bangs.

  Revision 1.13  2002/07/01 15:01:18  rainy
  Wharfdata is not used anymore.
  Added Toggle.

  Revision 1.12  2002/05/04 08:10:25  rainy
  Added interfaces for the new bangs.
  commandline is now stored in stl string.

  Revision 1.11  2002/04/27 10:27:13  rainy
  Added bangs to hide/show meters and measures

  Revision 1.10  2002/04/26 18:22:02  rainy
  Added possibility to hide the meter.

  Revision 1.9  2002/03/31 09:58:53  rainy
  Added some comments

  Revision 1.8  2002/01/16 16:05:25  rainy
  Version 0.6

  Revision 1.7  2001/12/23 10:11:32  rainy
  Refresh gets another parameter.

  Revision 1.6  2001/09/26 16:23:23  rainy
  Changed the version number

  Revision 1.5  2001/09/01 12:56:44  rainy
  Added refresh bang.

  Revision 1.4  2001/08/25 18:07:57  rainy
  Version is now 0.4.

  Revision 1.3  2001/08/25 17:06:24  rainy
  Changed few methods to inlines.
  Now the wharf data is stored fully.

  Revision 1.2  2001/08/19 09:03:28  rainy
  Added VERSION string.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

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
#ifdef _WIN64
#define APPVERSION L"0.14.1 (64-bit)"
#else
#define APPVERSION L"0.14.1 (32-bit)"
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

void BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, int numOfArgs);

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
	const std::wstring& GetSkinPath() { return m_SkinPath; };
	const std::wstring& GetPluginPath() { return m_PluginPath; };

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
	void WriteStats();
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
	static std::wstring FixPath(const std::wstring& path, PATH_FOLDER folder, const std::wstring& currentSkin);

	static PLATFORM IsNT();

private:
	void CreateMeterWindow(std::wstring config, std::wstring iniFile);
	bool DeleteMeterWindow(CMeterWindow* meterWindow);
	void ScanForConfigs(std::wstring& path);
	void ReadGeneralSettings(std::wstring& path);
	bool SetActiveConfig(std::wstring& skinName, std::wstring& skinIni);
	void Refresh(const WCHAR* arg);
	HMENU CreateSkinMenu(CMeterWindow* meterWindow, int index);
	void ChangeSkinIndex(HMENU subMenu, int index);
	int ScanForConfigsRecursive(std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu);
	HMENU CreateConfigMenu(std::vector<CONFIGMENU>& configMenuData);

	CTrayWindow* m_TrayWindow;

	std::vector<CONFIG> m_ConfigStrings;	    // All configs found in the given folder
	std::vector<CONFIGMENU> m_ConfigMenu;
	std::map<std::wstring, CMeterWindow*> m_Meters;			// The meter windows

	std::wstring m_Path;			    // Path to the main folder
	std::wstring m_IniFile;				// The main ini file
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
