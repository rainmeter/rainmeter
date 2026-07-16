/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__

#pragma warning(disable: 4503)

#include "../Common/Map.h"
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

enum class VariableExpandMode : BYTE
{
	AllKeys,
	HashOnly,
	DollarMouseOnly
};

class ConfigParser
{
public:
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
	Measure* GetMeasure(std::wstring_view name);

	bool GetVariable(std::wstring_view strVariable, std::wstring& strValue, bool isNewStyle = false);
	const std::wstring* GetVariableOriginalName(const std::wstring& strVariable);
	void SetVariable(const std::wstring& strVariable, const std::wstring& strValue);
	const StringMap<std::wstring>& GetVariables() { return m_Variables; }
	MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }
	void SetMonitorVariableMode(MonitorVariableMode mode) { m_MonitorVariableMode = mode; }

	const std::wstring& GetValue(const std::wstring& section, const std::wstring& option, const std::wstring& defaultValue);
	void SetValue(const std::wstring& section, const std::wstring& option, std::wstring value);
	void DeleteValue(const std::wstring& section, const std::wstring& option);

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

	bool ExpandSectionVariables(std::wstring& result, const VariableExpandMode expandMode, Meter* meter = nullptr, int depth = 0, size_t start = 0);
	bool ContainsKeyedSectionVariable(const std::wstring& str);
	static bool IsSectionVariableKey(WCHAR key);
	std::wstring GetDollarMouseVariable(std::wstring_view variable, Meter* meter);

	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static std::vector<std::wstring> TokenizeWithPairedPunctuation(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct);

	double ParseDouble(LPCTSTR str, double defValue);
	int ParseInt(LPCTSTR str, int defValue);
	uint32_t ParseUInt(LPCTSTR str, uint32_t defValue);
	uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue);
	D2D1_COLOR_F ParseColor(LPCTSTR str);
	D2D1_RECT_F ParseRect(LPCTSTR str);
	RECT ParseRECT(LPCTSTR str);

private:
	void ReadVariables();

	void ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection = nullptr, int depth = 0);

	bool GetSectionVariable(std::wstring& strVariable, std::wstring& strValue, void* logEntry = nullptr);

	std::optional<std::wstring> GetBuiltInVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetCurrentConfigVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetDollarSkinVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetDollarDisplayVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetMonitorVariable(std::wstring_view variableStr);

	static std::wstring StrToUpper(std::wstring_view str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	StringMap<Measure*> m_Measures;

	std::vector<std::wstring> m_InheritChain;

	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;
	MonitorVariableMode m_MonitorVariableMode;

	const std::wstring* m_CurrentSection;
	std::wstring m_CurrentPath;

	std::list<std::wstring> m_Sections;		// Ordered section
	StringMap<std::wstring> m_Values;

	ankerl::unordered_dense::set<std::wstring> m_FoundSections;
	std::list<std::wstring> m_ListVariables;
	std::list<std::wstring>::const_iterator m_SectionInsertPos;

	StringMap<std::wstring> m_Variables;
	StringMap<std::wstring> m_OriginalVariableNames;

	Skin* m_Skin;
};

#endif
