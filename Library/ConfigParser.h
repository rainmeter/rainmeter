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

#pragma warning(disable: 4503)

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
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

	void Initialize(LPCTSTR filename, CRainmeter* pRainmeter, CMeterWindow* meterWindow = NULL, LPCTSTR config = NULL);
	void AddMeasure(CMeasure* pMeasure);

	void SetVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_Variables, strVariable, strValue); }
	void SetBuiltInVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_BuiltInVariables, strVariable, strValue); }
	bool GetVariable(const std::wstring& strVariable, std::wstring& strValue);

	const std::wstring& GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault);
	void SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue);
	void DeleteValue(const std::wstring& strSection, const std::wstring& strKey);

	void SetStyleTemplate(const std::wstring& strStyle) { m_StyleTemplate =  Tokenize(strStyle, L"|"); Shrink(m_StyleTemplate); }
	void ClearStyleTemplate() { m_StyleTemplate.clear(); }

	const std::wstring& GetLastUsedStyle() { return m_LastUsedStyle; }
	bool GetLastReplaced() { return m_LastReplaced; }
	bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
	bool GetLastKeyDefined() { return !m_LastDefaultUsed; }
	bool GetLastValueDefined() { return m_LastValueDefined; }

	void ResetMonitorVariables(CMeterWindow* meterWindow = NULL);

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures = true);
	bool IsKeyDefined(LPCTSTR section, LPCTSTR key);
	bool IsValueDefined(LPCTSTR section, LPCTSTR key);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	double ReadFormula(LPCTSTR section, LPCTSTR key, double defValue);
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	unsigned int ReadUInt(LPCTSTR section, LPCTSTR key, unsigned int defValue);
	Gdiplus::Color ReadColor(LPCTSTR section, LPCTSTR key, const Gdiplus::Color& defValue);
	Gdiplus::Rect ReadRect(LPCTSTR section, LPCTSTR key, const Gdiplus::Rect& defValue);
	RECT ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue);
	std::vector<Gdiplus::REAL> ReadFloats(LPCTSTR section, LPCTSTR key);

	bool ReadFormula(const std::wstring& result, double* resultValue);

	const std::wstring& GetFilename() { return m_Filename; }
	const std::list<std::wstring>& GetSections() { return m_Sections; }

	bool ReplaceVariables(std::wstring& result);
	bool ReplaceMeasures(std::wstring& result);

	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static void Shrink(std::vector<std::wstring>& vec);
	static double ParseDouble(const std::wstring& string, double defValue, bool rejectExp = false);
	static Gdiplus::Color ParseColor(LPCTSTR string);
	static Gdiplus::Rect ParseRect(LPCTSTR string);
	static RECT ParseRECT(LPCTSTR string);

	static void ClearMultiMonitorVariables() { c_MonitorVariables.clear(); }
	static void UpdateWorkareaVariables() { SetMultiMonitorVariables(false); }

private:
	void SetBuiltInVariables(CRainmeter* pRainmeter, CMeterWindow* meterWindow);

	void ReadVariables();

	CMeasure* GetMeasure(const std::wstring& name);

	void ReadIniFile(const std::wstring& strFileName, LPCTSTR config = NULL, int depth = 0);

	void SetAutoSelectedMonitorVariables(CMeterWindow* meterWindow);

	static void SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue);

	static void SetMultiMonitorVariables(bool reset);
	static void SetMonitorVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(c_MonitorVariables, strVariable, strValue); }

	static std::wstring StrToLower(const std::wstring& str) { std::wstring strTmp(str); return StrToLowerC(strTmp); }
	static std::wstring StrToLower(const WCHAR* str) { std::wstring strTmp(str); return StrToLowerC(strTmp); }
	static std::wstring& StrToLowerC(std::wstring& str) { std::transform(str.begin(), str.end(), str.begin(), ::towlower); return str; }

	std::wstring m_Filename;

	hqMathParser* m_Parser;
	std::unordered_map<std::wstring, CMeasure*> m_Measures;

	std::vector<std::wstring> m_StyleTemplate;

	std::wstring m_LastUsedStyle;
	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;

	std::list<std::wstring> m_Sections;		// The sections must be an ordered array
	std::unordered_map<std::wstring, std::wstring> m_Values;

	std::unordered_set<std::wstring> m_FoundSections;
	std::list<std::wstring> m_ListVariables;

	std::unordered_map<std::wstring, std::wstring> m_BuiltInVariables;         // Built-in variables
	std::unordered_map<std::wstring, std::wstring> m_Variables;                // User-defined variables

	static std::unordered_map<std::wstring, std::wstring> c_MonitorVariables;  // Monitor variables
};

#endif
