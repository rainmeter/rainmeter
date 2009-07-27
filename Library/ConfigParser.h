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

#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__

#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include <gdiplus.h>

class CRainmeter;

class CConfigParser
{
public:
	CConfigParser();
	~CConfigParser();

	void Initialize(LPCTSTR filename, CRainmeter* pRainmeter);

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	Gdiplus::Color ReadColor(LPCTSTR section, LPCTSTR key, Gdiplus::Color defValue);
	std::vector<Gdiplus::REAL> ReadFloats(LPCTSTR section, LPCTSTR key);

	std::wstring& GetFilename() { return m_Filename; }

private:
	void ReadVariables();
	Gdiplus::Color ParseColor(LPCTSTR string);
	std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring delimiters);

	std::map<std::wstring, std::wstring> m_Variables;
	std::wstring m_Filename;
};

#endif
