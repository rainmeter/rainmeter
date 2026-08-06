// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasurePlugin.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "Export.h"
#include "System.h"
#include "../Common/RawString.h"
#include "../Common/StringParser.h"

MeasurePlugin::MeasurePlugin(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Plugin(),
	m_MonitorVariableMode(ConfigParser::MonitorVariableMode::DEFAULT_LOGICAL),
	m_ReloadFunc(),
	m_ID(),
	m_Update2(false),
	m_UpdateFunc(),
	m_GetStringFunc(),
	m_ExecuteBangFunc(),
	m_HandleSkinSettingChangeFunc()
{
	m_PluginData = nullptr;
}

MeasurePlugin::~MeasurePlugin()
{
	if (m_Plugin)
	{
		FARPROC finalizeFunc = GetProcAddress(m_Plugin, "Finalize");
		if (finalizeFunc)
		{
			if (IsNewApi())
			{
				((NEWFINALIZE)finalizeFunc)(m_PluginData);
			}
			else
			{
				((FINALIZE)finalizeFunc)(m_Plugin, m_ID);
			}
		}

		FreeLibrary(m_Plugin);
		m_Plugin = nullptr;
	}
}

void MeasurePlugin::UpdateValue()
{
	if (m_UpdateFunc)
	{
		if (IsNewApi())
		{
			m_Value = ((NEWUPDATE)m_UpdateFunc)(m_PluginData);
		}
		else
		{
			if (m_Update2)
			{
				m_Value = ((UPDATE2)m_UpdateFunc)(m_ID);
			}
			else
			{
				m_Value = ((UPDATE)m_UpdateFunc)(m_ID);
			}
		}

		// Reset to default
		System::ResetWorkingDirectory();
	}
}

void MeasurePlugin::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	static UINT id = 0;

	Measure::ReadOptions(parser, section);

	if (m_Initialized)
	{
		if (IsNewApi())
		{
			((NEWRELOAD)m_ReloadFunc)(m_PluginData, this, &m_MaxValue);
		}

		// DynamicVariables doesn't work with old plugins
		return;
	}

	const std::wstring& plugin = parser.ReadString(section, L"Plugin", L"");
	size_t pos = plugin.find_last_of(L"\\/");
	std::wstring pluginName;
	if (pos != std::wstring::npos)
	{
		pluginName.assign(plugin, pos, plugin.length() - pos);
	}
	else
	{
		pluginName = plugin;
	}

	// Append ".dll" if it doesn't exist
	if (!*PathFindExtension(plugin.c_str()))
	{
		pluginName.append(L".dll");
	}

	const bool logInitialLoad = GetRainmeter().GetDebug() && !GetModuleHandle(pluginName.c_str());

	// First try from program path
	std::wstring pluginFile = GetRainmeter().GetPluginPath();
	pluginFile += pluginName;
	m_Plugin = System::RmLoadLibrary(pluginFile.c_str());
	if (!m_Plugin)
	{
		if (GetRainmeter().HasUserPluginPath())
		{
			// Try from settings path
			pluginFile = GetRainmeter().GetUserPluginPath();
			pluginFile += pluginName;
			m_Plugin = System::RmLoadLibrary(pluginFile.c_str());
		}
		if (!m_Plugin)
		{
			const auto lastError = GetLastError();
			LogErrorF(
				this, L"Plugin: Unable to %s \"%s\" (error %ld)",
				lastError == ERROR_MOD_NOT_FOUND ? L"find DLL for" : L"load",
				pluginName.c_str(), lastError);
			return;
		}
	}

	WCHAR pluginPath[MAX_PATH] = { 0 };
	if (logInitialLoad && GetModuleFileName(m_Plugin, pluginPath, _countof(pluginPath)) > 0)
	{
		LogDebugF(L"Plugin loaded: %s", pluginPath);
	}

	FARPROC initializeFunc = GetProcAddress(m_Plugin, "Initialize");
	m_ReloadFunc = GetProcAddress(m_Plugin, "Reload");
	m_UpdateFunc = GetProcAddress(m_Plugin, "Update");
	m_GetStringFunc = GetProcAddress(m_Plugin, "GetString");
	m_ExecuteBangFunc = GetProcAddress(m_Plugin, "ExecuteBang");
	m_HandleSkinSettingChangeFunc = (HandleSkinSettingChangeFunc)GetProcAddress(m_Plugin, "HandleSkinSettingChange");;

	const WCHAR* pluginFileName = PathFindFileName(pluginName.c_str());

	// Chameleon expects monitor variables such as #SCREENAREAWIDTH# to resolve to physical pixels.
	m_MonitorVariableMode = (!IsDpiAware() && _wcsicmp(pluginFileName, L"Chameleon.dll") == 0) ?
		ConfigParser::MonitorVariableMode::FORCE_PHYSICAL :
		ConfigParser::MonitorVariableMode::DEFAULT_LOGICAL;

	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	double maxValue = 0.0;

	if (IsNewApi())
	{
		m_PluginData = (void*)(UINT_PTR)id;

		if (initializeFunc)
		{
			((NEWINITIALIZE)initializeFunc)(&m_PluginData, this);
		}

		((NEWRELOAD)m_ReloadFunc)(m_PluginData, this, &maxValue);
	}
	else
	{
		m_ID = id;

		if (!m_UpdateFunc)
		{
			m_UpdateFunc = GetProcAddress(m_Plugin, "Update2");
			m_Update2 = true;
		}

		if (initializeFunc)
		{
			maxValue = ((INITIALIZE)initializeFunc)(m_Plugin, m_Skin->GetFilePath().c_str(), section, m_ID);
		}
	}

	const std::wstring& szMaxValue = parser.ReadString(section, L"MaxValue", L"");
	if (szMaxValue.empty())
	{
		if (maxValue == 0.0)
		{
			m_MaxValue = 1.0;
			m_LogMaxValue = true;
			m_MedianValues.clear();
		}
		else
		{
			m_MaxValue = maxValue;
			m_LogMaxValue = false;
		}
	}

	// Reset to default
	SetDllDirectory(L"");
	System::ResetWorkingDirectory();

	++id;
}

const WCHAR* MeasurePlugin::GetStringValue()
{
	if (m_GetStringFunc)
	{
		const WCHAR* ret;
		if (IsNewApi())
		{
			ret = ((NEWGETSTRING)m_GetStringFunc)(m_PluginData);
		}
		else
		{
			ret = ((GETSTRING)m_GetStringFunc)(m_ID, 0);
		}

		if (ret) return CheckSubstitute(ret);
	}

	return nullptr;
}

void MeasurePlugin::Command(const std::wstring& command)
{
	if (m_ExecuteBangFunc)
	{
		const WCHAR* str = command.c_str();
		if (IsNewApi())
		{
			((NEWEXECUTEBANG)m_ExecuteBangFunc)(m_PluginData, str);
		}
		else
		{
			((EXECUTEBANG)m_ExecuteBangFunc)(str, m_ID);
		}
	}
	else
	{
		Measure::Command(command);
	}
}

bool MeasurePlugin::CommandWithReturn(const std::wstring& command, std::wstring& strValue, void* delayedLogEntry)
{
	if (!m_Initialized)
	{
		strValue = L"0";
		return true;
	}

	// A command is a function call, "Function(Arg1, Arg2)".
	StringParser parser(command);
	const std::wstring_view funcName = parser.ConsumeUntil(L'(');
	if (!funcName.empty() || !parser.IsConsumed())
	{
		if (funcName.empty() || !parser.ConsumeSuffix(L")"))
		{
			WCHAR errMsg[MAX_LINE_LENGTH];
			_snwprintf_s(errMsg, _TRUNCATE, L"Invalid function call: %s", command.c_str());
			if (delayedLogEntry)
			{
				std::wstring source = m_Skin->GetSkinPath();
				source += L" - [";
				source += GetOriginalName();
				source += L']';

				// Since plugins can accept single brackets as input, the nested variable parser
				// can send incomplete section variable to the plugin, so store a delayed message
				// in case the "actual" section variable is invalid. If the "final" variable the
				// parser finds is a valid variable, this error message will not be logged.
				// See: |ConfigParser::ParseVariables|
				auto* log = (Logger::Entry*)delayedLogEntry;
				*log = { Logger::Level::Error, L"", source.c_str(), errMsg };
			}
			else
			{
				LogErrorF(this, errMsg);
			}
			return false;
		}

		// Prevent calling known API functions
		std::string function = StringUtil::Narrow(funcName.data(), (int)funcName.length());
		if (function == "Initialize" ||
			function == "Reload" ||
			function == "Update" ||
			function == "GetString" ||
			function == "ExecuteBang" ||
			function == "Finalize" ||
			function == "Update2" ||				// Old API
			function == "GetPluginAuthor" ||		// Old API
			function == "GetPluginVersion")			// Old API
			return false;

		// Plugins expect an array of null terminated strings, so the arguments cannot be passed on as
		// views into |command|. A RawString is a single pointer to such a string, which makes the
		// vector itself the array the plugin expects.
		static_assert(sizeof(RawString) == sizeof(WCHAR*), "RawString must be a single string pointer.");

		std::vector<RawString> args;
		parser.ConsumeWhitespace();
		while (!parser.IsConsumed())
		{
			const auto arg = parser.ConsumeUntilOrRest(
				L',', StringParser::SkipWhitespace | StringParser::SkipQuoted);
			args.emplace_back(StringUtil::StripLeadingAndTrailingQuotes(arg, true));
			parser.ConsumeWhitespace();
		}

		void* custom = GetProcAddress(m_Plugin, function.c_str());
		if (custom)
		{
			auto* result = ((CUSTOMFUNCTION)custom)(m_PluginData, (int)args.size(), reinterpret_cast<const WCHAR**>(args.data()));
			if (result)
			{
				strValue = result;
				return true;
			}
			else
			{
				LogErrorF(this, L"Invalid return type in function: %s", std::wstring(funcName).c_str());
			}
		}
		else
		{
			LogErrorF(this, L"Cannot find function: %s", std::wstring(funcName).c_str());
		}
	}

	return false;
}

void MeasurePlugin::HandleSkinSettingChange(Skin* skin, RmSkinSettingChange setting)
{
	for (auto* measure : skin->GetMeasures())
	{
		if (measure->GetTypeID() == TypeID<MeasurePlugin>())
		{
			MeasurePlugin* plugin = (MeasurePlugin*)measure;
			plugin->HandleSkinSettingChange(setting);
		}
	}
}

void MeasurePlugin::HandleSkinSettingChange(RmSkinSettingChange setting)
{
	if (m_HandleSkinSettingChangeFunc)
	{
		m_HandleSkinSettingChangeFunc(m_PluginData, this, setting);
	}
}
