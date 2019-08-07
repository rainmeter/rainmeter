/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterPlugin.h"
#include "Rainmeter.h"
#include "Export.h"
#include "System.h"

MeterPlugin::MeterPlugin(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Plugin(),
	m_PluginMeasure(),
	m_PluginName(),
	m_PluginData(),
	m_MeterReloadFunc(),
	m_MeterUpdateFunc(),
	m_MeterDrawFunc()
{
}

MeterPlugin::~MeterPlugin()
{
	if (m_Plugin)
	{
		FARPROC finalizeFunc = GetProcAddress(m_Plugin, "MeterFinalize");
		if (finalizeFunc)
		{
			((NEWFINALIZE)finalizeFunc)(m_PluginData);
		}

		FreeLibrary(m_Plugin);
	}
}
/*
** Read the options specified in the ini file.
**
*/
void MeterPlugin::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	if (m_Initialized)
	{
		if (m_MeterReloadFunc)
		{
			((METERRELOAD)m_MeterReloadFunc)(m_PluginData, this);
		}
		return;
	}

	// First try from program path
	std::wstring pluginFile = GetRainmeter().GetPluginPath();
	pluginFile += m_PluginName;
	m_Plugin = System::RmLoadLibrary(pluginFile.c_str());
	if (!m_Plugin)
	{
		if (GetRainmeter().HasUserPluginPath())
		{
			// Try from settings path
			pluginFile = GetRainmeter().GetUserPluginPath();
			pluginFile += m_PluginName;
			m_Plugin = System::RmLoadLibrary(pluginFile.c_str());
		}
		if (!m_Plugin)
		{
			LogErrorF(
				this, L"Plugin: Unable to load \"%s\" (error %ld)",
				m_PluginName.c_str(), GetLastError());
			return;
		}
	}

	FARPROC initializeFunc = GetProcAddress(m_Plugin, "MeterInitialize");
	m_MeterReloadFunc = GetProcAddress(m_Plugin, "MeterReload");
	m_MeterUpdateFunc = GetProcAddress(m_Plugin, "MeterUpdate");
	m_MeterDrawFunc = GetProcAddress(m_Plugin, "MeterDraw");

	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	m_PluginData = nullptr;
	if (initializeFunc)
	{
		((NEWINITIALIZE)initializeFunc)(&m_PluginData, this);
	}

	if (m_MeterReloadFunc)
	{
		((METERRELOAD)m_MeterReloadFunc)(m_PluginData, this);
	}

	// Reset to default
	SetDllDirectory(L"");
	System::ResetWorkingDirectory();
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterPlugin::Update()
{
	if (Meter::Update())
	{
		if (m_MeterUpdateFunc) 
		{
			double value = NULL;
			LPCWSTR string = nullptr;
			void* measurePluginData = nullptr;

			if (!m_Measures.empty())
			{
				// get value and string from the measure
				value = m_Measures[0]->GetRelativeValue();
				string = m_Measures[0]->GetStringValue();

				// if the measure is a plugin measure from the same plugin, we pass the measure plugin data too
				if (m_PluginMeasure)
				{
					measurePluginData = m_PluginMeasure->GetPluginData();
				}
			}

			((METERUPDATE)m_MeterUpdateFunc)(m_PluginData, measurePluginData, value, string);
		}

		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterPlugin::Draw(Gfx::Canvas& canvas)
{
	if (m_MeterDrawFunc)
	{
		// if the measure is a plugin measure from the same plugin, we pass the measure plugin data too
		void* measurePluginData = nullptr;
		if (m_PluginMeasure)
		{
			measurePluginData = m_PluginMeasure->GetPluginData();
		}

		// Get the drawing target from the canvas,
		// since the canvas class is not available for the plugin
		ID2D1DeviceContext* target = canvas.GetTarget();

		return ((METERDRAW)m_MeterDrawFunc)(m_PluginData, measurePluginData, target);
	}
	return false;
}

void MeterPlugin::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	m_PluginMeasure = nullptr;
	if (BindPrimaryMeasure(parser, section, true))
	{
		if (!m_Initialized)
		{
			const std::wstring& plugin = parser.ReadString(section, L"Plugin", L"");
			size_t pos = plugin.find_last_of(L"\\/");
			if (pos != std::wstring::npos)
			{
				m_PluginName.assign(plugin, pos, plugin.length() - pos);
			}
			else
			{
				m_PluginName = plugin;
			}
		}

		// check if the measure loaded the same plugin library
		if (m_Measures[0]->GetTypeID() == TypeID<MeasurePlugin>() &&
			((MeasurePlugin*)m_Measures[0])->GetPluginName() == m_PluginName)
		{
			m_PluginMeasure = (MeasurePlugin*)m_Measures[0];
		}
	}
}
