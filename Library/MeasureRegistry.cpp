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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeasureRegistry.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureRegistry.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.6  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.5  2002/05/04 08:13:06  rainy
  Added support for multi_sz

  Revision 1.4  2002/04/27 10:28:57  rainy
  Added possibility to use other HKEYs also.

  Revision 1.3  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.2  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.1  2001/10/28 09:07:18  rainy
  Inital version

*/
#pragma warning(disable: 4996)

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
		throw CError(std::wstring(L"No such HKEY: ") + keyname, __LINE__, __FILE__);
	}

	m_RegKeyName = parser.ReadString(section, L"RegKey", L"");
	m_RegValueName = parser.ReadString(section, L"RegValue", L"");

	if (m_MaxValue == 0.0)
	{
		m_MaxValue = 1.0;
		m_LogMaxValue = true;
	}

	// Try to open the key
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

