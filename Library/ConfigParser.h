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
	struct ReadOptions
	{
		bool sectionVariables = true;
	};

	template<typename T>
	struct EnumOption
	{
		const WCHAR* name;
		T value;
	};

	enum class MonitorVariableMode : BYTE
	{
		DEFAULT_LOGICAL,
		FORCE_PHYSICAL
	};

	enum class ReadOptionInheritMode
	{
		InheritOnly,
		InheritAndMeterStyle,
		None
	};

	// Keeps the section's original name and interned ID available while its options are read. The
	// name is used for section variables and diagnostics, while numeric lookups use the ID. Use
	// GetInheritableOptionReader() when missing options should also come from @Inherit or
	// MeterStyle.
	class OptionReader
	{
	public:
		OptionReader(const OptionReader& other) = delete;
		OptionReader(OptionReader&& other) = default;
		OptionReader& operator=(OptionReader other) = delete;

		bool GetLastReplaced() { return m_LastReplaced; }
		bool GetLastDefaultUsed() { return m_LastDefaultUsed; }
		bool GetLastValueDefined() { return m_LastValueDefined; }

		std::wstring_view FindSectionName() { return m_Parser.FindSectionName(m_SectionID); }

		void SetMonitorVariableMode(MonitorVariableMode mode) { m_MonitorVariableMode = mode; }

		// Reads into |result|. Prefer this where the value is kept.
		void ReadString(std::wstring& result, IniOptionID option, std::wstring_view defValue, ReadOptions options = {}) { m_Parser.ReadString(result, *this, option, defValue, options); }
		void ReadString(std::wstring& result, std::wstring_view option, std::wstring_view defValue, ReadOptions options = {}) { ReadString(result, m_Parser.FindOptionID(option), defValue, options); }
		template<FixedWString Name> void ReadString(std::wstring& result, std::wstring_view defValue = {}, ReadOptions options = {}) { ReadString(result, IniNameRegistry::InternOption<Name>(), defValue, options); }

		// The returned reference is good only until the next ReadString() at the same nesting depth,
		// which reuses the buffer. Copy it to keep it; no argument may point into it.
		const std::wstring& ReadString(IniOptionID option, std::wstring_view defValue = {}, ReadOptions options = {}) { return m_Parser.ReadString(*this, option, defValue, options); }
		const std::wstring& ReadString(std::wstring_view option, std::wstring_view defValue = {}, ReadOptions options = {}) { return ReadString(m_Parser.FindOptionID(option), defValue, options); }
		template<FixedWString Name> const std::wstring& ReadString(std::wstring_view defValue = {}, ReadOptions options = {}) { return ReadString(IniNameRegistry::InternOption<Name>(), defValue, options); }

		template<typename T, size_t N> T ReadEnum(IniOptionID option, T defValue, const EnumOption<T> (&options)[N]) { return m_Parser.ReadEnum(*this, option, defValue, options); }
		template<typename T, size_t N> T ReadEnum(std::wstring_view option, T defValue, const EnumOption<T> (&options)[N]) { return ReadEnum(m_Parser.FindOptionID(option), defValue, options); }
		template<FixedWString Name, typename T, size_t N> T ReadEnum(T defValue, const EnumOption<T> (&options)[N]) { return ReadEnum(IniNameRegistry::InternOption<Name>(), defValue, options); }

		bool ReadBool(IniOptionID option, bool defValue) { return m_Parser.ReadBool(*this, option, defValue); }
		bool ReadBool(std::wstring_view option, bool defValue) { return ReadBool(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> bool ReadBool(bool defValue) { return ReadBool(IniNameRegistry::InternOption<Name>(), defValue); }

		int ReadInt(IniOptionID option, int defValue) { return m_Parser.ReadInt(*this, option, defValue); }
		int ReadInt(std::wstring_view option, int defValue) { return ReadInt(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> int ReadInt(int defValue) { return ReadInt(IniNameRegistry::InternOption<Name>(), defValue); }

		uint32_t ReadUInt(IniOptionID option, uint32_t defValue) { return m_Parser.ReadUInt(*this, option, defValue); }
		uint32_t ReadUInt(std::wstring_view option, uint32_t defValue) { return ReadUInt(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> uint32_t ReadUInt(uint32_t defValue) { return ReadUInt(IniNameRegistry::InternOption<Name>(), defValue); }

		uint64_t ReadUInt64(IniOptionID option, uint64_t defValue) { return m_Parser.ReadUInt64(*this, option, defValue); }
		uint64_t ReadUInt64(std::wstring_view option, uint64_t defValue) { return ReadUInt64(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> uint64_t ReadUInt64(uint64_t defValue) { return ReadUInt64(IniNameRegistry::InternOption<Name>(), defValue); }

		double ReadFloat(IniOptionID option, double defValue) { return m_Parser.ReadFloat(*this, option, defValue); }
		double ReadFloat(std::wstring_view option, double defValue) { return ReadFloat(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> double ReadFloat(double defValue) { return ReadFloat(IniNameRegistry::InternOption<Name>(), defValue); }

		D2D1_COLOR_F ReadColor(IniOptionID option, const D2D1_COLOR_F& defValue) { return m_Parser.ReadColor(*this, option, defValue); }
		D2D1_COLOR_F ReadColor(std::wstring_view option, const D2D1_COLOR_F& defValue) { return ReadColor(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> D2D1_COLOR_F ReadColor(const D2D1_COLOR_F& defValue) { return ReadColor(IniNameRegistry::InternOption<Name>(), defValue); }

		D2D1_RECT_F ReadRect(IniOptionID option, const D2D1_RECT_F& defValue) { return m_Parser.ReadRect(*this, option, defValue); }
		D2D1_RECT_F ReadRect(std::wstring_view option, const D2D1_RECT_F& defValue) { return ReadRect(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> D2D1_RECT_F ReadRect(const D2D1_RECT_F& defValue) { return ReadRect(IniNameRegistry::InternOption<Name>(), defValue); }

		RECT ReadRECT(IniOptionID option, const RECT& defValue) { return m_Parser.ReadRECT(*this, option, defValue); }
		RECT ReadRECT(std::wstring_view option, const RECT& defValue) { return ReadRECT(m_Parser.FindOptionID(option), defValue); }
		template<FixedWString Name> RECT ReadRECT(const RECT& defValue) { return ReadRECT(IniNameRegistry::InternOption<Name>(), defValue); }

	private:
		friend class ConfigParser;

		OptionReader(ConfigParser& parser, std::wstring_view sectionName, IniSectionID sectionID, ReadOptionInheritMode inheritMode);

		void ClearLastReadFlags() { m_LastReplaced = false; m_LastDefaultUsed = false; m_LastValueDefined = false; }
		void MarkReplaced() { m_LastReplaced = true; }
		void MarkDefaultUsed() { m_LastDefaultUsed = true; }
		void MarkValueDefined() { m_LastValueDefined = true; }

		const std::vector<IniSectionID>& GetInheritChain() const { return m_InheritChain; }
		std::wstring_view GetSectionName() const { return m_SectionName; }
		IniSectionID GetSectionID() const { return m_SectionID; }
		MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }

		ConfigParser& m_Parser;
		std::vector<IniSectionID> m_InheritChain;
		bool m_LastReplaced = false;
		bool m_LastDefaultUsed = false;
		bool m_LastValueDefined = false;
		MonitorVariableMode m_MonitorVariableMode = MonitorVariableMode::DEFAULT_LOGICAL;
		std::wstring_view m_SectionName;
		IniSectionID m_SectionID;
	};

	ConfigParser();
	~ConfigParser();

	ConfigParser(const ConfigParser& other) = delete;
	ConfigParser& operator=(ConfigParser other) = delete;

	OptionReader GetOptionReader(std::wstring_view sectionName, IniSectionID sectionID);
	OptionReader GetInheritableOptionReader(std::wstring_view sectionName, IniSectionID sectionID, bool allowMeterStyle = false);

	void Initialize(const std::wstring& filename, Skin* skin = nullptr, LPCTSTR skinSection = nullptr);

	void AddSection(Section* section);
	void ClearSections() { m_Sections.clear(); }

	Section* GetSection(std::wstring_view name);
	Section* GetSection(IniSectionID id);
	std::wstring_view FindSectionName(IniSectionID id) const;
	Measure* GetMeasure(std::wstring_view name);
	Meter* GetMeter(std::wstring_view name);

	bool GetVariable(std::wstring_view strVariable, std::wstring& strValue, bool isNewStyle = false);
	const std::wstring* GetVariableOriginalName(const std::wstring& strVariable);
	void SetVariable(std::wstring_view strVariable, std::wstring_view strValue);
	const StringMap<std::wstring>& GetVariables() { return m_Variables; }
	const std::wstring* GetValue(std::wstring_view section, std::wstring_view option) const;
	const std::wstring* GetValue(IniSectionID section, IniOptionID option) const;
	void SetValue(IniSectionID section, IniOptionID option, std::wstring value);
	void SetValue(std::wstring_view section, std::wstring_view option, std::wstring value);
	void DeleteValue(IniSectionID section, IniOptionID option);
	void DeleteValue(std::wstring_view section, std::wstring_view option);

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
	void ReadString(std::wstring& result, OptionReader& reader, IniOptionID option, std::wstring_view defValue, ReadOptions options = {});
	const std::wstring& ReadString(OptionReader& reader, IniOptionID option, std::wstring_view defValue = {}, ReadOptions options = {});

	template<typename T, size_t N>
	T ReadEnum(OptionReader& reader, IniOptionID option, T defValue, const EnumOption<T> (&options)[N])
	{
		const size_t index = MatchEnumOption(reader, option, &options[0].name, N, sizeof(EnumOption<T>));
		return index < N ? options[index].value : defValue;
	}

	bool ReadBool(OptionReader& reader, IniOptionID option, bool defValue) { return ReadInt(reader, option, (int)defValue) != 0; }
	int ReadInt(OptionReader& reader, IniOptionID option, int defValue);
	uint32_t ReadUInt(OptionReader& reader, IniOptionID option, uint32_t defValue);
	uint64_t ReadUInt64(OptionReader& reader, IniOptionID option, uint64_t defValue);
	double ReadFloat(OptionReader& reader, IniOptionID option, double defValue);
	D2D1_COLOR_F ReadColor(OptionReader& reader, IniOptionID option, const D2D1_COLOR_F& defValue);
	D2D1_RECT_F ReadRect(OptionReader& reader, IniOptionID option, const D2D1_RECT_F& defValue);
	RECT ReadRECT(OptionReader& reader, IniOptionID option, const RECT& defValue);

	void ReadVariables();

	void ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection = nullptr, int depth = 0);

	bool GetSectionVariable(std::wstring& strVariable, std::wstring& strValue, void* logEntry = nullptr);

	std::optional<std::wstring> GetBuiltInVariable(std::wstring_view variableStr, std::wstring_view currentSection);
	std::optional<std::wstring> GetCurrentConfigVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetDollarSkinVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetDollarDisplayVariable(std::wstring_view variableStr);
	std::optional<std::wstring> GetMonitorVariable(std::wstring_view variableStr, MonitorVariableMode monitorVariableMode);

	bool GetVariable(std::wstring_view strVariable, std::wstring& strValue, std::wstring_view currentSection, MonitorVariableMode monitorVariableMode, bool isNewStyle = false);
	bool ReplaceVariables(std::wstring& result, std::wstring_view currentSection, MonitorVariableMode monitorVariableMode, bool isNewStyle = false);
	bool ReplaceMeasures(std::wstring& result, std::wstring_view currentSection, MonitorVariableMode monitorVariableMode);
	bool ExpandSectionVariables(std::wstring& result, std::wstring_view currentSection, MonitorVariableMode monitorVariableMode, const VariableExpandMode expandMode, Meter* meter = nullptr, int depth = 0, size_t start = 0);

	void ReadStringInternal(std::wstring& result, OptionReader& reader, IniOptionID option, std::wstring_view defValue, ReadOptions options);
	const std::wstring& ReadStringInternal(OptionReader& reader, IniOptionID option, std::wstring_view defValue, ReadOptions options);
	IniSectionID FindSectionID(std::wstring_view section) const;
	IniOptionID FindOptionID(std::wstring_view option) const;
	const std::wstring& GetOptionName(IniOptionID option) const;
	size_t MatchEnumOption(OptionReader& reader, IniOptionID option, const WCHAR* const* names, size_t count, size_t stride);

	static std::wstring StrToUpper(std::wstring_view str) { std::wstring strTmp(str); StrToUpperC(strTmp); return strTmp; }
	static std::wstring& StrToUpperC(std::wstring& str) { _wcsupr(&str[0]); return str; }

	StringMap<Section*> m_Sections;

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
