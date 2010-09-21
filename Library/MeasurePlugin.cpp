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
CMeasurePlugin::CMeasurePlugin(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_Plugin = NULL;
	m_ID = 0;
	m_MaxValue = 0;

	InitializeFunc = NULL;
	UpdateFunc = NULL;
	UpdateFunc2 = NULL;
	FinalizeFunc = NULL;
	GetStringFunc = NULL;
	ExecuteBangFunc = NULL;
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
		if(FinalizeFunc) FinalizeFunc(m_Plugin, m_ID);
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

	WCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);

	SetCurrentDirectory((Rainmeter->GetSkinPath() + m_MeterWindow->GetSkinName()).c_str());

	if(UpdateFunc)
	{
		// Update the plugin
		m_Value = UpdateFunc(m_ID);
	}
	else if(UpdateFunc2)
	{
		// Update the plugin
		m_Value = UpdateFunc2(m_ID);
	}

	SetCurrentDirectory(buffer);

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
		m_PluginName = L"..\\" + m_PluginName;
	}
	m_PluginName = Rainmeter->GetPluginPath() + m_PluginName;

	DWORD err = 0;
	m_Plugin = CSystem::RmLoadLibrary(m_PluginName.c_str(), &err);
	
	if(m_Plugin == NULL)
	{
		if (CRainmeter::GetDebug())
		{
			DebugLog(L"Plugin: Unable to load plugin: \"%s\", ErrorCode=%i", m_PluginName.c_str(), err);
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
					DebugLog(L"Plugin: Unable to load plugin: \"%s\", ErrorCode=%i", pluginName.c_str(), err);
				}
			}
		}

		if (m_Plugin == NULL)
		{
			throw CError(std::wstring(L"Rainmeter plugin ") + m_PluginName + L" not found!", __LINE__, __FILE__);
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
		throw CError(std::wstring(L"Rainmeter plugin ") + m_PluginName + L" doesn't export Update or GetString function!", __LINE__, __FILE__);
	}

	// Initialize the plugin
	m_ID = id++;
	if(InitializeFunc) 
	{
		WCHAR buffer[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buffer);

		SetCurrentDirectory((Rainmeter->GetSkinPath() + m_MeterWindow->GetSkinName()).c_str());

		// Remove current directory from DLL search path
		CSystem::RmSetDllDirectory(L"");

		double maxValue;
		maxValue = InitializeFunc(m_Plugin, parser.GetFilename().c_str(), section, m_ID);

		SetCurrentDirectory(buffer);

		std::wstring szMaxValue = parser.ReadString(section, L"MaxValue", L"NotSet");
		if (szMaxValue == L"NotSet") 
		{
			m_MaxValue = maxValue;
		}
	}

	if(m_MaxValue == 0)
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
const WCHAR* CMeasurePlugin::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{
	if(GetStringFunc)
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
		DebugLog(L"[%s] doesn't support bangs.", m_Name.c_str());
	}
}