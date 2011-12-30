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
#include "MeasurePlugin.h"
#include "Rainmeter.h"
#include "Export.h"
#include "System.h"
#include "Error.h"

extern CRainmeter* Rainmeter;

/*
** CMeasureMemory
**
** The constructor
**
*/
CMeasurePlugin::CMeasurePlugin(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Plugin(),
	m_ReloadFunc(),
	m_ID(),
	m_UpdateFunc(),
	m_GetStringFunc(),
	m_ExecuteBangFunc()
{
	m_MaxValue = 0.0;
}

/*
** ~CMeasureMemory
**
** The destructor
**
*/
CMeasurePlugin::~CMeasurePlugin()
{
	if (m_Plugin)
	{
		FARPROC finalizeFunc = GetProcAddress(m_Plugin, "Finalize");
		if (IsNewApi())
		{
			((NEWFINALIZE)finalizeFunc)(m_PluginData);
		}
		else if (finalizeFunc)
		{
			((FINALIZE)finalizeFunc)(m_Plugin, m_ID);
		}

		FreeLibrary(m_Plugin);
	}
}

/*
** Update
**
** Gets the current value from the plugin
**
*/
bool CMeasurePlugin::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (IsNewApi())
	{
		m_Value = ((NEWUPDATE)m_UpdateFunc)(m_PluginData);
	}
	else if (m_UpdateFunc)
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

	return PostUpdate();
}

/*
** ReadConfig
**
** Reads the configs and loads & initializes the plugin
**
*/
void CMeasurePlugin::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	static UINT id = 0;

	CMeasure::ReadConfig(parser, section);
	if (m_Initialized)
	{
		if (IsNewApi())
		{
			((NEWRELOAD)m_ReloadFunc)(m_PluginData, this, &m_MaxValue);
		}
		
		// DynamicVariables doesn't work with old plugins
		return;
	}

	m_PluginName = parser.ReadString(section, L"Plugin", L"");

	size_t pos = m_PluginName.rfind(L'.');
	if (pos == std::wstring::npos)
	{
		m_PluginName += L".dll";
	}

	pos = m_PluginName.rfind(L'\\');
	if (pos != std::wstring::npos)
	{
		m_PluginName.insert(0, L"..\\");
	}
	m_PluginName.insert(0, Rainmeter->GetPluginPath());

	m_Plugin = CSystem::RmLoadLibrary(m_PluginName.c_str(), NULL);
	if (m_Plugin == NULL)
	{
		// Try to load from Rainmeter's folder
		pos = m_PluginName.rfind(L'\\');
		if (pos != std::wstring::npos)
		{
			std::wstring pluginName = Rainmeter->GetPath();
			pluginName.append(m_PluginName, pos + 1, m_PluginName.length() - (pos + 1));

			m_Plugin = CSystem::RmLoadLibrary(pluginName.c_str(), NULL);
		}

		if (m_Plugin == NULL)
		{
			std::wstring error = L"Plugin: \"" + m_PluginName;
			error += L"\" not found";
			throw CError(error);
		}
	}

	FARPROC initializeFunc = GetProcAddress(m_Plugin, "Initialize");
	m_ReloadFunc = GetProcAddress(m_Plugin, "Reload");
	m_UpdateFunc = GetProcAddress(m_Plugin, "Update");
	m_GetStringFunc = GetProcAddress(m_Plugin, "GetString");
	m_ExecuteBangFunc = GetProcAddress(m_Plugin, "ExecuteBang");

	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	if (IsNewApi())
	{
		((NEWINITIALIZE)initializeFunc)(&m_PluginData);
		((NEWRELOAD)m_ReloadFunc)(m_PluginData, this, &m_MaxValue);
	}
	else
	{
		m_ID = id;

		if (!m_UpdateFunc)
		{
			m_UpdateFunc = GetProcAddress(m_Plugin, "Update2");
			m_Update2 = true;
		}

		double maxValue = ((INITIALIZE)initializeFunc)(m_Plugin, parser.GetFilename().c_str(), section, m_ID);

		const std::wstring& szMaxValue = parser.ReadString(section, L"MaxValue", L"");
		if (szMaxValue.empty())
		{
			m_MaxValue = maxValue;
		}

		if (m_MaxValue == 0)
		{
			m_MaxValue = 1;
			m_LogMaxValue = true;
		}
	}

	// Reset to default
	SetDllDirectory(L"");
	CSystem::ResetWorkingDirectory();

	++id;
}

/*
** GetStringValue
**
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
** ExecuteBang
**
** Sends a bang to the plugin
**
*/
void CMeasurePlugin::ExecuteBang(const WCHAR* args)
{
	if (m_ExecuteBangFunc)
	{
		if (IsNewApi())
		{
			((NEWEXECUTEBANG)m_ExecuteBangFunc)(m_PluginData, args);
		}
		else
		{
			((EXECUTEBANG)m_ExecuteBangFunc)(args, m_ID);
		}
	}
	else
	{
		CMeasure::ExecuteBang(args);
	}
}
