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

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include "ConfigParser.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include <algorithm>

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

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
void CConfigParser::Initialize(LPCTSTR filename, CRainmeter* pRainmeter)
{
	m_Filename = filename;

	m_Variables.clear();
	m_Measures.clear();
	m_Keys.clear();
	m_Values.clear();

	// Set the default variables. Do this before the ini file is read so that the paths can be used with @include
	if (pRainmeter)
	{
		SetVariable(L"PROGRAMPATH", pRainmeter->GetPath());
		SetVariable(L"SETTINGSPATH", pRainmeter->GetSettingsPath());
		SetVariable(L"SKINSPATH", pRainmeter->GetSkinPath());
		SetVariable(L"PLUGINSPATH", pRainmeter->GetPluginPath());
		SetVariable(L"CURRENTPATH", CRainmeter::ExtractPath(filename));
		SetVariable(L"ADDONSPATH", pRainmeter->GetPath() + L"Addons\\");

		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

		TCHAR buffer[256];
		swprintf(buffer, L"%i", workArea.left);
		SetVariable(L"WORKAREAX", buffer);
		swprintf(buffer, L"%i", workArea.top);
		SetVariable(L"WORKAREAY", buffer);
		swprintf(buffer, L"%i", workArea.right - workArea.left);
		SetVariable(L"WORKAREAWIDTH", buffer);
		swprintf(buffer, L"%i", workArea.bottom - workArea.top);
		SetVariable(L"WORKAREAHEIGHT", buffer);
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CXSCREEN));
		SetVariable(L"SCREENAREAWIDTH", buffer);
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CYSCREEN));
		SetVariable(L"SCREENAREAHEIGHT", buffer);
	}

	ReadIniFile(m_Filename);
	ReadVariables();
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

/**
** Replaces environment and internal variables in the given string.
** 
** \param result The string where the variables are returned. The string is modified.
*/
void CConfigParser::ReplaceVariables(std::wstring& result)
{
	CRainmeter::ExpandEnvironmentVariables(result);

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
							std::wstring value = (*iter).second->GetStringValue(true, 1, 5, false);
	
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

	return wcstod(result.c_str(), NULL);
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
		result.push_back((Gdiplus::REAL)wcstod(tokens[i].c_str(), NULL));
	}
	return result;
}

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%i", defValue);

	const std::wstring& result = ReadString(section, key, buffer);

	return _wtoi(result.c_str());
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
	return wcstod(result.c_str(), NULL);
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
		}
		else
		{
			R = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			G = _wtoi(token);
		}
		else
		{
			G = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			B = _wtoi(token);
		}
		else
		{
			B = 255;
		}
		token = wcstok( NULL, L",");
		if (token != NULL)
		{
			A = _wtoi(token);
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

		if (wcslen(string) > 6 && !isspace(string[6]))
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
	// DebugLog(L"Reading file: %s", iniFile.c_str());

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
std::vector<std::wstring> CConfigParser::GetSections()
{
	std::vector<std::wstring> listSections;

	stdext::hash_map<std::wstring, std::vector<std::wstring> >::iterator iter = m_Keys.begin();
	for ( ; iter != m_Keys.end(); iter++)
	{
		listSections.push_back((*iter).first);
	}

	return listSections;
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

