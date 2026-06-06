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

const DWORD g_MainThreadId = GetCurrentThreadId();
const WCHAR* g_NonMainThreadError = L"ERROR: This function can only be called on the main thread";

bool IsMainThread()
{
	return GetCurrentThreadId() == g_MainThreadId;
}

bool SetStyleTemplateIfNeeded(MeasurePlugin* measure, ConfigParser& parser, LPCWSTR section)
{
	const std::wstring& style = parser.ReadString(section, L"MeterStyle", L"");
	if (!style.empty() && measure->GetSkin()->GetMeter(section))
	{
		parser.SetStyleTemplate(style);
		return true;
	}

	return false;
}

// Previously, a skin with W=100 was actually 100 pixels on the screen. After we implemented
// support for high DPI, the actual width on the screen will be larger when using a non-100%
// scaling factor. Old plugins such as InputTextX, BlurInput, and WebView2 are unaware of this
// change and are still expecting the options to be in screen pixels. Lets give them what they
// are expecting in such cases in order to avoid breaking backwards compatibility.
bool ShouldScalePluginCoordinateOption(MeasurePlugin* plugin, LPCWSTR option)
{
	if (plugin->IsDpiAware()) return false;
	return _wcsicmp(option, L"X") == 0 || _wcsicmp(option, L"Y") == 0 || _wcsicmp(option, L"W") == 0 || _wcsicmp(option, L"H") == 0;
}

int ReadScaledPluginCoordinateOption(MeasurePlugin* measure, ConfigParser& parser, LPCWSTR option, int defValue)
{
	const int value = parser.ReadInt(measure->GetName(), option, defValue);
	return measure->GetSkin()->ScaleToDevicePixels(value);
}

LPCWSTR __stdcall RmReadString(void* rm, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	if (!IsMainThread()) return g_NonMainThreadError;

	NULLCHECK(option);
	NULLCHECK(defValue);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();
	if (ShouldScalePluginCoordinateOption(measure, option))
	{
		WCHAR buffer[32] = { 0 };
		const auto defValueInt = ConfigParser::ParseInt(defValue, 0);
		const auto result = ReadScaledPluginCoordinateOption(measure, parser, option, defValueInt);
		_itow_s(, buffer, 10);
		g_Buffer = buffer;
		return g_Buffer.c_str();
	}

	return parser.ReadString(measure->GetName(), option, defValue, replaceMeasures != FALSE).c_str();
}

LPCWSTR __stdcall RmReadStringFromSection(void* rm, LPCWSTR section, LPCWSTR option, LPCWSTR defValue, BOOL replaceMeasures)
{
	if (!IsMainThread()) return g_NonMainThreadError;

	NULLCHECK(section);
	NULLCHECK(option);
	NULLCHECK(defValue);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();

	SetStyleTemplateIfNeeded(measure, parser, section);
	LPCWSTR result = parser.ReadString(section, option, defValue, replaceMeasures != FALSE).c_str();
	parser.ClearStyleTemplate();

	return result;
}

double __stdcall RmReadFormula(void* rm, LPCWSTR option, double defValue)
{
	if (!IsMainThread()) return 0.0;

	NULLCHECK(option);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();

	if (ShouldScalePluginCoordinateOption(measure, option))
	{
		return ReadScaledPluginCoordinateOption(measure, parser, option, (int)defValue);
	}

	return parser.ReadFloat(measure->GetName(), option, defValue);
}

double __stdcall RmReadFormulaFromSection(void* rm, LPCWSTR section, LPCWSTR option, double defValue)
{
	if (!IsMainThread()) return 0.0;

	NULLCHECK(section);
	NULLCHECK(option);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();

	SetStyleTemplateIfNeeded(measure, parser, section);
	const double result = parser.ReadFloat(section, option, defValue);
	parser.ClearStyleTemplate();

	return result;
}

LPCWSTR __stdcall RmReplaceVariables(void* rm, LPCWSTR str)
{
	if (!IsMainThread()) return g_NonMainThreadError;

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
	if (!IsMainThread()) return g_NonMainThreadError;

	NULLCHECK(relativePath);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	g_Buffer = relativePath;
	measure->GetSkin()->MakePathAbsolute(g_Buffer);
	return g_Buffer.c_str();
}

void* __stdcall RmGet(void* rm, int type)
{
	if (!IsMainThread()) return nullptr;

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
	if (!IsMainThread()) return g_NonMainThreadError;

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
	if (!IsMainThread()) return g_NonMainThreadError;

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

		if (subStrings.size() >= 1ULL)
		{
			std::wstring& config = subStrings[0];

			Skin* skin = GetRainmeter().GetSkin(config);
			if (skin)
			{
				WCHAR buf1[64] = { 0 };
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

		if (subStrings.size() >= 2ULL)
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

		if (subStrings.size() == 3ULL)
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
