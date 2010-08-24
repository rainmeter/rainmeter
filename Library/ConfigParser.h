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
class CMeterWindow;
class CMeasure;

class CConfigParser
{
public:
	CConfigParser();
	~CConfigParser();

	void Initialize(LPCTSTR filename, CRainmeter* pRainmeter, CMeterWindow* meterWindow = NULL);
	void AddMeasure(CMeasure* pMeasure);

	void SetVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_Variables, strVariable, strValue); }

	void SetStyleTemplate(const std::wstring& strStyle) { m_StyleTemplate = strStyle; }
	void ClearStyleTemplate() { m_StyleTemplate.clear(); }

	void ResetMonitorVariables(CMeterWindow* meterWindow = NULL);

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures = true, bool* bReplaced = NULL);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	double ReadFormula(LPCTSTR section, LPCTSTR key, double defValue);
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	Gdiplus::Color ReadColor(LPCTSTR section, LPCTSTR key, const Gdiplus::Color& defValue);
	std::vector<Gdiplus::REAL> ReadFloats(LPCTSTR section, LPCTSTR key);

	const std::wstring& GetFilename() { return m_Filename; }
	const std::vector<std::wstring>& GetSections();

	// Returns an int if the formula was read successfully, -1 for failure.
	int ReadFormula(const std::wstring& result, double* number);

	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring delimiters);
	static double ParseDouble(const std::wstring& string, double defValue, bool rejectExp = false);
	static Gdiplus::Color ParseColor(LPCTSTR string);

	static void ClearMultiMonitorVariables() { c_MonitorVariables.clear(); }
	static void UpdateWorkareaVariables() { SetMultiMonitorVariables(false); }

private:
	void SetBuiltInVariables(CRainmeter* pRainmeter, CMeterWindow* meterWindow);
	void SetBuiltInVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_BuiltInVariables, strVariable, strValue); }

	bool GetVariable(const std::wstring& strVariable, std::wstring& strValue);
	void ReadVariables();
	bool ReplaceVariables(std::wstring& result);
	bool ReplaceMeasures(std::wstring& result);
	CMeasure* GetMeasure(const std::wstring& name);

	void ReadIniFile(const std::vector<std::wstring>& iniFileMappings, const std::wstring& strFileName, int depth = 0);
	void SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue);
	const std::wstring& GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault);
	std::vector<std::wstring> GetKeys(const std::wstring& strSection);

	void SetAutoSelectedMonitorVariables(CMeterWindow* meterWindow);

	static void SetVariable(std::map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue);

	static void SetMultiMonitorVariables(bool reset);
	static void SetMonitorVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(c_MonitorVariables, strVariable, strValue); }

	std::wstring m_Filename;

	hqMathParser* m_Parser;
	std::map<std::wstring, CMeasure*> m_Measures;

	std::wstring m_StyleTemplate;

	std::vector<std::wstring> m_Sections;		// The sections must be an ordered array
	stdext::hash_map<std::wstring, std::vector<std::wstring> > m_Keys;
	stdext::hash_map<std::wstring, std::wstring> m_Values;

	std::map<std::wstring, std::wstring> m_BuiltInVariables;         // Built-in variables
	std::map<std::wstring, std::wstring> m_Variables;                // User-defined variables

	static std::map<std::wstring, std::wstring> c_MonitorVariables;  // Monitor variables
};

#endif
