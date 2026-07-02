/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__

#pragma warning(disable: 4503)

#include "../Common/ParseUtil.h"
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <d2d1.h>

class Rainmeter;
class Skin;
class Section;
class Measure;
class Meter;

class ConfigParser
{
public:
	enum class VariableType : BYTE
	{										// Old Style:                         New Style:
		Section,							// [MeasureName], [Meter:X], etc.     [&MeasureName], [&Meter:X], etc.
		Variable,							// #Variable#                         [#Variable]
		Mouse,								// $MouseX$, $MouseX:%$, etc.         [$MouseX], [$MouseX:%], etc.
		CharacterReference					// Not available.                     [\8364], [\x20AC], [\X20AC], etc.
	};

	enum class MonitorVariableMode : BYTE
	{
		DEFAULT_LOGICAL,
		FORCE_PHYSICAL
	};

	ConfigParser();
	~ConfigParser();

	ConfigParser(const ConfigParser& other) = delete;
	ConfigParser& operator=(ConfigParser other) = delete;

	void Initialize(const std::wstring& filename, Skin* skin = nullptr, LPCTSTR skinSection = nullptr);

	void AddMeasure(Measure* pMeasure);
	Measure* GetMeasure(const std::wstring& name);

	bool GetVariable(const std::wstring& strVariable, std::wstring& strValue, bool isNewStyle = false);
	const std::wstring* GetVariableOriginalName(const std::wstring& strVariable);
	void SetVariable(std::wstring strVariable, const std::wstring& strValue);
	const ankerl::unordered_dense::map<std::wstring, std::wstring>& GetVariables() { return m_Variables; }
	MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }
	void SetMonitorVariableMode(MonitorVariableMode mode) { m_MonitorVariableMode = mode; }

	const std::wstring& GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault);
	void SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue);
	void DeleteValue(const std::wstring& strSection, const std::wstring& strKey);

	bool ReadInheritOption(LPCTSTR section, bool allowMeterStyle = false);
	void SetInheritChain(const std::wstring& strInherit) { static const std::wstring delim(1, L'|'); Tokenize(strInherit, delim).swap(m_InheritChain); }
	void ClearInheritChain() { m_InheritChain.clear(); }

	bool GetLastReplaced() { return m_LastReplaced; }
	bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
	bool GetLastKeyDefined() { return !m_LastDefaultUsed; }
	bool GetLastValueDefined() { return m_LastValueDefined; }

	const std::wstring& ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures = true);
	bool IsKeyDefined(LPCTSTR section, LPCTSTR key);
	bool IsValueDefined(LPCTSTR section, LPCTSTR key);
	bool ReadBool(LPCTSTR section, LPCTSTR key, bool defValue) { return ReadInt(section, key, (int)defValue) != 0; }
	int ReadInt(LPCTSTR section, LPCTSTR key, int defValue);
	uint32_t ReadUInt(LPCTSTR section, LPCTSTR key, uint32_t defValue);
	uint64_t ReadUInt64(LPCTSTR section, LPCTSTR key, uint64_t defValue);
	double ReadFloat(LPCTSTR section, LPCTSTR key, double defValue);
	D2D1_COLOR_F ReadColor(LPCTSTR section, LPCTSTR key, const D2D1_COLOR_F& defValue);
	D2D1_RECT_F ReadRect(LPCTSTR section, LPCTSTR key, const D2D1_RECT_F& defValue);
	RECT ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue);
	std::vector<FLOAT> ReadFloats(LPCTSTR section, LPCTSTR key);

	bool ParseFormula(const std::wstring& formula, double* resultValue);
	std::wstring ParseFormulaWithModifiers(const std::wstring& formula);

	const std::list<std::wstring>& GetSections() { return m_Sections; }

	bool ReplaceVariables(std::wstring& result, bool isNewStyle = false);
	bool ReplaceMeasures(std::wstring& result);

	bool ParseVariables(std::wstring& result, const VariableType type, Meter* meter = nullptr);
	bool ContainsNewStyleVariable(const std::wstring& str);
	std::wstring GetMouseVariable(const std::wstring& variable, Meter* meter);

	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static std::vector<std::wstring> TokenizeWithPairedPunctuation(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct);

	static double ParseDouble(LPCTSTR str, double defValue);
	static int ParseInt(LPCTSTR str, int defValue);
	static uint32_t ParseUInt(LPCTSTR str, uint32_t defValue);
	static uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue);
	static D2D1_COLOR_F ParseColor(LPCTSTR str);
	static D2D1_RECT_F ParseRect(LPCTSTR str);
	static RECT ParseRECT(LPCTSTR str);

	static bool IsVariableKey(const WCHAR ch) { for (auto& k : c_VariableMap) { if (k.second == ch) return true; } return false; }

private:
	void ReadVariables();

	void ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection = nullptr, int depth = 0);

	bool GetSectionVariable(std::wstring& strVariable, std::wstring& strValue, void* logEntry = nullptr);

	std::optional<std::wstring> GetBuiltInVariable(const std::wstring& variableStr);
	std::optional<std::wstring> GetCurrentConfigVariable(const std::wstring& variableStr);
	std::optional<std::wstring> GetSectionSkinVariable(const std::wstring& variableStr);
	std::optional<std::wstring> GetSectionDisplayVariable(const std::wstring& variableStr);
	std::optional<std::wstring> GetMonitorVariable(const std::wstring& variableStr);

	static std::wstring StrToUpper(const std::wstring& str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring StrToUpper(const WCHAR* str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	ankerl::unordered_dense::map<std::wstring, Measure*> m_Measures;

	std::vector<std::wstring> m_InheritChain;

	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;
	MonitorVariableMode m_MonitorVariableMode;

	std::wstring m_CurrentSection;
	std::wstring m_CurrentPath;

	std::list<std::wstring> m_Sections;		// Ordered section
	ankerl::unordered_dense::map<std::wstring, std::wstring> m_Values;

	ankerl::unordered_dense::set<std::wstring> m_FoundSections;
	std::list<std::wstring> m_ListVariables;
	std::list<std::wstring>::const_iterator m_SectionInsertPos;

	ankerl::unordered_dense::map<std::wstring, std::wstring> m_Variables;
	ankerl::unordered_dense::map<std::wstring, std::wstring> m_OriginalVariableNames;

	Skin* m_Skin;

	static ankerl::unordered_dense::map<VariableType, WCHAR> c_VariableMap;
};

#endif
