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
#include <algorithm>

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

	m_Variables.clear();
	m_Measures.clear();
	m_Keys.clear();
	m_Values.clear();
	m_Sections.clear();

	// Set the default variables. Do this before the ini file is read so that the paths can be used with @include
	SetDefaultVariables(pRainmeter, meterWindow);

	// Set the SCREENAREA/WORKAREA variables
	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Set the SCREENAREA/WORKAREA variables for present monitor
	SetAutoSelectedMonitorVariables(meterWindow);

	ReadIniFile(m_Filename);
	ReadVariables();
}

/*
** SetDefaultVariables
**
**
*/
void CConfigParser::SetDefaultVariables(CRainmeter* pRainmeter, CMeterWindow* meterWindow)
{
	if (pRainmeter)
	{
		SetVariable(L"PROGRAMPATH", pRainmeter->GetPath());
		SetVariable(L"SETTINGSPATH", pRainmeter->GetSettingsPath());
		SetVariable(L"SKINSPATH", pRainmeter->GetSkinPath());
		SetVariable(L"PLUGINSPATH", pRainmeter->GetPluginPath());
		SetVariable(L"CURRENTPATH", CRainmeter::ExtractPath(m_Filename));
		SetVariable(L"ADDONSPATH", pRainmeter->GetPath() + L"Addons\\");
	}
	if (meterWindow)
	{
		SetVariable(L"CURRENTCONFIG", meterWindow->GetSkinName());
	}
}

/*
** ReadVariables
**
**
*/
void CConfigParser::ReadVariables()
{
	std::vector<std::wstring> listVariables = GetKeys(L"Variables");

	for (size_t i = 0; i < listVariables.size(); i++)
	{
		SetVariable(listVariables[i], ReadString(L"Variables", listVariables[i].c_str(), L"", false));
	}
}

/**
** Sets a new value for the variable. The DynamicVariables must be set to 1 in the
** meter/measure for the changes to be applied.
** 
** \param strVariable
** \param strValue
*/
void CConfigParser::SetVariable(const std::wstring& strVariable, const std::wstring& strValue)
{
	// DebugLog(L"Variable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), m_Variables.size());

	std::wstring strTmp(strVariable);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);
	m_Variables[strTmp] = strValue;
}

/*
** ResetVariables
**
**
*/
void CConfigParser::ResetVariables(CRainmeter* pRainmeter, CMeterWindow* meterWindow)
{
	m_Variables.clear();

	// Set the default variables. Do this before the ini file is read so that the paths can be used with @include
	SetDefaultVariables(pRainmeter, meterWindow);

	// Set the SCREENAREA/WORKAREA variables
	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Set the SCREENAREA/WORKAREA variables for present monitor
	SetAutoSelectedMonitorVariables(meterWindow);

	ReadVariables();
}

/*
** SetMultiMonitorVariables
**
** Sets new values for the SCREENAREA/WORKAREA variables.
** 
*/
void CConfigParser::SetMultiMonitorVariables(bool reset)
{
	TCHAR buffer[256];
	RECT workArea, scrArea;

	if (!reset && c_MonitorVariables.empty())
	{
		reset = true;  // Set all variables
	}

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

	swprintf(buffer, L"%i", workArea.left);
	SetMonitorVariable(L"WORKAREAX", buffer);
	SetMonitorVariable(L"PWORKAREAX", buffer);
	swprintf(buffer, L"%i", workArea.top);
	SetMonitorVariable(L"WORKAREAY", buffer);
	SetMonitorVariable(L"PWORKAREAY", buffer);
	swprintf(buffer, L"%i", workArea.right - workArea.left);
	SetMonitorVariable(L"WORKAREAWIDTH", buffer);
	SetMonitorVariable(L"PWORKAREAWIDTH", buffer);
	swprintf(buffer, L"%i", workArea.bottom - workArea.top);
	SetMonitorVariable(L"WORKAREAHEIGHT", buffer);
	SetMonitorVariable(L"PWORKAREAHEIGHT", buffer);

	if (reset)
	{
		scrArea.left = 0;
		scrArea.top = 0;
		scrArea.right = GetSystemMetrics(SM_CXSCREEN);
		scrArea.bottom = GetSystemMetrics(SM_CYSCREEN);

		swprintf(buffer, L"%i", scrArea.left);
		SetMonitorVariable(L"SCREENAREAX", buffer);
		SetMonitorVariable(L"PSCREENAREAX", buffer);
		swprintf(buffer, L"%i", scrArea.top);
		SetMonitorVariable(L"SCREENAREAY", buffer);
		SetMonitorVariable(L"PSCREENAREAY", buffer);
		swprintf(buffer, L"%i", scrArea.right - scrArea.left);
		SetMonitorVariable(L"SCREENAREAWIDTH", buffer);
		SetMonitorVariable(L"PSCREENAREAWIDTH", buffer);
		swprintf(buffer, L"%i", scrArea.bottom - scrArea.top);
		SetMonitorVariable(L"SCREENAREAHEIGHT", buffer);
		SetMonitorVariable(L"PSCREENAREAHEIGHT", buffer);

		swprintf(buffer, L"%i", GetSystemMetrics(SM_XVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAX", buffer);
		swprintf(buffer, L"%i", GetSystemMetrics(SM_YVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAY", buffer);
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CXVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAWIDTH", buffer);
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CYVIRTUALSCREEN));
		SetMonitorVariable(L"VSCREENAREAHEIGHT", buffer);
	}

	if (CMeterWindow::GetMonitorCount() > 0)
	{
		const MULTIMONITOR_INFO& multimonInfo = CMeterWindow::GetMultiMonitorInfo();
		const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

		for (size_t i = 0; i < monitors.size(); i++)
		{
			TCHAR buffer2[256];

			const RECT work = (monitors[i].active) ? monitors[i].work : workArea;

			swprintf(buffer, L"%i", work.left);
			swprintf(buffer2, L"WORKAREAX@%i", i + 1);
			SetMonitorVariable(buffer2, buffer);
			swprintf(buffer, L"%i", work.top);
			swprintf(buffer2, L"WORKAREAY@%i", i + 1);
			SetMonitorVariable(buffer2, buffer);
			swprintf(buffer, L"%i", work.right - work.left);
			swprintf(buffer2, L"WORKAREAWIDTH@%i", i + 1);
			SetMonitorVariable(buffer2, buffer);
			swprintf(buffer, L"%i", work.bottom - work.top);
			swprintf(buffer2, L"WORKAREAHEIGHT@%i", i + 1);
			SetMonitorVariable(buffer2, buffer);

			if (reset)
			{
				const RECT screen = (monitors[i].active) ? monitors[i].screen : scrArea;

				swprintf(buffer, L"%i", screen.left);
				swprintf(buffer2, L"SCREENAREAX@%i", i + 1);
				SetMonitorVariable(buffer2, buffer);
				swprintf(buffer, L"%i", screen.top);
				swprintf(buffer2, L"SCREENAREAY@%i", i + 1);
				SetMonitorVariable(buffer2, buffer);
				swprintf(buffer, L"%i", screen.right - screen.left);
				swprintf(buffer2, L"SCREENAREAWIDTH@%i", i + 1);
				SetMonitorVariable(buffer2, buffer);
				swprintf(buffer, L"%i", screen.bottom - screen.top);
				swprintf(buffer2, L"SCREENAREAHEIGHT@%i", i + 1);
				SetMonitorVariable(buffer2, buffer);
			}
		}
	}
}

/**
** Sets a new value for the SCREENAREA/WORKAREA variable.
** 
** \param strVariable
** \param strValue
*/
void CConfigParser::SetMonitorVariable(const std::wstring& strVariable, const std::wstring& strValue)
{
	// DebugLog(L"MonitorVariable: %s=%s (size=%i)", strVariable.c_str(), strValue.c_str(), c_MonitorVariables.size());

	std::wstring strTmp(strVariable);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);
	c_MonitorVariables[strTmp] = strValue;
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
		if (CMeterWindow::GetMonitorCount() > 0)
		{
			TCHAR buffer[256];
			int w1, w2, s1, s2;

			const MULTIMONITOR_INFO& multimonInfo = CMeterWindow::GetMultiMonitorInfo();
			const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

			if (meterWindow->GetXScreenDefined())
			{
				int screenIndex = meterWindow->GetXScreen();

				if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= (int)monitors.size() &&
					screenIndex != multimonInfo.primary && monitors[screenIndex-1].active))
				{
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

					swprintf(buffer, L"%i", w1);
					SetVariable(L"WORKAREAX", buffer);
					swprintf(buffer, L"%i", w2);
					SetVariable(L"WORKAREAWIDTH", buffer);
					swprintf(buffer, L"%i", s1);
					SetVariable(L"SCREENAREAX", buffer);
					swprintf(buffer, L"%i", s2);
					SetVariable(L"SCREENAREAWIDTH", buffer);
				}
			}

			if (meterWindow->GetYScreenDefined())
			{
				int screenIndex = meterWindow->GetYScreen();

				if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= (int)monitors.size() &&
					screenIndex != multimonInfo.primary && monitors[screenIndex-1].active))
				{
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

					swprintf(buffer, L"%i", w1);
					SetVariable(L"WORKAREAY", buffer);
					swprintf(buffer, L"%i", w2);
					SetVariable(L"WORKAREAHEIGHT", buffer);
					swprintf(buffer, L"%i", s1);
					SetVariable(L"SCREENAREAY", buffer);
					swprintf(buffer, L"%i", s2);
					SetVariable(L"SCREENAREAHEIGHT", buffer);
				}
			}
		}
	}
}

/**
** Replaces environment and internal variables in the given string.
** 
** \param result The string where the variables are returned. The string is modified.
*/
void CConfigParser::ReplaceVariables(std::wstring& result)
{
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
				std::wstring strTmp(result.begin() + pos + 1, result.begin() + end);
				std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);
				
				std::map<std::wstring, std::wstring>::iterator iter = m_Variables.find(strTmp);
				if (iter != m_Variables.end())
				{
					// Variable found, replace it with the value
					result.replace(result.begin() + pos, result.begin() + end + 1, (*iter).second);
					start = pos + (*iter).second.length();
				}
				else
				{
					std::map<std::wstring, std::wstring>::iterator iter2 = c_MonitorVariables.find(strTmp);
					if (iter2 != c_MonitorVariables.end())
					{
						// SCREENAREA/WORKAREA variable found, replace it with the value
						result.replace(result.begin() + pos, result.begin() + end + 1, (*iter2).second);
						start = pos + (*iter2).second.length();
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
	} while(loop);
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

	std::wstring strDefault = defValue;

	// If the template is defined read the value first from there.
	if (!m_StyleTemplate.empty())
	{
		strDefault = GetValue(m_StyleTemplate, key, strDefault);
	}

	result = GetValue(section, key, strDefault);
	if (result == defValue)
	{
		return result;
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

	ReplaceVariables(result);

	// Check for measures ([Measure])
	if (!m_Measures.empty() && bReplaceMeasures)
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
	
						std::map<std::wstring, CMeasure*>::iterator iter = m_Measures.find(var);
						if (iter != m_Measures.end())
						{
							std::wstring value = (*iter).second->GetStringValue(false, 1, 5, false);
	
							// Measure found, replace it with the value
							result.replace(result.begin() + pos, result.begin() + end + 1, value);
							start = pos + value.length();
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

	return result;
}

void CConfigParser::AddMeasure(CMeasure* pMeasure)
{
	if (pMeasure)
	{
		m_Measures[pMeasure->GetName()] = pMeasure;
	}
}

double CConfigParser::ReadFloat(LPCTSTR section, LPCTSTR key, double defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%f", defValue);

	const std::wstring& result = ReadString(section, key, buffer);

	return ParseDouble(result, defValue);
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
	for (size_t i = 0; i < tokens.size(); i++)
	{
		result.push_back((Gdiplus::REAL)ParseDouble(tokens[i], 0));
	}
	return result;
}

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%i", defValue);

	const std::wstring& result = ReadString(section, key, buffer);

	return (int)ParseDouble(result, defValue, true);
}

// Works as ReadFloat except if the value is surrounded by parenthesis in which case it tries to evaluate the formula
double CConfigParser::ReadFormula(LPCTSTR section, LPCTSTR key, double defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%f", defValue);

	const std::wstring& result = ReadString(section, key, buffer);

	// Formulas must be surrounded by parenthesis
	if (!result.empty() && result[0] == L'(' && result[result.size() - 1] == L')')
	{
		double resultValue = defValue;
		char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(result.substr(1, result.size() - 2).c_str()).c_str(), &resultValue);
		if (errMsg != NULL)
		{
			DebugLog(ConvertToWide(errMsg).c_str());
		}

		return resultValue;
	}

	return ParseDouble(result, defValue);
}

Color CConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, Color defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%i, %i, %i, %i", defValue.GetR(), defValue.GetG(), defValue.GetB(), defValue.GetA());

	const std::wstring& result = ReadString(section, key, buffer);

	return ParseColor(result.c_str());
}

/*
** Tokenize
**
** Splits the string from the delimiters
**
** http://www.digitalpeer.com/id/simple
*/
std::vector<std::wstring> CConfigParser::Tokenize(const std::wstring& str, const std::wstring delimiters)
{
	std::vector<std::wstring> tokens;
    	
	std::wstring::size_type lastPos = str.find_first_not_of(L";", 0);	// skip delimiters at beginning.
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
	if (rejectExp)
	{
		if (string.find_last_of(L"dDeE") != std::wstring::npos)  // contains exponent part
		{
			return defValue;
		}
	}

	std::wstring::size_type pos1 = string.find_first_not_of(L" \t\r\n");
	if (pos1 != std::wstring::npos)
	{
		// Trim white-space
		std::wstring temp(string, pos1, string.find_last_not_of(L" \t\r\n") - pos1 + 1);

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

//==============================================================================
/**
** Reads the given ini file and fills the m_Values and m_Keys maps.
** 
** \param iniFile The ini file to be read.
*/
void CConfigParser::ReadIniFile(const std::wstring& iniFile, int depth)
{
	if (CRainmeter::GetDebug())
	{
		DebugLog(L"Reading file: %s", iniFile.c_str());
	}

	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		MessageBox(NULL, L"It looks like you've made an infinite\nloop with the @include statements.\nPlease check your skin.", L"Rainmeter", MB_OK | MB_ICONERROR);
		return;
	}

	// Get all the sections (i.e. different meters)
	WCHAR* items = new WCHAR[MAX_LINE_LENGTH];
	int size = MAX_LINE_LENGTH;

	// Get all the sections
	while(true)
	{
		items[0] = 0;
		int res = GetPrivateProfileString( NULL, NULL, NULL, items, size, iniFile.c_str());
		if (res == 0) { delete [] items; return; }		// File not found
		if (res < size - 2) break;		// Fits in the buffer

		delete [] items;
		size *= 2;
		items = new WCHAR[size];
	};

	// Read the sections
	WCHAR* pos = items;
	while(wcslen(pos) > 0)
	{
		std::wstring strTmp(pos);
		std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);
		if (m_Keys.find(strTmp) == m_Keys.end())
		{
			m_Keys[strTmp] = std::vector<std::wstring>();
			m_Sections.push_back(pos);
		}
		pos = pos + wcslen(pos) + 1;
	}

	// Read the keys and values
	int bufferSize = MAX_LINE_LENGTH;
	WCHAR* buffer = new WCHAR[bufferSize];

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::iterator iter = m_Keys.begin();
	for ( ; iter != m_Keys.end(); iter++)
	{
		while(true)
		{
			items[0] = 0;
			int res = GetPrivateProfileString((*iter).first.c_str(), NULL, NULL, items, size, iniFile.c_str());
			if (res < size - 2) break;		// Fits in the buffer

			delete [] items;
			size *= 2;
			items = new WCHAR[size];
		};

		WCHAR* pos = items;
		while(wcslen(pos) > 0)
		{
			std::wstring strKey = pos;

			while(true)
			{
				buffer[0] = 0;
				int res = GetPrivateProfileString((*iter).first.c_str(), strKey.c_str(), L"", buffer, bufferSize, iniFile.c_str());
				if (res < bufferSize - 2) break;		// Fits in the buffer

				delete [] buffer;
				bufferSize *= 2;
				buffer = new WCHAR[bufferSize];
			};

			if (wcsnicmp(strKey.c_str(), L"@include", 8) == 0)
			{
				std::wstring strIncludeFile = buffer;
				ReplaceVariables(strIncludeFile);
				if (strIncludeFile.find(L':') == std::wstring::npos)
				{
					// It's a relative path so add the current path as a prefix
					strIncludeFile = CRainmeter::ExtractPath(iniFile) + strIncludeFile;
				}
				ReadIniFile(strIncludeFile, depth + 1);
			}
			else
			{
				SetValue((*iter).first, strKey, buffer);
			}

			pos = pos + wcslen(pos) + 1;
		}
	}
	delete [] buffer;
	delete [] items;
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
	// DebugLog(L"[%s] %s=%s (size: %i)", strSection.c_str(), strKey.c_str(), strValue.c_str(), m_Values.size());

	std::wstring strTmpSection(strSection);
	std::wstring strTmpKey(strKey);
	std::transform(strTmpSection.begin(), strTmpSection.end(), strTmpSection.begin(), ::tolower);
	std::transform(strTmpKey.begin(), strTmpKey.end(), strTmpKey.begin(), ::tolower);

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::iterator iter = m_Keys.find(strTmpSection);
	if (iter != m_Keys.end())
	{
		std::vector<std::wstring>& array = (*iter).second;
		array.push_back(strTmpKey);
	}
	m_Values[strTmpSection + L"::" + strTmpKey] = strValue;
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
	std::wstring strTmp(strSection + L"::" + strKey);
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

	stdext::hash_map<std::wstring, std::wstring>::iterator iter = m_Values.find(strTmp);
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
	std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::iterator iter = m_Keys.find(strTmp);
	if (iter != m_Keys.end())
	{
		return (*iter).second;
	}

	return std::vector<std::wstring>();
}

