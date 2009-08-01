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
#include <TCHAR.H>

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

	// Set the default variables
	if (pRainmeter)
	{
		m_Variables[L"PROGRAMPATH"] = pRainmeter->GetPath();
		m_Variables[L"SETTINGSPATH"] = pRainmeter->GetSettingsPath();
		m_Variables[L"SKINSPATH"] = pRainmeter->GetSkinPath();
		m_Variables[L"PLUGINSPATH"] = pRainmeter->GetPluginPath();
		m_Variables[L"CURRENTPATH"] = CRainmeter::ExtractPath(filename);
		m_Variables[L"ADDONSPATH"] = pRainmeter->GetPath() + L"Addons\\";

		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

		TCHAR buffer[256];
		swprintf(buffer, L"%i", workArea.right - workArea.left);
		m_Variables[L"WORKAREAWIDTH"] = buffer;
		swprintf(buffer, L"%i", workArea.bottom - workArea.top);
		m_Variables[L"WORKAREAHEIGHT"] = buffer;
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CXSCREEN));
		m_Variables[L"SCREENAREAWIDTH"] = buffer;
		swprintf(buffer, L"%i", GetSystemMetrics(SM_CYSCREEN));
		m_Variables[L"SCREENAREAHEIGHT"] = buffer;
	}

	ReadVariables();
}

/*
** ReadVariables
**
**
*/
void CConfigParser::ReadVariables()
{
	DWORD size;
	TCHAR* buffer;
	TCHAR* variable;
	int bufferSize = 4096;
	bool loop;

	do 
	{
		loop = false;
		buffer = new TCHAR[bufferSize];
		
		size = GetPrivateProfileString(L"Variables", NULL, L"", buffer, bufferSize, m_Filename.c_str());

		if (size == bufferSize - 1)
		{
			// Buffer too small, increase it and retry
			delete [] buffer;
			bufferSize *= 2;
			loop = true;
		}

	} while(loop);

	if (size > 0)
	{
		variable = new TCHAR[bufferSize];

		// Read all variables
		WCHAR* pos = buffer;
		while(wcslen(pos) > 0)
		{
			do 
			{
				loop = false;
				size = GetPrivateProfileString(L"Variables", pos, L"", variable, bufferSize, m_Filename.c_str());

				if (size == bufferSize - 1)
				{
					// Buffer too small, increase it and retry
					delete [] variable;
					bufferSize *= 2;
					variable = new TCHAR[bufferSize];
					loop = true;
				}

			} while(loop);

			if (wcslen(variable) > 0)
			{
				m_Variables[pos] = variable;
			}

			pos = pos + wcslen(pos) + 1;
		}

		delete [] variable;
	}
	delete [] buffer;
}

/*
** ReadString
**
**
*/
const std::wstring& CConfigParser::ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue)
{
	static std::wstring result;
	DWORD size;
	TCHAR* buffer;
	int bufferSize = 4096;
	bool loop;

	do 
	{
		loop = false;
		buffer = new TCHAR[bufferSize];
		buffer[0] = 0;
		
		size = GetPrivateProfileString(section, key, defValue, buffer, bufferSize, m_Filename.c_str());

		if (size == bufferSize - 1)
		{
			// Buffer too small, increase it and retry
			delete [] buffer;
			bufferSize *= 2;
			loop = true;
		}

	} while(loop);

	result = buffer;

	delete [] buffer;

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

	result = CRainmeter::ExpandEnvironmentVariables(result);

	// Check for variables (#VAR#)
	size_t start = 0;
	size_t end = std::wstring::npos;
	size_t pos = std::wstring::npos;
	loop = true;

	do 
	{
		pos = result.find(L'#', start);
		if (pos != std::wstring::npos)
		{
			size_t end = result.find(L'#', pos + 1);
			if (end != std::wstring::npos)
			{
				std::wstring var(result.begin() + pos + 1, result.begin() + end);
				
				std::map<std::wstring, std::wstring>::iterator iter = m_Variables.find(var);
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

	return result;
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
