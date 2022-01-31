/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRegistry.h"
#include "Rainmeter.h"

namespace {

const int MAX_KEY_LENGTH = 255;
const int MAX_VALUE_NAME = 16383;

}  // namespace

MeasureRegistry::MeasureRegistry(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_OutputType(OutputType::Value),
	m_RegKey(),
	m_HKey(HKEY_CURRENT_USER)
{
	m_MaxValue = 0.0;
}

MeasureRegistry::~MeasureRegistry()
{
	Dispose();
}

void MeasureRegistry::Dispose()
{
	if (m_RegKey)
	{
		RegCloseKey(m_RegKey);
	}
}

/*
** Gets the current value from the registry
**
*/
void MeasureRegistry::UpdateValue()
{
	if (m_RegKey != nullptr)
	{
		m_Value = 0.0;
		m_StringValue.clear();

		if (m_OutputType != OutputType::Value)
		{
			auto getList = [&](const DWORD objNum, const int objMaxSize, auto* func) -> void
			{
				WCHAR* objName = new WCHAR[objMaxSize];
				DWORD objSize = 0UL;
				for (DWORD i = 0UL; i < objNum; ++i)
				{
					objName[0] = L'\0';
					objSize = objMaxSize;
					if (func(m_RegKey, i, objName, &objSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
					{
						m_StringValue += objName;
						if (i < (objNum - 1UL))
						{
							m_StringValue += m_OutputDelimiter;
						}
					}
				}
				delete [] objName;
			};

			DWORD numSubKeys = 0UL;
			DWORD numValues = 0UL;
			if (ERROR_SUCCESS == RegQueryInfoKey(m_RegKey, nullptr, nullptr, nullptr, &numSubKeys,
				nullptr, nullptr, &numValues, nullptr, nullptr, nullptr, nullptr))
			{
				if (m_OutputType == OutputType::SubKeyList && numSubKeys > 0UL)
				{
					getList(numSubKeys, MAX_KEY_LENGTH, RegEnumKeyEx);
				}
				else if (m_OutputType == OutputType::ValueList && numValues > 0UL)
				{
					getList(numValues, MAX_VALUE_NAME, RegEnumValue);
				}
			}
		}
		else
		{
			const DWORD INCREMENT = 4096UL;
			DWORD size = INCREMENT;
			WCHAR* data = new WCHAR[size];
			DWORD type = 0UL;

			DWORD dwRet = RegQueryValueEx(m_RegKey, m_RegValueName.c_str(), nullptr,
				(LPDWORD)&type, (LPBYTE)data, (LPDWORD)&size);
			while (dwRet == ERROR_MORE_DATA)
			{
				size += INCREMENT;
				delete[] data;
				data = new WCHAR[size];
				dwRet = RegQueryValueEx(m_RegKey, m_RegValueName.c_str(), nullptr,
					(LPDWORD)&type, (LPBYTE)data, (LPDWORD)&size);
			}

			if (dwRet == ERROR_SUCCESS)
			{
				switch (type)
				{
				case REG_DWORD:
					m_Value = *((LPDWORD)data);
					break;

				case REG_SZ:
				case REG_EXPAND_SZ:
					m_Value = wcstod(data, nullptr);
					m_StringValue = data;
					break;

				case REG_MULTI_SZ:
				{
					m_Value = wcstod(data, nullptr);

					// |REG_MULTI_SZ| returns a sequence of null terminated strings, so convert the null
					// separators from the BYTE array (returned from RegQueryValueEx) into a newline
					const DWORD dwSize = size / sizeof(WCHAR);
					m_StringValue.resize(dwSize);

					for (ULONG pos = 0UL; pos < (dwSize - 1UL); ++pos)
					{
						if (data[pos])
						{
							m_StringValue[pos] = data[pos];
						}
						else
						{
							m_StringValue[pos] = L'\n';  // Substitute newline for null
						}
					}
				}
				break;

				case REG_QWORD:
					m_Value = (double)((LARGE_INTEGER*)data)->QuadPart;
					break;

				case REG_BINARY:
					for (DWORD i = 0UL; i < size; ++i)
					{
						WCHAR buffer[3];
						_snwprintf_s(buffer, 3, L"%02X", ((LPBYTE)data)[i]);
						m_StringValue.append(buffer);
					}

					break;
				}
			}
			else
			{
				RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0UL, KEY_READ, &m_RegKey);
			}

			delete [] data;
		}
	}
	else
	{
		RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0UL, KEY_READ, &m_RegKey);
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

	const WCHAR* type = parser.ReadString(section, L"OutputType", L"Value").c_str();
	if (_wcsicmp(type, L"SubKeyList") == 0)
	{
		m_OutputType = OutputType::SubKeyList;
	}
	else if (_wcsicmp(type, L"ValueList") == 0)
	{
		m_OutputType = OutputType::ValueList;
	}
	else
	{
		m_OutputType = OutputType::Value;
		if (_wcsicmp(type, L"Value") != 0)
		{
			LogErrorF(this, L"OutputType=%s is not valid", type);
		}
	}

	m_OutputDelimiter = parser.ReadString(section, L"OutputDelimiter", L"\n");

	m_RegKeyName = parser.ReadString(section, L"RegKey", L"");
	m_RegValueName = parser.ReadString(section, L"RegValue", L"");

	if (m_MaxValue == 0.0)
	{
		m_MaxValue = 1.0;
		m_LogMaxValue = true;
	}

	// Try to open the key
	Dispose();
	RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0UL, KEY_READ, &m_RegKey);
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

