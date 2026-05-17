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
	m_OutputType(OutputType::Value),
	m_RegKey(nullptr),
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
		m_RegKey = nullptr;
	}
}

/*
** Gets the current value from the registry
**
*/
void MeasureRegistry::UpdateValue()
{
	m_Value = 0.0;
	m_StringValue.clear();

	if (!m_RegKey)
	{
		RegOpenKeyEx(m_HKey, m_RegKeyName.c_str(), 0UL, KEY_READ, &m_RegKey);
	}

	if (m_RegKey)
	{
		if (m_OutputType != OutputType::Value)
		{
			auto getList = [&](const DWORD objNum, auto* func) -> void
			{
				for (DWORD i = 0; i < objNum; ++i)
				{
					// See size limits: https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-element-size-limits
					WCHAR buffer[16383];
					DWORD bufferSize = _countof(buffer);
					if (func(m_RegKey, i, buffer, &bufferSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
					{
						m_StringValue.append(buffer, bufferSize);
						if (i < (objNum - 1))
						{
							m_StringValue += m_OutputDelimiter;
						}
					}
				}
			};

			DWORD numSubKeys = 0UL;
			DWORD numValues = 0UL;
			if (ERROR_SUCCESS == RegQueryInfoKey(m_RegKey, nullptr, nullptr, nullptr, &numSubKeys,
				nullptr, nullptr, &numValues, nullptr, nullptr, nullptr, nullptr))
			{
				if (m_OutputType == OutputType::SubKeyList && numSubKeys > 0UL)
				{
					getList(numSubKeys, RegEnumKeyEx);
				}
				else if (m_OutputType == OutputType::ValueList && numValues > 0UL)
				{
					getList(numValues, RegEnumValue);
				}
			}
		}
		else
		{
			DWORD dataSize = 128;
			BYTE* data = new BYTE[dataSize];
			DWORD type = 0UL;

			DWORD resultSize = dataSize;
			DWORD result = RegQueryValueEx(m_RegKey, m_RegValueName.c_str(), nullptr, &type, data, &resultSize);
			while (result == ERROR_MORE_DATA)
			{
				// Apparently `resultSize` may be erratic in case we are dealing with HKEY_PERFORMANCE_DATA.
				dataSize = resultSize <= dataSize ? dataSize + 1024 : resultSize;
				delete [] data;
				data = new BYTE[dataSize];

				resultSize = dataSize;
				result = RegQueryValueEx(m_RegKey, m_RegValueName.c_str(), nullptr, &type, data, &resultSize);
			}

			if (result == ERROR_SUCCESS)
			{
				switch (type)
				{
				case REG_DWORD:
					m_Value = *((LPDWORD)data);
					break;

				case REG_SZ:
				case REG_EXPAND_SZ:
				case REG_MULTI_SZ:
					{
						if (resultSize < sizeof(WCHAR))
						{
							break;
						}

						WCHAR* rawStringData = (WCHAR*)data;
						DWORD rawStringLength = resultSize / sizeof(WCHAR);

						// Exclude the possible null-terminator from the length.
						if (rawStringData[rawStringLength - 1] == L'\0')
						{
							rawStringLength -= 1;
						}

						if (type == REG_SZ || type == REG_EXPAND_SZ)
						{
							// Use assign with length in case the data is not null-terminated.
							m_StringValue.assign(rawStringData, rawStringLength);
							m_Value = wcstod(m_StringValue.c_str(), nullptr);
						}
						else if (type == REG_MULTI_SZ)
						{
							bool convertedToNumber = false;
							m_StringValue.reserve(rawStringLength);
							for (DWORD i = 0; i < rawStringLength; ++i)
							{
								if (rawStringData[i])
								{
									m_StringValue.append(1, rawStringData[i]);
								}
								else
								{
									if (!convertedToNumber)
									{
										// Convert the first string to a number.
										m_Value = wcstod(m_StringValue.c_str(), nullptr);
										convertedToNumber = true;
									}

									// Substitute null for delimiter
									m_StringValue.append(m_OutputDelimiter);
								}
							}

							if (!convertedToNumber)
							{
								m_Value = wcstod(m_StringValue.c_str(), nullptr);
							}
						}
					}
					break;

				case REG_QWORD:
					m_Value = (double)((LARGE_INTEGER*)data)->QuadPart;
					break;

				case REG_BINARY:
					for (DWORD i = 0UL; i < resultSize; ++i)
					{
						WCHAR buffer[3];
						_snwprintf_s(buffer, 3, L"%02X", data[i]);
						m_StringValue.append(buffer);
					}

					break;
				}
			}
			else
			{
				Dispose();
			}

			delete [] data;
			data = nullptr;
		}
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
		m_HKey = HKEY_CURRENT_USER; // Default
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

