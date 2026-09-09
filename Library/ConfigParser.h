// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#pragma warning(disable: 4503)

#include "../Common/Map.h"
#include "../Common/ParseUtil.h"
#include "IniNameRegistry.h"
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

	// Keeps the section's original name and interned ID available while its options are read. The
	// name is used for section variables and diagnostics, while numeric lookups use the ID.
	// Missing options are also read from @Inherit, or MeterStyle when allowed. Inherited section
	// names are resolved once when the reader is created.
	class OptionReader
	{
	public:
		OptionReader(ConfigParser& parser, std::wstring_view sectionName, IniSectionID sectionID, bool allowMeterStyle = false);
		~OptionReader();

		OptionReader(const OptionReader& other) = delete;
		OptionReader& operator=(OptionReader other) = delete;

		ConfigParser& GetParser() { return m_Parser; }

	private:
		ConfigParser& m_Parser;
		std::vector<IniSectionID> m_PreviousChain;
		std::wstring_view m_PreviousSection;
		IniSectionID m_PreviousSectionID;
	};

	ConfigParser();
	~ConfigParser();

	ConfigParser(const ConfigParser& other) = delete;
	ConfigParser& operator=(ConfigParser other) = delete;

	void Initialize(const std::wstring& filename, Skin* skin = nullptr, LPCTSTR skinSection = nullptr);

	void AddSection(Section* section);
	void ClearSections() { m_Sections.clear(); }

	Section* GetSection(std::wstring_view name);
	Section* GetSection(IniSectionID id);
	std::wstring_view GetSectionName(IniSectionID id) const;
	Measure* GetMeasure(std::wstring_view name);
	Meter* GetMeter(std::wstring_view name);

	bool GetVariable(std::wstring_view strVariable, std::wstring& strValue, bool isNewStyle = false);
	const std::wstring* GetVariableOriginalName(const std::wstring& strVariable);
	void SetVariable(std::wstring_view strVariable, std::wstring_view strValue);
	const StringMap<std::wstring>& GetVariables() { return m_Variables; }
	MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }
	void SetMonitorVariableMode(MonitorVariableMode mode) { m_MonitorVariableMode = mode; }

	const std::wstring* GetValue(std::wstring_view section, std::wstring_view option) const;
	const std::wstring* GetValue(IniSectionID section, IniOptionID option) const;
	void SetValue(IniSectionID section, IniOptionID option, std::wstring value);
	void SetValue(std::wstring_view section, std::wstring_view option, std::wstring value);
	void DeleteValue(IniSectionID section, IniOptionID option);
	void DeleteValue(std::wstring_view section, std::wstring_view option);

	bool GetLastReplaced() { return m_LastReplaced; }
	bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
	bool GetLastKeyDefined() { return !m_LastDefaultUsed; }
	bool GetLastValueDefined() { return m_LastValueDefined; }

	bool IsKeyDefined(std::wstring_view section, std::wstring_view key);
	bool IsKeyDefined(IniSectionID section, IniOptionID option);
	bool IsKeyDefined(IniSectionID section, std::wstring_view option) { return IsKeyDefined(section, FindOptionID(option)); }
	template <FixedWString Name> bool IsKeyDefined(IniSectionID section) { return IsKeyDefined(section, GetStaticIniOptionID<Name>()); }
	bool IsValueDefined(std::wstring_view section, std::wstring_view key);
	bool IsValueDefined(IniSectionID section, IniOptionID option);
	bool IsValueDefined(IniSectionID section, std::wstring_view option) { return IsValueDefined(section, FindOptionID(option)); }
	template <FixedWString Name> bool IsValueDefined(IniSectionID section) { return IsValueDefined(section, GetStaticIniOptionID<Name>()); }

	struct ReadOptions
	{
		bool sectionVariables = true;
	};

	// Reads into |result|. Prefer this where the value is kept.
	void ReadString(std::wstring& result, std::wstring_view section, std::wstring_view key, std::wstring_view defValue, ReadOptions options = {});
	void ReadString(std::wstring& result, IniSectionID section, IniOptionID option, std::wstring_view defValue, ReadOptions options = {});
	void ReadString(std::wstring& result, IniSectionID section, std::wstring_view option, std::wstring_view defValue, ReadOptions options = {});

	template <FixedWString Name>
	void ReadString(std::wstring& result, IniSectionID section, std::wstring_view defValue = {}, ReadOptions options = {})
	{
		ReadString(result, section, GetStaticIniOptionID<Name>(), defValue, options);
	}

	// The returned reference is good only until the next ReadString() at the same nesting depth,
	// which reuses the buffer. Copy it to keep it; no argument may point into it.
	const std::wstring& ReadString(std::wstring_view section, std::wstring_view key, std::wstring_view defValue, ReadOptions options = {});
	const std::wstring& ReadString(IniSectionID section, IniOptionID option, std::wstring_view defValue = {}, ReadOptions options = {});
	const std::wstring& ReadString(IniSectionID section, std::wstring_view option, std::wstring_view defValue = {}, ReadOptions options = {});

	template <FixedWString Name>
	const std::wstring& ReadString(IniSectionID section, std::wstring_view defValue = {}, ReadOptions options = {})
	{
		return ReadString(section, GetStaticIniOptionID<Name>(), defValue, options);
	}

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

	template<typename T, size_t N>
	T ReadEnum(IniSectionID section, IniOptionID option, T defValue, const EnumOption<T> (&options)[N])
	{
		const size_t index = MatchEnumOption(section, option, &options[0].name, N, sizeof(EnumOption<T>));
		return index < N ? options[index].value : defValue;
	}

	template<typename T, size_t N>
	T ReadEnum(IniSectionID section, std::wstring_view option, T defValue, const EnumOption<T> (&options)[N])
	{
		return ReadEnum(section, FindOptionID(option), defValue, options);
	}

	template <FixedWString Name, typename T, size_t N>
	T ReadEnum(IniSectionID section, T defValue, const EnumOption<T> (&options)[N])
	{
		return ReadEnum(section, GetStaticIniOptionID<Name>(), defValue, options);
	}

	bool ReadBool(std::wstring_view section, std::wstring_view key, bool defValue) { return ReadInt(section, key, (int)defValue) != 0; }
	bool ReadBool(IniSectionID section, IniOptionID option, bool defValue) { return ReadInt(section, option, (int)defValue) != 0; }
	bool ReadBool(IniSectionID section, std::wstring_view option, bool defValue) { return ReadInt(section, option, (int)defValue) != 0; }
	template <FixedWString Name> bool ReadBool(IniSectionID section, bool defValue) { return ReadBool(section, GetStaticIniOptionID<Name>(), defValue); }
	int ReadInt(std::wstring_view section, std::wstring_view key, int defValue);
	int ReadInt(IniSectionID section, IniOptionID option, int defValue);
	int ReadInt(IniSectionID section, std::wstring_view option, int defValue) { return ReadInt(section, FindOptionID(option), defValue); }
	template <FixedWString Name> int ReadInt(IniSectionID section, int defValue) { return ReadInt(section, GetStaticIniOptionID<Name>(), defValue); }
	uint32_t ReadUInt(std::wstring_view section, std::wstring_view key, uint32_t defValue);
	uint32_t ReadUInt(IniSectionID section, IniOptionID option, uint32_t defValue);
	uint32_t ReadUInt(IniSectionID section, std::wstring_view option, uint32_t defValue) { return ReadUInt(section, FindOptionID(option), defValue); }
	template <FixedWString Name> uint32_t ReadUInt(IniSectionID section, uint32_t defValue) { return ReadUInt(section, GetStaticIniOptionID<Name>(), defValue); }
	uint64_t ReadUInt64(std::wstring_view section, std::wstring_view key, uint64_t defValue);
	uint64_t ReadUInt64(IniSectionID section, IniOptionID option, uint64_t defValue);
	uint64_t ReadUInt64(IniSectionID section, std::wstring_view option, uint64_t defValue) { return ReadUInt64(section, FindOptionID(option), defValue); }
	template <FixedWString Name> uint64_t ReadUInt64(IniSectionID section, uint64_t defValue) { return ReadUInt64(section, GetStaticIniOptionID<Name>(), defValue); }
	double ReadFloat(std::wstring_view section, std::wstring_view key, double defValue);
	double ReadFloat(IniSectionID section, IniOptionID option, double defValue);
	double ReadFloat(IniSectionID section, std::wstring_view option, double defValue) { return ReadFloat(section, FindOptionID(option), defValue); }
	template <FixedWString Name> double ReadFloat(IniSectionID section, double defValue) { return ReadFloat(section, GetStaticIniOptionID<Name>(), defValue); }
	D2D1_COLOR_F ReadColor(std::wstring_view section, std::wstring_view key, const D2D1_COLOR_F& defValue);
	D2D1_COLOR_F ReadColor(IniSectionID section, IniOptionID option, const D2D1_COLOR_F& defValue);
	D2D1_COLOR_F ReadColor(IniSectionID section, std::wstring_view option, const D2D1_COLOR_F& defValue) { return ReadColor(section, FindOptionID(option), defValue); }
	template <FixedWString Name> D2D1_COLOR_F ReadColor(IniSectionID section, const D2D1_COLOR_F& defValue) { return ReadColor(section, GetStaticIniOptionID<Name>(), defValue); }
	D2D1_RECT_F ReadRect(std::wstring_view section, std::wstring_view key, const D2D1_RECT_F& defValue);
	D2D1_RECT_F ReadRect(IniSectionID section, IniOptionID option, const D2D1_RECT_F& defValue);
	D2D1_RECT_F ReadRect(IniSectionID section, std::wstring_view option, const D2D1_RECT_F& defValue) { return ReadRect(section, FindOptionID(option), defValue); }
	template <FixedWString Name> D2D1_RECT_F ReadRect(IniSectionID section, const D2D1_RECT_F& defValue) { return ReadRect(section, GetStaticIniOptionID<Name>(), defValue); }
	RECT ReadRECT(std::wstring_view section, std::wstring_view key, const RECT& defValue);
	RECT ReadRECT(IniSectionID section, IniOptionID option, const RECT& defValue);
	RECT ReadRECT(IniSectionID section, std::wstring_view option, const RECT& defValue) { return ReadRECT(section, FindOptionID(option), defValue); }
	template <FixedWString Name> RECT ReadRECT(IniSectionID section, const RECT& defValue) { return ReadRECT(section, GetStaticIniOptionID<Name>(), defValue); }

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

	void ReadStringInternal(std::wstring& result, IniSectionID section, IniOptionID option, std::wstring_view sectionName, std::wstring_view defValue, ReadOptions options);
	const std::wstring& ReadStringInternal(IniSectionID section, IniOptionID option, std::wstring_view sectionName, std::wstring_view defValue, ReadOptions options);
	IniSectionID FindSectionID(std::wstring_view section) const;
	IniOptionID FindOptionID(std::wstring_view option) const;
	const std::wstring& GetOptionName(IniOptionID option) const;
	size_t MatchEnumOption(std::wstring_view section, std::wstring_view key, const WCHAR* const* names, size_t count, size_t stride);
	size_t MatchEnumOption(IniSectionID section, IniOptionID option, const WCHAR* const* names, size_t count, size_t stride);

	static std::wstring StrToUpper(std::wstring_view str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	StringMap<Section*> m_Sections;

	std::vector<IniSectionID> m_InheritChain;

	bool m_LastReplaced;
	bool m_LastDefaultUsed;
	bool m_LastValueDefined;
	MonitorVariableMode m_MonitorVariableMode;

	std::wstring_view m_CurrentSection;
	IniSectionID m_CurrentSectionID;
	std::wstring m_CurrentPath;

	std::vector<std::wstring> m_IniFiles;
	std::list<std::wstring> m_SectionNames;	// Ordered
	ankerl::unordered_dense::map<IniValueID, std::wstring> m_Values;

	ankerl::unordered_dense::set<IniSectionID> m_FoundSections;
	std::list<std::wstring> m_ListVariables;
	std::list<std::wstring>::const_iterator m_SectionNamesInsertPos;

	StringMap<std::wstring> m_Variables;
	StringMap<std::wstring> m_OriginalVariableNames;

	Skin* m_Skin;
};
