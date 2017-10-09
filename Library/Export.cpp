/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Rainmeter.h"
#include "Export.h"
#include "Skin.h"
#include "Measure.h"
#include "MeasurePlugin.h"

#define NULLCHECK(str) { if ((str) == nullptr) { (str) = L""; } }

static std::wstring g_Buffer;

LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	NULLCHECK(option);
	NULLCHECK(defValue);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();
	return parser.ReadString(measure->GetName(), option, defValue, replaceMeasures != FALSE).c_str();
}

double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue)
{
	NULLCHECK(option);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();
	return parser.ReadFloat(measure->GetName(), option, defValue);
}

LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str)
{
	NULLCHECK(str);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();
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
	measure->GetSkin()->MakePathAbsolute(g_Buffer);
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
			return (void*)measure->GetSkin();
		}

	case RMG_SETTINGSFILE:
		{
			return (void*)GetRainmeter().GetDataFile().c_str();
		}

	case RMG_SKINNAME:
		{
			Skin* window = measure->GetSkin();
			if (!window) break;
			return (void*)window->GetFolderPath().c_str();
		}

	case RMG_SKINWINDOWHANDLE:
		{
			Skin* window = measure->GetSkin();
			if (!window) break;
			return (void*)window->GetWindow();
		}
	}

	return nullptr;
}

void __stdcall RmExecute(void* skin, LPCWSTR command)
{
	if (command)
	{
		// WM_RAINMETER_EXECUTE used instead of ExecuteCommand for thread-safety
		SendMessage(GetRainmeter().GetWindow(), WM_RAINMETER_EXECUTE, (WPARAM)skin, (LPARAM)command);
	}
}

BOOL LSLog(int level, LPCWSTR unused, LPCWSTR message)
{
	NULLCHECK(message);

	// Ignore Debug messages from plugins unless in debug mode.
	if (level != (int)Logger::Level::Debug || GetRainmeter().GetDebug())
	{
		GetLogger().Log((Logger::Level)level, L"", message);
	}

	return TRUE;
}

void __stdcall RmLog(void* rm, int level, LPCWSTR message)
{
	NULLCHECK(message);

	MeasurePlugin* measure = (MeasurePlugin*)rm;

	// Ignore Debug messages from plugins unless in debug mode.
	if (level != (int)Logger::Level::Debug || GetRainmeter().GetDebug())
	{
		GetLogger().LogSection((Logger::Level)level, measure, message);
	}
}

void RmLogF(void* rm, int level, LPCWSTR format, ...)
{
	NULLCHECK(format);

	MeasurePlugin* measure = (MeasurePlugin*)rm;

	// Ignore Debug messages from plugins unless in debug mode.
	if (level != (int)Logger::Level::Debug || GetRainmeter().GetDebug())
	{
		va_list args;
		va_start(args, format);
		GetLogger().LogSectionVF((Logger::Level)level, measure, format, args);
		va_end(args);
	}
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
		Skin* skin = GetRainmeter().GetSkinByINI(data);
		if (skin)
		{
			g_Buffer = L"\"";
			g_Buffer += skin->GetFolderPath();
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
			std::wstring& config = subStrings[0];

			Skin* skin = GetRainmeter().GetSkin(config);
			if (skin)
			{
				WCHAR buf1[64];
				_snwprintf_s(buf1, _TRUNCATE, L"%lu", PtrToUlong(skin->GetWindow()));
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
			std::wstring& config = subStrings[0];
			Skin* skin = GetRainmeter().GetSkin(config);
			if (skin)
			{
				const std::wstring& variable = subStrings[1];

				const std::wstring* value = skin->GetParser().GetVariable(variable);
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
			Skin* skin = GetRainmeter().GetSkin(subStrings[0]);
			if (skin)
			{
				skin->SetVariable(subStrings[1], subStrings[2]);
				return L"success";
			}
		}

		return L"error";
	}

	return L"noop";
}
