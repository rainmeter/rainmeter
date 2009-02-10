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
/*
  $Header: /home/cvsroot/Rainmeter/Library/ConfigParser.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: ConfigParser.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.1  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

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
}

/*
** ~CConfigParser
**
** The destructor
**
*/
CConfigParser::~CConfigParser()
{
}

/*
** Initialize
**
**
*/
void CConfigParser::Initialize(LPCTSTR filename)
{
	m_Filename = filename;
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

	m_Variables.clear();
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

	if (result.find(L'%') != std::wstring::npos) 
	{
		WCHAR buffer[4096];	// lets hope the buffer is large enough...

		// Expand the environment variables
		DWORD ret = ExpandEnvironmentStrings(result.c_str(), buffer, 4096);
		if (ret != 0 && ret < 4096)
		{
			result = buffer;
		}
		else
		{
			DebugLog(L"Unable to expand the environment strings.");
		}
	}

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

int CConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%i", defValue);

	const std::wstring& result = ReadString(section, key, buffer);

	return _wtoi(result.c_str());
}

Color CConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, Color defValue)
{
	TCHAR buffer[256];
	swprintf(buffer, L"%i, %i, %i, %i", defValue.GetR(), defValue.GetG(), defValue.GetB(), defValue.GetA());

	const std::wstring& result = ReadString(section, key, buffer);

	return ParseColor(result.c_str());
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
