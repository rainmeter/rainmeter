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

#define NULLCHECK(str) { if ((str) == nullptr) { (str) = L""; } }

static std::wstring g_Buffer;

LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	NULLCHECK(option);
	NULLCHECK(defValue);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadString(measure->GetName(), option, defValue, (bool)replaceMeasures).c_str();
}

double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue)
{
	NULLCHECK(option);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetMeterWindow()->GetParser();
	return parser.ReadFloat(measure->GetName(), option, defValue);
}

LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str)
{
	NULLCHECK(str);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetMeterWindow()->GetParser();
	g_Buffer = str;
	parser.ReplaceVariables(g_Buffer);
	parser.ReplaceMeasures(g_Buffer);
	return g_Buffer.c_str();
}

LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath)
{
	NULLCHECK(relativePath);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	g_Buffer = relativePath;
	measure->GetMeterWindow()->MakePathAbsolute(g_Buffer);
	return g_Buffer.c_str();
}

void* __stdcall RmGet(void* rm, int type)
{
	MeasurePlugin* measure = (MeasurePlugin*)rm;

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
			return (void*)GetRainmeter().GetDataFile().c_str();
		}

	case RMG_SKINNAME:
		{
			MeterWindow* window = measure->GetMeterWindow();
			if (!window) break;
			return (void*)window->GetFolderPath().c_str();
		}

	case RMG_SKINWINDOWHANDLE:
		{
			MeterWindow* window = measure->GetMeterWindow();
			if (!window) break;
			return (void*)window->GetWindow();
		}
	}

	return nullptr;
}

void __stdcall RmExecute(void* skin, LPCWSTR command)
{
	MeterWindow* mw = (MeterWindow*)skin;
	if (command)
	{
		// WM_RAINMETER_EXECUTE used instead of ExecuteCommand for thread-safety
		SendMessage(GetRainmeter().GetWindow(), WM_RAINMETER_EXECUTE, (WPARAM)mw, (LPARAM)command);
	}
}

BOOL LSLog(int nLevel, LPCWSTR unused, LPCWSTR pszMessage)
{
	NULLCHECK(pszMessage);

	// Ignore Level::Debug messages from plugins unless in debug mode
	if (nLevel != (int)Logger::Level::Debug || GetRainmeter().GetDebug())
	{
		GetLogger().Log((Logger::Level)nLevel, L"", pszMessage);
	}

	return TRUE;
}

// Deprecated!
LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue)
{
	NULLCHECK(section);
	NULLCHECK(option);
	NULLCHECK(defValue);

	ConfigParser* parser = GetRainmeter().GetCurrentParser();
	if (parser)
	{
		return parser->ReadString(section, option, defValue, false).c_str();
	}

	return defValue;
}

// Deprecated!
LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data)
{
	if (command == nullptr || *command == L'\0')
	{
		return L"noop";
	}

	NULLCHECK(data);

	if (_wcsicmp(command, L"GetConfig") == 0)
	{
		MeterWindow* meterWindow = GetRainmeter().GetMeterWindowByINI(data);
		if (meterWindow)
		{
			g_Buffer = L"\"";
			g_Buffer += meterWindow->GetFolderPath();
			g_Buffer += L"\"";
			return g_Buffer.c_str();
		}

		return L"";
	}
	else if (_wcsicmp(command, L"GetWindow") == 0)
	{
		std::vector<std::wstring> subStrings = CommandHandler::ParseString(data);

		if (subStrings.size() >= 1)
		{
			const std::wstring& config = subStrings[0];

			MeterWindow* meterWindow = GetRainmeter().GetMeterWindow(config);
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
		std::vector<std::wstring> subStrings = CommandHandler::ParseString(data);

		if (subStrings.size() >= 2)
		{
			const std::wstring& config = subStrings[0];

			MeterWindow* meterWindow = GetRainmeter().GetMeterWindow(config);
			if (meterWindow)
			{
				const std::wstring& variable = subStrings[1];

				const std::wstring* value = meterWindow->GetParser().GetVariable(variable);
				if (value)
				{
					return (*value).c_str();
				}
			}
		}

		return L"";
	}
	else if (_wcsicmp(command, L"SetVariable") == 0)
	{
		std::vector<std::wstring> subStrings = CommandHandler::ParseString(data);

		if (subStrings.size() == 3)
		{
			MeterWindow* meterWindow = GetRainmeter().GetMeterWindow(subStrings[0]);
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
