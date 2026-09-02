// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#pragma warning(disable: 4503)

#include "../Common/Map.h"
#include "../Common/ParseUtil.h"
#include <windows.h>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <d2d1.h>

class MathParser;
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

	// Applies the inherit chain of |section| (@Inherit, or MeterStyle if |allowMeterStyle|) for the
	// lifetime of the scope. Options missing from a section being read within the scope are looked
	// up from the inherited sections instead. The previous chain, if any, is restored afterwards.
	class InheritChainScope
	{
	public:
		InheritChainScope(ConfigParser& parser, LPCTSTR section, bool allowMeterStyle = false);
		~InheritChainScope();

		InheritChainScope(const InheritChainScope& other) = delete;
		InheritChainScope& operator=(InheritChainScope other) = delete;

	private:
		ConfigParser& m_Parser;
		std::vector<std::wstring> m_PreviousChain;
	};

	ConfigParser();
	~ConfigParser();

	ConfigParser(const ConfigParser& other) = delete;
	ConfigParser& operator=(ConfigParser other) = delete;

	void Initialize(const std::wstring& filename, Skin* skin = nullptr, LPCTSTR skinSection = nullptr);

	void AddSection(Section* section);
	void ClearSections() { m_Sections.clear(); }

	Section* GetSection(std::wstring_view name);
	Measure* GetMeasure(std::wstring_view name);
	Meter* GetMeter(std::wstring_view name);

	bool GetVariable(std::wstring_view strVariable, std::wstring& strValue, bool isNewStyle = false);
	const std::wstring* GetVariableOriginalName(const std::wstring& strVariable);
	void SetVariable(std::wstring_view strVariable, std::wstring_view strValue);
	const StringMap<std::wstring>& GetVariables() { return m_Variables; }
	MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }
	void SetMonitorVariableMode(MonitorVariableMode mode) { m_MonitorVariableMode = mode; }

	const std::wstring* GetValue(std::wstring_view section, std::wstring_view option);
	void SetValue(std::wstring_view section, std::wstring_view option, std::wstring value);
	void DeleteValue(std::wstring_view section, std::wstring_view option);

	bool GetLastReplaced() { return m_LastReplaced; }
	bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
	bool GetLastKeyDefined() { return !m_LastDefaultUsed; }
	bool GetLastValueDefined() { return m_LastValueDefined; }

	bool IsKeyDefined(std::wstring_view section, std::wstring_view key);
	bool IsValueDefined(std::wstring_view section, std::wstring_view key);

	struct ReadOptions
	{
		bool sectionVariables = true;
	};

	// Reads into |result|. Prefer this where the value is kept.
	void ReadString(std::wstring& result, std::wstring_view section, std::wstring_view key, std::wstring_view defValue, ReadOptions options = {});

	// The returned reference is good only until the next ReadString() at the same nesting depth,
	// which reuses the buffer. Copy it to keep it; no argument may point into it.
	const std::wstring& ReadString(std::wstring_view section, std::wstring_view key, std::wstring_view defValue, ReadOptions options = {});

	template<typename T>
	struct EnumOption
	{
		const WCHAR* name;
		T value;
	};

	template<typename T, size_t N>
	T ReadEnum(std::wstring_view section, std::wstring_view key, T defValue, const EnumOption<T> (&options)[N])
	{
		const size_t index = MatchEnumOption(section, key, &options[0].name, N, sizeof(EnumOption<T>));
		return index < N ? options[index].value : defValue;
	}

	bool ReadBool(std::wstring_view section, std::wstring_view key, bool defValue) { return ReadInt(section, key, (int)defValue) != 0; }
	int ReadInt(std::wstring_view section, std::wstring_view key, int defValue);
	uint32_t ReadUInt(std::wstring_view section, std::wstring_view key, uint32_t defValue);
	uint64_t ReadUInt64(std::wstring_view section, std::wstring_view key, uint64_t defValue);
	double ReadFloat(std::wstring_view section, std::wstring_view key, double defValue);
	D2D1_COLOR_F ReadColor(std::wstring_view section, std::wstring_view key, const D2D1_COLOR_F& defValue);
	D2D1_RECT_F ReadRect(std::wstring_view section, std::wstring_view key, const D2D1_RECT_F& defValue);
	RECT ReadRECT(std::wstring_view section, std::wstring_view key, const RECT& defValue);
	std::vector<FLOAT> ReadFloats(std::wstring_view section, std::wstring_view key);

	bool ParseFormula(std::wstring_view formula, double* resultValue);
	std::wstring ParseFormulaWithModifiers(const std::wstring& formula);

	const std::vector<std::wstring>& GetIniFiles() const { return m_IniFiles; }
	const std::list<std::wstring>& GetSectionNames() { return m_SectionNames; }

	bool ReplaceVariables(std::wstring& result, bool isNewStyle = false);
	bool ReplaceMeasures(std::wstring& result);
	std::optional<std::wstring> GetDollarVariable(std::wstring_view variableStr);

	bool ExpandSectionVariables(std::wstring& result, const VariableExpandMode expandMode, Meter* meter = nullptr, int depth = 0, size_t start = 0);
	bool ContainsKeyedSectionVariable(const std::wstring& str);
	static bool IsSectionVariableKey(WCHAR key);
	std::wstring GetDollarMouseVariable(std::wstring_view variable, Meter* meter);

	// Resolves [$Input] against an editable String meter. Returns a value rather than a bool so
	// that an empty field expands to nothing instead of being left as a literal.
	std::optional<std::wstring> GetDollarInputVariable(std::wstring_view variable, Section* section);

	// Returns the skin's math parser, or a skinless one if the parser is not tied to a skin.
	const MathParser& GetMathParser() const;

	double ParseDouble(LPCTSTR str, double defValue);
	int ParseInt(LPCTSTR str, int defValue);
	double ParseDouble(std::wstring_view str, double defValue);
	int ParseInt(std::wstring_view str, int defValue);
	uint32_t ParseUInt(LPCTSTR str, uint32_t defValue);
	uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue);
	D2D1_COLOR_F ParseColor(LPCTSTR str);
	D2D1_COLOR_F ParseColor(std::wstring_view str);
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

	size_t MatchEnumOption(std::wstring_view section, std::wstring_view key, const WCHAR* const* names, size_t count, size_t stride);

	static std::wstring StrToUpper(std::wstring_view str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	StringMap<Section*> m_Sections;

	std::vector<std::wstring> m_InheritChain;

	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;
	MonitorVariableMode m_MonitorVariableMode;

	std::wstring_view m_CurrentSection;
	std::wstring m_CurrentPath;

	std::vector<std::wstring> m_IniFiles;
	std::list<std::wstring> m_SectionNames;	// Ordered
	StringMap<std::wstring> m_Values;

	StringSet m_FoundSections;
	std::list<std::wstring> m_ListVariables;
	std::list<std::wstring>::const_iterator m_SectionNamesInsertPos;

	StringMap<std::wstring> m_Variables;
	StringMap<std::wstring> m_OriginalVariableNames;

	Skin* m_Skin;
};
