/*
  Copyright (C) 2011 Birunthan Mohanathas, Peter Souza

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
#include "Rainmeter.h"
#include "Export.h"
#include "MeterWindow.h"
#include "Measure.h"
#include "MeasurePlugin.h"

#define NULLCHECK(str) { if ((str) == NULL) { (str) = L""; } }

extern CRainmeter* Rainmeter;

static std::wstring g_Buffer;

LPCWSTR RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	NULLCHECK(option);
	NULLCHECK(defValue);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	CConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadString(measure->GetName(), option, defValue, (bool)replaceMeasures).c_str();
}

double RmReadFormula(void* rm, LPCWSTR option, double defValue)
{
	NULLCHECK(option);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	CConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadFormula(measure->GetName(), option, defValue);
}

LPCWSTR RmPathToAbsolute(void* rm, LPCWSTR relativePath)
{
	NULLCHECK(relativePath);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	g_Buffer = relativePath;
	measure->GetMeterWindow()->MakePathAbsolute(g_Buffer);
	return g_Buffer.c_str();
}

void* RmGet(void* rm, int type)
{
	CMeasurePlugin* measure = (CMeasurePlugin*)rm;

	switch (type)
	{
	case RMG_MEASURENAME:
		{
			return (void*)measure->GetName();
		}

	case RMG_SKIN:
		{
			return (void*)measure->GetMeterWindow();
		}

	case RMG_SETTINGSFILE:
		{
			g_Buffer = Rainmeter->GetSettingsPath();
			g_Buffer += L"Plugins.ini";
			return (void*)g_Buffer.c_str();
		}
	}

	return NULL;
}

void RmExecute(void* skin, LPCWSTR command)
{
	CMeterWindow* mw = (CMeterWindow*)skin;

	// Fake WM_COPYDATA message to deliver bang
	COPYDATASTRUCT cds;
	cds.cbData = 1;
	cds.dwData = 1;
	cds.lpData = (void*)command;
	mw->OnCopyData(WM_COPYDATA, NULL, (LPARAM)&cds);
}

BOOL LSLog(int nLevel, LPCWSTR unused, LPCWSTR pszMessage)
{
	NULLCHECK(pszMessage);

	// Ignore LOG_DEBUG messages from plugins unless in debug mode
	if (nLevel != LOG_DEBUG || Rainmeter->GetDebug())
	{
		Log(nLevel, pszMessage);
	}

	return TRUE;
}

// Deprecated!
LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue)
{
	NULLCHECK(section);
	NULLCHECK(option);
	NULLCHECK(defValue);

	CConfigParser* parser = Rainmeter->GetCurrentParser();
	if (parser)
	{
		return parser->ReadString(section, option, defValue, false).c_str();
	}

	return defValue;
}

// Deprecated!
LPCWSTR PluginBridge(LPCWSTR _sCommand, LPCWSTR _sData)
{
	if (_sCommand == NULL || *_sCommand == L'\0')
	{
		return L"noop";
	}

	NULLCHECK(_sData);

	std::wstring sCommand = _sCommand;
	std::transform(sCommand.begin(), sCommand.end(), sCommand.begin(), ::towlower);

	// Command       GetConfig
	// Data          unquoted full path and filename given to the plugin on initialize
	//               (note: this is CaSe-SeNsItIvE!)
	// Execution     none
	// Result        the config name if found or a blank string if not
	if (sCommand == L"getconfig")
	{
		// returns the config name, lookup by INI file

		CMeterWindow *meterWindow = Rainmeter->GetMeterWindowByINI(_sData);
		if (meterWindow)
		{
			g_Buffer = L"\"";
			g_Buffer += meterWindow->GetSkinName();
			g_Buffer += L"\"";
			return g_Buffer.c_str();
		}

		return L"";
	}

	// Command       GetWindow
	// Data          [the config name]
	// Execution     none
	// Result        the HWND to the specified config window if found, 'error' otherwise
	if (sCommand == L"getwindow")
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(_sData);

		if (subStrings.size() >= 1)
		{
			const std::wstring& config = subStrings[0];

			CMeterWindow *meterWindow = Rainmeter->GetMeterWindow(config);
			if (meterWindow)
			{
				WCHAR buf1[64];
				_snwprintf_s(buf1, _TRUNCATE, L"%lu", PtrToUlong(meterWindow->GetWindow()));
				g_Buffer = buf1;
				return g_Buffer.c_str();
			}
		}

		return L"error";
	}

	// Command       GetVariable
	// Data          [the config name]
	// Execution     none
	// Result        the value of the variable
	if (sCommand == L"getvariable")
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(_sData);

		if (subStrings.size() >= 2)
		{
			const std::wstring& config = subStrings[0];

			CMeterWindow *meterWindow = Rainmeter->GetMeterWindow(config);
			if (meterWindow)
			{
				const std::wstring& variable = subStrings[1];
				std::wstring result_from_parser;

				if (meterWindow->GetParser().GetVariable(variable, result_from_parser))
				{
					g_Buffer = result_from_parser;
					return g_Buffer.c_str();
				}
			}
		}

		return L"";
	}

	// Command       SetVariable
	// Data          [the config name] [variable data]
	// Execution     the indicated variable is updated
	// Result        'success' if the config was found, 'error' otherwise
	if (sCommand == L"setvariable")
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(_sData);

		if (subStrings.size() >= 2)
		{
			const std::wstring& config = subStrings[0];
			std::wstring arguments;

			for (size_t i = 1, isize = subStrings.size(); i < isize; ++i)
			{
				if (i != 1) arguments += L" ";
				arguments += subStrings[i];
			}

			CMeterWindow *meterWindow = Rainmeter->GetMeterWindow(config);
			if (meterWindow)
			{
				meterWindow->RunBang(BANG_SETVARIABLE, arguments.c_str());
				return L"success";
			}
		}

		return L"error";
	}

	return L"noop";
}
