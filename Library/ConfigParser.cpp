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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "ConfigParser.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "System.h"
#include "Measure.h"
#include "resource.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

std::unordered_map<std::wstring, std::wstring> CConfigParser::c_MonitorVariables;

/*
** CConfigParser
**
** The constructor
**
*/
CConfigParser::CConfigParser() :
	m_Parser(MathParser_Create(NULL)),
	m_LastReplaced(false),
	m_LastDefaultUsed(false),
	m_LastValueDefined(false)
{
}

/*
** ~CConfigParser
**
** The destructor
**
*/
CConfigParser::~CConfigParser()
{
	MathParser_Destroy(m_Parser);
}

/*
** Initialize
**
**
*/
void CConfigParser::Initialize(LPCTSTR filename, CRainmeter* pRainmeter, CMeterWindow* meterWindow, LPCTSTR config)
{
	m_Filename = filename;

	m_Measures.clear();
	m_Sections.clear();
	m_Values.clear();
	m_BuiltInVariables.clear();
	m_Variables.clear();

	m_StyleTemplate.clear();
	m_LastUsedStyle.clear();
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	// Set the built-in variables. Do this before the ini file is read so that the paths can be used with @include
	SetBuiltInVariables(pRainmeter, meterWindow);
	ResetMonitorVariables(meterWindow);

	std::vector<std::wstring> iniFileMappings;
	CSystem::GetIniFileMappingList(iniFileMappings);

	ReadIniFile(iniFileMappings, m_Filename, config);
	ReadVariables();

	// Clear and minimize
	std::unordered_set<std::wstring>().swap(m_FoundSections);
	std::vector<std::wstring>().swap(m_ListVariables);
}

/*
** SetBuiltInVariables
**
**
*/
void CConfigParser::SetBuiltInVariables(CRainmeter* pRainmeter, CMeterWindow* meterWindow)
{
	if (pRainmeter)
	{
		SetBuiltInVariable(L"PROGRAMPATH", pRainmeter->GetPath());
		SetBuiltInVariable(L"PROGRAMDRIVE", pRainmeter->GetDrive());
		SetBuiltInVariable(L"SETTINGSPATH", pRainmeter->GetSettingsPath());
		SetBuiltInVariable(L"SKINSPATH", pRainmeter->GetSkinPath());
		SetBuiltInVariable(L"PLUGINSPATH", pRainmeter->GetPluginPath());
		SetBuiltInVariable(L"CURRENTPATH", CRainmeter::ExtractPath(m_Filename));
		SetBuiltInVariable(L"ADDONSPATH", pRainmeter->GetAddonPath());
		SetBuiltInVariable(L"CRLF", L"\n");
	}
	if (meterWindow)
	{
		SetBuiltInVariable(L"CURRENTFILE", meterWindow->GetSkinIniFile());
		SetBuiltInVariable(L"CURRENTCONFIG", meterWindow->GetSkinName());
		SetBuiltInVariable(L"ROOTCONFIGPATH", meterWindow->GetSkinRootPath());
	}
}

/*
** ReadVariables
**
** Sets all user-defined variables.
**
*/
void CConfigParser::ReadVariables()
{
	std::vector<std::wstring>::const_iterator iter = m_ListVariables.begin();
	for ( ; iter != m_ListVariables.end(); ++iter)
	{
		SetVariable((*iter), ReadString(L"Variables", (*iter).c_str(), L"", false));
	}
}

/**
** Sets a new value for the variable. The DynamicVariables must be set to 1 in the
** meter/measure for the changes to be applied.
**
** \param variables
** \param strVariable
** \param strValue
*/
void CConfigParser::SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue)
{
	// LogWithArgs(LOG_DEBUG, L"Variable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), (int)variables.size());

	variables[StrToLower(strVariable)] = strValue;
}

/**
** Gets a value for the variable.
**
** \param strVariable
** \param strValue
** \return true if variable is found
*/
bool CConfigParser::GetVariable(const std::wstring& strVariable, std::wstring& strValue)
{
	std::wstring strTmp = StrToLower(strVariable);

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
** ResetMonitorVariables
**
**
*/
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
** SetMultiMonitorVariables
**
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

	_snwprintf_s(buffer, _TRUNCATE, L"%i", workArea.left);
	SetMonitorVariable(L"WORKAREAX", buffer);
	SetMonitorVariable(L"PWORKAREAX", buffer);
	_snwprintf_s(buffer, _TRUNCATE, L"%i", workArea.top);
	SetMonitorVariable(L"WORKAREAY", buffer);
	SetMonitorVariable(L"PWORKAREAY", buffer);
	_snwprintf_s(buffer, _TRUNCATE, L"%i", workArea.right - workArea.left);
	SetMonitorVariable(L"WORKAREAWIDTH", buffer);
	SetMonitorVariable(L"PWORKAREAWIDTH", buffer);
	_snwprintf_s(buffer, _TRUNCATE, L"%i", workArea.bottom - workArea.top);
	SetMonitorVariable(L"WORKAREAHEIGHT", buffer);
	SetMonitorVariable(L"PWORKAREAHEIGHT", buffer);

	if (reset)
	{
		scrArea.left = 0;
		scrArea.top = 0;
		scrArea.right = GetSystemMetrics(SM_CXSCREEN);
		scrArea.bottom = GetSystemMetrics(SM_CYSCREEN);

		_snwprintf_s(buffer, _TRUNCATE, L"%i", scrArea.left);
		SetMonitorVariable(L"SCREENAREAX", buffer);
		SetMonitorVariable(L"PSCREENAREAX", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", scrArea.top);
		SetMonitorVariable(L"SCREENAREAY", buffer);
		SetMonitorVariable(L"PSCREENAREAY", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", scrArea.right - scrArea.left);
		SetMonitorVariable(L"SCREENAREAWIDTH", buffer);
		SetMonitorVariable(L"PSCREENAREAWIDTH", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", scrArea.bottom - scrArea.top);
		SetMonitorVariable(L"SCREENAREAHEIGHT", buffer);
		SetMonitorVariable(L"PSCREENAREAHEIGHT", buffer);

		_snwprintf_s(buffer, _TRUNCATE, L"%i", GetSystemMetrics(SM_XVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAX", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", GetSystemMetrics(SM_YVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAY", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", GetSystemMetrics(SM_CXVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAWIDTH", buffer);
		_snwprintf_s(buffer, _TRUNCATE, L"%i", GetSystemMetrics(SM_CYVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAHEIGHT", buffer);
	}

	if (CSystem::GetMonitorCount() > 0)
	{
		const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		for (size_t i = 0, isize = monitors.size(); i < isize; ++i)
		{
			WCHAR buffer2[64];

			const RECT work = (monitors[i].active) ? monitors[i].work : workArea;

			_snwprintf_s(buffer, _TRUNCATE, L"%i", work.left);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAX@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", work.top);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAY@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", work.right - work.left);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAWIDTH@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", work.bottom - work.top);
			_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAHEIGHT@%i", (int)i + 1);
			SetMonitorVariable(buffer2, buffer);

			if (reset)
			{
				const RECT screen = (monitors[i].active) ? monitors[i].screen : scrArea;

				_snwprintf_s(buffer, _TRUNCATE, L"%i", screen.left);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAX@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_snwprintf_s(buffer, _TRUNCATE, L"%i", screen.top);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAY@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_snwprintf_s(buffer, _TRUNCATE, L"%i", screen.right - screen.left);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAWIDTH@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
				_snwprintf_s(buffer, _TRUNCATE, L"%i", screen.bottom - screen.top);
				_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAHEIGHT@%i", (int)i + 1);
				SetMonitorVariable(buffer2, buffer);
			}
		}
	}
}

/*
** SetAutoSelectedMonitorVariables
**
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

			const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
			const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

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

			_snwprintf_s(buffer, _TRUNCATE, L"%i", w1);
			SetBuiltInVariable(L"WORKAREAX", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", w2);
			SetBuiltInVariable(L"WORKAREAWIDTH", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", s1);
			SetBuiltInVariable(L"SCREENAREAX", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", s2);
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

			_snwprintf_s(buffer, _TRUNCATE, L"%i", w1);
			SetBuiltInVariable(L"WORKAREAY", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", w2);
			SetBuiltInVariable(L"WORKAREAHEIGHT", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", s1);
			SetBuiltInVariable(L"SCREENAREAY", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", s2);
			SetBuiltInVariable(L"SCREENAREAHEIGHT", buffer);
		}
		else
		{
			RECT r;

			// Set default WORKAREA
			SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.left);
			SetBuiltInVariable(L"WORKAREAX", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.top);
			SetBuiltInVariable(L"WORKAREAY", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.right - r.left);
			SetBuiltInVariable(L"WORKAREAWIDTH", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.bottom - r.top);
			SetBuiltInVariable(L"WORKAREAHEIGHT", buffer);

			// Set default SCREENAREA
			r.left = 0;
			r.top = 0;
			r.right = GetSystemMetrics(SM_CXSCREEN);
			r.bottom = GetSystemMetrics(SM_CYSCREEN);

			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.left);
			SetBuiltInVariable(L"SCREENAREAX", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.top);
			SetBuiltInVariable(L"SCREENAREAY", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.right - r.left);
			SetBuiltInVariable(L"SCREENAREAWIDTH", buffer);
			_snwprintf_s(buffer, _TRUNCATE, L"%i", r.bottom - r.top);
			SetBuiltInVariable(L"SCREENAREAHEIGHT", buffer);
		}
	}
}

/**
** Replaces environment and internal variables in the given string.
**
** \param result The string where the variables are returned. The string is modified.
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
	size_t start = 0;
	size_t end = std::wstring::npos;
	size_t pos = std::wstring::npos;
	bool loop = true;

	do
	{
		pos = result.find(L'#', start);
		if (pos != std::wstring::npos)
		{
			end = result.find(L'#', pos + 1);
			if (end != std::wstring::npos)
			{
				if (result[pos + 1] == L'*' && result[end - 1] == L'*')
				{
					result.erase(pos + 1, 1);
					result.erase(end - 2, 1);
					start = end - 1;
				}
				else
				{
					std::wstring strVariable = result.substr(pos + 1, end - (pos + 1));
					std::wstring strValue;

					if (GetVariable(strVariable, strValue))
					{
						// Variable found, replace it with the value
						result.replace(pos, end - pos + 1, strValue);
						start = pos + strValue.length();
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

/**
** Replaces measures in the given string.
**
** \param result The string where the measure values are returned. The string is modified.
*/
bool CConfigParser::ReplaceMeasures(std::wstring& result)
{
	bool replaced = false;

	// Check for measures ([Measure])
	if (!m_Measures.empty())
	{
		size_t start = 0;
		size_t end = std::wstring::npos;
		size_t pos = std::wstring::npos;
		size_t pos2 = std::wstring::npos;
		bool loop = true;
		do
		{
			pos = result.find(L'[', start);
			if (pos != std::wstring::npos)
			{
				end = result.find(L']', pos + 1);
				if (end != std::wstring::npos)
				{
					pos2 = result.find(L'[', pos + 1);
					if (pos2 == std::wstring::npos || end < pos2)
					{
						if (result[pos + 1] == L'*' && result[end - 1] == L'*')
						{
							result.erase(pos + 1, 1);
							result.erase(end - 2, 1);
							start = end - 1;
						}
						else
						{
							std::wstring var = result.substr(pos + 1, end - (pos + 1));

							CMeasure* measure = GetMeasure(var);
							if (measure)
							{
								std::wstring value = measure->GetStringValue(AUTOSCALE_OFF, 1, -1, false);

								// Measure found, replace it with the value
								result.replace(pos, end - pos + 1, value);
								start = pos + value.length();
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
						start = pos2;
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
	}

	return replaced;
}

/*
** ReadString
**
**
*/
const std::wstring& CConfigParser::ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures)
{
	static std::wstring result;

	// Clear last status
	m_LastUsedStyle.clear();
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	const std::wstring strSection = section;
	const std::wstring strKey = key;
	const std::wstring strDefault = defValue;

	const std::wstring& strValue = GetValue(strSection, strKey, strDefault);
	if (&strValue == &strDefault)
	{
		// If the template is defined read the value from there.
		if (!m_StyleTemplate.empty())
		{
			std::vector<std::wstring>::const_reverse_iterator iter = m_StyleTemplate.rbegin();
			for ( ; iter != m_StyleTemplate.rend(); ++iter)
			{
				const std::wstring& strStyleValue = GetValue((*iter), strKey, strDefault);

				//LogWithArgs(LOG_DEBUG, L"StyleTemplate: [%s] %s (from [%s]) : strDefault=%s (0x%p), strStyleValue=%s (0x%p)",
				//	section, key, (*iter).c_str(), strDefault.c_str(), &strDefault, strStyleValue.c_str(), &strStyleValue);

				if (&strStyleValue != &strDefault)
				{
					result = strStyleValue;
					m_LastUsedStyle = (*iter);
					break;
				}
			}
		}

		if (m_LastUsedStyle.empty())  // No template found
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

		const std::wstring CURRENTSECTION = L"CURRENTSECTION";
		SetBuiltInVariable(CURRENTSECTION, strSection);  // Set temporarily

		if (ReplaceVariables(result))
		{
			m_LastReplaced = true;
		}

		SetBuiltInVariable(CURRENTSECTION, L"");  // Reset

		if (bReplaceMeasures && ReplaceMeasures(result))
		{
			m_LastReplaced = true;
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
	const std::wstring& result = ReadString(section, key, L"", false);
	return (!m_LastDefaultUsed && !result.empty());
}

void CConfigParser::AddMeasure(CMeasure* pMeasure)
{
	if (pMeasure)
	{
		m_Measures[StrToLower(pMeasure->GetName())] = pMeasure;
	}
}

CMeasure* CConfigParser::GetMeasure(const std::wstring& name)
{
	std::unordered_map<std::wstring, CMeasure*>::const_iterator iter = m_Measures.find(StrToLower(name));
	if (iter != m_Measures.end())
	{
		return (*iter).second;
	}

	return NULL;
}

double CConfigParser::ReadFloat(LPCTSTR section, LPCTSTR key, double defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseDouble(result, defValue);
}

std::vector<Gdiplus::REAL> CConfigParser::ReadFloats(LPCTSTR section, LPCTSTR key)
{
	std::vector<Gdiplus::REAL> result;
	const std::wstring& string = ReadString(section, key, L"");
	if (!string.empty())
	{
		std::wstring tmp = string;
		if (tmp[tmp.length() - 1] != L';')
		{
			tmp += L";";
		}

		// Tokenize and parse the floats
		std::vector<std::wstring> tokens = Tokenize(tmp, L";");
		std::vector<std::wstring>::const_iterator iter = tokens.begin();
		for ( ; iter != tokens.end(); ++iter)
		{
			result.push_back((Gdiplus::REAL)ParseDouble((*iter), 0));
		}
	}
	return result;
}

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : (int)ParseDouble(result, defValue, true);
}

unsigned int CConfigParser::ReadUInt(LPCTSTR section, LPCTSTR key, unsigned int defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : (unsigned int)ParseDouble(result, defValue, true);
}

// Works as ReadFloat except if the value is surrounded by parenthesis in which case it tries to evaluate the formula
double CConfigParser::ReadFormula(LPCTSTR section, LPCTSTR key, double defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	// Formulas must be surrounded by parenthesis
	if (!result.empty() && result[0] == L'(' && result[result.size() - 1] == L')')
	{
		double resultValue = defValue;
		char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(result.c_str()).c_str(), &resultValue);
		if (errMsg != NULL)
		{
			std::wstring error = L"ReadFormula: ";
			error += ConvertToWide(errMsg);
			error += L" in key \"";
			error += key ? key : L"";
			error += L"\" in section [";
			error += section ? section : L"";
			error += L"].";
			Log(LOG_ERROR, error.c_str());
		}

		return resultValue;
	}

	return (m_LastDefaultUsed) ? defValue : ParseDouble(result, defValue);
}

// Returns true if the formula was read successfully, false for failure.
// Pass a pointer to a double.
bool CConfigParser::ReadFormula(const std::wstring& result, double* resultValue)
{
	// Formulas must be surrounded by parenthesis
	if (!result.empty() && result[0] == L'(' && result[result.size() - 1] == L')')
	{
		char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(result.c_str()).c_str(), resultValue);
		if (errMsg != NULL)
		{
			std::wstring error = L"ReadFormula: ";
			error += ConvertToWide(errMsg);
			error += L": ";
			error += result;
			Log(LOG_ERROR, error.c_str());
			return false;
		}

		return true;
	}

	return false;
}

Color CConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, const Color& defValue)
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

	return (m_LastDefaultUsed) ? defValue : ParseRECT(result.c_str());
}

/*
** Tokenize
**
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
** Shrink
**
** Trims empty element in vector and white-space in each string.
**
*/
void CConfigParser::Shrink(std::vector<std::wstring>& vec)
{
	if (!vec.empty())
	{
		std::vector<std::wstring>::iterator iter = vec.begin();
		while (iter != vec.end())
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
				iter = vec.erase(iter);
			}
		}
	}
}

/*
** ParseDouble
**
** This is a helper method that parses the floating-point value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
double CConfigParser::ParseDouble(const std::wstring& string, double defValue, bool rejectExp)
{
	std::wstring::size_type pos;

	// Ignore inline comments which start with ';'
	if ((pos = string.find_first_of(L';')) != std::wstring::npos)
	{
		std::wstring temp(string, 0, pos);
		return ParseDouble(temp, defValue, rejectExp);
	}

	if (rejectExp)
	{
		// Reject if the given string includes the exponential part
		if (string.find_last_of(L"dDeE") != std::wstring::npos)
		{
			return defValue;
		}
	}

	if ((pos = string.find_first_not_of(L" \t\r\n")) != std::wstring::npos)
	{
		// Trim white-space
		std::wstring temp(string, pos, string.find_last_not_of(L" \t\r\n") - pos + 1);

		WCHAR* end = NULL;
		errno = 0;
		double resultValue = wcstod(temp.c_str(), &end);
		if (end && *end == L'\0' && errno != ERANGE)
		{
			return resultValue;
		}
	}

	return defValue;
}

/*
** ParseColor
**
** This is a helper method that parses the color values from the given string.
** The color can be supplied as three/four comma separated values or as one
** hex-value.
**
*/
Color CConfigParser::ParseColor(LPCTSTR string)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (wcschr(string, L','))
	{
		WCHAR* parseSz = _wcsdup(string);
		WCHAR* token;

		token = wcstok(parseSz, L",");
		if (token)
		{
			R = _wtoi(token);
			R = max(R, 0);
			R = min(R, 255);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			G = _wtoi(token);
			G = max(G, 0);
			G = min(G, 255);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			B = _wtoi(token);
			B = max(B, 0);
			B = min(B, 255);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			A = _wtoi(token);
			A = max(A, 0);
			A = min(A, 255);
		}
		free(parseSz);
	}
	else
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

	return Color(A, R, G, B);
}

/*
** ParseInt4
**
** This is a helper template that parses four comma separated values from the given string.
**
*/
template <typename T>
void ParseInt4(LPCTSTR string, T& v1, T& v2, T& v3, T& v4)
{
	if (wcschr(string, L','))
	{
		WCHAR* parseSz = _wcsdup(string);
		WCHAR* token;

		token = wcstok(parseSz, L",");
		if (token)
		{
			v1 = _wtoi(token);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			v2 = _wtoi(token);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			v3 = _wtoi(token);
		}
		token = wcstok(NULL, L",");
		if (token)
		{
			v4 = _wtoi(token);
		}
		free(parseSz);
	}
}

/*
** ParseRect
**
** This is a helper method that parses the Gdiplus::Rect values from the given string.
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
** ParseRECT
**
** This is a helper method that parses the RECT values from the given string.
** The rect can be supplied as four comma separated values (left/top/right/bottom).
**
*/
RECT CConfigParser::ParseRECT(LPCTSTR string)
{
	RECT r = {0};
	ParseInt4(string, r.left, r.top, r.right, r.bottom);
	return r;
}

//==============================================================================
/**
** Reads the given ini file and fills the m_Values and m_Keys maps.
**
** \param iniFile The ini file to be read.
*/
void CConfigParser::ReadIniFile(const std::vector<std::wstring>& iniFileMappings, const std::wstring& iniFile, LPCTSTR config, int depth)
{
	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		MessageBox(NULL, GetString(ID_STR_INCLUDEINFINITELOOP), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
		return;
	}

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		LogWithArgs(LOG_ERROR, L"Unable to read file: %s", iniFile.c_str());
		return;
	}

	// Avoid "IniFileMapping"
	std::wstring iniRead = CSystem::GetTemporaryFile(iniFileMappings, iniFile);
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
	DWORD itemsSize = MAX_LINE_LENGTH;
	WCHAR* items = new WCHAR[itemsSize];
	WCHAR* pos = NULL;
	WCHAR* epos = NULL;

	if (config == NULL)
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
				std::wstring strTmp = StrToLower(pos);
				if (m_FoundSections.insert(strTmp).second)
				{
					m_Sections.push_back(pos);
				}
				sections.push_back(pos);
				pos += strTmp.size() + 1;
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
		sections.push_back(L"Rainmeter");
		sections.push_back(config);

		if (depth == 0)  // Add once
		{
			m_Sections.push_back(L"Rainmeter");
			m_Sections.push_back(config);
			m_FoundSections.insert(L"rainmeter");
			m_FoundSections.insert(StrToLower(config));
		}
	}

	// Read the keys and values
	std::list<std::wstring>::const_iterator iter = sections.begin();
	for ( ; iter != sections.end(); ++iter)
	{
		std::unordered_set<std::wstring> foundKeys;
		bool isVariables = (_wcsicmp((*iter).c_str(), L"Variables") == 0);
		bool isMetadata = (config == NULL && _wcsicmp((*iter).c_str(), L"Metadata") == 0);

		// Read all "key=value" from the section
		do
		{
			items[0] = 0;
			DWORD res = GetPrivateProfileSection((*iter).c_str(), items, itemsSize, iniRead.c_str());
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
				std::wstring key = pos;
				std::wstring::size_type len = key.length(), sep = key.find_first_of(L'=');
				if (sep != std::wstring::npos && sep != 0)
				{
					std::wstring value = key.substr(sep + 1, len - sep);
					key.erase(sep);

					if (foundKeys.insert(StrToLowerC(key)).second)
					{
						// Trim surrounded quotes from value
						std::wstring::size_type valueLen = value.length();
						if (valueLen >= 2 && (
							(value[0] == L'\"' && value[valueLen - 1] == L'\"') ||
							(value[0] == L'\'' && value[valueLen - 1] == L'\'')))
						{
							valueLen -= 2;
							value.assign(value, 1, valueLen);
						}

						if (wcsncmp(key.c_str(), L"@include", 8) == 0)
						{
							ReadVariables();
							ReplaceVariables(value);
							if (!CSystem::IsAbsolutePath(value))
							{
								// It's a relative path so add the current path as a prefix
								value.insert(0, CRainmeter::ExtractPath(iniFile));
							}
							ReadIniFile(iniFileMappings, value, config, depth + 1);
						}
						else
						{
							if (!isMetadata)  // Uncache Metadata's key-value pair in the skin
							{
								SetValue((*iter), key, value);
							}

							if (isVariables)
							{
								m_ListVariables.push_back(key);
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

//==============================================================================
/**
** Sets the value for the key under the given section.
**
** \param strSection The name of the section.
** \param strKey The name of the key.
** \param strValue The value for the key.
*/
void CConfigParser::SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue)
{
	// LogWithArgs(LOG_DEBUG, L"[%s] %s=%s (size: %i)", strSection.c_str(), strKey.c_str(), strValue.c_str(), (int)m_Values.size());

	std::wstring strTmp = strSection + L"~";
	strTmp += strKey;

	m_Values[StrToLowerC(strTmp)] = strValue;
}

//==============================================================================
/**
** Deletes the value for the key under the given section.
**
** \param strSection The name of the section.
** \param strKey The name of the key.
** \param strValue The value for the key.
*/
void CConfigParser::DeleteValue(const std::wstring& strSection, const std::wstring& strKey)
{
	std::wstring strTmp = strSection + L"~";
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::iterator iter = m_Values.find(StrToLowerC(strTmp));
	if (iter != m_Values.end())
	{
		m_Values.erase(iter);
	}
}

//==============================================================================
/**
** Returns the value for the key under the given section.
**
** \param strSection The name of the section.
** \param strKey The name of the key.
** \param strDefault The default value for the key.
** \return The value for the key.
*/
const std::wstring& CConfigParser::GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault)
{
	std::wstring strTmp = strSection + L"~";
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToLowerC(strTmp));
	if (iter != m_Values.end())
	{
		return (*iter).second;
	}

	return strDefault;
}
