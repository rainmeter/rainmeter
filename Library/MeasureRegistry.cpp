/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRegistry.h"
#include "Rainmeter.h"

MeasureRegistry::MeasureRegistry(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_RegKey(),
	m_HKey(HKEY_CURRENT_USER)
{
	m_MaxValue = 0.0;
}

MeasureRegistry::~MeasureRegistry()
{
	if (m_RegKey) RegCloseKey(m_RegKey);
}

/*
** Gets the current value from the registry
**
*/
void MeasureRegistry::UpdateValue()
{
	if (m_RegKey != nullptr)
	{
		DWORD size = 4096;
		WCHAR* data = new WCHAR[size];
		DWORD type = 0;

		if (RegQueryValueEx(
				m_RegKey,
				m_RegValueName.c_str(),
				nullptr,
				(LPDWORD)&type,
				(LPBYTE)data,
				(LPDWORD)&size) == ERROR_SUCCESS)
		{
			switch (type)
			{
			case REG_DWORD:
				m_Value = *((LPDWORD)data);
				m_StringValue.clear();
				break;

			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
				m_Value = wcstod(data, nullptr);
				m_StringValue = data;
				break;

			case REG_QWORD:
				m_Value = (double)((LARGE_INTEGER*)data)->QuadPart;
				m_StringValue.clear();
				break;

			default:	// Other types are not supported
				m_Value = 0.0;
				m_StringValue.clear();
			}
		}
		else
		{
			m_Value = 0.0;
			m_StringValue.clear();
			RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey);
		}

		delete [] data;
	}
	else
	{
		RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey);
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureRegistry::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	const WCHAR* keyname = parser.ReadString(section, L"RegHKey", L"HKEY_CURRENT_USER").c_str();
	if (_wcsicmp(keyname, L"HKEY_CURRENT_USER") == 0)
	{
		m_HKey = HKEY_CURRENT_USER;
	}
	else if (_wcsicmp(keyname, L"HKEY_LOCAL_MACHINE") == 0)
	{
		m_HKey = HKEY_LOCAL_MACHINE;
	}
	else if (_wcsicmp(keyname, L"HKEY_CLASSES_ROOT") == 0)
	{
		m_HKey = HKEY_CLASSES_ROOT;
	}
	else if (_wcsicmp(keyname, L"HKEY_CURRENT_CONFIG") == 0)
	{
		m_HKey = HKEY_CURRENT_CONFIG;
	}
	else if (_wcsicmp(keyname, L"HKEY_PERFORMANCE_DATA") == 0)
	{
		m_HKey = HKEY_PERFORMANCE_DATA;
	}
	else if (_wcsicmp(keyname, L"HKEY_DYN_DATA") == 0)
	{
		m_HKey = HKEY_DYN_DATA;
	}
	else
	{
		LogErrorF(this, L"RegHKey=%s is not valid", keyname);
	}

	m_RegKeyName = parser.ReadString(section, L"RegKey", L"");
	m_RegValueName = parser.ReadString(section, L"RegValue", L"");

	if (m_MaxValue == 0.0)
	{
		m_MaxValue = 1.0;
		m_LogMaxValue = true;
	}

	// Try to open the key
	if (m_RegKey) RegCloseKey(m_RegKey);
	RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0, KEY_READ, &m_RegKey);
}

/*
** If the measured registry value is a string display it. Otherwise convert the
** value to string as normal.
**
*/
const WCHAR* MeasureRegistry::GetStringValue()
{
	return !m_StringValue.empty() ? CheckSubstitute(m_StringValue.c_str()) : nullptr;
}

