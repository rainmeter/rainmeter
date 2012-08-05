/*
  Copyright (C) 2004 Kimmo Pekkola

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
#include "ConfigParser.h"
#include "MathParser.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "System.h"
#include "Measure.h"
#include "Meter.h"
#include "resource.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

std::unordered_map<std::wstring, std::wstring> CConfigParser::c_MonitorVariables;

/*
** The constructor
**
*/
CConfigParser::CConfigParser() :
	m_LastReplaced(false),
	m_LastDefaultUsed(false),
	m_LastValueDefined(false),
	m_CurrentSection()
{
}

/*
** The destructor
**
*/
CConfigParser::~CConfigParser()
{
}

void CConfigParser::Initialize(const std::wstring& filename, CMeterWindow* meterWindow, LPCTSTR skinSection, const std::wstring* resourcePath)
{
	m_Measures.clear();
	m_Sections.clear();
	m_Values.clear();
	m_BuiltInVariables.clear();
	m_Variables.clear();

	m_StyleTemplate.clear();
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	m_CurrentSection = NULL;
	m_SectionInsertPos = m_Sections.end();

	// Set the built-in variables. Do this before the ini file is read so that the paths can be used with @include
	SetBuiltInVariables(filename, resourcePath, meterWindow);
	ResetMonitorVariables(meterWindow);

	CSystem::UpdateIniFileMappingList();

	ReadIniFile(filename, skinSection);
	ReadVariables();

	// Clear and minimize
	m_FoundSections.clear();
	m_ListVariables.clear();
	m_SectionInsertPos = m_Sections.end();

	m_MeterWindow = meterWindow;
}

void CConfigParser::SetBuiltInVariables(const std::wstring& filename, const std::wstring* resourcePath, CMeterWindow* meterWindow)
{
	auto insertVariable = [&](const WCHAR* name, std::wstring value)
	{
		return m_BuiltInVariables.insert(std::make_pair(name, value));
	};

	insertVariable(L"PROGRAMPATH", Rainmeter->GetPath());
	insertVariable(L"PROGRAMDRIVE", Rainmeter->GetDrive());
	insertVariable(L"SETTINGSPATH", Rainmeter->GetSettingsPath());
	insertVariable(L"SKINSPATH", Rainmeter->GetSkinPath());
	insertVariable(L"PLUGINSPATH", Rainmeter->GetPluginPath());
	insertVariable(L"CURRENTPATH", CRainmeter::ExtractPath(filename));
	insertVariable(L"ADDONSPATH", Rainmeter->GetAddonPath());

	if (meterWindow)
	{
		insertVariable(L"CURRENTFILE", meterWindow->GetFileName());
		insertVariable(L"CURRENTCONFIG", meterWindow->GetFolderPath());
		insertVariable(L"ROOTCONFIGPATH", meterWindow->GetRootPath());
	}

	insertVariable(L"CRLF", L"\n");

	m_CurrentSection = &(insertVariable(L"CURRENTSECTION", L"").first->second);	// shortcut

	if (resourcePath)
	{
		SetVariable(L"@", *resourcePath);
	}
}

/*
** Sets all user-defined variables.
**
*/
void CConfigParser::ReadVariables()
{
	std::list<std::wstring>::const_iterator iter = m_ListVariables.begin();
	for ( ; iter != m_ListVariables.end(); ++iter)
	{
		SetVariable((*iter), ReadString(L"Variables", (*iter).c_str(), L"", false));
	}
}

/*
** Sets a new value for the variable. The DynamicVariables must be set to 1 in the
** meter/measure for the changes to be applied.
**
*/
void CConfigParser::SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue)
{
	// LogWithArgs(LOG_DEBUG, L"Variable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), (int)variables.size());

	const std::wstring strTmp = StrToUpper(strVariable);

	variables[strTmp] = strValue;
}
void CConfigParser::SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const WCHAR* strVariable, const WCHAR* strValue)
{
	// LogWithArgs(LOG_DEBUG, L"Variable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), (int)variables.size());

	const std::wstring strTmp = StrToUpper(strVariable);

	variables[strTmp] = strValue;
}

/*
** Gets a value for the variable. Returns true if variable found.
**
*/
bool CConfigParser::GetVariable(const std::wstring& strVariable, std::wstring& strValue)
{
	const std::wstring strTmp = StrToUpper(strVariable);

	// #1: Built-in variables
	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_BuiltInVariables.find(strTmp);
	if (iter != m_BuiltInVariables.end())
	{
		// Built-in variable found
		strValue = (*iter).second;
		return true;
	}

	// #2: Monitor variables
	iter = c_MonitorVariables.find(strTmp);
	if (iter != c_MonitorVariables.end())
	{
		// SCREENAREA/WORKAREA variable found
		strValue = (*iter).second;
		return true;
	}

	// #3: User-defined variables
	iter = m_Variables.find(strTmp);
	if (iter != m_Variables.end())
	{
		// Variable found
		strValue = (*iter).second;
		return true;
	}

	// Not found
	return false;
}

/*
** Gets the value of a section variable. Returns true if strValue is set.
** The selector is stripped from strVariable.
**
*/
bool CConfigParser::GetSectionVariable(std::wstring& strVariable, std::wstring& strValue)
{
	size_t colonPos = strVariable.find_last_of(L':');
	if (colonPos == std::wstring::npos)
	{
		return false;
	}

	const std::wstring selector = strVariable.substr(colonPos + 1);
	const WCHAR* selectorSz = selector.c_str();
	strVariable.resize(colonPos);

	if (!selector.empty() && iswalpha(selectorSz[0]))
	{
		// [Meter:X], [Meter:Y], [Meter:W], [Meter:H]
		CMeter* meter = m_MeterWindow->GetMeter(strVariable);
		if (meter)
		{
			WCHAR buffer[32];
			if (_wcsicmp(selectorSz, L"X") == 0)
			{
				_itow_s(meter->GetX(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"Y") == 0)
			{
				_itow_s(meter->GetY(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"W") == 0)
			{
				_itow_s(meter->GetW(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"H") == 0)
			{
				_itow_s(meter->GetH(), buffer, 10);
			}
			else
			{
				return false;
			}

			strValue = buffer;
			return true;
		}
	}
	else
	{
		// Number: [Measure:], [Measure:dec]
		// Percentual: [Measure:%], [Measure:%, dec]
		// Scale: [Measure:/scale], [Measure:/scale, dec]
		CMeasure* measure = m_MeterWindow->GetMeasure(strVariable);
		if (measure)
		{
			double scale = 1.0;
			bool percentual = false;

			const WCHAR* decimalsSz = wcschr(selectorSz, L',');
			if (decimalsSz)
			{
				++decimalsSz;
			}

			if (*selectorSz == L'%')  // Percentual
			{
				percentual = true;
			}
			else if (*selectorSz == L'/')  // Scale
			{
				errno = 0;
				scale = _wtoi(selectorSz + 1);
				if (errno == EINVAL)
				{
					scale = 1.0;
				}
			}
			else
			{
				if (decimalsSz)
				{
					return false;
				}
				else
				{
					decimalsSz = selectorSz;
				}
			}

			int decimals = decimalsSz && *decimalsSz ? _wtoi(decimalsSz) : 15;
			double value = percentual ? measure->GetRelativeValue() * 100.0 : measure->GetValue() / scale;

			WCHAR format[32];
			WCHAR buffer[128];
			_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
			int bufferLen = _snwprintf_s(buffer, _TRUNCATE, format, value);
			
			if (!decimalsSz)
			{
				// Remove trailing zeros if decimal count was not specified.
				measure->RemoveTrailingZero(buffer, bufferLen);
				bufferLen = (int)wcslen(buffer);
			}

			strValue.assign(buffer, bufferLen);
			return true;
		}
	}
	
	return false;
}

void CConfigParser::ResetMonitorVariables(CMeterWindow* meterWindow)
{
	// Set the SCREENAREA/WORKAREA variables
	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Set the SCREENAREA/WORKAREA variables for present monitor
	SetAutoSelectedMonitorVariables(meterWindow);
}

/*
** Sets new values for the SCREENAREA/WORKAREA variables.
**
*/
void CConfigParser::SetMultiMonitorVariables(bool reset)
{
	WCHAR buffer[32];
	RECT workArea, scrArea;

	if (!reset && c_MonitorVariables.empty())
	{
		reset = true;  // Set all variables
	}

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

	_itow_s(workArea.left, buffer, 10);
	SetMonitorVariable(L"WORKAREAX", buffer);
	SetMonitorVariable(L"PWORKAREAX", buffer);
	_itow_s(workArea.top, buffer, 10);
	SetMonitorVariable(L"WORKAREAY", buffer);
	SetMonitorVariable(L"PWORKAREAY", buffer);
	_itow_s(workArea.right - workArea.left, buffer, 10);
	SetMonitorVariable(L"WORKAREAWIDTH", buffer);
	SetMonitorVariable(L"PWORKAREAWIDTH", buffer);
	_itow_s(workArea.bottom - workArea.top, buffer, 10);
	SetMonitorVariable(L"WORKAREAHEIGHT", buffer);
	SetMonitorVariable(L"PWORKAREAHEIGHT", buffer);

	if (reset)
	{
		scrArea.left = 0;
		scrArea.top = 0;
		scrArea.right = GetSystemMetrics(SM_CXSCREEN);
		scrArea.bottom = GetSystemMetrics(SM_CYSCREEN);

		_itow_s(scrArea.left, buffer, 10);
		SetMonitorVariable(L"SCREENAREAX", buffer);
		SetMonitorVariable(L"PSCREENAREAX", buffer);
		_itow_s(scrArea.top, buffer, 10);
		SetMonitorVariable(L"SCREENAREAY", buffer);
		SetMonitorVariable(L"PSCREENAREAY", buffer);
		_itow_s(scrArea.right - scrArea.left, buffer, 10);
		SetMonitorVariable(L"SCREENAREAWIDTH", buffer);
		SetMonitorVariable(L"PSCREENAREAWIDTH", buffer);
		_itow_s(scrArea.bottom - scrArea.top, buffer, 10);
		SetMonitorVariable(L"SCREENAREAHEIGHT", buffer);
		SetMonitorVariable(L"PSCREENAREAHEIGHT", buffer);

		_itow_s(GetSystemMetrics(SM_XVIRTUALSCREEN), buffer, 10);
		SetMonitorVariable(L"VSCREENAREAX", buffer);
		_itow_s(GetSystemMetrics(SM_YVIRTUALSCREEN), buffer, 10);
		SetMonitorVariable(L"VSCREENAREAY", buffer);
		_itow_s(GetSystemMetrics(SM_CXVIRTUALSCREEN), buffer, 10);
		SetMonitorVariable(L"VSCREENAREAWIDTH", buffer);
		_itow_s(GetSystemMetrics(SM_CYVIRTUALSCREEN), buffer, 10);
		SetMonitorVariable(L"VSCREENAREAHEIGHT", buffer);
	}

	if (CSystem::GetMonitorCount() > 0)
	{
		const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

		for (size_t i = 0, isize = monitors.size(); i < isize; ++i)
		{
			WCHAR buffer2[64];

			const RECT work = (monitors[i].active) ? monitors[i].work : workArea;

			_itow_s(work.left, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAX@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_itow_s(work.top, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAY@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_itow_s(work.right - work.left, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAWIDTH@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_itow_s(work.bottom - work.top, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAHEIGHT@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);

			if (reset)
			{
				const RECT screen = (monitors[i].active) ? monitors[i].screen : scrArea;

				_itow_s(screen.left, buffer, 10);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAX@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_itow_s(screen.top, buffer, 10);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAY@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_itow_s(screen.right - screen.left, buffer, 10);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAWIDTH@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_itow_s(screen.bottom - screen.top, buffer, 10);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAHEIGHT@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
			}
		}
	}
}

/*
** Sets new SCREENAREA/WORKAREA variables for present monitor.
**
*/
void CConfigParser::SetAutoSelectedMonitorVariables(CMeterWindow* meterWindow)
{
	if (meterWindow)
	{
		WCHAR buffer[32];

		if (CSystem::GetMonitorCount() > 0)
		{
			int w1, w2, s1, s2;
			int screenIndex;

			const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
			const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

			// Set X / WIDTH
			screenIndex = multimonInfo.primary;
			if (meterWindow->GetXScreenDefined())
			{
				int i = meterWindow->GetXScreen();
				if (i >= 0 && (i == 0 || i <= (int)monitors.size() && monitors[i-1].active))
				{
					screenIndex = i;
				}
			}

			if (screenIndex == 0)
			{
				s1 = w1 = multimonInfo.vsL;
				s2 = w2 = multimonInfo.vsW;
			}
			else
			{
				w1 = monitors[screenIndex-1].work.left;
				w2 = monitors[screenIndex-1].work.right - monitors[screenIndex-1].work.left;
				s1 = monitors[screenIndex-1].screen.left;
				s2 = monitors[screenIndex-1].screen.right - monitors[screenIndex-1].screen.left;
			}

			_itow_s(w1, buffer, 10);
			SetBuiltInVariable(L"WORKAREAX", buffer);
			_itow_s(w2, buffer, 10);
			SetBuiltInVariable(L"WORKAREAWIDTH", buffer);
			_itow_s(s1, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAX", buffer);
			_itow_s(s2, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAWIDTH", buffer);

			// Set Y / HEIGHT
			screenIndex = multimonInfo.primary;
			if (meterWindow->GetYScreenDefined())
			{
				int i = meterWindow->GetYScreen();
				if (i >= 0 && (i == 0 || i <= (int)monitors.size() && monitors[i-1].active))
				{
					screenIndex = i;
				}
			}

			if (screenIndex == 0)
			{
				s1 = w1 = multimonInfo.vsL;
				s2 = w2 = multimonInfo.vsW;
			}
			else
			{
				w1 = monitors[screenIndex-1].work.top;
				w2 = monitors[screenIndex-1].work.bottom - monitors[screenIndex-1].work.top;
				s1 = monitors[screenIndex-1].screen.top;
				s2 = monitors[screenIndex-1].screen.bottom - monitors[screenIndex-1].screen.top;
			}

			_itow_s(w1, buffer, 10);
			SetBuiltInVariable(L"WORKAREAY", buffer);
			_itow_s(w2, buffer, 10);
			SetBuiltInVariable(L"WORKAREAHEIGHT", buffer);
			_itow_s(s1, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAY", buffer);
			_itow_s(s2, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAHEIGHT", buffer);
		}
		else
		{
			RECT r;

			// Set default WORKAREA
			SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

			_itow_s(r.left, buffer, 10);
			SetBuiltInVariable(L"WORKAREAX", buffer);
			_itow_s(r.top, buffer, 10);
			SetBuiltInVariable(L"WORKAREAY", buffer);
			_itow_s(r.right - r.left, buffer, 10);
			SetBuiltInVariable(L"WORKAREAWIDTH", buffer);
			_itow_s(r.bottom - r.top, buffer, 10);
			SetBuiltInVariable(L"WORKAREAHEIGHT", buffer);

			// Set default SCREENAREA
			r.left = 0;
			r.top = 0;
			r.right = GetSystemMetrics(SM_CXSCREEN);
			r.bottom = GetSystemMetrics(SM_CYSCREEN);

			_itow_s(r.left, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAX", buffer);
			_itow_s(r.top, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAY", buffer);
			_itow_s(r.right - r.left, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAWIDTH", buffer);
			_itow_s(r.bottom - r.top, buffer, 10);
			SetBuiltInVariable(L"SCREENAREAHEIGHT", buffer);
		}
	}
}

/*
** Replaces environment and internal variables in the given string.
**
*/
bool CConfigParser::ReplaceVariables(std::wstring& result)
{
	bool replaced = false;

	CRainmeter::ExpandEnvironmentVariables(result);

	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Check for variables (#VAR#)
	size_t start = 0, end;
	bool loop = true;

	do
	{
		start = result.find(L'#', start);
		if (start != std::wstring::npos)
		{
			size_t si = start + 1;
			end = result.find(L'#', si);
			if (end != std::wstring::npos)
			{
				size_t ei = end - 1;
				if (si != ei && result[si] == L'*' && result[ei] == L'*')
				{
					result.erase(ei, 1);
					result.erase(si, 1);
					start = ei;
				}
				else
				{
					std::wstring strVariable = result.substr(si, end - si);
					std::wstring strValue;

					if (GetVariable(strVariable, strValue))
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1, strValue);
						start += strValue.length();
						replaced = true;
					}
					else
					{
						start = end;
					}
				}
			}
			else
			{
				loop = false;
			}
		}
		else
		{
			loop = false;
		}
	}
	while (loop);

	return replaced;
}

/*
** Replaces measures in the given string.
**
*/
bool CConfigParser::ReplaceMeasures(std::wstring& result)
{
	bool replaced = false;

	size_t start = 0;
	while ((start = result.find(L'[', start)) != std::wstring::npos)
	{
		size_t si = start + 1;
		size_t end = result.find(L']', si);
		if (end == std::wstring::npos)
		{
			break;
		}

		size_t next = result.find(L'[', si);
		if (next == std::wstring::npos || end < next)
		{
			size_t ei = end - 1;
			if (si != ei && result[si] == L'*' && result[ei] == L'*')
			{
				result.erase(ei, 1);
				result.erase(si, 1);
				start = ei;
			}
			else
			{
				std::wstring var = result.substr(si, end - si);

				CMeasure* measure = GetMeasure(var);
				if (measure)
				{
					const WCHAR* value = measure->GetStringValue(AUTOSCALE_OFF, 1, -1, false);
					size_t valueLen = wcslen(value);

					// Measure found, replace it with the value
					result.replace(start, end - start + 1, value, valueLen);
					start += valueLen;
					replaced = true;
				}
				else
				{
					std::wstring value;
					if (GetSectionVariable(var, value))
					{
						// Replace section variable with the value.
						result.replace(start, end - start + 1, value);
						start += value.length();
						replaced = true;
					}
					else
					{
						start = end;
					}
				}
			}
		}
		else
		{
			start = next;
		}
	}
	
	return replaced;
}

const std::wstring& CConfigParser::ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures)
{
	static std::wstring result;

	// Clear last status
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	const std::wstring strSection = section;
	const std::wstring strKey = key;
	const std::wstring strDefault = defValue;

	const std::wstring& strValue = GetValue(strSection, strKey, strDefault);
	if (&strValue == &strDefault)
	{
		bool foundStyleValue = false;

		// If the template is defined read the value from there.
		std::vector<std::wstring>::const_reverse_iterator iter = m_StyleTemplate.rbegin();
		for ( ; iter != m_StyleTemplate.rend(); ++iter)
		{
			const std::wstring& strStyleValue = GetValue((*iter), strKey, strDefault);

			//LogWithArgs(LOG_DEBUG, L"StyleTemplate: [%s] %s (from [%s]) : strDefault=%s (0x%p), strStyleValue=%s (0x%p)",
			//	section, key, (*iter).c_str(), strDefault.c_str(), &strDefault, strStyleValue.c_str(), &strStyleValue);

			if (&strStyleValue != &strDefault)
			{
				result = strStyleValue;
				foundStyleValue = true;
				break;
			}
		}

		if (!foundStyleValue)
		{
			result = strDefault;
			m_LastDefaultUsed = true;
			return result;
		}
	}
	else
	{
		result = strValue;
	}

	if (!result.empty())
	{
		m_LastValueDefined = true;

		if (result.size() >= 3)
		{
			if (result.find(L'#') != std::wstring::npos)
			{
				SetCurrentSection(strSection);  // Set temporarily

				if (ReplaceVariables(result))
				{
					m_LastReplaced = true;
				}

				ClearCurrentSection();  // Reset
			}
			else
			{
				CRainmeter::ExpandEnvironmentVariables(result);
			}

			if (bReplaceMeasures && ReplaceMeasures(result))
			{
				m_LastReplaced = true;
			}
		}
	}

	return result;
}

bool CConfigParser::IsKeyDefined(LPCTSTR section, LPCTSTR key)
{
	ReadString(section, key, L"", false);
	return !m_LastDefaultUsed;
}

bool CConfigParser::IsValueDefined(LPCTSTR section, LPCTSTR key)
{
	ReadString(section, key, L"", false);
	return m_LastValueDefined;
}

void CConfigParser::AddMeasure(CMeasure* pMeasure)
{
	if (pMeasure)
	{
		m_Measures[StrToUpper(pMeasure->GetOriginalName())] = pMeasure;
	}
}

CMeasure* CConfigParser::GetMeasure(const std::wstring& name)
{
	std::unordered_map<std::wstring, CMeasure*>::const_iterator iter = m_Measures.find(StrToUpper(name));
	if (iter != m_Measures.end())
	{
		return (*iter).second;
	}

	return NULL;
}

std::vector<Gdiplus::REAL> CConfigParser::ReadFloats(LPCTSTR section, LPCTSTR key)
{
	std::vector<Gdiplus::REAL> result;
	const std::wstring& string = ReadString(section, key, L"");
	if (!string.empty())
	{
		// Tokenize and parse the floats
		std::vector<std::wstring> tokens = Tokenize(string, L";");
		std::vector<std::wstring>::const_iterator iter = tokens.begin();
		for ( ; iter != tokens.end(); ++iter)
		{
			result.push_back((Gdiplus::REAL)ParseDouble((*iter).c_str(), 0.0));
		}
	}
	return result;
}

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (int)dblValue;
			}

			LogWithArgs(LOG_ERROR, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			int intValue = wcstol(string, NULL, 10);
			if (errno != ERANGE)
			{
				return intValue;
			}
		}
	}

	return defValue;
}

uint32_t CConfigParser::ReadUInt(LPCTSTR section, LPCTSTR key, uint32_t defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (uint32_t)dblValue;
			}

			LogWithArgs(LOG_ERROR, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			uint32_t uintValue = wcstoul(string, NULL, 10);
			if (errno != ERANGE)
			{
				return uintValue;
			}
		}
	}

	return defValue;
}

uint64_t CConfigParser::ReadUInt64(LPCTSTR section, LPCTSTR key, uint64_t defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (uint64_t)dblValue;
			}

			LogWithArgs(LOG_ERROR, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			uint64_t uint64Value = _wcstoui64(string, NULL, 10);
			if (errno != ERANGE)
			{
				return uint64Value;
			}
		}
	}

	return defValue;
}

double CConfigParser::ReadFloat(LPCTSTR section, LPCTSTR key, double defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		double value;
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			const WCHAR* errMsg = MathParser::CheckedParse(string, &value);
			if (!errMsg)
			{
				return value;
			}

			LogWithArgs(LOG_ERROR, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			value = wcstod(string, NULL);
			if (errno != ERANGE)
			{
				return value;
			}
		}
	}

	return defValue;
}

// Returns true if the formula was read successfully, false for failure.
bool CConfigParser::ParseFormula(const std::wstring& formula, double* resultValue)
{
	// Formulas must be surrounded by parenthesis
	if (!formula.empty() && formula[0] == L'(' && formula[formula.size() - 1] == L')')
	{
		const WCHAR* string = formula.c_str();
		const WCHAR* errMsg = MathParser::CheckedParse(string, resultValue);
		if (errMsg != NULL)
		{
			LogWithArgs(LOG_ERROR, L"Formula: %s: %s", errMsg, string);
			return false;
		}

		return true;
	}

	return false;
}

ARGB CConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, ARGB defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseColor(result.c_str());
}

Rect CConfigParser::ReadRect(LPCTSTR section, LPCTSTR key, const Rect& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseRect(result.c_str());
}

RECT CConfigParser::ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	RECT r;
	if (m_LastDefaultUsed)
	{
		r = defValue;
	}
	else
	{
		r = ParseRECT(result.c_str());
	}
	return r;
}

/*
** Splits the string from the delimiters
**
** http://www.digitalpeer.com/id/simple
*/
std::vector<std::wstring> CConfigParser::Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);	// skip delimiters at beginning.
	std::wstring::size_type pos = str.find_first_of(delimiters, lastPos);	// find first "non-delimiter".

	while (std::wstring::npos != pos || std::wstring::npos != lastPos)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));    	// found a token, add it to the vector.
		lastPos = str.find_first_not_of(delimiters, pos);    	// skip delimiters.  Note the "not_of"
		pos = str.find_first_of(delimiters, lastPos);    	// find next "non-delimiter"
	}

	return tokens;
}

/*
** Trims empty element in vector and white-space in each string.
**
*/
void CConfigParser::Shrink(std::vector<std::wstring>& vec)
{
	if (!vec.empty())
	{
		std::vector<std::wstring>::reverse_iterator iter = vec.rbegin();
		while (iter != vec.rend())
		{
			std::wstring::size_type pos = (*iter).find_first_not_of(L" \t\r\n");
			if (pos != std::wstring::npos)
			{
				std::wstring::size_type lastPos = (*iter).find_last_not_of(L" \t\r\n");
				if (pos != 0 || lastPos != ((*iter).size() - 1))
				{
					// Trim white-space
					(*iter).assign((*iter), pos, lastPos - pos + 1);
				}
				++iter;
			}
			else
			{
				// Remove empty element
				vec.erase((++iter).base());
			}
		}
	}
}

/*
** Helper method that parses the floating-point value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
double CConfigParser::ParseDouble(LPCTSTR string, double defValue)
{
	assert(string);

	double value;
	if (*string == L'(')
	{
		const WCHAR* errMsg = MathParser::CheckedParse(string, &value);
		if (!errMsg)
		{
			return value;
		}

		LogWithArgs(LOG_ERROR, L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		double value = wcstod(string, NULL);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

/*
** Helper method that parses the integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
int CConfigParser::ParseInt(LPCTSTR string, int defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (int)dblValue;
		}

		LogWithArgs(LOG_ERROR, L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		int intValue = wcstol(string, NULL, 10);
		if (errno != ERANGE)
		{
			return intValue;
		}
	}

	return defValue;
}

/*
** Helper method that parses the unsigned integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
uint32_t CConfigParser::ParseUInt(LPCTSTR string, uint32_t defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (uint32_t)dblValue;
		}

		LogWithArgs(LOG_ERROR, L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		uint32_t uintValue = wcstoul(string, NULL, 10);
		if (errno != ERANGE)
		{
			return uintValue;
		}
	}

	return defValue;
}

/*
** Helper method that parses the 64bit unsigned integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
uint64_t CConfigParser::ParseUInt64(LPCTSTR string, uint64_t defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (uint64_t)dblValue;
		}

		LogWithArgs(LOG_ERROR, L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		uint64_t uint64Value = _wcstoui64(string, NULL, 10);
		if (errno != ERANGE)
		{
			return uint64Value;
		}
	}

	return defValue;
}

/*
** Helper template that parses four comma separated values from the given string.
**
*/
template <typename T>
bool ParseInt4(LPCTSTR string, T& v1, T& v2, T& v3, T& v4)
{
	if (wcschr(string, L','))
	{
		WCHAR* parseSz = _wcsdup(string);
		WCHAR* token;

		token = wcstok(parseSz, L",");
		if (token)
		{
			v1 = CConfigParser::ParseInt(token, 0);

			token = wcstok(NULL, L",");
			if (token)
			{
				v2 = CConfigParser::ParseInt(token, 0);

				token = wcstok(NULL, L",");
				if (token)
				{
					v3 = CConfigParser::ParseInt(token, 0);

					token = wcstok(NULL, L",");
					if (token)
					{
						v4 = CConfigParser::ParseInt(token, 0);
					}
				}
			}
		}
		free(parseSz);
		return true;
	}

	return false;
}

/*
** Helper method that parses the color values from the given string.
** The color can be supplied as three/four comma separated values or as one
** hex-value.
**
*/
ARGB CConfigParser::ParseColor(LPCTSTR string)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (!ParseInt4(string, R, G, B, A))
	{
		if (wcsncmp(string, L"0x", 2) == 0)
		{
			string += 2;  // skip prefix
		}

		size_t len = wcslen(string);
		if (len >= 8 && !iswspace(string[6]))
		{
			swscanf(string, L"%02x%02x%02x%02x", &R, &G, &B, &A);
		}
		else if (len >= 6)
		{
			swscanf(string, L"%02x%02x%02x", &R, &G, &B);
		}
	}

	return Color::MakeARGB(A, R, G, B);
}

/*
** Helper method that parses the Gdiplus::Rect values from the given string.
** The rect can be supplied as four comma separated values (X/Y/Width/Height).
**
*/
Rect CConfigParser::ParseRect(LPCTSTR string)
{
	Rect r;
	ParseInt4(string, r.X, r.Y, r.Width, r.Height);
	return r;
}

/*
** Helper method that parses the RECT values from the given string.
** The rect can be supplied as four comma separated values (left/top/right/bottom).
**
*/
RECT CConfigParser::ParseRECT(LPCTSTR string)
{
	RECT r = {0};
	ParseInt4(string, r.left, r.top, r.right, r.bottom);
	return r;
}

/*
** Reads the given ini file and fills the m_Values and m_Keys maps.
**
*/
void CConfigParser::ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection, int depth)
{
	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		Rainmeter->ShowMessage(NULL, GetString(ID_STR_INCLUDEINFINITELOOP), MB_OK | MB_ICONERROR);
		return;
	}

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		LogWithArgs(LOG_ERROR, L"Unable to read file: %s", iniFile.c_str());
		return;
	}

	// Avoid "IniFileMapping"
	std::wstring iniRead = CSystem::GetTemporaryFile(iniFile);
	bool temporary = (!iniRead.empty() && (iniRead.size() != 1 || iniRead[0] != L'?'));

	if (temporary)
	{
		if (Rainmeter->GetDebug()) LogWithArgs(LOG_DEBUG, L"Reading file: %s (Temp: %s)", iniFile.c_str(), iniRead.c_str());
	}
	else
	{
		if (Rainmeter->GetDebug()) LogWithArgs(LOG_DEBUG, L"Reading file: %s", iniFile.c_str());
		iniRead = iniFile;
	}

	// Get all the sections (i.e. different meters)
	std::list<std::wstring> sections;
	std::unordered_set<std::wstring> unique;
	std::wstring key, value;  // buffer

	DWORD itemsSize = MAX_LINE_LENGTH;
	WCHAR* items = new WCHAR[itemsSize];
	WCHAR* pos = NULL;
	WCHAR* epos = NULL;

	if (skinSection == NULL)
	{
		// Get all the sections
		do
		{
			items[0] = 0;
			DWORD res = GetPrivateProfileSectionNames(items, itemsSize, iniRead.c_str());
			if (res == 0)		// File not found
			{
				delete [] items;
				if (temporary) CSystem::RemoveFile(iniRead);
				return;
			}
			if (res < itemsSize - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			itemsSize *= 2;
			items = new WCHAR[itemsSize];
		}
		while (true);

		// Read the sections
		pos = items;
		while (pos < epos)
		{
			if (*pos)
			{
				value = pos;  // section name
				StrToUpperC(key.assign(value));
				if (unique.insert(key).second)
				{
					if (m_FoundSections.insert(key).second)
					{
						m_Sections.insert(m_SectionInsertPos, value);
					}
					sections.push_back(value);
				}
				pos += value.size() + 1;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}
	else
	{
		// Special case: Read only "Rainmeter" and specified section from "Rainmeter.ini"
		const std::wstring strRainmeter = L"Rainmeter";
		const std::wstring strFolder = skinSection;

		sections.push_back(strRainmeter);
		sections.push_back(strFolder);

		if (depth == 0)  // Add once
		{
			m_Sections.push_back(strRainmeter);
			m_Sections.push_back(strFolder);
		}
	}

	// Read the keys and values
	for (auto it = sections.cbegin(); it != sections.cend(); ++it)
	{
		unique.clear();

		const WCHAR* sectionName = (*it).c_str();
		bool isVariables = (_wcsicmp(sectionName, L"Variables") == 0);
		bool isMetadata = (skinSection == NULL && !isVariables && _wcsicmp(sectionName, L"Metadata") == 0);
		bool resetInsertPos = true;

		// Read all "key=value" from the section
		do
		{
			items[0] = 0;
			DWORD res = GetPrivateProfileSection(sectionName, items, itemsSize, iniRead.c_str());
			if (res < itemsSize - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			itemsSize *= 2;
			items = new WCHAR[itemsSize];
		}
		while (true);

		pos = items;
		while (pos < epos)
		{
			if (*pos)
			{
				size_t len = wcslen(pos);
				WCHAR* sep = wmemchr(pos, L'=', len);
				if (sep != NULL && sep != pos)
				{
					size_t clen = sep - pos;  // key's length

					StrToUpperC(key.assign(pos, clen));
					if (unique.insert(key).second)
					{
						++sep;
						clen = len - (clen + 1);  // value's length

						// Trim surrounded quotes from value
						if (clen >= 2 && (sep[0] == L'"' || sep[0] == L'\'') && sep[clen - 1] == sep[0])
						{
							clen -= 2;
							++sep;
						}

						if (wcsncmp(key.c_str(), L"@INCLUDE", 8) == 0)
						{
							if (clen > 0)
							{
								value.assign(sep, clen);
								ReadVariables();
								ReplaceVariables(value);
								if (!CSystem::IsAbsolutePath(value))
								{
									// Relative to the ini folder
									value.insert(0, CRainmeter::ExtractPath(iniFile));
								}

								if (resetInsertPos)
								{
									auto jt = it;
									if (++jt == sections.end())  // Special case: @include was used in the last section of the current file
									{
										// Set the insertion place to the last
										m_SectionInsertPos = m_Sections.end();
										resetInsertPos = false;
									}
									else
									{
										// Find the appropriate insertion place
										for (jt = m_Sections.cbegin(); jt != m_Sections.cend(); ++jt)
										{
											if (_wcsicmp((*jt).c_str(), sectionName) == 0)
											{
												m_SectionInsertPos = ++jt;
												resetInsertPos = false;
												break;
											}
										}
									}
								}

								ReadIniFile(value, skinSection, depth + 1);
							}
						}
						else
						{
							if (!isMetadata)  // Uncache Metadata's key-value pair in the skin
							{
								value.assign(sep, clen);
								SetValue((*it), key, value);

								if (isVariables)
								{
									m_ListVariables.push_back(key);
								}
							}
						}
					}
				}
				pos += len + 1;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}

	delete [] items;
	if (temporary) CSystem::RemoveFile(iniRead);
}

/*
** Sets the value for the key under the given section.
**
*/
void CConfigParser::SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue)
{
	// LogWithArgs(LOG_DEBUG, L"[%s] %s=%s (size: %i)", strSection.c_str(), strKey.c_str(), strValue.c_str(), (int)m_Values.size());

	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	m_Values[StrToUpperC(strTmp)] = strValue;
}

/*
** Deletes the value for the key under the given section.
**
*/
void CConfigParser::DeleteValue(const std::wstring& strSection, const std::wstring& strKey)
{
	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToUpperC(strTmp));
	if (iter != m_Values.end())
	{
		m_Values.erase(iter);
	}
}

/*
** Returns the value for the key under the given section.
**
*/
const std::wstring& CConfigParser::GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault)
{
	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToUpperC(strTmp));
	return (iter != m_Values.end()) ? (*iter).second : strDefault;
}
