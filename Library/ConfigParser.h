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
#include <hash_map>
#include <gdiplus.h>
#include "ccalc-0.5.1/mparser.h"

class CRainmeter;
class CMeasure;

class CConfigParser
{
public:
	CConfigParser();
	~CConfigParser();

	void Initialize(LPCTSTR filename, CRainmeter* pRainmeter);
	void AddMeasure(CMeasure* pMeasure);
	void SetVariable(const std::wstring& strVariable, const std::wstring& strValue);

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures = true);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	double ReadFormula(LPCTSTR section, LPCTSTR key, double defValue);
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	Gdiplus::Color ReadColor(LPCTSTR section, LPCTSTR key, Gdiplus::Color defValue);
	std::vector<Gdiplus::REAL> ReadFloats(LPCTSTR section, LPCTSTR key);

	std::wstring& GetFilename() { return m_Filename; }

private:
	void ReadVariables();
	Gdiplus::Color ParseColor(LPCTSTR string);
	std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring delimiters);

	void ReadIniFile(const std::wstring& strFileName);
	void SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue);
	const std::wstring& GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault);
	std::vector<std::wstring> GetSections();
	std::vector<std::wstring> GetKeys(const std::wstring& strSection);

	std::map<std::wstring, std::wstring> m_Variables;
	std::wstring m_Filename;

	hqMathParser* m_Parser;
	std::map<std::wstring, CMeasure*> m_Measures;

	stdext::hash_map<std::wstring, std::vector<std::wstring> > m_Keys;
	stdext::hash_map<std::wstring, std::wstring> m_Values;
};

#endif
