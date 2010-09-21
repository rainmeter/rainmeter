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

#include "StdAfx.h"
#include "Rainmeter.h"
#include "System.h"
#include "Error.h"
#include "AboutDialog.h"
#include "MeasureNet.h"
#include "MeterString.h"
#include "Resource.h"
#include "UpdateCheck.h"

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
std::vector<std::wstring> CRainmeter::ParseString(LPCTSTR str)
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
		for (size_t i = 0; i < result.size(); ++i)
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
			return parser->ReadString(section, key, defValue, false).c_str();
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
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(arg);
		std::wstring config;
		std::wstring argument;

		// Don't include the config name from the arg if there is one
		for (size_t i = 0; i < numOfArgs; ++i)
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
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();

				for (; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
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
** BangGroupWithArgs
**
** Parses Bang args for Group
**
*/
void BangGroupWithArgs(BANGCOMMAND bang, const WCHAR* arg, size_t numOfArgs)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(arg);

		if (subStrings.size() > numOfArgs)
		{
			std::multimap<int, CMeterWindow*> windows;
			Rainmeter->GetMeterWindowsByLoadOrder(windows, subStrings[numOfArgs]);

			std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
			for (; iter != windows.end(); ++iter)
			{
				std::wstring argument = L"\"";
				for (size_t i = 0; i < numOfArgs; ++i)
				{
					argument += subStrings[i];
					argument += L"\" \"";
				}
				argument += (*iter).second->GetSkinName();
				argument += L"\"";
				BangWithArgs(bang, argument.c_str(), numOfArgs);
			}
		}
		else
		{
			DebugLog(L"Incorrect number of arguments for the group bang!");
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
** RainmeterHideFade
**
** Callback for the !RainmeterHideFade bang
**
*/
void RainmeterHideFade(HWND, const char* arg)
{
	BangWithArgs(BANG_HIDEFADE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterShowFade
**
** Callback for the !RainmeterShowFade bang
**
*/
void RainmeterShowFade(HWND, const char* arg)
{
	BangWithArgs(BANG_SHOWFADE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterToggleFade
**
** Callback for the !RainmeterToggleFade bang
**
*/
void RainmeterToggleFade(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLEFADE, ConvertToWide(arg).c_str(), 0);
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
** RainmeterDisableMeasure
**
** Callback for the !RainmeterDisableMeasure bang
**
*/
void RainmeterDisableMeasure(HWND, const char* arg)
{
	BangWithArgs(BANG_DISABLEMEASURE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterEnableMeasure
**
** Callback for the !RainmeterEnableMeasure bang
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
** RainmeterRefreshApp
**
** Callback for the !RainmeterRefreshApp bang
**
*/
void RainmeterRefreshApp(HWND, const char* arg)
{
	if (Rainmeter)
	{
		// Refresh needs to be delayed since it crashes if done during Update()
		PostMessage(Rainmeter->GetTrayWindow()->GetWindow(), WM_DELAYED_REFRESH_ALL, (WPARAM)NULL, (LPARAM)NULL);
	}
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
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 1)
		{
			const std::vector<CRainmeter::CONFIG>& configs = Rainmeter->GetAllConfigs();

			for (size_t i = 0; i < configs.size(); ++i)
			{
				if (_wcsicmp(configs[i].config.c_str(), subStrings[0].c_str()) == 0)
				{
					for (size_t j = 0; j < configs[i].iniFiles.size(); ++j)
					{
						if (_wcsicmp(configs[i].iniFiles[j].c_str(), subStrings[1].c_str()) == 0)
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
			DebugLog(L"Unable to parse the arguments for !RainmeterActivateConfig");
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
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 0)
		{
			CMeterWindow* mw = Rainmeter->GetMeterWindow(subStrings[0]);
			if (mw)
			{
				Rainmeter->DeactivateConfig(mw, -1);
				return;
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
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() >= 2)
		{
			CMeterWindow* mw = Rainmeter->GetMeterWindow(subStrings[0]);
			if (mw)
			{
				Rainmeter->DeactivateConfig(mw, -1);
				return;
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
** RainmeterClickThrough
**
** Callback for the !RainmeterClickThrough bang
**
*/
void RainmeterClickThrough(HWND, const char* arg)
{
	BangWithArgs(BANG_CLICKTHROUGH, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterDraggable
**
** Callback for the !RainmeterDraggable bang
**
*/
void RainmeterDraggable(HWND, const char* arg)
{
	BangWithArgs(BANG_DRAGGABLE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSnapEdges
**
** Callback for the !RainmeterSnapEdges bang
**
*/
void RainmeterSnapEdges(HWND, const char* arg)
{
	BangWithArgs(BANG_SNAPEDGES, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterKeepOnScreen
**
** Callback for the !RainmeterKeepOnScreen bang
**
*/
void RainmeterKeepOnScreen(HWND, const char* arg)
{
	BangWithArgs(BANG_KEEPONSCREEN, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSetTransparency
**
** Callback for the !RainmeterSetTransparency bang
**
*/
void RainmeterSetTransparency(HWND, const char* arg)
{
	BangWithArgs(BANG_SETTRANSPARENCY, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSetVariable
**
** Callback for the !RainmeterSetVariable bang
**
*/
void RainmeterSetVariable(HWND, const char* arg)
{
	BangWithArgs(BANG_SETVARIABLE, ConvertToWide(arg).c_str(), 2);
}

/*
** RainmeterHideGroup
**
** Callback for the !RainmeterHideGroup bang
**
*/
void RainmeterHideGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_HIDE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterShowGroup
**
** Callback for the !RainmeterShowGroup bang
**
*/
void RainmeterShowGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_SHOW, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterToggleGroup
**
** Callback for the !RainmeterToggleGroup bang
**
*/
void RainmeterToggleGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_TOGGLE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterHideFadeGroup
**
** Callback for the !RainmeterHideFadeGroup bang
**
*/
void RainmeterHideFadeGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_HIDEFADE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterShowFadeGroup
**
** Callback for the !RainmeterShowFadeGroup bang
**
*/
void RainmeterShowFadeGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_SHOWFADE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterToggleFadeGroup
**
** Callback for the !RainmeterToggleFadeGroup bang
**
*/
void RainmeterToggleFadeGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_TOGGLEFADE, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterHideMeterGroup
**
** Callback for the !RainmeterHideMeterGroup bang
**
*/
void RainmeterHideMeterGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_HIDEMETERGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterShowMeterGroup
**
** Callback for the !RainmeterShowMeterGroup bang
**
*/
void RainmeterShowMeterGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_SHOWMETERGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterToggleMeterGroup
**
** Callback for the !RainmeterToggleMeterGroup bang
**
*/
void RainmeterToggleMeterGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLEMETERGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterDisableMeasureGroup
**
** Callback for the !RainmeterDisableMeasureGroup bang
**
*/
void RainmeterDisableMeasureGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_DISABLEMEASUREGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterEnableMeasureGroup
**
** Callback for the !RainmeterEnableMeasureGroup bang
**
*/
void RainmeterEnableMeasureGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_ENABLEMEASUREGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterToggleMeasureGroup
**
** Callback for the !RainmeterToggleMeasureGroup bang
**
*/
void RainmeterToggleMeasureGroup(HWND, const char* arg)
{
	BangWithArgs(BANG_TOGGLEMEASUREGROUP, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterRefreshGroup
**
** Callback for the !RainmeterRefreshGroup bang
**
*/
void RainmeterRefreshGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_REFRESH, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterRedrawGroup
**
** Callback for the !RainmeterRedrawGroup bang
**
*/
void RainmeterRedrawGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_REDRAW, ConvertToWide(arg).c_str(), 0);
}

/*
** RainmeterDeactivateConfigGroup
**
** Callback for the !RainmeterDeactivateConfigGroup bang
**
*/
void RainmeterDeactivateConfigGroup(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 0)
		{
			std::multimap<int, CMeterWindow*> windows;
			Rainmeter->GetMeterWindowsByLoadOrder(windows, subStrings[0]);

			std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
			for (; iter != windows.end(); ++iter)
			{
				Rainmeter->DeactivateConfig((*iter).second, -1);
			}
		}
		else
		{
			DebugLog(L"Unable to parse the arguments for !RainmeterDeactivateConfigGroup");
		}
	}
}

/*
** RainmeterZPosGroup
**
** Callback for the !RainmeterZPosGroup bang
**
*/
void RainmeterZPosGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_ZPOS, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterClickThroughGroup
**
** Callback for the !RainmeterClickThroughGroup bang
**
*/
void RainmeterClickThroughGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_CLICKTHROUGH, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterDraggableGroup
**
** Callback for the !RainmeterDraggableGroup bang
**
*/
void RainmeterDraggableGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_DRAGGABLE, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSnapEdgesGroup
**
** Callback for the !RainmeterSnapEdgesGroup bang
**
*/
void RainmeterSnapEdgesGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_SNAPEDGES, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterKeepOnScreenGroup
**
** Callback for the !RainmeterKeepOnScreenGroup bang
**
*/
void RainmeterKeepOnScreenGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_KEEPONSCREEN, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSetTransparencyGroup
**
** Callback for the !RainmeterSetTransparencyGroup bang
**
*/
void RainmeterSetTransparencyGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_SETTRANSPARENCY, ConvertToWide(arg).c_str(), 1);
}

/*
** RainmeterSetVariableGroup
**
** Callback for the !RainmeterSetVariableGroup bang
**
*/
void RainmeterSetVariableGroup(HWND, const char* arg)
{
	BangGroupWithArgs(BANG_SETVARIABLE, ConvertToWide(arg).c_str(), 2);
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
	if (Rainmeter)
	{
		OpenAboutDialog(Rainmeter->GetTrayWindow()->GetWindow(), Rainmeter->GetInstance());
	}
}

/*
** RainmeterSkinMenu
**
** Callback for the !RainmeterSkinMenu bang
**
*/
void RainmeterSkinMenu(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 0)
		{
			CMeterWindow* mw = Rainmeter->GetMeterWindow(subStrings[0]);
			if (mw)
			{
				POINT pos;
				GetCursorPos(&pos);
				Rainmeter->ShowContextMenu(pos, mw);
				return;
			}
			DebugLog(L"The config is not active: \"%s\"", subStrings[0].c_str());
		}
		else
		{
			DebugLog(L"Unable to parse the arguments for !RainmeterSkinMenu");
		}
	}
}

/*
** RainmeterTrayMenu
**
** Callback for the !RainmeterTrayMenu bang
**
*/
void RainmeterTrayMenu(HWND, const char* arg)
{
	if (Rainmeter)
	{
		POINT pos;
		GetCursorPos(&pos);
		Rainmeter->ShowContextMenu(pos, NULL);
	}
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
** RainmeterWriteKeyValue
**
** Callback for the !RainmeterWriteKeyValue bang
**
*/
void RainmeterWriteKeyValue(HWND, const char* arg)
{
	if (Rainmeter)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(ConvertToWide(arg).c_str());

		if (subStrings.size() > 3)
		{
			const std::wstring& iniFile = subStrings[3];

			if (iniFile.find(L"..\\") != std::string::npos || iniFile.find(L"../") != std::string::npos)
			{
				DebugLog(L"!RainmeterWriteKeyValue: Illegal characters in path - \"..\\\": %s", iniFile.c_str());
				return;
			}

			if (_wcsnicmp(iniFile.c_str(), Rainmeter->GetSkinPath().c_str(), Rainmeter->GetSkinPath().size()) != 0 &&
				_wcsnicmp(iniFile.c_str(), Rainmeter->GetPath().c_str(), Rainmeter->GetPath().size()) != 0)
			{
				DebugLog(L"!RainmeterWriteKeyValue: Illegal path outside of Rainmeter directories: %s", iniFile.c_str());
				return;
			}

			// Verify whether the file exists
			if (_waccess(iniFile.c_str(), 0) == -1)
			{
				DebugLog(L"!RainmeterWriteKeyValue: File not found: %s", iniFile.c_str());
				return;
			}

			// Verify whether the file is read-only
			DWORD attr = GetFileAttributes(iniFile.c_str());
			if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
			{
				DebugLog(L"!RainmeterWriteKeyValue: File is read-only: %s", iniFile.c_str());
				return;
			}

			// Avoid "IniFileMapping"
			std::vector<std::wstring> iniFileMappings;
			CSystem::GetIniFileMappingList(iniFileMappings);
			std::wstring iniWrite = CSystem::GetTemporaryFile(iniFileMappings, iniFile);
			if (iniWrite == L"<>")  // error occurred
			{
				DebugLog(L"!RainmeterWriteKeyValue: Failed to create a temporary file: %s", iniFile.c_str());
				return;
			}

			bool temporary = !iniWrite.empty();

			if (temporary)
			{
				if (CRainmeter::GetDebug()) DebugLog(L"!RainmeterWriteKeyValue: Writing file: %s (Temp: %s)", iniFile.c_str(), iniWrite.c_str());
			}
			else
			{
				if (CRainmeter::GetDebug()) DebugLog(L"!RainmeterWriteKeyValue: Writing file: %s", iniFile.c_str());
				iniWrite = iniFile;
			}

			const std::wstring& strSection = subStrings[0];
			const std::wstring& strKey = subStrings[1];
			const std::wstring& strValue = subStrings[2];

			int formula = -1;
			BOOL write = 0;

			if (subStrings.size() > 4)
			{
				CMeterWindow* mw = Rainmeter->GetMeterWindow(subStrings[4]);
				if (mw)
				{
					double value;
					formula = mw->GetParser().ReadFormula(strValue, &value);
					
					// Formula read fine
					if (formula != -1)
					{
						TCHAR buffer[256];
						swprintf(buffer, L"%f", value);

						const std::wstring& resultString = buffer;

						write = WritePrivateProfileString(strSection.c_str(), strKey.c_str(), resultString.c_str(), iniWrite.c_str());
					}
				}
			}

			if (formula == -1)
			{
				write = WritePrivateProfileString(strSection.c_str(), strKey.c_str(), strValue.c_str(), iniWrite.c_str());
			}

			if (temporary)
			{
				if (write != 0)
				{
					WritePrivateProfileString(NULL, NULL, NULL, iniWrite.c_str());  // FLUSH

					// Copy the file back
					if (!CSystem::CopyFiles(iniWrite, iniFile))
					{
						DebugLog(L"!RainmeterWriteKeyValue: Failed to copy a temporary file to the original filepath: %s (Temp: %s)", iniFile.c_str(), iniWrite.c_str());
					}
				}
				else  // failed
				{
					DebugLog(L"!RainmeterWriteKeyValue: Failed to write a value to the file: %s (Temp: %s)", iniFile.c_str(), iniWrite.c_str());
				}

				// Remove a temporary file
				CSystem::RemoveFile(iniWrite);
			}
			else
			{
				if (write == 0)  // failed
				{
					DebugLog(L"!RainmeterWriteKeyValue: Failed to write a value to the file: %s", iniFile.c_str());
				}
			}
		}
		else
		{
			DebugLog(L"Unable to parse the arguments for !RainmeterWriteKeyValue");
		}
	}
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

/*
** RainmeterQuit
**
** Callback for the !RainmeterQuit bang
**
*/
void RainmeterQuit(HWND, const char* arg)
{
	if (Rainmeter)
	{
		// Quit needs to be delayed since it crashes if done during Update()
		PostMessage(Rainmeter->GetTrayWindow()->GetWindow(), WM_COMMAND, MAKEWPARAM(ID_CONTEXT_QUIT, 0), (LPARAM)NULL);
	}
}


// -----------------------------------------------------------------------------------------------
//
//                                The class starts here
//
// -----------------------------------------------------------------------------------------------

GlobalConfig CRainmeter::c_GlobalConfig;
bool CRainmeter::c_Debug = false;

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

	c_Debug = false;

	m_MenuActive = false;

	m_Logging = false;

	m_DesktopWorkAreaChanged = false;
	m_DesktopWorkAreaType = false;

	m_DisableVersionCheck = FALSE;
	m_NewVersion = FALSE;

	m_Instance = NULL;
	m_CurrentParser = NULL;

	m_TrayWindow = NULL;

	InitializeCriticalSection(&m_CsLogData);

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
	while (m_Meters.size() > 0)
	{
		DeleteMeterWindow((*m_Meters.begin()).second, false);	// This removes the window from the vector
	}

	if (m_TrayWindow) delete m_TrayWindow;

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

	DeleteCriticalSection(&m_CsLogData);

	GdiplusShutdown(m_GDIplusToken);
}

/* 
** ExecuteBang
**
** Runs a bang command. This is called from the main application
** when a command is given as a command line argument.
**
*/
void ExecuteBang(LPCTSTR szBang)
{
	if (Rainmeter) Rainmeter->ExecuteCommand(szBang, NULL);
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
		*(pos + 1) = L'\0';
	} 
	else 
	{
		tmpName[0] = L'\0';
	}

	m_Path = tmpName;

	if(!c_DummyLitestep) InitalizeLitestep();

	bool bDefaultIniLocation = false;

	if (c_CmdLine.empty())
	{
		m_IniFile = m_Path + L"Rainmeter.ini";

		// If the ini file doesn't exist in the program folder store it to the %APPDATA% instead so that things work better in Vista/Win7
		if (_waccess(m_IniFile.c_str(), 0) == -1)
		{
			m_IniFile = L"%APPDATA%\\Rainmeter\\Rainmeter.ini";
			ExpandEnvironmentVariables(m_IniFile);
			bDefaultIniLocation = true;

			// If the ini file doesn't exist in the %APPDATA% either, create a default Rainmeter.ini file.
			if (_waccess(m_IniFile.c_str(), 0) == -1)
			{
				CreateDefaultConfigFile(m_IniFile);
			}
		}
	}
	else
	{
		// The command line defines the location of Rainmeter.ini (or whatever it calls it).
		std::wstring iniFile = c_CmdLine;
		if (iniFile[0] == L'\"')
		{
			if (iniFile.length() == 1)
			{
				iniFile.clear();
			}
			else if (iniFile[iniFile.length() - 1] == L'\"')
			{
				iniFile = iniFile.substr(1, iniFile.length() - 2);
			}
		}

		ExpandEnvironmentVariables(iniFile);

		if (iniFile.empty() || iniFile[iniFile.length() - 1] == L'\\')
		{
			iniFile += L"Rainmeter.ini";
		}
		else if (iniFile.length() <= 4 || _wcsicmp(iniFile.substr(iniFile.length() - 4).c_str(), L".ini") != 0)
		{
			iniFile += L"\\Rainmeter.ini";
		}

		if (iniFile[0] != L'\\' && iniFile[0] != L'/' && iniFile.find_first_of(L':') == std::wstring::npos)
		{
			// Make absolute path
			iniFile.insert(0, m_Path);
		}

		m_IniFile = iniFile;

		// If the ini file doesn't exist, create a default Rainmeter.ini file.
		if (_waccess(m_IniFile.c_str(), 0) == -1)
		{
			CreateDefaultConfigFile(m_IniFile);
		}
		bDefaultIniLocation = true;
	}

	// Set the log file location
	m_LogFile = m_IniFile;
	size_t logFileLen = m_LogFile.length();
	if (logFileLen > 4 && _wcsicmp(m_LogFile.substr(logFileLen - 4).c_str(), L".ini") == 0)
	{
		m_LogFile.replace(logFileLen - 4, 4, L".log");
	}
	else
	{
		m_LogFile += L".log";	// Append the extension so that we don't accidentally overwrite the ini file
	}

	// Read Logging settings beforehand
	m_Logging = 0!=GetPrivateProfileInt(L"Rainmeter", L"Logging", 0, m_IniFile.c_str());
	c_Debug = 0!=GetPrivateProfileInt(L"Rainmeter", L"Debug", 0, m_IniFile.c_str());

	if (m_Logging)
	{
		StartLogging();
	}

	m_PluginPath = tmpName;
	m_PluginPath += L"Plugins\\";
	m_SkinPath = m_Path + L"Skins\\";

	// Read the skin folder from the ini file
	WCHAR tmpSz[MAX_LINE_LENGTH];
	if (GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", tmpSz, MAX_LINE_LENGTH, m_IniFile.c_str()) > 0) 
	{
		m_SkinPath = tmpSz;
		ExpandEnvironmentVariables(m_SkinPath);
	}
	else if (bDefaultIniLocation)
	{
		// If the skin path is not defined in the Rainmeter.ini file use My Documents/Rainmeter/Skins
		TCHAR szPath[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, szPath);
		if (SUCCEEDED(hr))
		{
			// Make the folders if they don't exist yet
			m_SkinPath = szPath;
			m_SkinPath += L"\\Rainmeter";
			CreateDirectory(m_SkinPath.c_str(), NULL);
			m_SkinPath += L"\\Skins";
			DWORD result = CreateDirectory(m_SkinPath.c_str(), NULL);
			m_SkinPath += L"\\";
			if (result != 0)
			{
				// The folder was created successfully which means that it wasn't available yet.
				// Copy the default skin to the Skins folder
				std::wstring strFrom(m_Path + L"Skins\\" + L"*.*");
				std::wstring strTo(m_SkinPath);
				CSystem::CopyFiles(strFrom, strTo);

				// This shouldn't be copied
				std::wstring strNote = strTo + L"Read me before copying skins to here.txt";
				CSystem::RemoveFile(strNote);

				// Copy also the themes to the %APPDATA%
				strFrom = std::wstring(m_Path + L"Themes\\" + L"*.*");
				strTo = std::wstring(GetSettingsPath() + L"Themes");
				CreateDirectory(strTo.c_str(), NULL);
				strTo += L"\\";
				CSystem::CopyFiles(strFrom, strTo);
			}
		}
		else
		{
			DebugLog(L"Unable to get the My Documents location.");
		}
	}
	WritePrivateProfileString(L"Rainmeter", L"SkinPath", m_SkinPath.c_str(), m_IniFile.c_str());

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

		if (!m_SkinPath.empty())
		{
			WCHAR ch = m_SkinPath[m_SkinPath.size() - 1];
			if (ch != L'\\' && ch != L'/')
			{
				m_SkinPath += L"\\";
			}
		}
	}

	DebugLog(L"Path: %s", m_Path.c_str());
	DebugLog(L"IniFile: %s", m_IniFile.c_str());
	DebugLog(L"SkinPath: %s", m_SkinPath.c_str());
	DebugLog(L"PluginPath: %s", m_PluginPath.c_str());

	// Test that the Rainmeter.ini file is writable
	TestSettingsFile(bDefaultIniLocation);

	// If the skin folder is somewhere else than in the program path
	if (_wcsnicmp(m_Path.c_str(), m_SkinPath.c_str(), m_Path.size()) != 0)
	{
		CheckSkinVersions();
	}

	CSystem::Initialize(Instance);
	CMeasureNet::InitializeNewApi();

	if (c_Debug)
	{
		LSLog(LOG_DEBUG, APPNAME, L"Enumerating installed font families...");
		CMeterString::EnumerateInstalledFontFamilies();
	}

	// Tray must exist before configs are read
	m_TrayWindow = new CTrayWindow(m_Instance);

	ScanForConfigs(m_SkinPath);
	ScanForThemes(GetSettingsPath() + L"Themes");

	if(m_ConfigStrings.empty())
	{
		std::wstring error = L"There are no available skins at:\n" + m_SkinPath;
		MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
	}

	ReadGeneralSettings(m_IniFile);

	WritePrivateProfileString(L"Rainmeter", L"CheckUpdate", NULL , m_IniFile.c_str());

	if (!m_DisableVersionCheck)
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
		UpdateDesktopWorkArea(false);
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
		AddBangCommand("!RainmeterHideFade", RainmeterHideFade);
		AddBangCommand("!RainmeterShowFade", RainmeterShowFade);
		AddBangCommand("!RainmeterToggleFade", RainmeterToggleFade);
		AddBangCommand("!RainmeterHideMeter", RainmeterHideMeter);
		AddBangCommand("!RainmeterShowMeter", RainmeterShowMeter);
		AddBangCommand("!RainmeterToggleMeter", RainmeterToggleMeter);
		AddBangCommand("!RainmeterMoveMeter", RainmeterMoveMeter);
		AddBangCommand("!RainmeterDisableMeasure", RainmeterDisableMeasure);
		AddBangCommand("!RainmeterEnableMeasure", RainmeterEnableMeasure);
		AddBangCommand("!RainmeterToggleMeasure", RainmeterToggleMeasure);
		AddBangCommand("!RainmeterActivateConfig", RainmeterActivateConfig);
		AddBangCommand("!RainmeterDeactivateConfig", RainmeterDeactivateConfig);
		AddBangCommand("!RainmeterToggleConfig", RainmeterToggleConfig);
		AddBangCommand("!RainmeterMove", RainmeterMove);
		AddBangCommand("!RainmeterZPos", RainmeterZPos);
		AddBangCommand("!RainmeterClickThrough", RainmeterClickThrough);
		AddBangCommand("!RainmeterDraggable", RainmeterDraggable);
		AddBangCommand("!RainmeterSnapEdges", RainmeterSnapEdges);
		AddBangCommand("!RainmeterKeepOnScreen", RainmeterKeepOnScreen);
		AddBangCommand("!RainmeterSetTransparency", RainmeterSetTransparency);
		AddBangCommand("!RainmeterSetVariable", RainmeterSetVariable);

		AddBangCommand("!RainmeterRefreshGroup", RainmeterRefreshGroup);
		AddBangCommand("!RainmeterRedrawGroup", RainmeterRedrawGroup);
		AddBangCommand("!RainmeterHideGroup", RainmeterHideGroup);
		AddBangCommand("!RainmeterShowGroup", RainmeterShowGroup);
		AddBangCommand("!RainmeterToggleGroup", RainmeterToggleGroup);
		AddBangCommand("!RainmeterHideFadeGroup", RainmeterHideFadeGroup);
		AddBangCommand("!RainmeterShowFadeGroup", RainmeterShowFadeGroup);
		AddBangCommand("!RainmeterToggleFadeGroup", RainmeterToggleFadeGroup);
		AddBangCommand("!RainmeterHideMeterGroup", RainmeterHideMeterGroup);
		AddBangCommand("!RainmeterShowMeterGroup", RainmeterShowMeterGroup);
		AddBangCommand("!RainmeterToggleMeterGroup", RainmeterToggleMeterGroup);
		AddBangCommand("!RainmeterDisableMeasureGroup", RainmeterDisableMeasureGroup);
		AddBangCommand("!RainmeterEnableMeasureGroup", RainmeterEnableMeasureGroup);
		AddBangCommand("!RainmeterToggleMeasureGroup", RainmeterToggleMeasureGroup);
		AddBangCommand("!RainmeterDeactivateConfigGroup", RainmeterDeactivateConfigGroup);
		AddBangCommand("!RainmeterZPosGroup", RainmeterZPosGroup);
		AddBangCommand("!RainmeterClickThroughGroup", RainmeterClickThroughGroup);
		AddBangCommand("!RainmeterDraggableGroup", RainmeterDraggableGroup);
		AddBangCommand("!RainmeterSnapEdgesGroup", RainmeterSnapEdgesGroup);
		AddBangCommand("!RainmeterKeepOnScreenGroup", RainmeterKeepOnScreenGroup);
		AddBangCommand("!RainmeterSetTransparencyGroup", RainmeterSetTransparencyGroup);
		AddBangCommand("!RainmeterSetVariableGroup", RainmeterSetVariableGroup);

		AddBangCommand("!RainmeterRefreshApp", RainmeterRefreshApp);
		AddBangCommand("!RainmeterLsBoxHook", RainmeterLsHook);
		AddBangCommand("!RainmeterAbout", RainmeterAbout);
		AddBangCommand("!RainmeterSkinMenu", RainmeterSkinMenu);
		AddBangCommand("!RainmeterTrayMenu", RainmeterTrayMenu);
		AddBangCommand("!RainmeterResetStats", RainmeterResetStats);
		AddBangCommand("!RainmeterWriteKeyValue", RainmeterWriteKeyValue);
		AddBangCommand("!RainmeterPluginBang", RainmeterPluginBang);
		AddBangCommand("!RainmeterQuit", RainmeterQuit);
	}

	// Create meter windows for active configs
	std::multimap<int, int>::const_iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		const CONFIG& config = m_ConfigStrings[(*iter).second];
		if (config.active > 0 && config.active <= (int)config.iniFiles.size())
		{
			ActivateConfig((*iter).second, config.active - 1);
		}
	}

	return Result;	// Alles OK
}

/* 
** CheckSkinVersions
**
** Checks if any of the skins in the program folder are newer than in the skin folder.
**
*/
void CRainmeter::CheckSkinVersions()
{
	// List all skins in the program folder
	std::wstring strMainSkinsPath = m_Path + L"Skins\\";
	std::vector<CONFIGMENU> menu;
	ScanForConfigsRecursive(strMainSkinsPath, L"", 0, menu, true);

	for (size_t i = 0; i < menu.size(); ++i)
	{
		// DebugLog(L"%s", menu[i].name.c_str());

		// Read the version files
		std::wstring strNewVersionFile = strMainSkinsPath + menu[i].name + L"\\version";
		std::wstring strCurrentVersionFile = m_SkinPath + menu[i].name + L"\\version";

		std::string strVersion;
		std::wstring strVersionNew;
		std::wstring strVersionCurrent;
		std::wstring strVersionInIni;

		std::ifstream newFile(strNewVersionFile.c_str(), std::ios_base::in);
		if (getline(newFile, strVersion))
		{
			strVersionNew = ConvertToWide(strVersion.c_str());
			// DebugLog(L"New: %s", strVersionNew.c_str());

			// Compare with the version entry in the Rainmeter.ini
			WCHAR tmpSz[256] = {0};
			GetPrivateProfileString(menu[i].name.c_str(), L"Version", L"", tmpSz, 256, m_IniFile.c_str());
			strVersionInIni = tmpSz;

			// DebugLog(L"In Ini: %s", strVersionInIni.c_str());

			// Compare with the version file in the skin folder
			std::ifstream currentFile(strCurrentVersionFile.c_str(), std::ios_base::in);
			if (getline(currentFile, strVersion))
			{
				strVersionCurrent = ConvertToWide(strVersion.c_str());
				// DebugLog(L"Current: %s", strVersionCurrent.c_str());
			}
		}

		// If the skin doesn't define a version file no need to do anything
		if (!strVersionNew.empty())
		{
			// Compare the version files
			if (CompareVersions(strVersionNew, strVersionInIni) == 1 &&
				CompareVersions(strVersionNew, strVersionCurrent) == 1)
			{
				// Check if the old skin exists at all
				struct _stat64i32 s;
				std::wstring strSkinPath = m_SkinPath + menu[i].name;
				if (_wstat(strSkinPath.c_str(), &s) == 0)
				{
					std::wstring strMessage = L"A new version of config \"" + menu[i].name + L"\" is available.\n\n";
					strMessage += L"New version: " + (strVersionNew.empty() ? L"Unknown" : strVersionNew) + L"\n";
					strMessage += L"Current version: " + (strVersionCurrent.empty() ? L"Unknown" : strVersionCurrent) + L"\n";
					strMessage += L"\n";
					strMessage += L"Do you want to upgrade?";
					strMessage += L"\n\n";
					strMessage += L"(If you select 'Yes' your current config\nwill be moved into the 'Backup' folder)";

					if (IDYES == MessageBox(NULL, strMessage.c_str(), APPNAME, MB_YESNO | MB_ICONQUESTION))
					{
						// Make sure that the folder exists
						CreateDirectory(std::wstring(m_SkinPath + L"Backup").c_str(), NULL);

						// Check for illegal characters from the version number
						if (strVersionCurrent.find_first_of(L"\\/\"*:?<>|") == std::wstring::npos)
						{
							std::wstring strTarget = m_SkinPath + L"Backup\\" + menu[i].name + L"-" + strVersionCurrent;
							if (CSystem::CopyFiles(m_SkinPath + menu[i].name, strTarget, true))	// Move the folder to "backup"
							{
								// Upgrade the skin
								CSystem::CopyFiles(strMainSkinsPath + menu[i].name, m_SkinPath);

								// TODO: Temporary 'fix': If this was Enigma upgrade the themes too
								if (menu[i].name == L"Enigma" || menu[i].name == L"Gnometer")
								{
									std::wstring strMainThemes = m_Path + L"Themes";
									std::wstring strCurrentThemes = GetSettingsPath();
									CSystem::CopyFiles(strMainThemes, strCurrentThemes);
								}
								// End of temporary 'fix'
							}
							else
							{
								std::wstring strMessage = L"Failed to upgrade the config.\nUnable to backup the current config.";
								MessageBox(NULL, strMessage.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
							}
						}
						else
						{
							std::wstring strMessage = L"Failed to upgrade the config.\nThe version number contains illegal characters.";
							MessageBox(NULL, strMessage.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
						}
					}
				}
				else
				{
					std::wstring strMessage = L"A new version of config \"" + menu[i].name + L"\" is available\n";
					strMessage += L"Do you want to add it to your skin and themes libraries?";
					if (IDYES == MessageBox(NULL, strMessage.c_str(), APPNAME, MB_YESNO | MB_ICONQUESTION))
					{
						CSystem::CopyFiles(strMainSkinsPath + menu[i].name, m_SkinPath);
						std::wstring strMainThemes = m_Path + L"Themes";
						std::wstring strCurrentThemes = GetSettingsPath();
						CSystem::CopyFiles(strMainThemes, strCurrentThemes);
					}
				}

				// Even if the user doesn't want to upgrade mark it to the Rainmeter.ini so we don't ask the upgrade question again
				WritePrivateProfileString(menu[i].name.c_str(), L"Version", strVersionNew.c_str(), m_IniFile.c_str());
			}
		}
	}
}

/*	
** CompareVersions
**
** Compares two version strings. Returns 0 if they are equal, 1 if A > B and -1 if A < B.
**
*/
int CRainmeter::CompareVersions(std::wstring strA, std::wstring strB)
{
	if (strA.empty() && strB.empty()) return 0;
	if (strA.empty()) return -1;
	if (strB.empty()) return 1;

	std::vector<std::wstring> arrayA = CConfigParser::Tokenize(strA, L".");
	std::vector<std::wstring> arrayB = CConfigParser::Tokenize(strB, L".");
	
	size_t len = max(arrayA.size(), arrayB.size());
	for (size_t i = 0; i < len; ++i)
	{
		int a = 0;
		int b = 0;

		if (i < arrayA.size())
		{
			a = _wtoi(arrayA[i].c_str());
		}
		if (i < arrayB.size())
		{
			b = _wtoi(arrayB[i].c_str());
		}

		if (a > b) return 1;
		if (a < b) return -1;
	}
	return 0;
}

/* 
** CreateDefaultConfigFile
**
** Creates the default Rainmeter.ini file. Gnometer\Bars and Gnometer\Clock configs
** are enabled.
**
*/
void CRainmeter::CreateDefaultConfigFile(std::wstring strFile)
{
	size_t pos = strFile.find_last_of(L'\\');
	if (pos != std::wstring::npos)
	{
		std::wstring strPath(strFile.begin(), strFile.begin() + pos);
		CreateDirectory(strPath.c_str(), NULL);
	}

	std::wstring defaultIni = GetPath() + L"Default.ini";
	if (_waccess(defaultIni.c_str(), 0) == -1)
	{
		// The default.ini wasn't found -> create new
		std::ofstream out(strFile.c_str(), std::ios::out);
		if (out) 
		{
			out << std::string("[Rainmeter]\n\n[Gnometer\\Bars]\nActive=1\n\n[Gnometer\\Clock]\nActive=1\n");
			out.close();
		}
	}
	else
	{
		CSystem::CopyFiles(defaultIni, GetIniFile());
	}
}

void CRainmeter::ReloadSettings()
{
	ScanForConfigs(m_SkinPath);
	ScanForThemes(GetSettingsPath() + L"Themes");
	ReadGeneralSettings(m_IniFile);
}

void CRainmeter::ActivateConfig(int configIndex, int iniIndex)
{
	if (configIndex >= 0 && configIndex < (int)m_ConfigStrings.size())
	{
		const std::wstring skinIniFile = m_ConfigStrings[configIndex].iniFiles[iniIndex];
		const std::wstring skinConfig = m_ConfigStrings[configIndex].config;
		const std::wstring skinPath = m_ConfigStrings[configIndex].path;

		// Verify that the config is not already active
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Meters.find(skinConfig);
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

		// Verify whether the ini-file exists
		std::wstring skinIniPath = skinPath;
		skinIniPath += skinConfig;
		skinIniPath += L"\\";
		skinIniPath += skinIniFile;

		if (_waccess(skinIniPath.c_str(), 0) == -1)
		{
			std::wstring message = L"Unable to activate skin \"";
			message += skinConfig.c_str();
			message += L"\\";
			message += skinIniFile.c_str();
			message += L"\": Ini-file not found.";
			LSLog(LOG_DEBUG, APPNAME, message.c_str());
			MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			return;
		}

		m_ConfigStrings[configIndex].active = iniIndex + 1;
		WriteActive(skinConfig, iniIndex);

		try 
		{
			CreateMeterWindow(skinPath, skinConfig, skinIniFile);
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
	else if (configIndex == -1 && meterWindow)
	{
		// Deactivate the config by using the meter window's config name
		const std::wstring skinConfig = meterWindow->GetSkinName();
		for (size_t i = 0; i < m_ConfigStrings.size(); ++i)
		{
			if (_wcsicmp(skinConfig.c_str(), m_ConfigStrings[i].config.c_str()) == 0)
			{
				m_ConfigStrings[i].active = 0;
				break;
			}
		}
	}

	if (meterWindow)
	{
		// Disable the config in the ini-file
		WriteActive(meterWindow->GetSkinName(), -1);

		return DeleteMeterWindow(meterWindow, true);
	}
	return false;
}

void CRainmeter::WriteActive(const std::wstring& config, int iniIndex)
{
	WCHAR buffer[256];
	wsprintf(buffer, L"%i", iniIndex + 1);
	WritePrivateProfileString(config.c_str(), L"Active", buffer, m_IniFile.c_str());
}

void CRainmeter::CreateMeterWindow(std::wstring path, std::wstring config, std::wstring iniFile)
{
	CMeterWindow* mw = new CMeterWindow(path, config, iniFile);

	if (mw)
	{
		m_Meters[config] = mw;
		mw->Initialize(*this);

		UpdateAboutDialog();
	}
}

void CRainmeter::ClearDeleteLaterList()
{
	while (!m_DelayDeleteList.empty())
	{
		CMeterWindow* meterWindow = m_DelayDeleteList.front();

		// Remove from the delete later list
		m_DelayDeleteList.remove(meterWindow);

		// Remove from the meter window list if it is still there
		std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.begin();
		for (; iter != m_Meters.end(); ++iter)
		{
			if ((*iter).second == meterWindow)
			{
				m_Meters.erase(iter);

				UpdateAboutDialog();
				break;
			}
		}

		delete meterWindow;
	}
}

bool CRainmeter::DeleteMeterWindow(CMeterWindow* meterWindow, bool bLater)
{
	if (bLater)
	{
		m_DelayDeleteList.push_back(meterWindow);
		meterWindow->RunBang(BANG_HIDEFADE, NULL);	// Fade out the window
	}
	else
	{
		m_DelayDeleteList.remove(meterWindow);	// Remove the window from the delete later list if it is there

		std::map<std::wstring, CMeterWindow*>::iterator iter = m_Meters.begin();

		for (; iter != m_Meters.end(); ++iter)
		{
			if (meterWindow == NULL)
			{
				// Delete all meter windows
				delete (*iter).second;
			} 
			else if ((*iter).second == meterWindow)
			{
				m_Meters.erase(iter);
				delete meterWindow;

				UpdateAboutDialog();
				return true;
			}
		}

		if (meterWindow == NULL)
		{
			m_Meters.clear();
		}

		UpdateAboutDialog();
	}

	return false;
}

CMeterWindow* CRainmeter::GetMeterWindow(const std::wstring& config)
{
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Meters.begin();

	for (; iter != m_Meters.end(); ++iter)
	{
		if (_wcsicmp((*iter).first.c_str(), config.c_str()) == 0)
		{
			return (*iter).second;
		}
	}

	return NULL;
}

CMeterWindow* CRainmeter::GetMeterWindow(HWND hwnd)
{
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Meters.begin();

	for (; iter != m_Meters.end(); ++iter)
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
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Meters.begin();
	for (; iter != m_Meters.end(); ++iter)
	{
		CMeterWindow* mw = (*iter).second;
		if (mw && (group.empty() || mw->BelongsToGroup(group)))
		{
			windows.insert(std::pair<int, CMeterWindow*>(GetLoadOrder((*iter).first), mw));
		}
	}
}

void CRainmeter::SetConfigOrder(int configIndex)
{
	WCHAR buffer[256];
	int order;

	if (GetPrivateProfileString(m_ConfigStrings[configIndex].config.c_str(), L"LoadOrder", L"", buffer, 256, m_IniFile.c_str()) > 0)
	{
		if (_wcsicmp(buffer, L"LAST") == 0)
		{
			order = INT_MAX;
		}
		else if (_wcsicmp(buffer, L"FIRST") == 0)
		{
			order = INT_MIN;
		}
		else
		{
			order = _wtoi(buffer);
		}
	}
	else  // LoadOrder not exists
	{
		order = 0;
	}

	std::multimap<int, int>::iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		if ((*iter).second == configIndex)  // already exists
		{
			if ((*iter).first != order)
			{
				m_ConfigOrders.erase(iter);
				break;
			}
			else
			{
				return;
			}
		}
	}

	m_ConfigOrders.insert(std::pair<int, int>(order, configIndex));
}

int CRainmeter::GetLoadOrder(const std::wstring& config)
{
	std::multimap<int, int>::const_iterator iter = m_ConfigOrders.begin();
	for ( ; iter != m_ConfigOrders.end(); ++iter)
	{
		if (m_ConfigStrings[(*iter).second].config == config)
		{
			return (*iter).first;
		}
	}

	// LoadOrder not exists
	return 0;
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
		RemoveBangCommand("!RainmeterHideFade");
		RemoveBangCommand("!RainmeterShowFade");
		RemoveBangCommand("!RainmeterToggleFade");
		RemoveBangCommand("!RainmeterHideMeter");
		RemoveBangCommand("!RainmeterShowMeter");
		RemoveBangCommand("!RainmeterToggleMeter");
		RemoveBangCommand("!RainmeterMoveMeter");
		RemoveBangCommand("!RainmeterHideMeasure");
		RemoveBangCommand("!RainmeterShowMeasure");
		RemoveBangCommand("!RainmeterToggleMeasure");
		RemoveBangCommand("!RainmeterActivateConfig");
		RemoveBangCommand("!RainmeterDeactivateConfig");
		RemoveBangCommand("!RainmeterToggleConfig");
		RemoveBangCommand("!RainmeterMove");
		RemoveBangCommand("!RainmeterZPos");
		RemoveBangCommand("!RainmeterClickThrough");
		RemoveBangCommand("!RainmeterDraggable");
		RemoveBangCommand("!RainmeterSnapEdges");
		RemoveBangCommand("!RainmeterKeepOnScreen");
		RemoveBangCommand("!RainmeterSetTransparency");
		RemoveBangCommand("!RainmeterSetVariable");

		RemoveBangCommand("!RainmeterRefreshGroup");
		RemoveBangCommand("!RainmeterRedrawGroup");
		RemoveBangCommand("!RainmeterHideGroup");
		RemoveBangCommand("!RainmeterShowGroup");
		RemoveBangCommand("!RainmeterToggleGroup");
		RemoveBangCommand("!RainmeterHideFadeGroup");
		RemoveBangCommand("!RainmeterShowFadeGroup");
		RemoveBangCommand("!RainmeterToggleFadeGroup");
		RemoveBangCommand("!RainmeterHideMeterGroup");
		RemoveBangCommand("!RainmeterShowMeterGroup");
		RemoveBangCommand("!RainmeterToggleMeterGroup");
		RemoveBangCommand("!RainmeterHideMeasureGroup");
		RemoveBangCommand("!RainmeterShowMeasureGroup");
		RemoveBangCommand("!RainmeterToggleMeasureGroup");
		RemoveBangCommand("!RainmeterDeactivateConfigGroup");
		RemoveBangCommand("!RainmeterZPosGroup");
		RemoveBangCommand("!RainmeterClickThroughGroup");
		RemoveBangCommand("!RainmeterDraggableGroup");
		RemoveBangCommand("!RainmeterSnapEdgesGroup");
		RemoveBangCommand("!RainmeterKeepOnScreenGroup");
		RemoveBangCommand("!RainmeterSetTransparencyGroup");
		RemoveBangCommand("!RainmeterSetVariableGroup");

		RemoveBangCommand("!RainmeterRefreshApp");
		RemoveBangCommand("!RainmeterLsBoxHook");
		RemoveBangCommand("!RainmeterAbout");
		RemoveBangCommand("!RainmeterSkinMenu");
		RemoveBangCommand("!RainmeterTrayMenu");
		RemoveBangCommand("!RainmeterResetStats");
		RemoveBangCommand("!RainmeterWriteKeyValue");
		RemoveBangCommand("!RainmeterPluginBang");
		RemoveBangCommand("!RainmeterQuit");
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
	m_ConfigOrders.clear();

	ScanForConfigsRecursive(path, L"", 0, m_ConfigMenu, false);
}

int CRainmeter::ScanForConfigsRecursive(std::wstring& path, std::wstring base, int index, std::vector<CONFIGMENU>& menu, bool DontRecurse)
{
    WIN32_FIND_DATA fileData;      // Data structure describes the file found
    WIN32_FIND_DATA fileDataIni;   // Data structure describes the file found
    HANDLE hSearch;                // Search handle returned by FindFirstFile
    HANDLE hSearchIni;             // Search handle returned by FindFirstFile

	if (!base.empty())
	{
		// Scan for ini-files
		CONFIG config;
		config.path = path;
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

			// Check whether the extension is ".ini"
			std::wstring ext = fileDataIni.cFileName;
			std::wstring::size_type pos = ext.find_last_of(L'.');
			if (pos != std::wstring::npos && _wcsicmp(&(ext.c_str()[pos]), L".ini") == 0)
			{
				CONFIGMENU menuItem;
				menuItem.name = fileDataIni.cFileName;
				menuItem.index = m_ConfigStrings.size();
				menu.push_back(menuItem);

				config.iniFiles.push_back(fileDataIni.cFileName);
				config.commands.push_back(ID_CONFIG_FIRST + index++);
			}
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
			!(wcscmp(L"Backup", fileData.cFileName) == 0 && base.empty()) &&		// Skip the backup folder
			wcscmp(L".", fileData.cFileName) != 0 &&
			wcscmp(L"..", fileData.cFileName) != 0)
		{
			CONFIGMENU menuItem;
			menuItem.name = fileData.cFileName;
			menuItem.index = -1;
			menu.push_back(menuItem);

			if (!DontRecurse)
			{
				std::vector<CONFIGMENU>::iterator iter = menu.end() - 1;
				index = ScanForConfigsRecursive(path, base + fileData.cFileName, index, (*iter).children, false);

				// Remove menu item if it has no child
				if ((*iter).children.empty())
				{
					menu.erase(iter);
				}
			}
		}
	} while(FindNextFile(hSearch, &fileData));

    FindClose(hSearch);

	return index;
}

/* 
** ScanForThemes
**
** Scans the given folder for themes
*/
void CRainmeter::ScanForThemes(std::wstring& path)
{
	m_Themes.clear();

	WIN32_FIND_DATA fileData;      // Data structure describes the file found
    HANDLE hSearch;                // Search handle returned by FindFirstFile

	// Scan for folders
	std::wstring folders = path + L"\\*";
    hSearch = FindFirstFile(folders.c_str(), &fileData);
	do
	{
		if(hSearch == INVALID_HANDLE_VALUE) break;    // No more files found

		if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && 
			wcscmp(L".", fileData.cFileName) != 0 &&
			wcscmp(L"..", fileData.cFileName) != 0)
		{
			m_Themes.push_back(fileData.cFileName);
		}
	} while(FindNextFile(hSearch, &fileData));

    FindClose(hSearch);
}

void CRainmeter::SaveSettings()
{
	WritePrivateProfileString(L"Rainmeter", L"CheckUpdate", NULL , m_IniFile.c_str());
	WritePrivateProfileString(L"Rainmeter", L"DisableVersionCheck", m_DisableVersionCheck ? L"1" : L"0" , m_IniFile.c_str());
}

BOOL CRainmeter::ExecuteBang(const std::wstring& bang, const std::wstring& arg, CMeterWindow* meterWindow)
{
	if (_wcsicmp(bang.c_str(), L"!RainmeterRefresh") == 0)
	{
		BangWithArgs(BANG_REFRESH, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterRefreshApp") == 0)
	{
		RainmeterRefreshApp(NULL, NULL);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterRedraw") == 0)
	{
		BangWithArgs(BANG_REDRAW, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHide") == 0)
	{
		BangWithArgs(BANG_HIDE, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShow") == 0)
	{
		BangWithArgs(BANG_SHOW, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggle") == 0)
	{
		BangWithArgs(BANG_TOGGLE, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHideFade") == 0)
	{
		BangWithArgs(BANG_HIDEFADE, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShowFade") == 0)
	{
		BangWithArgs(BANG_SHOWFADE, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleFade") == 0)
	{
		BangWithArgs(BANG_TOGGLEFADE, arg.c_str(), 0);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHideMeter") == 0)
	{
		BangWithArgs(BANG_HIDEMETER, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShowMeter") == 0)
	{
		BangWithArgs(BANG_SHOWMETER, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleMeter") == 0)
	{
		BangWithArgs(BANG_TOGGLEMETER, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDisableMeasure") == 0)
	{
		BangWithArgs(BANG_DISABLEMEASURE, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterEnableMeasure") == 0)
	{
		BangWithArgs(BANG_ENABLEMEASURE, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleMeasure") == 0)
	{
		BangWithArgs(BANG_TOGGLEMEASURE, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterActivateConfig") == 0)
	{
		RainmeterActivateConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDeactivateConfig") == 0)
	{
		RainmeterDeactivateConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleConfig") == 0)
	{
		RainmeterToggleConfig(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterMove") == 0)
	{
		BangWithArgs(BANG_MOVE, arg.c_str(), 2);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterChangeZPos") == 0)	// For backwards compatibility
	{
		BangWithArgs(BANG_ZPOS, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterZPos") == 0)
	{
		BangWithArgs(BANG_ZPOS, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterClickThrough") == 0)
	{
		BangWithArgs(BANG_CLICKTHROUGH, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDraggable") == 0)
	{
		BangWithArgs(BANG_DRAGGABLE, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSnapEdges") == 0)
	{
		BangWithArgs(BANG_SNAPEDGES, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterKeepOnScreen") == 0)
	{
		BangWithArgs(BANG_KEEPONSCREEN, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSetTransparency") == 0)
	{
		BangWithArgs(BANG_SETTRANSPARENCY, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSetVariable") == 0)
	{
		BangWithArgs(BANG_SETVARIABLE, arg.c_str(), 2);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterRefreshGroup") == 0)
	{
		RainmeterRefreshGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterRedrawGroup") == 0)
	{
		RainmeterRedrawGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHideGroup") == 0)
	{
		RainmeterHideGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShowGroup") == 0)
	{
		RainmeterShowGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleGroup") == 0)
	{
		RainmeterToggleGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHideFadeGroup") == 0)
	{
		RainmeterHideFadeGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShowFadeGroup") == 0)
	{
		RainmeterShowFadeGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleFadeGroup") == 0)
	{
		RainmeterToggleFadeGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterHideMeterGroup") == 0)
	{
		BangWithArgs(BANG_HIDEMETERGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterShowMeterGroup") == 0)
	{
		BangWithArgs(BANG_SHOWMETERGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleMeterGroup") == 0)
	{
		BangWithArgs(BANG_TOGGLEMETERGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDisableMeasureGroup") == 0)
	{
		BangWithArgs(BANG_DISABLEMEASUREGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterEnableMeasureGroup") == 0)
	{
		BangWithArgs(BANG_ENABLEMEASUREGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterToggleMeasureGroup") == 0)
	{
		BangWithArgs(BANG_TOGGLEMEASUREGROUP, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDeactivateConfigGroup") == 0)
	{
		RainmeterDeactivateConfigGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterZPosGroup") == 0)
	{
		RainmeterZPosGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterClickThroughGroup") == 0)
	{
		RainmeterClickThroughGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterDraggableGroup") == 0)
	{
		RainmeterDraggableGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSnapEdgesGroup") == 0)
	{
		RainmeterSnapEdgesGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterKeepOnScreenGroup") == 0)
	{
		RainmeterKeepOnScreenGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSetTransparencyGroup") == 0)
	{
		RainmeterSetTransparencyGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSetVariableGroup") == 0)
	{
		RainmeterSetVariableGroup(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterAbout") == 0)
	{
		RainmeterAbout(NULL, NULL);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterSkinMenu") == 0)
	{
		RainmeterSkinMenu(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterTrayMenu") == 0)
	{
		RainmeterTrayMenu(NULL, NULL);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterResetStats") == 0)
	{
		RainmeterResetStats(NULL, NULL);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterMoveMeter") == 0)
	{
		BangWithArgs(BANG_MOVEMETER, arg.c_str(), 3);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterWriteKeyValue") == 0)
	{
		RainmeterWriteKeyValue(NULL, ConvertToAscii(arg.c_str()).c_str());
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterPluginBang") == 0)
	{
		BangWithArgs(BANG_PLUGIN, arg.c_str(), 1);
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterLsBoxHook") == 0)
	{
		// Nothing to do here (this works only with Litestep)
	}
	else if (_wcsicmp(bang.c_str(), L"!RainmeterQuit") == 0)
	{
		RainmeterQuit(NULL, NULL);
	}
	else if (_wcsicmp(bang.c_str(), L"!Execute") == 0)
	{
		// Special case for multibang execution
		std::wstring::size_type start = std::wstring::npos;
		std::wstring::size_type end = std::wstring::npos;
		int count = 0;
		for (size_t i = 0; i < arg.size(); ++i)
		{
			if (arg[i] == L'[')
			{
				if (count == 0)
				{
					start = i;
				}
				++count;
			}
			else if (arg[i] == L']')
			{
				--count;

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
		MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
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

	if (_wcsnicmp(L"!execute", command, 8) == 0)
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
							std::list<CMeasure*>::const_iterator iter = meterWindow->GetMeasures().begin();
							for( ; iter != meterWindow->GetMeasures().end(); ++iter)
							{
								if (_wcsicmp((*iter)->GetName(), measureName.c_str()) == 0)
								{
									std::wstring value = (*iter)->GetStringValue(false, 1, -1, false);
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
		// Check for built-ins
		if (_wcsnicmp(L"PLAY ", strCommand.c_str(), 5) == 0 ||
			_wcsnicmp(L"PLAYLOOP ", strCommand.c_str(), 9) == 0)
		{
			// Strip built-in command
			size_t pos = strCommand.find(L' ');
			strCommand.erase(0, pos + 1);

			if (!strCommand.empty())
			{
				DWORD flags = SND_FILENAME | SND_ASYNC;
				if (pos == 8)  // PLAYLOOP
				{
					flags |= SND_LOOP | SND_NODEFAULT;
				}

				// Strip the quotes
				std::wstring::size_type len = strCommand.length();
				if (len >= 2 && strCommand[0] == L'\"' && strCommand[len - 1] == L'\"')
				{
					strCommand.swap(strCommand.substr(1, len - 2));
				}

				PlaySound(strCommand.c_str(), NULL, flags);
			}
			return;
		}
		else if (wcsncmp(L"PLAYSTOP", strCommand.c_str(), 8) == 0)
		{
			PlaySound(NULL, NULL, SND_PURGE);
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
				meterWindow->OnCopyData(WM_COPYDATA, NULL, (LPARAM)&CopyDataStruct);
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
	// Clear old settings
	m_DesktopWorkAreas.clear();

	CConfigParser parser;
	parser.Initialize(iniFile.c_str(), this);

	// Read Logging settings
	m_Logging = 0!=parser.ReadInt(L"Rainmeter", L"Logging", 0);
	c_Debug = 0!=parser.ReadInt(L"Rainmeter", L"Debug", 0);

	if (m_Logging)
	{
		StartLogging();
	}

	if (m_TrayWindow)
	{
		m_TrayWindow->ReadConfig(parser);
	}

	c_GlobalConfig.netInSpeed = parser.ReadFloat(L"Rainmeter", L"NetInSpeed", 0.0);
	c_GlobalConfig.netOutSpeed = parser.ReadFloat(L"Rainmeter", L"NetOutSpeed", 0.0);

	m_ConfigEditor = parser.ReadString(L"Rainmeter", L"ConfigEditor", L"");
	if (m_ConfigEditor.empty())
	{
		// Get the program path associated with .ini files
		WCHAR buffer[MAX_PATH] = {0};
		DWORD cchOut = MAX_PATH;

		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".ini", L"open", buffer, &cchOut);
		if (SUCCEEDED(hr) && cchOut > 0)
		{
			m_ConfigEditor = buffer;
		}
		else
		{
			m_ConfigEditor = L"Notepad";
		}
	}
	if (!m_ConfigEditor.empty() && m_ConfigEditor[0] != L'\"')
	{
		m_ConfigEditor.insert(0, L"\"");
		m_ConfigEditor.append(L"\"");
	}

	m_LogViewer = parser.ReadString(L"Rainmeter", L"LogViewer", L"");
	if (m_LogViewer.empty())
	{
		// Get the program path associated with .log files
		WCHAR buffer[MAX_PATH] = {0};
		DWORD cchOut = MAX_PATH;

		HRESULT hr = AssocQueryString(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".log", L"open", buffer, &cchOut);
		if (SUCCEEDED(hr) && cchOut > 0)
		{
			m_LogViewer = buffer;
		}
		else
		{
			m_LogViewer = L"Notepad";
		}
	}
	if (!m_LogViewer.empty() && m_LogViewer[0] != L'\"')
	{
		m_LogViewer.insert(0, L"\"");
		m_LogViewer.append(L"\"");
	}

	if (c_Debug)
	{
		DebugLog(L"ConfigEditor: %s", m_ConfigEditor.c_str());
		DebugLog(L"LogViewer: %s", m_LogViewer.c_str());
	}

	m_TrayExecuteL = parser.ReadString(L"Rainmeter", L"TrayExecuteL", L"", false);
	m_TrayExecuteR = parser.ReadString(L"Rainmeter", L"TrayExecuteR", L"", false);
	m_TrayExecuteM = parser.ReadString(L"Rainmeter", L"TrayExecuteM", L"", false);
	m_TrayExecuteDL = parser.ReadString(L"Rainmeter", L"TrayExecuteDL", L"", false);
	m_TrayExecuteDR = parser.ReadString(L"Rainmeter", L"TrayExecuteDR", L"", false);
	m_TrayExecuteDM = parser.ReadString(L"Rainmeter", L"TrayExecuteDM", L"", false);

	m_DisableVersionCheck = 0!=parser.ReadInt(L"Rainmeter", L"DisableVersionCheck", 0);

	std::wstring area = parser.ReadString(L"Rainmeter", L"DesktopWorkArea", L"");
	if (!area.empty())
	{
		RECT r;
		swscanf(area.c_str(), L"%i,%i,%i,%i", &r.left, &r.top, &r.right, &r.bottom);
		m_DesktopWorkAreas[0] = r;
		m_DesktopWorkAreaChanged = true;
	}

	for (UINT i = 1; i <= CSystem::GetMonitorCount(); ++i)
	{
		WCHAR buffer[64];
		wsprintf(buffer, L"DesktopWorkArea@%i", i);
		area = parser.ReadString(L"Rainmeter", buffer, L"");
		if (!area.empty())
		{
			RECT r;
			swscanf(area.c_str(), L"%i,%i,%i,%i", &r.left, &r.top, &r.right, &r.bottom);
			m_DesktopWorkAreas[i] = r;
			m_DesktopWorkAreaChanged = true;
		}
	}

	m_DesktopWorkAreaType = 0!=parser.ReadInt(L"Rainmeter", L"DesktopWorkAreaType", 0);

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
				error = L"The selected skin (L" + skinName + L"\\" + skinIni + L") cannot be found.";
				MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			}
			return;
		}
	}

	for (size_t i = 0; i < m_ConfigStrings.size(); ++i)
	{
		int active  = parser.ReadInt(m_ConfigStrings[i].config.c_str(), L"Active", 0);

		// Make sure there is a ini file available
		if (active > 0 && active <= (int)m_ConfigStrings[i].iniFiles.size())
		{
			m_ConfigStrings[i].active = active;
		}

		SetConfigOrder(i);
	}
}

/* 
** SetActiveConfig
**
** Makes the given config active. If the config cannot be found this returns false.
*/
bool CRainmeter::SetActiveConfig(std::wstring& skinName, std::wstring& skinIni)
{
	for (size_t i = 0; i < m_ConfigStrings.size(); ++i)
	{
		m_ConfigStrings[i].active = 0;	// Disable all other configs

		if (skinName == m_ConfigStrings[i].config)
		{
			for (size_t j = 0; j < m_ConfigStrings[i].iniFiles.size(); ++j)
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
** RefreshAll
**
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
	CSystem::PrepareHelperWindow(CSystem::GetWorkerW());

	// Refresh all
	std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
	for ( ; iter != windows.end(); ++iter)
	{
		CMeterWindow* mw = (*iter).second;
		if (mw)
		{
			// Verify whether the cached information is valid
			int found = 0;
			std::wstring skinConfig = mw->GetSkinName();
			for (size_t i = 0; i < m_ConfigStrings.size(); ++i)
			{
				if (_wcsicmp(skinConfig.c_str(), m_ConfigStrings[i].config.c_str()) == 0)
				{
					found = 1;
					std::wstring skinIniFile = mw->GetSkinIniFile();
					for (size_t j = 0; j < m_ConfigStrings[i].iniFiles.size(); ++j)
					{
						if (_wcsicmp(skinIniFile.c_str(), m_ConfigStrings[i].iniFiles[j].c_str()) == 0)
						{
							found = 2;
							if (m_ConfigStrings[i].active != j + 1)
							{
								// Switch to new ini-file order
								m_ConfigStrings[i].active = j + 1;
								WriteActive(skinConfig, j);
							}
							break;
						}
					}

					if (found == 1)  // Not found in ini-files
					{
						DeactivateConfig(mw, i);

						std::wstring message = L"Unable to refresh skin \"";
						message += skinConfig.c_str();
						message += L"\\";
						message += skinIniFile.c_str();
						message += L"\": Ini-file not found.";
						LSLog(LOG_DEBUG, APPNAME, message.c_str());
						MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
					}
					break;
				}
			}

			if (found != 2)
			{
				if (found == 0)  // Not found in configs
				{
					DeactivateConfig(mw, -2);  // -2 = Deactivate the config forcibly

					std::wstring message = L"Unable to refresh config \"";
					message += skinConfig.c_str();
					message += L"\": Config not found.";
					LSLog(LOG_DEBUG, APPNAME, message.c_str());
					MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}
				continue;
			}

			try
			{
				mw->Refresh(false, true);
			}
			catch (CError& error)
			{
				MessageBox(mw->GetWindow(), error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			}
		}
	}
}

/* 
** UpdateDesktopWorkArea
**
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
			for (size_t i = 0; i < m_OldDesktopWorkAreas.size(); ++i)
			{
				RECT r = m_OldDesktopWorkAreas[i];

				BOOL result = SystemParametersInfo(SPI_SETWORKAREA, 0, &r, 0);

				if (c_Debug)
				{
					std::wstring format = L"Resetting WorkArea@%i: L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)";
					if (!result)
					{
						format += L" => FAIL.";
					}
					DebugLog(format.c_str(), i + 1, r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
			changed = true;
		}
	}
	else
	{
		const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		if (m_OldDesktopWorkAreas.empty())
		{
			// Store old work areas for changing them back
			for (UINT i = 0; i < CSystem::GetMonitorCount(); ++i)
			{
				m_OldDesktopWorkAreas.push_back(monitors[i].work);
			}
		}

		if (c_Debug)
		{
			DebugLog(L"DesktopWorkAreaType: %s", m_DesktopWorkAreaType ? L"Margin" : L"Default");
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

				if (c_Debug)
				{
					std::wstring format = L"Applying DesktopWorkArea";
					if (i != 0)
					{
						WCHAR buffer[64];
						wsprintf(buffer, L"@%i", i);
						format += buffer;
					}
					format += L": L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)";
					if (!result)
					{
						format += L" => FAIL.";
					}
					DebugLog(format.c_str(), r.left, r.top, r.right, r.bottom, r.right - r.left, r.bottom - r.top);
				}
			}
		}
	}

	if (changed && CSystem::GetWindow())
	{
		// Update CSystem::MULTIMONITOR_INFO for for work area variables
		SendMessageTimeout(CSystem::GetWindow(), WM_SETTINGCHANGE, SPI_SETWORKAREA, 0, SMTO_ABORTIFHUNG, 1000, NULL);
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
** Writes the statistics to the ini-file. If bForce is false the stats are written only once per minute.
**
*/
void CRainmeter::WriteStats(bool bForce)
{
	static DWORD lastWrite = 0;

	if (bForce || (lastWrite + 1000 * 60 < GetTickCount()))
	{
		lastWrite = GetTickCount();

		// Write the date for statistics
		WritePrivateProfileString(L"Statistics", L"Since", m_StatsDate.c_str(), m_IniFile.c_str());

		// Only Net measure has stats at the moment
		CMeasureNet::WriteStats(m_IniFile);

		WritePrivateProfileString(NULL, NULL, NULL, m_IniFile.c_str());
	}
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
** ShowContextMenu
**
** Opens the context menu in given coordinates.
**
*/
void CRainmeter::ShowContextMenu(POINT pos, CMeterWindow* meterWindow) 
{
	if (!m_MenuActive)
	{
		m_MenuActive = true;

		// Show context menu, if no actions were executed
		HMENU menu = LoadMenu(m_Instance, MAKEINTRESOURCE(IDR_CONTEXT_MENU));

		if(menu)
		{
			HMENU subMenu = GetSubMenu(menu, 0);
			if(subMenu)
			{
				if (!GetDummyLitestep())
				{
					// Disable Quit/Logging if ran as a Litestep plugin
					EnableMenuItem(subMenu, ID_CONTEXT_QUIT, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(subMenu, 6, MF_BYPOSITION | MF_GRAYED);  // "Logging" menu
				}
				else
				{
					if (_waccess(m_LogFile.c_str(), 0) == -1)
					{
						EnableMenuItem(subMenu, ID_CONTEXT_SHOWLOGFILE, MF_BYCOMMAND | MF_GRAYED);
						EnableMenuItem(subMenu, ID_CONTEXT_DELETELOGFILE, MF_BYCOMMAND | MF_GRAYED);
						EnableMenuItem(subMenu, ID_CONTEXT_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
					}
					else
					{
						if (m_Logging)
						{
							EnableMenuItem(subMenu, ID_CONTEXT_STARTLOG, MF_BYCOMMAND | MF_GRAYED);
						}
						else
						{
							EnableMenuItem(subMenu, ID_CONTEXT_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
						}
					}

					if (c_Debug)
					{
						CheckMenuItem(subMenu, ID_CONTEXT_DEBUGLOG, MF_BYCOMMAND | MF_CHECKED);
					}
				}

				HMENU configMenu = CreateConfigMenu(m_ConfigMenu);
				if (!configMenu)
				{
					configMenu = CreatePopupMenu();
					AppendMenu(configMenu, MF_GRAYED, 0, L"No configs available");
				}
				if (configMenu)
				{
					AppendMenu(configMenu, MF_SEPARATOR, 0, NULL);
					AppendMenu(configMenu, 0, ID_CONTEXT_OPENSKINSFOLDER, L"Open Skins\' Folder");
					AppendMenu(configMenu, 0, ID_CONTEXT_MANAGESKINS, L"Manage Skins...");

					InsertMenu(subMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)configMenu, L"Configs");
				}

				HMENU themeMenu = CreateThemeMenu();
				if (themeMenu)
				{
					InsertMenu(subMenu, 4, MF_BYPOSITION | MF_POPUP, (UINT_PTR)themeMenu, L"Themes");
					InsertMenu(subMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
				}

				if (meterWindow)
				{
					HMENU rainmeterMenu = subMenu;
					subMenu = CreateSkinMenu(meterWindow, 0, configMenu);
					InsertMenu(subMenu, 9, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
					InsertMenu(subMenu, 10, MF_BYPOSITION | MF_POPUP, (UINT_PTR)rainmeterMenu, L"Rainmeter Menu");
				}
				else
				{
					InsertMenu(subMenu, 11, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

					// Create a menu for all active configs
					std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();

					int index = 0;
					for (; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
					{
						CMeterWindow* mw = ((*iter).second);
						HMENU skinMenu = CreateSkinMenu(mw, index, configMenu);
						InsertMenu(subMenu, 11, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, mw->GetSkinName().c_str());
						++index;
					}

					// Put Update notifications in the Tray menu
					if (m_NewVersion)
					{
						InsertMenu(subMenu, 0, MF_BYPOSITION, ID_CONTEXT_NEW_VERSION, L"New Version Available");
						InsertMenu(subMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
						SetMenuDefaultItem(subMenu, ID_CONTEXT_NEW_VERSION, MF_BYCOMMAND);
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
				  TPM_RIGHTBUTTON | TPM_LEFTALIGN, 
				  pos.x,
				  pos.y,
				  0,
				  hWnd,
				  NULL
				);

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


HMENU CRainmeter::CreateConfigMenu(std::vector<CONFIGMENU>& configMenuData)
{
	HMENU configMenu = NULL;

	if (configMenuData.size() > 0)
	{
		configMenu = CreatePopupMenu();

		for (size_t i = 0; i < configMenuData.size(); ++i)
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

HMENU CRainmeter::CreateThemeMenu()
{
	HMENU themeMenu = CreatePopupMenu();

	for (size_t i = 0; i < m_Themes.size(); ++i)
	{
		AppendMenu(themeMenu, 0, ID_THEME_FIRST + i, m_Themes[i].c_str());
	}

	if (!m_Themes.empty())
	{
		AppendMenu(themeMenu, MF_SEPARATOR, 0, NULL);
	}

	AppendMenu(themeMenu, 0, ID_CONTEXT_MANAGETHEMES, L"Manage Themes...");

	return themeMenu;
}

HMENU CRainmeter::CreateSkinMenu(CMeterWindow* meterWindow, int index, HMENU configMenu)
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
		HMENU settingsMenu = GetSubMenu(skinMenu, 0);
		if (settingsMenu)
		{
			HMENU posMenu = GetSubMenu(settingsMenu, 0);
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

				HMENU monitorMenu = GetSubMenu(posMenu, 0);
				if (monitorMenu)
				{
					CreateMonitorMenu(monitorMenu, meterWindow);
				}
			}
		}

		// Tick the transparency
		if (!meterWindow->GetNativeTransparency())
		{
			EnableMenuItem(settingsMenu, 1, MF_BYPOSITION | MF_GRAYED);  // "Transparency" menu
			EnableMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_CLICKTHROUGH, MF_BYCOMMAND | MF_GRAYED);
		}
		else
		{
			HMENU alphaMenu = GetSubMenu(settingsMenu, 1);
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
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_HIDEONMOUSE, MF_BYCOMMAND | MF_CHECKED);
		}
		else if (meterWindow->GetWindowHide() != HIDEMODE_NONE)
		{
			EnableMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_HIDEONMOUSE, MF_BYCOMMAND | MF_GRAYED);
		}

		if (meterWindow->GetSnapEdges())
		{
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_SNAPTOEDGES, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetSavePosition())
		{
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_REMEMBERPOSITION, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetWindowDraggable())
		{
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_DRAGGABLE, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetClickThrough())
		{
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_CLICKTHROUGH, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetKeepOnScreen())
		{
			CheckMenuItem(settingsMenu, ID_CONTEXT_SKINMENU_KEEPONSCREEN, MF_BYCOMMAND | MF_CHECKED);
		}

		// Add the name of the Skin to the menu and disable the item
		const std::wstring& skinName = meterWindow->GetSkinName();
		InsertMenu(skinMenu, 0, MF_BYPOSITION, ID_CONTEXT_SKINMENU_OPENSKINSFOLDER, skinName.c_str());
		InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		SetMenuDefaultItem(skinMenu, 0, MF_BYPOSITION);
	
		ChangeSkinIndex(skinMenu, index);

		// Add the variants menu
		for (size_t i = 0; i < m_ConfigStrings.size(); ++i)
		{
			const CONFIG& config = m_ConfigStrings[i];
			if (_wcsicmp(config.config.c_str(), skinName.c_str()) == 0)
			{
				HMENU variantsMenu = CreatePopupMenu();
				for (size_t j = 0; j < config.iniFiles.size(); ++j)
				{
					InsertMenu(variantsMenu, j, MF_BYPOSITION, config.commands[j], config.iniFiles[j].c_str());

					if (config.active == j + 1)
					{
						CheckMenuItem(variantsMenu, j, MF_BYPOSITION | MF_CHECKED);
					}
				}
				InsertMenu(skinMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)variantsMenu, L"Variants");
				break;
			}
		}

		// Add config's root menu
		int itemCount = GetMenuItemCount(configMenu) - 3;  // Subtract 3 for appended menus
		if (itemCount > 0)
		{
			std::wstring root = meterWindow->GetSkinName();
			std::wstring::size_type pos = root.find_first_of(L'\\');
			if (pos != std::wstring::npos)
			{
				root.erase(pos);
			}

			for (int i = 0; i < itemCount; ++i)
			{
				WCHAR buffer[MAX_PATH] = {0};
				MENUITEMINFO itemInfo = {sizeof(MENUITEMINFO)};
				itemInfo.fMask = MIIM_STRING;
				itemInfo.dwTypeData = buffer;
				itemInfo.cch = MAX_PATH;
				if (GetMenuItemInfo(configMenu, (UINT)i, TRUE, &itemInfo))
				{
					if (_wcsicmp(root.c_str(), buffer) == 0)
					{
						HMENU configRootMenu = GetSubMenu(configMenu, i);
						if (configRootMenu)
						{
							InsertMenu(skinMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)configRootMenu, root.c_str());
							InsertMenu(skinMenu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
						}
						break;
					}
				}
			}
		}
	}

	return skinMenu;
}

void CRainmeter::CreateMonitorMenu(HMENU monitorMenu, CMeterWindow* meterWindow)
{
	bool screenDefined = meterWindow->GetXScreenDefined();
	int screenIndex = meterWindow->GetXScreen();

	// for the "Specified monitor" (@n)
	if (CSystem::GetMonitorCount() > 0)
	{
		const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		for (size_t i = 0; i < monitors.size(); ++i)
		{
			WCHAR buffer[64];
			wsprintf(buffer, L"@%i: ", i + 1);
			std::wstring item = buffer;

			size_t len = wcslen(monitors[i].monitorName);
			if (len > 32)
			{
				item += std::wstring(monitors[i].monitorName, 32);
				item += L"...";
			}
			else
			{
				item += monitors[i].monitorName;
			}

			UINT pos = i + 3;
			InsertMenu(monitorMenu, pos, MF_BYPOSITION, ID_MONITOR_FIRST + i + 1, item.c_str());

			if (screenDefined && screenIndex == (int)i + 1)
			{
				CheckMenuItem(monitorMenu, pos, MF_BYPOSITION | MF_CHECKED);
			}

			if (!monitors[i].active)
			{
				EnableMenuItem(monitorMenu, pos, MF_BYPOSITION | MF_GRAYED);
			}
		}
	}

	// Tick the configs
	if (!screenDefined)
	{
		CheckMenuItem(monitorMenu, ID_CONTEXT_SKINMENU_MONITOR_PRIMARY, MF_BYCOMMAND | MF_CHECKED);
	}

	if (screenDefined && screenIndex == 0)
	{
		CheckMenuItem(monitorMenu, ID_MONITOR_FIRST, MF_BYCOMMAND | MF_CHECKED);
	}

	if (meterWindow->GetAutoSelectScreen())
	{
		CheckMenuItem(monitorMenu, ID_CONTEXT_SKINMENU_MONITOR_AUTOSELECT, MF_BYCOMMAND | MF_CHECKED);
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
	if (_waccess(m_LogFile.c_str(), 0) == -1)
	{
		// Create log file
		HANDLE file = CreateFile(m_LogFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			ResetLoggingFlag();	// Re-enable logging
			SetLogging(true);

			std::wstring message = L"Log file created at: ";
			message += m_LogFile;
			MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		}
		else
		{
			// Disable logging
			SetLogging(false);
			ResetLoggingFlag();

			std::wstring message = L"Unable to create log file: ";
			message += m_LogFile;
			MessageBox(NULL, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
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

void CRainmeter::DeleteLogFile()
{
	// Check if the file exists
	if (_waccess(m_LogFile.c_str(), 0) != -1)
	{
		std::wstring message = L"Do you want to delete the following log file?\n";
		message += m_LogFile;
		int res = MessageBox(NULL, message.c_str(), L"Rainmeter", MB_YESNO | MB_TOPMOST | MB_ICONQUESTION);
		if (res == IDYES)
		{
			// Disable logging
			SetLogging(false);
			ResetLoggingFlag();

			CSystem::RemoveFile(m_LogFile);
		}
	}
}

void CRainmeter::AddAboutLogInfo(const LOG_INFO& logInfo)
{
	EnterCriticalSection(&m_CsLogData);

	m_LogData.push_front(logInfo);
	if (m_LogData.size() > MAXABOUTLOGLINES)
	{
		m_LogData.pop_back();
	}

	LeaveCriticalSection(&m_CsLogData);
}

void CRainmeter::SetLogging(bool logging)
{
	m_Logging = logging;
	WritePrivateProfileString(L"Rainmeter", L"Logging", logging ? L"1" : L"0", m_IniFile.c_str());
}

void CRainmeter::SetDebug(bool debug)
{
	c_Debug = debug;
	WritePrivateProfileString(L"Rainmeter", L"Debug", debug ? L"1" : L"0", m_IniFile.c_str());
}

void CRainmeter::TestSettingsFile(bool bDefaultIniLocation)
{
	WritePrivateProfileString(L"Rainmeter", L"WriteTest", L"TRUE", m_IniFile.c_str());
	WritePrivateProfileString(NULL, NULL, NULL, m_IniFile.c_str());	// FLUSH

	WCHAR tmpSz[5];
	bool bSuccess = (GetPrivateProfileString(L"Rainmeter", L"WriteTest", L"", tmpSz, 5, m_IniFile.c_str()) > 0);
	if (bSuccess)
	{
		bSuccess = (wcscmp(L"TRUE", tmpSz) == 0);
		WritePrivateProfileString(L"Rainmeter", L"WriteTest", NULL, m_IniFile.c_str());	
	}
	if (!bSuccess)
	{
		DebugLog(L"The Rainmeter.ini file is NOT writable.");

		std::wstring error;
		error += L"The Rainmeter.ini file is not writable. This means that the\n";
		error += L"application will not be able to save any settings permanently.\n\n";

		if (!bDefaultIniLocation)
		{
			std::wstring strTarget = L"%APPDATA%\\Rainmeter\\";
			ExpandEnvironmentVariables(strTarget);

			error += L"You should quit Rainmeter and move the settings file from\n\n";
			error += m_IniFile;
			error += L"\n\nto\n\n";
			error += strTarget;
			error += L"\n\nAlternatively you can simply remove the file and\n";
			error += L"it will be automatically recreated in the correct location\n";
			error += L"when Rainmeter is restarted the next time (you\'ll lose your\n";
			error += L"current settings though).\n";
		}
		else
		{
			error += L"Make sure that the settings file is not set as read-only and\n";
			error += L"that it is located in a folder where you have write permissions.\n\n";
			error += L"The settings file is located at:\n";
			error += m_IniFile;
		}

		MessageBox(NULL, error.c_str(), APPNAME, MB_OK | MB_ICONERROR);
	}
	else
	{
		DebugLog(L"The Rainmeter.ini file is writable.");
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
	if (strPath.find(L'%') != std::wstring::npos)
	{
		WCHAR buffer[4096];	// lets hope the buffer is large enough...

		// %APPDATA% is a special case
		std::wstring::size_type pos = strPath.find(L"%APPDATA%");
		if (pos != std::wstring::npos)
		{
			HRESULT hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
			if (SUCCEEDED(hr))
			{
				std::wstring path = buffer;
				do
				{
					strPath.replace(pos, 9, path);
				}
				while ((pos = strPath.find(L"%APPDATA%", pos + path.length())) != std::wstring::npos);
			}
		}

		if (strPath.find(L'%') != std::wstring::npos)
		{
			// Expand the environment variables
			DWORD ret = ExpandEnvironmentStrings(strPath.c_str(), buffer, 4096);
			if (ret != 0 && ret < 4096)
			{
				strPath = buffer;
			}
			else
			{
				DebugLog(L"Unable to expand the environment strings for string: %s", strPath.c_str());
			}
		}
	}
}
