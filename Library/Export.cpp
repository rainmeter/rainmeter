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

LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	NULLCHECK(option);
	NULLCHECK(defValue);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	CConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadString(measure->GetName(), option, defValue, (bool)replaceMeasures).c_str();
}

double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue)
{
	NULLCHECK(option);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	CConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadFormula(measure->GetName(), option, defValue);
}

LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath)
{
	NULLCHECK(relativePath);

	CMeasurePlugin* measure = (CMeasurePlugin*)rm;
	g_Buffer = relativePath;
	measure->GetMeterWindow()->MakePathAbsolute(g_Buffer);
	return g_Buffer.c_str();
}

void* __stdcall RmGet(void* rm, int type)
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

void __stdcall RmExecute(void* skin, LPCWSTR command)
{
	CMeterWindow* mw = (CMeterWindow*)skin;
	if (command)
	{
		// WM_RAINMETER_EXECUTE used instead of ExecuteCommand for thread-safety
		SendMessage(Rainmeter->GetWindow(), WM_RAINMETER_EXECUTE, (WPARAM)mw, (LPARAM)command);
	}
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
LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data)
{
	if (command == NULL || *command == L'\0')
	{
		return L"noop";
	}

	NULLCHECK(data);

	if (_wcsicmp(command, L"GetConfig") == 0)
	{
		CMeterWindow* meterWindow = Rainmeter->GetMeterWindowByINI(data);
		if (meterWindow)
		{
			g_Buffer = L"\"";
			g_Buffer += meterWindow->GetSkinName();
			g_Buffer += L"\"";
			return g_Buffer.c_str();
		}

		return L"";
	}
	else if (_wcsicmp(command, L"GetWindow") == 0)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(data);

		if (subStrings.size() >= 1)
		{
			const std::wstring& config = subStrings[0];

			CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(config);
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
	else if (_wcsicmp(command, L"GetVariable") == 0)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(data);

		if (subStrings.size() >= 2)
		{
			const std::wstring& config = subStrings[0];

			CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(config);
			if (meterWindow)
			{
				const std::wstring& variable = subStrings[1];

				if (meterWindow->GetParser().GetVariable(variable, g_Buffer))
				{
					return g_Buffer.c_str();
				}
			}
		}

		return L"";
	}
	else if (_wcsicmp(command, L"SetVariable") == 0)
	{
		std::vector<std::wstring> subStrings = CRainmeter::ParseString(data);

		if (subStrings.size() == 3)
		{
			CMeterWindow* meterWindow = Rainmeter->GetMeterWindow(subStrings[0]);
			if (meterWindow)
			{
				meterWindow->SetVariable(subStrings[1], subStrings[2]);
				return L"success";
			}
		}

		return L"error";
	}

	return L"noop";
}
