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
#include "MeasureRegistry.h"
#include "Rainmeter.h"
#include "Error.h"

/*
** CMeasureRegistry
**
** The constructor
**
*/
CMeasureRegistry::CMeasureRegistry(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_RegKey = NULL;
	m_HKey = NULL;
	m_MaxValue = 0.0;
}

/*
** ~CMeasureRegistry
**
** The destructor
**
*/
CMeasureRegistry::~CMeasureRegistry()
{
	if(m_RegKey) RegCloseKey(m_RegKey);
}

/*
** Update
**
** Gets the current value from the registry
**
*/
bool CMeasureRegistry::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if(m_RegKey != NULL)
	{
		DWORD size = 4096;
		WCHAR data[4096];
		DWORD type = 0;

		if(RegQueryValueEx(m_RegKey,
						m_RegValueName.c_str(),
						NULL,
						(LPDWORD)&type,
						(LPBYTE)&data,
						(LPDWORD)&size) == ERROR_SUCCESS)
		{
			switch(type)
			{
			case REG_DWORD:
				m_Value = *((LPDWORD)&data);
				m_StringValue.erase();
				break;

			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
				m_Value = 0.0;
				m_StringValue = data;
				break;

			case REG_QWORD:
				m_Value = (double)((LARGE_INTEGER*)&data)->QuadPart;
				m_StringValue.erase();
				break;

			default:	// Other types are not supported
				m_Value = 0.0;
				m_StringValue.erase();
			}
		}
		else
		{
			m_Value = 0.0;
			m_StringValue.erase();
			RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey);
		}
	}
	else
	{
		RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey);
	}

	return PostUpdate();
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureRegistry::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	std::wstring keyname;
	keyname = parser.ReadString(section, L"RegHKey", L"HKEY_CURRENT_USER");

	if(_wcsicmp(keyname.c_str(), L"HKEY_CLASSES_ROOT") == 0)
	{
		m_HKey = HKEY_CLASSES_ROOT;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_CURRENT_CONFIG") == 0)
	{
		m_HKey = HKEY_CURRENT_CONFIG;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_CURRENT_USER") == 0)
	{
		m_HKey = HKEY_CURRENT_USER;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_LOCAL_MACHINE") == 0)
	{
		m_HKey = HKEY_LOCAL_MACHINE;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_CLASSES_ROOT") == 0)
	{
		m_HKey = HKEY_CLASSES_ROOT;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_PERFORMANCE_DATA") == 0)
	{
		m_HKey = HKEY_PERFORMANCE_DATA;
	}
	else if(_wcsicmp(keyname.c_str(), L"HKEY_DYN_DATA") == 0)
	{
		m_HKey = HKEY_DYN_DATA;
	}
	else
	{
		throw CError(std::wstring(L"HKEY=") + keyname + L" is not valid in measure [" + section + L"].", __LINE__, __FILE__);
	}

	m_RegKeyName = parser.ReadString(section, L"RegKey", L"");
	m_RegValueName = parser.ReadString(section, L"RegValue", L"");

	if (m_MaxValue == 0.0)
	{
		m_MaxValue = 1.0;
		m_LogMaxValue = true;
	}

	// Try to open the key
	if(m_RegKey) RegCloseKey(m_RegKey);
	RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey); 
}

/*
** GetStringValue
**
** If the measured registry value is a string display it. Otherwise convert the
** value to string as normal.
**
*/
const WCHAR* CMeasureRegistry::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{
	if (m_StringValue.empty())
	{
		return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
	}

	return CheckSubstitute(m_StringValue.c_str());
}

