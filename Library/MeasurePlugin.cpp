/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasurePlugin.h"
#include "Rainmeter.h"
#include "Export.h"
#include "System.h"

std::unordered_map<std::wstring, UINT> MeasurePlugin::s_PluginReferences;

MeasurePlugin::MeasurePlugin(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Plugin(),
	m_ReloadFunc(),
	m_ID(),
	m_Update2(false),
	m_PluginData(),
	m_UpdateFunc(),
	m_GetStringFunc(),
	m_ExecuteBangFunc()
{
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

		WCHAR pluginPath[MAX_PATH];
		if (GetModuleFileName(m_Plugin, pluginPath, MAX_PATH) > 0UL)
		{
			std::wstring tmpStr = pluginPath;
			StringUtil::ToLowerCase(tmpStr);

			auto iter = s_PluginReferences.find(tmpStr);
			if (iter != s_PluginReferences.end())
			{
				--iter->second;
				if (iter->second == 0)
				{
					if (GetRainmeter().GetDebug())
					{
						LogDebugF(L"Plugin unloaded: %s", pluginPath);
					}
					s_PluginReferences.erase(tmpStr);
				}
			}
		}

		FreeLibrary(m_Plugin);
		m_Plugin = nullptr;
	}
}

/*
** Gets the current value from the plugin
**
*/
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

/*
** Reads the options and loads the plugin
**
*/
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
			LogErrorF(
				this, L"Plugin: Unable to load \"%s\" (error %ld)",
				pluginName.c_str(), GetLastError());
			return;
		}
	}

	// Log plugin references
	{
		WCHAR pluginPath[MAX_PATH];
		if (GetModuleFileName(m_Plugin, pluginPath, MAX_PATH) > 0UL)
		{
			std::wstring tmpStr = pluginPath;
			StringUtil::ToLowerCase(tmpStr);

			auto iter = s_PluginReferences.find(tmpStr);
			if (iter == s_PluginReferences.end())
			{
				s_PluginReferences.emplace(tmpStr, 1U);
				if (GetRainmeter().GetDebug())
				{
					LogDebugF(L"Plugin loaded: %s", pluginPath);
				}
			}
			else
			{
				++iter->second;
			}
		}
	}

	FARPROC initializeFunc = GetProcAddress(m_Plugin, "Initialize");
	m_ReloadFunc = GetProcAddress(m_Plugin, "Reload");
	m_UpdateFunc = GetProcAddress(m_Plugin, "Update");
	m_GetStringFunc = GetProcAddress(m_Plugin, "GetString");
	m_ExecuteBangFunc = GetProcAddress(m_Plugin, "ExecuteBang");

	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	double maxValue = 0.0;

	if (IsNewApi())
	{
		{
			// Suppress C4312: 'type cast': conversion from 'UINT' to 'void*' of greater size
			#pragma warning(suppress: 4312)
			m_PluginData = (void*)id;
		}

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

/*
** Gets the string value from the plugin.
**
*/
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

/*
** Sends a bang to the plugin
**
*/
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

	WCHAR errMsg[MAX_LINE_LENGTH];

	size_t sPos = command.find_first_of(L'(');
	if (sPos != std::wstring::npos)
	{
		size_t ePos = command.find_last_of(L')');
		if (ePos == std::wstring::npos ||
			sPos > ePos ||
			command.size() < 3)
		{
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
		std::string function = StringUtil::Narrow(command.substr(0, sPos));
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

		// Parse arguments
		auto _args = ConfigParser::Tokenize2(
			command.substr(sPos + 1, ePos - sPos - 1),
			L',',
			PairedPunctuation::BothQuotes);

		// Convert strings in array to raw type
		std::vector<LPCWSTR> args;
		for (auto& str : _args)
		{
			StringUtil::StripLeadingAndTrailingQuotes(str, true);
			args.emplace_back(str.c_str());
		}

		void* custom = GetProcAddress(m_Plugin, function.c_str());
		if (custom)
		{
			LPCWSTR result = ((CUSTOMFUNCTION)custom)(m_PluginData, (const int)args.size(), args.data());
			if (result)
			{
				strValue = result;
				return true;
			}
			else
			{
				LogErrorF(this, L"Invalid return type in function: %s", command.substr(0, sPos).c_str());
			}
		}
		else
		{
			LogErrorF(this, L"Cannot find function: %s", command.substr(0, sPos).c_str());
		}
	}

	return false;
}
