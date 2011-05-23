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
	m_ID(),
	InitializeFunc(),
	UpdateFunc(),
	UpdateFunc2(),
	FinalizeFunc(),
	GetStringFunc(),
	ExecuteBangFunc()
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
		if (FinalizeFunc) FinalizeFunc(m_Plugin, m_ID);
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

	if (UpdateFunc)
	{
		// Update the plugin
		m_Value = UpdateFunc(m_ID);
	}
	else if (UpdateFunc2)
	{
		// Update the plugin
		m_Value = UpdateFunc2(m_ID);
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
	static UINT id = 1;

	CMeasure::ReadConfig(parser, section);

	// DynamicVariables is now disabled in MeasurePlugin due to a limitation of the re-initialization.
	// Do not set m_DynamicVariables to "true".
	m_DynamicVariables = false;

	m_PluginName = parser.ReadString(section, L"Plugin", L"");

	size_t pos = m_PluginName.rfind(L".");
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

	DWORD err = 0;
	m_Plugin = CSystem::RmLoadLibrary(m_PluginName.c_str(), &err);

	if (m_Plugin == NULL)
	{
		if (CRainmeter::GetDebug())
		{
			LogWithArgs(LOG_ERROR, L"Plugin: Unable to load plugin: \"%s\", ErrorCode=%u", m_PluginName.c_str(), err);
		}

		// Try to load from Rainmeter's folder
		pos = m_PluginName.rfind(L'\\');
		if (pos != std::wstring::npos)
		{
			std::wstring pluginName = Rainmeter->GetPath() + m_PluginName.substr(pos + 1);

			err = 0;
			m_Plugin = CSystem::RmLoadLibrary(pluginName.c_str(), &err);

			if (m_Plugin == NULL)
			{
				if (CRainmeter::GetDebug())
				{
					LogWithArgs(LOG_ERROR, L"Plugin: Unable to load plugin: \"%s\", ErrorCode=%u", pluginName.c_str(), err);
				}
			}
		}

		if (m_Plugin == NULL)
		{
			std::wstring error = L"Rainmeter plugin " + m_PluginName;
			error += L" not found!";
			throw CError(error, __LINE__, __FILE__);
		}
	}

	InitializeFunc = (INITIALIZE)GetProcAddress(m_Plugin, "Initialize");
	FinalizeFunc = (FINALIZE)GetProcAddress(m_Plugin, "Finalize");
	UpdateFunc = (UPDATE)GetProcAddress(m_Plugin, "Update");
	UpdateFunc2 = (UPDATE2)GetProcAddress(m_Plugin, "Update2");
	GetStringFunc = (GETSTRING)GetProcAddress(m_Plugin, "GetString");
	ExecuteBangFunc = (EXECUTEBANG)GetProcAddress(m_Plugin, "ExecuteBang");

	if (UpdateFunc == NULL && UpdateFunc2 == NULL && GetStringFunc == NULL)
	{
		FreeLibrary(m_Plugin);

		std::wstring error = L"Rainmeter plugin " + m_PluginName;
		error += L" doesn't export Update or GetString function!";
		throw CError(error, __LINE__, __FILE__);
	}

	// Initialize the plugin
	m_ID = id++;
	if (InitializeFunc)
	{
		// Remove current directory from DLL search path
		SetDllDirectory(L"");

		double maxValue;
		maxValue = InitializeFunc(m_Plugin, parser.GetFilename().c_str(), section, m_ID);

		// Reset to default
		SetDllDirectory(L"");
		CSystem::ResetWorkingDirectory();

		std::wstring szMaxValue = parser.ReadString(section, L"MaxValue", L"NotSet");
		if (szMaxValue == L"NotSet")
		{
			m_MaxValue = maxValue;
		}
	}

	if (m_MaxValue == 0)
	{
		m_MaxValue = 1;
		m_LogMaxValue = true;
	}
}

/*
** GetStringValue
**
** Gets the string value from the plugin.
**
*/
const WCHAR* CMeasurePlugin::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	if (GetStringFunc)
	{
		const WCHAR* ret = GetStringFunc(m_ID, 0);
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
	if (ExecuteBangFunc)
	{
		ExecuteBangFunc(args, m_ID);
	}
	else
	{
		LogWithArgs(LOG_WARNING, L"[%s] doesn't support bangs.", m_Name.c_str());
	}
}