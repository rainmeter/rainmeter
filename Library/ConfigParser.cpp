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

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

std::map<std::wstring, std::wstring> CConfigParser::c_MonitorVariables;

/*
** CConfigParser
**
** The constructor
**
*/
CConfigParser::CConfigParser()
{
	m_Parser = MathParser_Create(NULL);
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
void CConfigParser::Initialize(LPCTSTR filename, CRainmeter* pRainmeter, CMeterWindow* meterWindow)
{
	m_Filename = filename;

	m_BuiltInVariables.clear();
	m_Variables.clear();
	m_Measures.clear();
	m_Keys.clear();
	m_Values.clear();
	m_Sections.clear();

	// Set the built-in variables. Do this before the ini file is read so that the paths can be used with @include
	SetBuiltInVariables(pRainmeter, meterWindow);
	ResetMonitorVariables(meterWindow);

	std::vector<std::wstring> iniFileMappings;
	CSystem::GetIniFileMappingList(iniFileMappings);

	ReadIniFile(iniFileMappings, m_Filename);
	ReadVariables();
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

		if (meterWindow)
		{
			const std::wstring& config = meterWindow->GetSkinName();
			std::wstring path = pRainmeter->GetSkinPath();

			std::wstring::size_type loc;
			if ((loc = config.find_first_of(L'\\')) != std::wstring::npos)
			{
				path += config.substr(0, loc + 1);
			}
			else
			{
				path += config;
				path += L"\\";
			}
			SetBuiltInVariable(L"ROOTCONFIGPATH", path);
		}
	}
	if (meterWindow)
	{
		SetBuiltInVariable(L"CURRENTCONFIG", meterWindow->GetSkinName());
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
	std::vector<std::wstring> listVariables = GetKeys(L"Variables");

	for (size_t i = 0; i < listVariables.size(); ++i)
	{
		SetVariable(listVariables[i], ReadString(L"Variables", listVariables[i].c_str(), L"", false));
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
void CConfigParser::SetVariable(std::map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue)
{
	// LogWithArgs(LOG_DEBUG, L"Variable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), (int)m_Variables.size());

	std::wstring strTmp(strVariable);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);
	variables[strTmp] = strValue;
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
	std::wstring strTmp(strVariable);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);

	// #1: Built-in variables
	std::map<std::wstring, std::wstring>::const_iterator iter = m_BuiltInVariables.find(strTmp);
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

		for (size_t i = 0; i < monitors.size(); ++i)
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
				std::wstring strVariable(result.begin() + pos + 1, result.begin() + end);
				std::wstring strValue;

				if (GetVariable(strVariable, strValue))
				{
					// Variable found, replace it with the value
					result.replace(result.begin() + pos, result.begin() + end + 1, strValue);
					start = pos + strValue.length();
					replaced = true;
				}
				else
				{
					start = end;
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
	} while(loop);

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
						std::wstring var(result.begin() + pos + 1, result.begin() + end);

						CMeasure* measure = GetMeasure(var);
						if (measure)
						{
							std::wstring value = measure->GetStringValue(false, 1, -1, false);

							// Measure found, replace it with the value
							result.replace(result.begin() + pos, result.begin() + end + 1, value);
							start = pos + value.length();
							replaced = true;
						}
						else
						{
							start = end;
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
		} while(loop);
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

	if (section == NULL)
	{
		section = L"";
	}
	if (key == NULL)
	{
		key = L"";
	}
	if (defValue == NULL)
	{
		defValue = L"";
	}

	// Clear last status
	m_LastUsedStyle.clear();
	m_LastReplaced = false;
	m_LastDefaultUsed = false;

	std::wstring strDefault = defValue;

	// If the template is defined read the value first from there.
	if (!m_StyleTemplate.empty())
	{
		std::vector<std::wstring>::const_reverse_iterator iter = m_StyleTemplate.rbegin();
		for ( ; iter != m_StyleTemplate.rend(); ++iter)
		{
			if (!(*iter).empty())
			{
				std::wstring strSection = (*iter);

				std::wstring::size_type pos = strSection.find_first_not_of(L" \t\r\n");
				if (pos != std::wstring::npos)
				{
					std::wstring::size_type lastPos = strSection.find_last_not_of(L" \t\r\n");
					if (pos != 0 || lastPos != (strSection.length() - 1))
					{
						// Trim white-space
						strSection.swap(std::wstring(strSection, pos, lastPos - pos + 1));
					}

					const std::wstring& strStyle = GetValue(strSection, key, strDefault);

					//LogWithArgs(LOG_DEBUG, L"[%s] %s (from [%s]) : strDefault=%s (0x%p), strStyle=%s (0x%p)",
					//	section, key, strSection.c_str(), strDefault.c_str(), &strDefault, strStyle.c_str(), &strStyle);

					if (&strStyle != &strDefault)
					{
						strDefault = strStyle;
						m_LastUsedStyle = strSection;
						break;
					}
				}
			}
		}
	}

	const std::wstring& strValue = GetValue(section, key, strDefault);
	result = strValue;

	if (!m_LastUsedStyle.empty())
	{
		if (&strValue != &strDefault)
		{
			m_LastUsedStyle.clear();
		}
	}
	else
	{
		if (&strValue == &strDefault)
		{
			m_LastDefaultUsed = true;
			return result;
		}
	}

	// Check Litestep vars
	if (Rainmeter && !Rainmeter->GetDummyLitestep())
	{
		std::string ansi = ConvertToAscii(result.c_str());
		char buffer[4096];	// lets hope the buffer is large enough...

		if (ansi.size() < 4096)
		{
			VarExpansion(buffer, ansi.c_str());
			result = ConvertToWide(buffer);
		}
	}

	SetVariable(L"CURRENTSECTION", section);  // Set temporarily

	if (ReplaceVariables(result))
	{
		m_LastReplaced = true;
	}

	SetVariable(L"CURRENTSECTION", L"");  // Reset

	if (bReplaceMeasures && ReplaceMeasures(result))
	{
		m_LastReplaced = true;
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
		m_Measures[pMeasure->GetName()] = pMeasure;
	}
}

CMeasure* CConfigParser::GetMeasure(const std::wstring& name)
{
	std::map<std::wstring, CMeasure*>::const_iterator iter = m_Measures.begin();
	for ( ; iter != m_Measures.end(); ++iter)
	{
		if (_wcsicmp((*iter).first.c_str(), name.c_str()) == 0)
		{
			return (*iter).second;
		}
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
	std::wstring tmp = ReadString(section, key, L"");
	if (!tmp.empty() && tmp[tmp.length() - 1] != L';')
	{
		tmp += L";";
	}

	// Tokenize and parse the floats
	std::vector<std::wstring> tokens = Tokenize(tmp, L";");
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		result.push_back((Gdiplus::REAL)ParseDouble(tokens[i], 0));
	}
	return result;
}

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : (int)ParseDouble(result, defValue, true);
}

// Works as ReadFloat except if the value is surrounded by parenthesis in which case it tries to evaluate the formula
double CConfigParser::ReadFormula(LPCTSTR section, LPCTSTR key, double defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	// Formulas must be surrounded by parenthesis
	if (!result.empty() && result[0] == L'(' && result[result.size() - 1] == L')')
	{
		double resultValue = defValue;
		char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(result.substr(1, result.size() - 2).c_str()).c_str(), &resultValue);
		if (errMsg != NULL)
		{
			Log(LOG_ERROR, ConvertToWide(errMsg).c_str());
		}

		return resultValue;
	}

	return (m_LastDefaultUsed) ? defValue : ParseDouble(result, defValue);
}

// Returns an int if the formula was read successfully, -1 for failure.
// Pass a pointer to a double.
int CConfigParser::ReadFormula(const std::wstring& result, double* resultValue)
{
	// Formulas must be surrounded by parenthesis
	if (!result.empty() && result[0] == L'(' && result[result.size() - 1] == L')')
	{
		char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(result.substr(1, result.size() - 2).c_str()).c_str(), resultValue);
		
		if (errMsg != NULL)
		{
			Log(LOG_ERROR, ConvertToWide(errMsg).c_str());
			return -1;
		}

		return 1;
	}

	return -1;
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
	int R, G, B, A;

	if(wcschr(string, L',') != NULL)
	{
		WCHAR* parseSz = _wcsdup(string);
		WCHAR* token;

		token = wcstok(parseSz, L",");
		if (token != NULL)
		{
			R = _wtoi(token);
			R = max(R, 0);
			R = min(R, 255);
		}
		else
		{
			R = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			G = _wtoi(token);
			G = max(G, 0);
			G = min(G, 255);
		}
		else
		{
			G = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			B = _wtoi(token);
			B = max(B, 0);
			B = min(B, 255);
		}
		else
		{
			B = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			A = _wtoi(token);
			A = max(A, 0);
			A = min(A, 255);
		}
		else
		{
			A = 255;
		}
		free(parseSz);
	} 
	else
	{
		const WCHAR* start = string;

		if (wcsncmp(string, L"0x", 2) == 0)
		{
			start = string + 2;
		}

		if (wcslen(string) > 6 && !iswspace(string[6]))
		{
			swscanf(string, L"%02x%02x%02x%02x", &R, &G, &B, &A);
		} 
		else 
		{
			swscanf(string, L"%02x%02x%02x", &R, &G, &B);
			A = 255;	// Opaque
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
void CConfigParser::ReadIniFile(const std::vector<std::wstring>& iniFileMappings, const std::wstring& iniFile, int depth)
{
	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		MessageBox(NULL, L"It looks like you've made an infinite\nloop with the @include statements.\nPlease check your skin.", L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONERROR);
		return;
	}

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		LogWithArgs(LOG_WARNING, L"Unable to read file: %s", iniFile.c_str());
		return;
	}

	// Avoid "IniFileMapping"
	std::wstring iniRead = CSystem::GetTemporaryFile(iniFileMappings, iniFile);
	bool temporary = (!iniRead.empty() && iniRead != L"<>");

	if (temporary)
	{
		if (CRainmeter::GetDebug()) LogWithArgs(LOG_DEBUG, L"Reading file: %s (Temp: %s)", iniFile.c_str(), iniRead.c_str());
	}
	else
	{
		if (CRainmeter::GetDebug()) LogWithArgs(LOG_DEBUG, L"Reading file: %s", iniFile.c_str());
		iniRead = iniFile;
	}

	// Get all the sections (i.e. different meters)
	WCHAR* items = new WCHAR[MAX_LINE_LENGTH];
	int size = MAX_LINE_LENGTH;
	WCHAR* epos = NULL;

	// Get all the sections
	while(true)
	{
		items[0] = 0;
		int res = GetPrivateProfileString( NULL, NULL, NULL, items, size, iniRead.c_str());
		if (res == 0)		// File not found
		{
			delete [] items;
			if (temporary) CSystem::RemoveFile(iniRead);
			return;
		}
		if (res < size - 2)		// Fits in the buffer
		{
			epos = items + res;
			break;
		}

		delete [] items;
		size *= 2;
		items = new WCHAR[size];
	}

	// Read the sections
	std::list<std::wstring> sections;
	WCHAR* pos = items;
	while (pos < epos)
	{
		if (*pos)
		{
			std::wstring strTmp(pos);
			std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);
			if (m_Keys.find(strTmp) == m_Keys.end())
			{
				m_Keys[strTmp] = std::vector<std::wstring>();
				m_Sections.push_back(pos);
			}
			sections.push_back(pos);
			pos += wcslen(pos) + 1;
		}
		else  // Empty string
		{
			++pos;
		}
	}

	// Read the keys and values
	int bufferSize = MAX_LINE_LENGTH;
	WCHAR* buffer = new WCHAR[bufferSize];

	std::list<std::wstring>::const_iterator iter = sections.begin();
	for ( ; iter != sections.end(); ++iter)
	{
		while(true)
		{
			items[0] = 0;
			int res = GetPrivateProfileString((*iter).c_str(), NULL, NULL, items, size, iniRead.c_str());
			if (res < size - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			size *= 2;
			items = new WCHAR[size];
		}

		pos = items;
		while (pos < epos)
		{
			if (*pos)
			{
				std::wstring strKey = pos;

				while(true)
				{
					buffer[0] = 0;
					int res = GetPrivateProfileString((*iter).c_str(), strKey.c_str(), L"", buffer, bufferSize, iniRead.c_str());
					if (res < bufferSize - 2) break;		// Fits in the buffer

					delete [] buffer;
					bufferSize *= 2;
					buffer = new WCHAR[bufferSize];
				}

				if (_wcsnicmp(strKey.c_str(), L"@include", 8) == 0)
				{
					std::wstring strIncludeFile = buffer;
					ReadVariables();
					ReplaceVariables(strIncludeFile);
					if (strIncludeFile.find(L':') == std::wstring::npos &&
						(strIncludeFile.length() < 2 || (strIncludeFile[0] != L'\\' && strIncludeFile[0] != L'/') || (strIncludeFile[1] != L'\\' && strIncludeFile[1] != L'/')))
					{
						// It's a relative path so add the current path as a prefix
						strIncludeFile = CRainmeter::ExtractPath(iniFile) + strIncludeFile;
					}
					ReadIniFile(iniFileMappings, strIncludeFile, depth + 1);
				}
				else
				{
					SetValue((*iter), strKey, buffer);
				}

				pos += wcslen(pos) + 1;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}
	delete [] buffer;
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

	std::wstring strTmpSection(strSection);
	std::wstring strTmpKey(strKey);
	std::transform(strTmpSection.begin(), strTmpSection.end(), strTmpSection.begin(), ::towlower);
	std::transform(strTmpKey.begin(), strTmpKey.end(), strTmpKey.begin(), ::towlower);

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::iterator iter = m_Keys.find(strTmpSection);
	if (iter != m_Keys.end())
	{
		std::vector<std::wstring>& array = (*iter).second;
		array.push_back(strTmpKey);
	}

	strTmpSection += L"::";
	strTmpSection += strTmpKey;
	m_Values[strTmpSection] = strValue;
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
	std::wstring strTmp(strSection + L"::");
	strTmp += strKey;
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);

	stdext::hash_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(strTmp);
	if (iter != m_Values.end())
	{
		return (*iter).second;
	}

	return strDefault;
}

//==============================================================================
/**
** Returns the list of sections in the ini file.
** 
** \return A list of sections in the ini file.
*/
const std::vector<std::wstring>& CConfigParser::GetSections()
{
	return m_Sections;
}

//==============================================================================
/**
** Returns a list of keys under the given section.
** 
** \param strSection The name of the section.
** \return A list of keys under the given section.
*/
std::vector<std::wstring> CConfigParser::GetKeys(const std::wstring& strSection)
{
	std::wstring strTmp(strSection);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::towlower);

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::const_iterator iter = m_Keys.find(strTmp);
	if (iter != m_Keys.end())
	{
		return (*iter).second;
	}

	return std::vector<std::wstring>();
}
