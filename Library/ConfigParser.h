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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <cstdint>
#include <gdiplus.h>

class CRainmeter;
class CMeterWindow;
class CMeasure;

class CConfigParser
{
public:
	CConfigParser();
	~CConfigParser();

	void Initialize(const std::wstring& filename, CMeterWindow* meterWindow = NULL, LPCTSTR skinSection = NULL, const std::wstring* resourcePath = NULL);

	void AddMeasure(CMeasure* pMeasure);
	CMeasure* GetMeasure(const std::wstring& name);

	bool GetVariable(const std::wstring& strVariable, std::wstring& strValue);
	void SetVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_Variables, strVariable, strValue); }
	void SetBuiltInVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(m_BuiltInVariables, strVariable, strValue); }

	const std::unordered_map<std::wstring, std::wstring>& GetVariables() { return m_Variables; }

	void SetCurrentSection(const std::wstring& strSection) { m_CurrentSection->assign(strSection); }
	void ClearCurrentSection() { m_CurrentSection->clear(); }

	const std::wstring& GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault);
	void SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue);
	void DeleteValue(const std::wstring& strSection, const std::wstring& strKey);

	void SetStyleTemplate(const std::wstring& strStyle) { static const std::wstring delim(1, L'|'); Tokenize(strStyle, delim).swap(m_StyleTemplate); Shrink(m_StyleTemplate); }
	void ClearStyleTemplate() { m_StyleTemplate.clear(); }

	bool GetLastReplaced() { return m_LastReplaced; }
	bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
	bool GetLastKeyDefined() { return !m_LastDefaultUsed; }
	bool GetLastValueDefined() { return m_LastValueDefined; }

	void ResetMonitorVariables(CMeterWindow* meterWindow = NULL);

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures = true);
	bool IsKeyDefined(LPCTSTR section, LPCTSTR key);
	bool IsValueDefined(LPCTSTR section, LPCTSTR key);
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	uint32_t ReadUInt(LPCTSTR section, LPCTSTR key, uint32_t defValue);
	uint64_t ReadUInt64(LPCTSTR section, LPCTSTR key, uint64_t defValue);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	Gdiplus::ARGB ReadColor(LPCTSTR section, LPCTSTR key, Gdiplus::ARGB defValue);
	Gdiplus::Rect ReadRect(LPCTSTR section, LPCTSTR key, const Gdiplus::Rect& defValue);
	RECT ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue);
	std::vector<Gdiplus::REAL> ReadFloats(LPCTSTR section, LPCTSTR key);

	bool ParseFormula(const std::wstring& formula, double* resultValue);

	const std::list<std::wstring>& GetSections() { return m_Sections; }

	bool ReplaceVariables(std::wstring& result);
	bool ReplaceMeasures(std::wstring& result);

	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static void Shrink(std::vector<std::wstring>& vec);
	static double ParseDouble(LPCTSTR string, double defValue);
	static int ParseInt(LPCTSTR string, int defValue);
	static uint32_t ParseUInt(LPCTSTR string, uint32_t defValue);
	static uint64_t ParseUInt64(LPCTSTR string, uint64_t defValue);
	static Gdiplus::ARGB ParseColor(LPCTSTR string);
	static Gdiplus::Rect ParseRect(LPCTSTR string);
	static RECT ParseRECT(LPCTSTR string);

	static void ClearMultiMonitorVariables() { c_MonitorVariables.clear(); }
	static void UpdateWorkareaVariables() { SetMultiMonitorVariables(false); }

private:
	void SetBuiltInVariables(const std::wstring& filename, const std::wstring* resourcePath, CMeterWindow* meterWindow);

	void ReadVariables();

	void ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection = NULL, int depth = 0);

	void SetAutoSelectedMonitorVariables(CMeterWindow* meterWindow);

	static void SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const std::wstring& strVariable, const std::wstring& strValue);
	static void SetVariable(std::unordered_map<std::wstring, std::wstring>& variables, const WCHAR* strVariable, const WCHAR* strValue);

	static void SetMultiMonitorVariables(bool reset);
	static void SetMonitorVariable(const std::wstring& strVariable, const std::wstring& strValue) { SetVariable(c_MonitorVariables, strVariable, strValue); }
	static void SetMonitorVariable(const WCHAR* strVariable, const WCHAR* strValue) { SetVariable(c_MonitorVariables, strVariable, strValue); }

	static std::wstring StrToUpper(const std::wstring& str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring StrToUpper(const WCHAR* str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	std::unordered_map<std::wstring, CMeasure*> m_Measures;

	std::vector<std::wstring> m_StyleTemplate;

	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;

	std::wstring* m_CurrentSection;

	std::list<std::wstring> m_Sections;		// Ordered section
	std::unordered_map<std::wstring, std::wstring> m_Values;

	std::unordered_set<std::wstring> m_FoundSections;
	std::list<std::wstring> m_ListVariables;
	std::list<std::wstring>::const_iterator m_SectionInsertPos;

	std::unordered_map<std::wstring, std::wstring> m_BuiltInVariables;
	std::unordered_map<std::wstring, std::wstring> m_Variables;

	static std::unordered_map<std::wstring, std::wstring> c_MonitorVariables;
};

#endif
