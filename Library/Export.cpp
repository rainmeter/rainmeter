/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/CriticalSection.h"
#include "Rainmeter.h"
#include "Export.h"
#include "Skin.h"
#include "Measure.h"
#include "MeasurePlugin.h"

#define NULLCHECK(str) { if ((str) == nullptr) { (str) = L""; } }

static CriticalSection g_ThreadBuffersLock;
static std::map<DWORD, std::wstring> g_ThreadBuffers;

const DWORD g_MainThreadId = GetCurrentThreadId();
const WCHAR* g_NonMainThreadError = L"ERROR: This function can only be called on the main thread";

bool IsMainThread()
{
	return GetCurrentThreadId() == g_MainThreadId;
}

std::wstring& GetThreadLocalStringBuffer()
{
	const auto threadId = GetCurrentThreadId();
	if (threadId == g_MainThreadId)
	{
		static std::wstring s_MainThreadBuffer;
		return s_MainThreadBuffer;
	}

	std::wstring* threadBuffer = nullptr;
	{
		CriticalSectionLock lock(g_ThreadBuffersLock);
		threadBuffer = &g_ThreadBuffers[threadId];
	}

	return *threadBuffer;
}

enum RmExportType
{
	EXPORT_REPLACE_VARIABLES,
	EXPORT_PATH_TO_ABSOLUTE,
	EXPORT_GET
};

struct RmReplaceVariablesMessageParams
{
	void* rm;
	LPCWSTR str;
	std::wstring* resultBuffer;
};

struct RmPathToAbsoluteMessageParams
{
	void* rm;
	LPCWSTR relativePath;
	std::wstring* resultBuffer;
};

struct RmGetMessageParams
{
	void* rm;
	int type;
	void* result;
};

// Some plugin exports are called from worker threads even though the documentation says that
// it's not allowed... To avoid memory corruption, use SendMessage to obtain the relevant info
// from the main thread in a thread-safe manner. Note that this will now block until the main
// thread has a chance to respond.
template<typename Params>
LRESULT SendExportSyncMessage(RmExportType message, Params* params)
{
	return SendMessage(GetRainmeter().GetWindow(), WM_RAINMETER_HANDLE_EXPORT_SYNC, (WPARAM)message, (LPARAM)params);
}

void HandleExportSyncMessage(WPARAM wParam, LPARAM lParam)
{
	const auto message = (RmExportType)wParam;
	if (message == EXPORT_REPLACE_VARIABLES)
	{
		auto params = ((RmReplaceVariablesMessageParams*)lParam);
		*params->resultBuffer = RmReplaceVariables(params->rm, params->str);
	}
	else if (message == EXPORT_PATH_TO_ABSOLUTE)
	{
		auto params = (RmPathToAbsoluteMessageParams*)lParam;
		*params->resultBuffer = RmPathToAbsolute(params->rm, params->relativePath);
	}
	else if (message == EXPORT_GET)
	{
		auto params = (RmGetMessageParams*)lParam;
		params->result = RmGet(params->rm, params->type);
	}
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

	const WCHAR* sizeOptions[] = { L"X", L"Y", L"W", L"H", L"FontSize" };
	for (auto* sizeOption : sizeOptions)
	{
		if (_wcsicmp(sizeOption, option) == 0) return true;
	}

	return false;
}

int ReadScaledPluginCoordinateOption(MeasurePlugin* measure, ConfigParser& parser, LPCWSTR option, int defValue)
{
	const int value = parser.ReadInt(measure->GetName(), option, defValue);
	return measure->GetSkin()->LogicalToPhysical(value);
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
		static WCHAR buffer[32] = { 0 };
		buffer[0] = L'\0';

		const auto defValueInt = ConfigParser::ParseInt(defValue, 0);
		parser.SetMonitorVariableMode(measure->GetMonitorVariableMode());
		const auto result = ReadScaledPluginCoordinateOption(measure, parser, option, defValueInt);
		parser.SetMonitorVariableMode(ConfigParser::MonitorVariableMode::DEFAULT_LOGICAL);
		_itow_s(result, buffer, 10);
		return buffer;
	}

	parser.SetMonitorVariableMode(measure->GetMonitorVariableMode());
	LPCWSTR result = parser.ReadString(measure->GetName(), option, defValue, replaceMeasures != FALSE).c_str();
	parser.SetMonitorVariableMode(ConfigParser::MonitorVariableMode::DEFAULT_LOGICAL);
	return result;
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

	parser.SetMonitorVariableMode(measure->GetMonitorVariableMode());
	double result;
	if (ShouldScalePluginCoordinateOption(measure, option))
	{
		result = ReadScaledPluginCoordinateOption(measure, parser, option, (int)defValue);
	}
	else
	{
		result = parser.ReadFloat(measure->GetName(), option, defValue);
	}
	parser.SetMonitorVariableMode(ConfigParser::MonitorVariableMode::DEFAULT_LOGICAL);

	return result;
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
	if (!IsMainThread())
	{
		RmReplaceVariablesMessageParams params = { rm, str, &GetThreadLocalStringBuffer() };
		SendExportSyncMessage(EXPORT_REPLACE_VARIABLES, &params);
		return params.resultBuffer->c_str();
	}

	NULLCHECK(str);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	ConfigParser& parser = measure->GetSkin()->GetParser();
	auto& threadBuffer = GetThreadLocalStringBuffer();
	threadBuffer = str;
	parser.ReplaceVariables(threadBuffer);
	parser.ReplaceMeasures(threadBuffer);
	return threadBuffer.c_str();
}

LPCWSTR __stdcall RmPathToAbsolute(void* rm, LPCWSTR relativePath)
{
	if (!IsMainThread())
	{
		RmPathToAbsoluteMessageParams params = { rm, relativePath, &GetThreadLocalStringBuffer() };
		SendExportSyncMessage(EXPORT_PATH_TO_ABSOLUTE, &params);
		return params.resultBuffer->c_str();
	}

	NULLCHECK(relativePath);

	MeasurePlugin* measure = (MeasurePlugin*)rm;
	auto& threadBuffer = GetThreadLocalStringBuffer();
	threadBuffer = relativePath;
	measure->GetSkin()->MakePathAbsolute(threadBuffer);
	return threadBuffer.c_str();
}

void* __stdcall RmGet(void* rm, int type)
{
	if (!IsMainThread())
	{
		RmGetMessageParams params = { rm, type, nullptr };
		SendExportSyncMessage(EXPORT_GET, &params);
		return params.result;
	}


	// Most of the info returned here shouldn't change while the plugin measure is loaded so
	// hopefully we can get away with just returning the actual data and not a thread-specific
	// copy.
	MeasurePlugin* measure = (MeasurePlugin*)rm;
	switch (type)
	{
	case RMG_MEASURENAME:
			return (void*)measure->GetName();

	case RMG_SKIN:
			return (void*)measure->GetSkin();

	case RMG_SETTINGSFILE:
			return (void*)GetRainmeter().GetDataFile().c_str();

	case RMG_SKINNAME:
			return (void*)measure->GetSkin()->GetFolderPath().c_str();

	case RMG_SKINWINDOWHANDLE:
			return (void*)measure->GetSkin()->GetWindow();
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

	auto& threadBuffer = GetThreadLocalStringBuffer();

	if (_wcsicmp(command, L"GetConfig") == 0)
	{
		Skin* skin = GetRainmeter().GetSkinByINI(data);
		if (skin)
		{
			threadBuffer = L"\"";
			threadBuffer += skin->GetFolderPath();
			threadBuffer += L"\"";
			return threadBuffer.c_str();
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
				auto& threadBuffer = GetThreadLocalStringBuffer();
				WCHAR buf1[64] = { 0 };
				_snwprintf_s(buf1, _TRUNCATE, L"%lu", PtrToUlong(skin->GetWindow()));
				threadBuffer = buf1;
				return threadBuffer.c_str();
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

				auto& threadBuffer = GetThreadLocalStringBuffer();
				if (skin->GetParser().GetVariable(variable, threadBuffer))
				{
					return threadBuffer.c_str();
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
