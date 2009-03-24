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

#pragma warning(disable: 4996)
#pragma warning(disable: 4786)

#include "Rainmeter.h"
#include "Error.h"
#include "AboutDialog.h"
#include "MeasureNet.h"
#include "Resource.h"
#include "UpdateCheck.h"
#include <assert.h>
#include <time.h>
#include <commctrl.h>
#include <gdiplus.h>

using namespace Gdiplus;

CRainmeter* Rainmeter; // The module

bool CRainmeter::c_DummyLitestep=false;
std::wstring CRainmeter::c_CmdLine;

/*
** ParseString
**
** Splits the given string into substrings
**
*/
std::vector<std::wstring> ParseString(LPCTSTR str)
{
	std::vector<std::wstring> result;
	if (str)
	{
		std::wstring arg = str;

		// Split the argument between first space.
		// Or if string is in quotes, the after the second quote.

		size_t quotePos = arg.find(L"\"");
		size_t spacePos = arg.find(L" ");
		while (quotePos != std::wstring::npos || spacePos != std::wstring::npos)
		{
			size_t endPos = 0;
			std::wstring newStr;

			if (spacePos == std::wstring::npos) spacePos = arg.size() - 1;

			if (quotePos == 0)
			{
				arg.erase(0, 1);	// Eat the quote

				// Find the second quote
				quotePos = arg.find(L"\"");
				endPos = quotePos;
				newStr = arg.substr(0, endPos); 
				arg.erase(0, endPos + 1);
			}
			else
			{
				endPos = spacePos;
				newStr = arg.substr(0, endPos); 
				arg.erase(0, endPos + 1);
			}

			if (newStr.size() > 0)
			{
				result.push_back(newStr);
			}

			quotePos = arg.find(L"\"");
			spacePos = arg.find(L" ");
		}

		if (arg.size() > 0)
		{
			result.push_back(arg);
		}

		// Strip the quotes from all strings
		for (size_t i = 0; i < result.size(); i++)
		{
			size_t pos = result[i].find(L"\"");
			while (pos != std::wstring::npos)
			{
				result[i].erase(result[i].begin() + pos, result[i].begin() + pos + 1);
				pos = result[i].find(L"\"");
			}
		}
	}

	return result;
}
	
/*
** initModuleEx
**
** This is called when the plugin is initialized
**
*/
int initModuleEx(HWND ParentWnd, HINSTANCE dllInst, LPCSTR szPath)
{
	int Result=1;
	
	try 
	{
		Rainmeter=new CRainmeter;

		if(Rainmeter) 
		{
			Result=Rainmeter->Initialize(ParentWnd, dllInst, szPath);
		}

	} 
    catch(CError& error) 
    {
		MessageBox(ParentWnd, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
	}

	return Result;
}

/*
** quitModule
**
** This is called when the plugin quits.
**
*/
void quitModule(HINSTANCE dllInst)
{
	if(Rainmeter) 
	{
		Rainmeter->Quit(dllInst);
		delete Rainmeter;
		Rainmeter = NULL;
	}
}

/*
** Initialize
**
** Init Rainmeter
**
*/
void Initialize(bool DummyLS, LPCTSTR CmdLine)
{
	CRainmeter::SetDummyLitestep(DummyLS);
	CRainmeter::SetCommandLine(CmdLine);
}


/*
** ReadConfigString
**
** Reads a config string. Used by the plugins.
**
*/
LPCTSTR ReadConfigString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue)
{
	if (Rainmeter) 
	{
		CConfigParser* parser = Rainmeter->GetCurrentParser();
		if (parser)
		{
			return parser->ReadString(section, key, defValue).c_str();
		}
	}	
	return NULL;
}

/*
** BangWithArgs
**
** Parses Bang args
**
*/
void BangWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs)
{
	if(Rainmeter) 
	{
		std::vector<std::wstring> subStrings = ParseString(arg);
		std::wstring config;
		std::wstring argument;

		// Don't include the config name from the arg if there is one
		for (size_t i = 0; i < numOfArgs; i++)
		{
			if (i != 0) argument += L" ";
			if (i < subStrings.size())
			{
				argument += subStrings[i];
			}
		}

		if (subStrings.size() >= numOfArgs)
		{
			if (subStrings.size() > numOfArgs)
			{
				config = subStrings[numOfArgs];
			}

			if ((!config.empty()) && (config != L"*"))
			{
				// Config defined, so bang only that
				CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(config);

				if (meterWindow)
				{
					if (bang == BANG_LSHOOK)
					{
						// LsHook is a special case
						meterWindow->RunBang(bang, arg);
					}
					else
					{
						meterWindow->RunBang(bang, argument.c_str());
					}
				}
				else
				{
					std::wstring dbg;
					dbg = L"Unknown config name: " + config;
					LSLog(LOG_DEBUG, APPNAME, dbg.c_str());
				}
			}
			else
			{
				// No config defined -> apply to all.
				std::map<std::wstring, CMeterWindow*>::iterator iter = Rainmeter->GetAllMeterWindows().begin();

				for (; iter != Rainmeter->GetAllMeterWindows().end(); iter++)
				{
					((*iter).second)->RunBang(bang, argument.c_str());
				}
			}
		}
		else
		{
			DebugLog(L"Incorrect number of arguments for the bang!");
		}
	}
}


/*
** RainmeterHide
**
** Callback for the !RainmeterHide bang
**
*/
void RainmeterHide(HWND, const char* arg)
{
	BangWithArgs(BANG_HIDE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterShow
**
** Callback for the !RainmeterShow bang
**
*/
void RainmeterShow(HWND, const char* arg)
{
	BangWithArgs(BANG_SHOW, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterToggle
**
** Callback for the !RainmeterToggle bang
**
*/
void RainmeterToggle(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterHideMeter
**
** Callback for the !RainmeterHideMeter bang
**
*/
void RainmeterHideMeter(HWND, const char* arg)
{
	BangWithArgs(BANG_HIDEMETER, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterShowMeter
**
** Callback for the !RainmeterShowMeter bang
**
*/
void RainmeterShowMeter(HWND, const char* arg)
{
	BangWithArgs(BANG_SHOWMETER, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterToggleMeter
**
** Callback for the !RainmeterToggleMeter bang
**
*/
void RainmeterToggleMeter(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLEMETER, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterHideMeasure
**
** Callback for the !RainmeterHideMeasure bang
**
*/
void RainmeterDisableMeasure(HWND, const char* arg)
{
	BangWithArgs(BANG_DISABLEMEASURE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterShowMeasure
**
** Callback for the !RainmeterShowMeasure bang
**
*/
void RainmeterEnableMeasure(HWND, const char* arg)
{
	BangWithArgs(BANG_ENABLEMEASURE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterToggleMeasure
**
** Callback for the !RainmeterToggleMeasure bang
**
*/
void RainmeterToggleMeasure(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLEMEASURE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterRefresh
**
** Callback for the !RainmeterRefresh bang
**
*/
void RainmeterRefresh(HWND, const char* arg)
{
	BangWithArgs(BANG_REFRESH, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterRedraw
**
** Callback for the !RainmeterRedraw bang
**
*/
void RainmeterRedraw(HWND, const char* arg)
{
	BangWithArgs(BANG_REDRAW, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterActivateConfig
**
** Callback for the !RainmeterActivateConfig bang
**
*/
void RainmeterActivateConfig(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 1)
		{
			const std::vector<CRainmeter::CONFIG>& configs = Rainmeter->GetAllConfigs();

			for (size_t i = 0; i < configs.size(); i++)
			{
				if (wcsnicmp(configs[i].config.c_str(), subStrings[0].c_str(), configs[i].config.length()) == 0)
				{
					for (size_t j = 0; j < configs[i].iniFiles.size(); j++)
					{
						if (wcsnicmp(configs[i].iniFiles[j].c_str(), subStrings[1].c_str(), configs[i].iniFiles[j].length()) == 0)
						{
							Rainmeter->ActivateConfig(i, j);
							return;
						}
					}
				}
			}
			DebugLog(L"No such config: \"%s\" \"%s\"", subStrings[0].c_str(), subStrings[1].c_str());
		}
		else
		{
			// If we got this far, something went wrong
			DebugLog(L"Cannot parse parameters for !RainmeterActivateConfig");
		}
	}
}

/*
** RainmeterDeactivateConfig
**
** Callback for the !RainmeterDeactivateConfig bang
**
*/
void RainmeterDeactivateConfig(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 0)
		{
			std::map<std::wstring, CMeterWindow*>::iterator iter = Rainmeter->GetAllMeterWindows().begin();
			for (; iter != Rainmeter->GetAllMeterWindows().end(); iter++)
			{
				CMeterWindow* mw = ((*iter).second);
				if (wcsicmp(subStrings[0].c_str(), mw->GetSkinName().c_str()) == 0)
				{
					Rainmeter->DeactivateConfig(mw, -1);		// -1 = Deactivate all ini-files
					return;
				}
			}
			DebugLog(L"The config is not active: \"%s\"", subStrings[0].c_str());
		}
		else
		{
			DebugLog(L"Unable to parse the arguments for !RainmeterDeactivateConfig");
		}
	}
}

/*
** RainmeterToggleConfig
**
** Callback for the !RainmeterToggleConfig bang
**
*/
void RainmeterToggleConfig(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() >= 2)
		{
			std::map<std::wstring, CMeterWindow*>::iterator iter = Rainmeter->GetAllMeterWindows().begin();
			for (; iter != Rainmeter->GetAllMeterWindows().end(); iter++)
			{
				CMeterWindow* mw = ((*iter).second);
				if (wcsicmp(subStrings[0].c_str(), mw->GetSkinName().c_str()) == 0)
				{
					Rainmeter->DeactivateConfig(mw, -1);		// -1 = Deactivate all ini-files
					return;
				}
			}

			// If the config wasn't active, activate it
			RainmeterActivateConfig(NULL, arg);
		}
		else
		{
			DebugLog(L"Unable to parse the arguments for !RainmeterToggleConfig");
		}
	}
}

/*
** RainmeterMove
**
** Callback for the !RainmeterMove bang
**
*/
void RainmeterMove(HWND, const char* arg)
{
	BangWithArgs(BANG_MOVE, ConvertToWide(arg).c_str(), 2);
}

/*
** RainmeterZPos
**
** Callback for the !RainmeterZPos bang
**
*/
void RainmeterZPos(HWND, const char* arg)
{
	BangWithArgs(BANG_ZPOS, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterLsHook
**
** Callback for the !RainmeterLsHook bang
**
*/
void RainmeterLsHook(HWND, const char* arg)
{
	BangWithArgs(BANG_LSHOOK, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterAbout
**
** Callback for the !RainmeterAbout bang
**
*/
void RainmeterAbout(HWND, const char* arg)
{
	BangWithArgs(BANG_ABOUT, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterResetStats
**
** Callback for the !RainmeterResetStats bang
**
*/
void RainmeterResetStats(HWND, const char* arg)
{
	if (Rainmeter) 
	{
		Rainmeter->ResetStats();
	}
}

/*
** RainmeterMoveMeter
**
** Callback for the !RainmeterMoveMeter bang
**
*/
void RainmeterMoveMeter(HWND, const char* arg)
{
	BangWithArgs(BANG_MOVEMETER, ConvertToWide(arg).c_str(), 3);
}

/*
** RainmeterPluginBang
**
** Callback for the !RainmeterPluginBang bang
**
*/
void RainmeterPluginBang(HWND, const char* arg)
{
	BangWithArgs(BANG_PLUGIN, ConvertToWide(arg).c_str(), 1);
}

// -----------------------------------------------------------------------------------------------
//
//                                The class starts here
//
// -----------------------------------------------------------------------------------------------

GlobalConfig CRainmeter::c_GlobalConfig;

/* 
** CRainmeter
**
** Constructor
**
*/
CRainmeter::CRainmeter()
{
	c_GlobalConfig.netInSpeed = 0;
	c_GlobalConfig.netOutSpeed = 0;

	m_DesktopWorkAreaChanged = false;
	m_DesktopWorkArea.left = m_DesktopWorkArea.top = m_DesktopWorkArea.right = m_DesktopWorkArea.bottom = 0;

	m_CheckUpdate = FALSE;

	m_Instance = NULL;
	m_CurrentParser = NULL;

	m_TrayWindow = NULL;

	m_ConfigEditor = L"Notepad";

	INITCOMMONCONTROLSEX initCtrls;
	initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initCtrls.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&initCtrls);

    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_GDIplusToken, &gdiplusStartupInput, NULL);
}

/* 
** ~CRainmeter
**
** Destructor
**
*/
CRainmeter::~CRainmeter()
{
	// Change the work area back
	if (m_DesktopWorkAreaChanged)
	{
		SystemParametersInfo(SPI_SETWORKAREA, 0, &m_DesktopWorkArea, 0);
	}

	while (m_Meters.size() > 0)
	{
		DeleteMeterWindow((*m_Meters.begin()).second);	// This removes the window from the vector
	}

	if (m_TrayWindow) delete m_TrayWindow;

	WriteStats();

	GdiplusShutdown(m_GDIplusToken);
}

/* 
** Initialize
**
** The main initialization function for the module.
** May throw CErrors !!!!
**
*/
int CRainmeter::Initialize(HWND Parent, HINSTANCE Instance, LPCSTR szPath)
{
	int Result=0;

	if(Parent==NULL || Instance==NULL) 
	{
		throw CError(CError::ERROR_NULL_PARAMETER, __LINE__, __FILE__);
	}	

	m_Instance = Instance;
	WCHAR tmpName[MAX_LINE_LENGTH];
	GetModuleFileName(m_Instance, tmpName, MAX_LINE_LENGTH);

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(tmpName, L'\\');
	if(pos) 
	{
		*(pos + 1)='\0';
	} 
	else 
	{
		tmpName[0]='\0';
	}

	if(!c_DummyLitestep) InitalizeLitestep();

	m_Path = tmpName;
	m_PluginPath = tmpName;
	m_PluginPath += L"Plugins\\";
	m_IniFile = m_Path + L"Rainmeter.ini";
	m_SkinPath = m_Path + L"Skins\\";

	if (!c_DummyLitestep)
	{
		char tmpSz[MAX_LINE_LENGTH];

		// Check if step.rc has overrides these values
		if (GetRCString("RainmeterIniFile", tmpSz, NULL, MAX_LINE_LENGTH - 1))
		{
			m_IniFile = ConvertToWide(tmpSz);
		}

		if (GetRCString("RainmeterSkinPath", tmpSz, NULL, MAX_LINE_LENGTH - 1))
		{
			m_SkinPath = ConvertToWide(tmpSz);
		}

		if (GetRCString("RainmeterPluginPath", tmpSz, NULL, MAX_LINE_LENGTH - 1))
		{
			m_PluginPath = ConvertToWide(tmpSz);
		}

		if (!m_SkinPath.empty() && m_SkinPath[m_SkinPath.size() - 1] != L'\\')
		{
			m_SkinPath += L"\\";
		}
	}

	DebugLog(L"Path: %s", m_Path.c_str());
	DebugLog(L"IniFile: %s", m_IniFile.c_str());
	DebugLog(L"SkinPath: %s", m_SkinPath.c_str());
	DebugLog(L"PluginPath: %s", m_PluginPath.c_str());

	// Tray must exist before configs are read
	m_TrayWindow = new CTrayWindow(m_Instance);

	ScanForConfigs(m_SkinPath);
	ReadGeneralSettings(m_IniFile);

	if (m_CheckUpdate)
	{
		CheckUpdate();
	}

	ResetStats();
	ReadStats();

	if (_waccess(m_IniFile.c_str(), 0) == -1)
	{
		m_TrayWindow->ShowBalloonHelp();
	}

	// Change the work area if necessary
	if (m_DesktopWorkAreaChanged)
	{
		RECT rc;
		rc = m_DesktopWorkArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &m_DesktopWorkArea, 0);	// Store the old value
		SystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0);
	}

	// If we're running as Litestep's plugin, register the !bangs
	if(!c_DummyLitestep) 
	{
		int Msgs[] = { LM_GETREVID, 0 };
		// Register RevID message to Litestep
		if (m_TrayWindow && m_TrayWindow->GetWindow()) ::SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)m_TrayWindow->GetWindow(), (LPARAM)Msgs);

		AddBangCommand("!RainmeterRefresh", RainmeterRefresh);
		AddBangCommand("!RainmeterRedraw", RainmeterRedraw);
		AddBangCommand("!RainmeterHide", RainmeterHide);
		AddBangCommand("!RainmeterShow", RainmeterShow);
		AddBangCommand("!RainmeterToggle", RainmeterToggle);
		AddBangCommand("!RainmeterHideMeter", RainmeterHideMeter);
		AddBangCommand("!RainmeterShowMeter", RainmeterShowMeter);
		AddBangCommand("!RainmeterToggleMeter", RainmeterToggleMeter);
		AddBangCommand("!RainmeterDisableMeasure", RainmeterDisableMeasure);
		AddBangCommand("!RainmeterEnableMeasure", RainmeterEnableMeasure);
		AddBangCommand("!RainmeterToggleMeasure", RainmeterToggleMeasure);
		AddBangCommand("!RainmeterActivateConfig", RainmeterActivateConfig);
		AddBangCommand("!RainmeterToggleConfig", RainmeterToggleConfig);
		AddBangCommand("!RainmeterDeactivateConfig", RainmeterDeactivateConfig);
		AddBangCommand("!RainmeterMove", RainmeterMove);
		AddBangCommand("!RainmeterZPos", RainmeterZPos);
		AddBangCommand("!RainmeterLsBoxHook", RainmeterLsHook);
		AddBangCommand("!RainmeterAbout", RainmeterAbout);
		AddBangCommand("!RainmeterResetStats", RainmeterResetStats);
		AddBangCommand("!RainmeterMoveMeter", RainmeterMoveMeter);
		AddBangCommand("!RainmeterPluginBang", RainmeterPluginBang);
	}

	// Create meter windows for active configs
	for (size_t i = 0; i < m_ConfigStrings.size(); i++)
	{
		if (m_ConfigStrings[i].active > 0 && m_ConfigStrings[i].active <= m_ConfigStrings[i].iniFiles.size())
		{
			ActivateConfig(i, m_ConfigStrings[i].active - 1);
		}
	}

	return Result;	// Alles OK
}

void CRainmeter::ReloadSettings()
{
	ScanForConfigs(m_SkinPath);
	ReadGeneralSettings(m_IniFile);
}

void CRainmeter::ActivateConfig(int configIndex, int iniIndex)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size())
	{
		WCHAR buffer[256];
		std::wstring skinIniFile = m_ConfigStrings[configIndex].iniFiles[iniIndex];
		std::wstring skinConfig = m_ConfigStrings[configIndex].config;

		// Verify that the config is not already active
		std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.find(skinConfig);
		if (iter != m_Meters.end())
		{
			if (((*iter).second)->GetSkinIniFile() == skinIniFile)
			{
				DebugLog(L"MeterWindow \"%s\" is already active.", skinConfig.c_str());
				return;
			}
			else
			{
				// Deactivate the existing config
				DeactivateConfig((*iter).second, configIndex);
			}
		}

		try 
		{
			m_ConfigStrings[configIndex].active = iniIndex + 1;

			CreateMeterWindow(skinConfig, skinIniFile);

			wsprintf(buffer, L"%i", iniIndex + 1);
			WritePrivateProfileString(skinConfig.c_str(), L"Active", buffer, m_IniFile.c_str());
		} 
		catch(CError& error) 
		{
			MessageBox(NULL, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}
}

bool CRainmeter::DeactivateConfig(CMeterWindow* meterWindow, int configIndex)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size())
	{
		m_ConfigStrings[configIndex].active = 0;	// Deactivate the config
	}
	else
	{
		// Deactive all
		for(size_t i = 0; i < m_ConfigStrings.size(); i++)
		{
			m_ConfigStrings[i].active = 0;
		}
	}

	if (meterWindow)
	{
		// Disable the config in the ini-file
		WritePrivateProfileString(meterWindow->GetSkinName().c_str(), L"Active", L"0", m_IniFile.c_str());

		return DeleteMeterWindow(meterWindow);
	}
	return false;
}

void CRainmeter::CreateMeterWindow(std::wstring config, std::wstring iniFile)
{
	CMeterWindow* mw = new CMeterWindow(config, iniFile);

	if (mw)
	{
		m_Meters[config] = mw;
		mw->Initialize(*this);
	}
}

bool CRainmeter::DeleteMeterWindow(CMeterWindow* meterWindow)
{
	std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.begin();

	for (; iter != m_Meters.end(); iter++)
	{
		if (meterWindow == NULL)
		{
			// Delete all meter windows
			delete (*iter).second;
		} 
		else if ((*iter).second == meterWindow)
		{
			delete meterWindow;
			m_Meters.erase(iter);
			return true;
		}
	}

	if (meterWindow == NULL)
	{
		m_Meters.clear();
	}

	return false;
}

CMeterWindow* CRainmeter::GetMeterWindow(const std::wstring& config)
{
	std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.begin();

	for (; iter != m_Meters.end(); iter++)
	{
		if ((*iter).first == config)
		{
			return (*iter).second;
		}
	}

	return NULL;
}
/* 
** Quit
**
** Called when the module quits
**
*/
void CRainmeter::Quit(HINSTANCE dllInst)
{
	// If we're running as Litestep's plugin, unregister the !bangs
	if(!c_DummyLitestep) 
	{
		int Msgs[] = { LM_GETREVID, 0 };
		// Unregister RevID message
		if (m_TrayWindow && m_TrayWindow->GetWindow()) ::SendMessage(GetLitestepWnd(), LM_UNREGISTERMESSAGE, (WPARAM)m_TrayWindow->GetWindow(), (LPARAM)Msgs);

		RemoveBangCommand("!RainmeterRefresh");
		RemoveBangCommand("!RainmeterRedraw");
		RemoveBangCommand("!RainmeterHide");
		RemoveBangCommand("!RainmeterShow");
		RemoveBangCommand("!RainmeterToggle");
		RemoveBangCommand("!RainmeterHideMeter");
		RemoveBangCommand("!RainmeterShowMeter");
		RemoveBangCommand("!RainmeterToggleMeter");
		RemoveBangCommand("!RainmeterHideMeasure");
		RemoveBangCommand("!RainmeterShowMeasure");
		RemoveBangCommand("!RainmeterToggleMeasure");
		RemoveBangCommand("!RainmeterActivateConfig");
		RemoveBangCommand("!RainmeterDeactivateConfig");
		RemoveBangCommand("!RainmeterToggleConfig");
		RemoveBangCommand("!RainmeterMove");
		RemoveBangCommand("!RainmeterZPos");
		RemoveBangCommand("!RainmeterLsBoxHook");
		RemoveBangCommand("!RainmeterAbout");
		RemoveBangCommand("!RainmeterResetStats");
		RemoveBangCommand("!RainmeterMoveMeter");
		RemoveBangCommand("!RainmeterPluginBang");
	}
}

/* 
** ScanForConfigs
**
** Scans all the subfolders and locates the ini-files.
*/
void CRainmeter::ScanForConfigs(std::wstring& path)
{
	m_ConfigStrings.clear();
	m_ConfigMenu.clear();
	
	ScanForConfigsRecursive(path, L"", 0, m_ConfigMenu);

	if(m_ConfigStrings.empty())
	{
		std::wstring error = L"There are no skins available in folder\n" + path;
		MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
	}
}

int CRainmeter::ScanForConfigsRecursive(std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu)
{
    WIN32_FIND_DATA fileData;      // Data structure describes the file found
    WIN32_FIND_DATA fileDataIni;   // Data structure describes the file found
    HANDLE hSearch;                // Search handle returned by FindFirstFile
    HANDLE hSearchIni;             // Search handle returned by FindFirstFile

	if (!base.empty())
	{
		// Scan for ini-files
		CONFIG config;
		config.config = base;
		config.active = false;

		// Scan all .ini files from the subfolder
		std::wstring inis = path;
		inis += base;
		inis += L"\\*.ini";
		hSearchIni = FindFirstFile(inis.c_str(), &fileDataIni);

		do
		{
			if(hSearchIni == INVALID_HANDLE_VALUE) break;    // No more files found

			CONFIGMENU menuItem;
			menuItem.name = fileDataIni.cFileName;
			menuItem.index = m_ConfigStrings.size();
			menu.push_back(menuItem);

			config.iniFiles.push_back(fileDataIni.cFileName);
			config.commands.push_back(ID_CONFIG_FIRST + index++);

		} while (FindNextFile(hSearchIni, &fileDataIni));

		if (!config.iniFiles.empty())
		{
			m_ConfigStrings.push_back(config);
		}
		FindClose(hSearchIni);

		base += L"\\";
	}

	// Scan for folders
	std::wstring files = path + base + L"*";
    hSearch = FindFirstFile(files.c_str(), &fileData);
	do
	{
		if(hSearch == INVALID_HANDLE_VALUE) break;    // No more files found

		if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
			wcscmp(L".", fileData.cFileName) != 0 &&
			wcscmp(L"..", fileData.cFileName) != 0)
		{
			CONFIGMENU menuItem;
			menuItem.name = fileData.cFileName;
			menuItem.index = -1;
			menu.push_back(menuItem);

			std::vector<CONFIGMENU>::iterator iter = menu.end() - 1;
			index = ScanForConfigsRecursive(path, base + fileData.cFileName, index, (*iter).children);
		}
	} while(FindNextFile(hSearch, &fileData));

    FindClose(hSearch);

	return index;
}

void CRainmeter::SaveSettings()
{
	// Just one setting for writing at the moment
	WritePrivateProfileString(L"Rainmeter", L"CheckUpdate", m_CheckUpdate ? L"1" : L"0" , m_IniFile.c_str());
}

BOOL CRainmeter::ExecuteBang(const std::wstring& bang, const std::wstring& arg, CMeterWindow* meterWindow)
{
	if (wcsicmp(bang.c_str(), L"!RainmeterRefresh") == 0)
	{
		BangWithArgs(BANG_REFRESH, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterRedraw") == 0)
	{
		BangWithArgs(BANG_REDRAW, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterHide") == 0)
	{
		BangWithArgs(BANG_HIDE, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterShow") == 0)
	{
		BangWithArgs(BANG_SHOW, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterToggle") == 0)
	{
		BangWithArgs(BANG_TOGGLE, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterHideMeter") == 0)
	{
		BangWithArgs(BANG_HIDEMETER, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterShowMeter") == 0)
	{
		BangWithArgs(BANG_SHOWMETER, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterToggleMeter") == 0)
	{
		BangWithArgs(BANG_TOGGLEMETER, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterDisableMeasure") == 0)
	{
		BangWithArgs(BANG_DISABLEMEASURE, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterEnableMeasure") == 0)
	{
		BangWithArgs(BANG_ENABLEMEASURE, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterToggleMeasure") == 0)
	{
		BangWithArgs(BANG_TOGGLEMEASURE, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterActivateConfig") == 0)
	{
		RainmeterActivateConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterDeactivateConfig") == 0)
	{
		RainmeterDeactivateConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterToggleConfig") == 0)
	{
		RainmeterToggleConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterMove") == 0)
	{
		BangWithArgs(BANG_MOVE, arg.c_str(), 2);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterChangeZPos") == 0)	// For backwards combatibility
	{
		BangWithArgs(BANG_ZPOS, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterZPos") == 0)
	{
		BangWithArgs(BANG_ZPOS, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterAbout") == 0)
	{
		BangWithArgs(BANG_ABOUT, arg.c_str(), 0);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterResetStats") == 0)
	{
		RainmeterResetStats(NULL, NULL);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterMoveMeter") == 0)
	{
		BangWithArgs(BANG_MOVEMETER, arg.c_str(), 3);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterPluginBang") == 0)
	{
		BangWithArgs(BANG_PLUGIN, arg.c_str(), 1);
	}
	else if (wcsicmp(bang.c_str(), L"!RainmeterLsBoxHook") == 0)
	{
		// Nothing to do here (this works only with Litestep)
	}
	else if (wcsicmp(bang.c_str(), L"!Execute") == 0)
	{
		// Special case for multibang execution
		std::wstring::size_type start = std::wstring::npos;
		std::wstring::size_type end = std::wstring::npos;
		int count = 0;
		for (size_t i = 0; i < arg.size(); i++)
		{
			if (arg[i] == L'[')
			{
				if (count == 0)
				{
					start = i;
				}
				count++;
			}
			else if (arg[i] == L']')
			{
				count--;

				if (count == 0 && start != std::wstring::npos)
				{
					end = i;

					std::wstring command = arg.substr(start + 1, end - (start + 1));
					// trim leading whitespace
					std::wstring::size_type notwhite = command.find_first_not_of(L" \t\n");
					command.erase(0, notwhite);
					ExecuteCommand(command.c_str(), meterWindow);
				}
			}
		}
	}
	else
	{
		std::wstring error = L"Unknown !bang: ";
		error += bang;
		MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
		return FALSE;
	}

	return TRUE;
}


/*
** ParseCommand
**
** Replaces the measure names with the actual text values.
**
*/
std::wstring CRainmeter::ParseCommand(const WCHAR* command, CMeterWindow* meterWindow)
{
	std::wstring strCommand = command;

	if (wcsnicmp(L"!execute", command, 8) == 0)
	{
		return strCommand;
	}

	// Find the [measures]
	size_t start = 0, end = 0;
	while (start != std::wstring::npos && end != std::wstring::npos)
	{
		start = strCommand.find(L'[', start);
		if (start != std::wstring::npos)
		{
			end = strCommand.find(L']', start);
			if (end != std::wstring::npos)
			{
				std::wstring measureName = strCommand.substr(start + 1, end - (start + 1));
				if (!measureName.empty())
				{
					// Ignore bangs
					if (measureName[0] == L'!')
					{
						start = end + 1;
					}
					else
					{
						if (meterWindow) 
						{
							std::list<CMeasure*>::iterator iter = meterWindow->GetMeasures().begin();
							for( ; iter != meterWindow->GetMeasures().end(); iter++)
							{
								if (wcsicmp((*iter)->GetName(), measureName.c_str()) == 0)
								{
									std::wstring value = (*iter)->GetStringValue(false, 1, 0, false);
									strCommand.replace(start, (end - start) + 1, value);
									start += value.length();
									break;
								}
							}
							if (iter == meterWindow->GetMeasures().end())
							{
								DebugLog(L"No such measure [%s] for execute string: %s", measureName.c_str(), command);
								start = end + 1;
							}
						}
					}
				}
			}
		}
	}

	return strCommand;
}

/*
** ExecuteCommand
**
** Runs the given command or bang
**
*/
void CRainmeter::ExecuteCommand(const WCHAR* command, CMeterWindow* meterWindow) 
{
	if (command == NULL) return;

	std::wstring strCommand = ParseCommand(command, meterWindow);

	if (!strCommand.empty())
	{
		// Check for build-ins
		if (wcsncmp(L"PLAY ", strCommand.c_str(), 5) == 0)
		{
			BOOL ret = PlaySound(strCommand.c_str() + 5, NULL, SND_FILENAME);
			return;
		}
		else if (wcsncmp(L"PLAYSTOP", strCommand.c_str(), 8) == 0)
		{
			PlaySound(NULL, NULL, SND_PURGE);
			return;
		}
		else if (wcsncmp(L"PLAYLOOP ", strCommand.c_str(), 9) == 0)
		{
			PlaySound(strCommand.c_str() + 9, NULL, SND_ASYNC | SND_FILENAME | SND_LOOP | SND_NODEFAULT);
			return;
		}
		
		// Run the command
		if(strCommand.c_str()[0] == L'!' && Rainmeter->GetDummyLitestep())
		{
			if (meterWindow) 
			{
				// Fake WM_COPY to deliver bangs
				COPYDATASTRUCT CopyDataStruct;
				CopyDataStruct.cbData = (DWORD)((wcslen(command) + 1) * sizeof(WCHAR));
				CopyDataStruct.dwData = 1;
				CopyDataStruct.lpData = (void*)strCommand.c_str();
				meterWindow->OnCopyData(NULL, (LPARAM)&CopyDataStruct);
			}
			else
			{
				std::wstring bang, arg;
				size_t pos = strCommand.find(L' ');
				if (pos != std::wstring::npos) 
				{
					bang = strCommand.substr(0, pos);
					strCommand.erase(0, pos + 1);
					arg = strCommand;
				}
				else
				{
					bang = strCommand;
				}
				ExecuteBang(bang, arg, meterWindow);
			}
		}
		else
		{
			// This can run bangs also
			LSExecute(NULL, strCommand.c_str(), SW_SHOWNORMAL);
		}
	}
}

/*
** ReadGeneralSettings
**	
** Reads the general settings from the Rainmeter.ini file
**
*/
void CRainmeter::ReadGeneralSettings(std::wstring& iniFile)
{
	CConfigParser parser;
	parser.Initialize(iniFile.c_str());

	if (m_TrayWindow)
	{
		m_TrayWindow->ReadConfig(parser);
	}

	c_GlobalConfig.netInSpeed = parser.ReadFloat(L"Rainmeter", L"NetInSpeed", c_GlobalConfig.netInSpeed);
	c_GlobalConfig.netOutSpeed = parser.ReadFloat(L"Rainmeter", L"NetOutSpeed", c_GlobalConfig.netOutSpeed);

	m_ConfigEditor = parser.ReadString(L"Rainmeter", L"ConfigEditor", m_ConfigEditor.c_str());

	m_TrayExecuteL = parser.ReadString(L"Rainmeter", L"TrayExecuteL", m_TrayExecuteL.c_str());
	m_TrayExecuteR = parser.ReadString(L"Rainmeter", L"TrayExecuteR", m_TrayExecuteR.c_str());
	m_TrayExecuteM = parser.ReadString(L"Rainmeter", L"TrayExecuteM", m_TrayExecuteM.c_str());
	m_TrayExecuteDL = parser.ReadString(L"Rainmeter", L"TrayExecuteDL", m_TrayExecuteDL.c_str());
	m_TrayExecuteDR = parser.ReadString(L"Rainmeter", L"TrayExecuteDR", m_TrayExecuteDR.c_str());
	m_TrayExecuteDM = parser.ReadString(L"Rainmeter", L"TrayExecuteDM", m_TrayExecuteDM.c_str());

	m_CheckUpdate = parser.ReadInt(L"Rainmeter", L"CheckUpdate", m_CheckUpdate);

	std::wstring area = parser.ReadString(L"Rainmeter", L"DesktopWorkArea", L"");
	if (!area.empty())
	{
		swscanf(area.c_str(), L"%i,%i,%i,%i", &m_DesktopWorkArea.left, &m_DesktopWorkArea.top, 
			&m_DesktopWorkArea.right, &m_DesktopWorkArea.bottom);
		m_DesktopWorkAreaChanged = true;
	}

	// Check which configs are active
	if (!c_DummyLitestep)
	{
		char tmpSz[MAX_LINE_LENGTH];
		std::wstring skinName;
		std::wstring skinIni = L"Rainmeter.ini";

		// Check if step.rc has overrides these values
		if (GetRCString("RainmeterCurrentConfig", tmpSz, "", MAX_LINE_LENGTH - 1))
		{
			skinName = ConvertToWide(tmpSz);
		}

		if (GetRCString("RainmeterCurrentConfigIni", tmpSz, "Rainmeter.ini", MAX_LINE_LENGTH - 1))
		{
			skinIni = ConvertToWide(tmpSz);
		}
		
		if (!skinName.empty())
		{
			if (!SetActiveConfig(skinName, skinIni))
			{
				std::wstring error;
				error = L"The selected config (L" + skinName + L"\\" + skinIni + L") cannot be found.";
				MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
			}
			return;
		}
	}

	for (size_t i = 0; i < m_ConfigStrings.size(); i++)
	{
		int active  = parser.ReadInt(m_ConfigStrings[i].config.c_str(), L"Active", 0);

		// Make sure there is a ini file available
		if (active > 0 && active <= m_ConfigStrings[i].iniFiles.size())
		{
			m_ConfigStrings[i].active = active;
		}
	}
}

/* 
** SetActiveConfig
**
** Makes the given config active. If the config cannot be found this returns false.
*/
bool CRainmeter::SetActiveConfig(std::wstring& skinName, std::wstring& skinIni)
{
	for (size_t i = 0; i < m_ConfigStrings.size(); i++)
	{
		m_ConfigStrings[i].active = 0;	// Disable all other configs

		if (skinName == m_ConfigStrings[i].config)
		{
			for (size_t j = 0; j < m_ConfigStrings[i].iniFiles.size(); j++)
			{
				if (skinIni == m_ConfigStrings[i].iniFiles[j])
				{
					m_ConfigStrings[i].active = j + 1;
					return true;
				}
			}
		}
	}

	return false;
}

/* 
** Refresh
**
** Refreshes Rainmeter. If argument is given the config is refreshed
** otherwise all active meters are refreshed
*/
void CRainmeter::Refresh(const WCHAR* arg)
{
	std::wstring config, iniFile;

	try 
	{
		if (arg != NULL && wcslen(arg) > 0) 
		{
			std::wstring config = arg;
			CMeterWindow* meterWindow = GetMeterWindow(config);
			meterWindow->Refresh(false);
		}
		else
		{
			std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.begin();

			// Refresh all
			for (; iter != m_Meters.end(); iter++)
			{
				(*iter).second->Refresh(false);
			}
		}
	} 
	catch(CError& error) 
	{
		MessageBox(NULL, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
	}
}

/*
** ReadStats
**
** Reads the statistics from the ini-file
**
*/
void CRainmeter::ReadStats()
{
	WCHAR tmpSz[MAX_LINE_LENGTH];

	if(GetPrivateProfileString(L"Statistics", L"Since", L"", tmpSz, MAX_LINE_LENGTH, m_IniFile.c_str()) > 0) 
	{
 		m_StatsDate = tmpSz;
	}

	// Only Net measure has stats at the moment
	CMeasureNet::ReadStats(m_IniFile);
}

/*
** WriteStats
**
** Writes the statistics to the ini-file
**
*/
void CRainmeter::WriteStats()
{
	// Write the date for statistics
	WritePrivateProfileString(L"Statistics", L"Since", m_StatsDate.c_str(), m_IniFile.c_str());

	// Only Net measure has stats at the moment
	CMeasureNet::WriteStats(m_IniFile);

	WritePrivateProfileString(NULL, NULL, NULL, m_IniFile.c_str());
}

/*
** ResetStats
**
** Clears the statistics
**
*/
void CRainmeter::ResetStats()
{
	// Set the stats-date string
	struct tm *newtime;
    time_t long_time;
    time(&long_time);
    newtime = localtime(&long_time);
	m_StatsDate = _wasctime(newtime);
	m_StatsDate.resize(m_StatsDate.size() - 1);
	
	// Only Net measure has stats at the moment
	CMeasureNet::ResetStats();
}


/*
** IsNT
**
** Checks which OS you are running
**
*/
PLATFORM CRainmeter::IsNT()
{
	// Check if you are running a real OS

	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if(!GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		// Something's wrong, lets assime Win9x
		return PLATFORM_9X;
	}

	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		// You got NT
		if(osvi.dwMajorVersion <= 4) return PLATFORM_NT4;
		if(osvi.dwMajorVersion == 5) return PLATFORM_2K;
		return PLATFORM_XP;
	}
	
	return PLATFORM_9X;	// Wintendo alert!
}

/*
** ShowContextMenu
**
** Opens the context menu in given coordinates.
**
*/
void CRainmeter::ShowContextMenu(POINT pos, CMeterWindow* meterWindow) 
{
	// Show context menu, if no actions were executed
	HMENU menu = LoadMenu(m_Instance, MAKEINTRESOURCE(IDR_CONTEXT_MENU));

	if(menu)
	{
		HMENU configMenu = NULL;

		HMENU subMenu = GetSubMenu(menu, 0);
		if(subMenu)
		{
			if (!GetDummyLitestep())
			{
				// Disable Quit if ran as a Litestep plugin
				EnableMenuItem(subMenu, ID_CONTEXT_QUIT, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(subMenu, ID_CONTEXT_SHOWLOGFILE, MF_BYCOMMAND | MF_GRAYED);
			}

			HMENU configMenu = CreateConfigMenu(m_ConfigMenu);
			if (configMenu)
			{
				InsertMenu(subMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)configMenu, L"Configs");
			}

			if (meterWindow)
			{
				HMENU skinMenu = CreateSkinMenu(meterWindow, 0);
				InsertMenu(subMenu, 8, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, L"Skin menu");
			}
			else
			{
				// Create a menu for all active configs
				std::map<std::wstring, CMeterWindow*>::iterator iter = Rainmeter->GetAllMeterWindows().begin();

				int index = 0;
				for (; iter != Rainmeter->GetAllMeterWindows().end(); iter++)
				{
					CMeterWindow* mw = ((*iter).second);
					HMENU skinMenu = CreateSkinMenu(mw, index);
					InsertMenu(subMenu, 8, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, mw->GetSkinName().c_str());
					index++;
				}
			}


			TrackPopupMenu(
			  subMenu,
			  TPM_RIGHTBUTTON | TPM_LEFTALIGN, 
			  pos.x,
			  pos.y,
			  0,
			  meterWindow ? meterWindow->GetWindow() : m_TrayWindow->GetWindow(),
			  NULL
			);		
		}

		DestroyMenu(menu);
	}
}


HMENU CRainmeter::CreateConfigMenu(std::vector<CONFIGMENU>& configMenuData)
{
	HMENU configMenu = NULL;

	if (configMenuData.size() > 0)
	{
		configMenu = CreatePopupMenu();

		for (size_t i = 0; i < configMenuData.size(); i++)
		{
			if (configMenuData[i].index == -1)
			{
				HMENU submenu = CreateConfigMenu(configMenuData[i].children);
				if (submenu)
				{
					InsertMenu(configMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)submenu, configMenuData[i].name.c_str());
				}
			}
			else
			{
				CONFIG& config = m_ConfigStrings[configMenuData[i].index];
				InsertMenu(configMenu, i, MF_BYPOSITION, config.commands[i], configMenuData[i].name.c_str());

				if (config.active == i + 1)
				{
					CheckMenuItem(configMenu, i, MF_BYPOSITION | MF_CHECKED);
				}
			}
		}
	}
	return configMenu;
}

HMENU CRainmeter::CreateSkinMenu(CMeterWindow* meterWindow, int index)
{
	HMENU skinMenu = LoadMenu(m_Instance, MAKEINTRESOURCE(IDR_SKIN_MENU));

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
		HMENU posMenu = GetSubMenu(skinMenu, 0);
		if (posMenu)
		{
			switch(meterWindow->GetWindowZPosition()) 
			{
			case ZPOSITION_ONDESKTOP:
				CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_ONDESKTOP, MF_BYCOMMAND | MF_CHECKED);
				break;

			case ZPOSITION_ONBOTTOM:
				CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_BOTTOM, MF_BYCOMMAND | MF_CHECKED);
				break;

			case ZPOSITION_ONTOP:
				CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
				break;

			case ZPOSITION_ONTOPMOST:
				CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_VERYTOPMOST, MF_BYCOMMAND | MF_CHECKED);
				break;

			default:
				CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_NORMAL, MF_BYCOMMAND | MF_CHECKED);
			}

			if(meterWindow->GetXFromRight()) CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_FROMRIGHT, MF_BYCOMMAND | MF_CHECKED);
			if(meterWindow->GetYFromBottom()) CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_FROMBOTTOM, MF_BYCOMMAND | MF_CHECKED);
			if(meterWindow->GetXPercentage()) CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_XPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);
			if(meterWindow->GetYPercentage()) CheckMenuItem(posMenu, ID_CONTEXT_SKINMENU_YPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);

			if (!c_DummyLitestep)
			{
				EnableMenuItem(posMenu, ID_CONTEXT_SKINMENU_ONDESKTOP, MF_BYCOMMAND | MF_GRAYED);
			}
		}

		// Tick the transparency
		if (!meterWindow->GetNativeTransparency())
		{
			EnableMenuItem(skinMenu, 1, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(skinMenu, ID_CONTEXT_SKINMENU_CLICKTHROUGH, MF_BYCOMMAND | MF_GRAYED);
		}
		else
		{
			HMENU alphaMenu = GetSubMenu(skinMenu, 1);
			if (alphaMenu)
			{
				int value = (int)(10 - (meterWindow->GetAlphaValue() / 255.0) * 10.0);
				value = min(9, value);
				value = max(0, value);
				CheckMenuItem(alphaMenu, value, MF_BYPOSITION | MF_CHECKED);

				if (meterWindow->GetWindowHide() == HIDEMODE_FADEIN) 
				{
					CheckMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_CHECKED);
					EnableMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
				}
				else if (meterWindow->GetWindowHide() == HIDEMODE_FADEOUT) 
				{
					CheckMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_CHECKED);
					EnableMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
				}
				else if (meterWindow->GetWindowHide() == HIDEMODE_HIDE) 
				{
					EnableMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(alphaMenu, ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
				}
			}
		}

		// Tick the configs
		if (meterWindow->GetWindowHide() == HIDEMODE_HIDE)
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_HIDEONMOUSE, MF_BYCOMMAND | MF_CHECKED);
		}
		else if (meterWindow->GetWindowHide() != HIDEMODE_NONE)
		{
			EnableMenuItem(skinMenu, ID_CONTEXT_SKINMENU_HIDEONMOUSE, MF_BYCOMMAND | MF_GRAYED);
		}

		if (meterWindow->GetSnapEdges())
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_SNAPTOEDGES, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetSavePosition())
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_REMEMBERPOSITION, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetWindowDraggable())
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_DRAGGABLE, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetClickThrough())
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_CLICKTHROUGH, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetKeepOnScreen())
		{
			CheckMenuItem(skinMenu, ID_CONTEXT_SKINMENU_KEEPONSCREEN, MF_BYCOMMAND | MF_CHECKED);
		}

		// Add the name of the Skin to the menu and disable the item
		InsertMenu(skinMenu, 0, MF_BYPOSITION, 0, meterWindow->GetSkinName().c_str());
		InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		SetMenuDefaultItem(skinMenu, 0, MF_BYPOSITION);
		EnableMenuItem(skinMenu, 0, MF_BYPOSITION | MF_GRAYED);
	
		ChangeSkinIndex(skinMenu, index);
	}

	return skinMenu;
}

void CRainmeter::ChangeSkinIndex(HMENU menu, int index)
{
	int count = GetMenuItemCount(menu);

	for (int i = 0; i < count; i++)
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

/*
** FixPath
**
** Converts relative path to absolute.
**
*/
std::wstring CRainmeter::FixPath(const std::wstring& path, PATH_FOLDER folder, const std::wstring& currentSkin)
{
	if (path.empty() || path.find(L':') != std::wstring::npos)
	{
		return path;	// It's already absolute path (or it's empty)
	}

	std::wstring root;

	switch(folder) 
	{
	case PATH_FOLDER_INI:
		root = Rainmeter->GetPath();
		break;

	case PATH_FOLDER_SKINS:
		root = Rainmeter->GetSkinPath();
		break;

	case PATH_FOLDER_CURRENT_SKIN:
		root = Rainmeter->GetSkinPath() + currentSkin;
		break;

	case PATH_FOLDER_PLUGIN:
		root = Rainmeter->GetPluginPath();
		break;
	}

	if (root[root.length() - 1] != L'\\')
	{
		root += L"\\";
	}

	return root + path;
}
