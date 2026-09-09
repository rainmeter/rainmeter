// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <memory>
#include "IfActions.h"
#include "LocaleUtil.h"
#include "Util.h"
#include "Section.h"

enum AUTOSCALE
{
	AUTOSCALE_1024  = 1,    // scales by 1024
	AUTOSCALE_1000  = 2,    // scales by 1000

	AUTOSCALE_1024K = 101,  // scales by 1024, and uses kilo as the lowest unit
	AUTOSCALE_1000K = 102,  // scales by 1000, and uses kilo as the lowest unit

	AUTOSCALE_OFF   = 0,
	AUTOSCALE_ON    = AUTOSCALE_1024
};

class MeasureValueSet
{
public:
	MeasureValueSet(double val, std::wstring_view str) : m_Value(val), m_StringValue(str) {}
	void Set(double val, std::wstring_view str) { m_Value = val; m_StringValue = str; }
	bool IsChanged(double val, std::wstring_view str) { if (m_Value != val || m_StringValue != str) { Set(val, str); return true; } return false; }
private:
	double m_Value;
	std::wstring m_StringValue;
};

class Meter;
class Skin;
class ConfigParser;
class Pcre;

class __declspec(novtable) Measure : public Section
{
public:
	virtual ~Measure();

	Measure(const Measure& other) = delete;

	UINT GetBaseTypeID() override { return TypeID<Measure>(); }

	virtual void Initialize();

	using Section::ReadOptions;
	void ReadOptions(ConfigParser::OptionReader& reader) override;

	bool Update(bool rereadOptions = false);

	void Disable();
	void Enable();
	bool IsDisabled() { return m_Disabled; }

	void Pause();
	void Unpause();
	bool IsPaused() { return m_Paused; }

	virtual void Command(const std::wstring& command);

	double GetValue();
	double GetRelativeValue();
	double GetValueRange();
	double GetMinValue() { return m_MinValue; }
	double GetMaxValue() { return m_MaxValue; }

	virtual std::optional<std::wstring_view> GetStringValue();
	std::wstring_view GetStringOrFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);
	std::wstring_view GetFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);

	static void GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords);
	static void RemoveTrailingZero(WCHAR* str, int strLen);

	const std::wstring& GetOnChangeAction() { return m_OnChangeAction; }
	void DoChangeAction(bool execute = true);

	static Measure* Create(const WCHAR* measure, Skin* skin, const WCHAR* name);

protected:
	Measure(Skin* skin, const WCHAR* name);

	virtual void UpdateValue() = 0;

	// Reads NumberConversionFormat, which selects the separators used by the measures that convert
	// a string into their number value.
	LocaleUtil::NumberFormat ReadNumberFormatOption(ConfigParser::OptionReader& reader);

	bool ParseSubstitute(std::wstring buffer);
	std::wstring ExtractWord(std::wstring& buffer);
	std::wstring_view CheckSubstitute(std::wstring_view buffer);
	void MakePlainSubstitute(std::wstring& str, const std::wstring& pattern, const std::wstring& replacement);

	double m_Value;

	bool m_Invert;

	bool m_LogMaxValue;
	double m_MinValue;
	double m_MaxValue;

	struct Substitute
	{
		Substitute(std::wstring pattern, std::wstring replacement);
		~Substitute();
		Substitute(Substitute&&) noexcept;
		Substitute& operator=(Substitute&&) noexcept;

		std::wstring pattern;
		std::wstring replacement;
		std::unique_ptr<Pcre> regexp;
	};

	std::vector<Substitute> m_Substitute;
	bool m_RegExpSubstitute;

	struct AverageData
	{
		std::vector<double> values;
		UINT pos = 0;
		UINT size = 0;
	};

	std::unique_ptr<AverageData> m_Average;

	IfActions m_IfActions;

	bool m_Disabled;
	bool m_Paused;
	bool m_Initialized;

	std::wstring m_OnChangeAction;
	MeasureValueSet* m_OldValue;
	bool m_ValueAssigned;
};
