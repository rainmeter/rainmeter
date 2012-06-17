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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeasurePlugin.h"
#include "Rainmeter.h"
#include "Export.h"
#include "System.h"
#include "Error.h"

extern CRainmeter* Rainmeter;

/*
** The constructor
**
*/
CMeasurePlugin::CMeasurePlugin(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
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

/*
** The destructor
**
*/
CMeasurePlugin::~CMeasurePlugin()
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
	}
}

/*
** Gets the current value from the plugin
**
*/
void CMeasurePlugin::UpdateValue()
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
		CSystem::ResetWorkingDirectory();
	}
}

/*
** Reads the options and loads the plugin
**
*/
void CMeasurePlugin::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	static UINT id = 0;

	CMeasure::ReadOptions(parser, section);

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
	std::wstring pluginFile = Rainmeter->GetPluginPath();
	pluginFile += pluginName;
	m_Plugin = CSystem::RmLoadLibrary(pluginFile.c_str());
	if (!m_Plugin)
	{
		// Try from settings path
		pluginFile = Rainmeter->GetUserPluginPath();
		pluginFile += pluginName;
		m_Plugin = CSystem::RmLoadLibrary(pluginFile.c_str());
		if (!m_Plugin)
		{
			LogWithArgs(LOG_ERROR, L"Plugin: \"%s\" not found", pluginName.c_str());
			return;
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
		m_PluginData = (void*)id;

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
			maxValue = ((INITIALIZE)initializeFunc)(m_Plugin, m_MeterWindow->GetFilePath().c_str(), section, m_ID);
		}
	}

	const std::wstring& szMaxValue = parser.ReadString(section, L"MaxValue", L"");
	if (szMaxValue.empty())
	{
		if (maxValue == 0.0)
		{
			m_MaxValue = 1.0;
			m_LogMaxValue = true;

			// Plugin options changed, so reset
			m_MedianMaxValues.clear();
			m_MedianMinValues.clear();
		}
		else
		{
			m_MaxValue = maxValue;
		}
	}

	// Reset to default
	SetDllDirectory(L"");
	CSystem::ResetWorkingDirectory();

	++id;
}

/*
** Gets the string value from the plugin.
**
*/
const WCHAR* CMeasurePlugin::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
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

	return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
}

/*
** Sends a bang to the plugin
**
*/
void CMeasurePlugin::Command(const std::wstring& command)
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
		CMeasure::Command(command);
	}
}
