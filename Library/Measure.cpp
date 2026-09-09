// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Measure.h"
#include "MeasureActionTimer.h"
#include "MeasureAdvancedCPU.h"
#include "MeasureAudioLevel.h"
#include "MeasureCPU.h"
#include "MeasureCoreTemp.h"
#include "MeasureMediaKey.h"
#include "MeasureMemory.h"
#include "MeasureMouse.h"
#include "MeasurePhysicalMemory.h"
#include "MeasurePerfMon.h"
#include "MeasurePing.h"
#include "MeasureVirtualMemory.h"
#include "MeasureNetIn.h"
#include "MeasureNetOut.h"
#include "MeasureNetTotal.h"
#include "MeasureNowPlaying.h"
#include "MeasureDiskSpace.h"
#include "MeasureDragDrop.h"
#include "MeasureFileView.h"
#include "MeasureFolderInfo.h"
#include "MeasureInputText.h"
#include "MeasureiTunes.h"
#include "MeasureUptime.h"
#include "MeasurePlugin.h"
#include "MeasurePower.h"
#include "MeasureProcess.h"
#include "MeasureQuote.h"
#include "MeasureRecycleManager.h"
#include "MeasureRegistry.h"
#include "MeasureResMon.h"
#include "MeasureRunCommand.h"
#include "MeasureString.h"
#include "MeasureTime.h"
#include "MeasureUsageMonitor.h"
#include "MeasureCalc.h"
#include "MeasureScript.h"
#include "MeasureSpeedFan.h"
#include "MeasureSysInfo.h"
#include "MeasureLoop.h"
#include "MeasureWebParser.h"
#include "MeasureWifiStatus.h"
#include "MeasureAudio.h"
#include "MeasureWindowMessage.h"
#include "Rainmeter.h"
#include "Util.h"
#include "Pcre.h"
#include "../Common/StringParser.h"

#define OVECCOUNT 300	// Should be a multiple of 3

enum AUTOSCALE_INDEX
{
	AUTOSCALE_INDEX_1024 = 0,
	AUTOSCALE_INDEX_1000 = 1
};

static const double g_TblScale[2][4] = {
	{
		1024.0 * 1024.0 * 1024.0 * 1024.0,
		1024.0 * 1024.0 * 1024.0,
		1024.0 * 1024.0,
		1024.0
	},
	{
		1000.0 * 1000.0 * 1000.0 * 1000.0,
		1000.0 * 1000.0 * 1000.0,
		1000.0 * 1000.0,
		1000.0
	}
};

Measure::Substitute::Substitute(std::wstring pattern, std::wstring replacement) :
	pattern(std::move(pattern)),
	replacement(std::move(replacement))
{
}

Measure::Substitute::~Substitute() = default;
Measure::Substitute::Substitute(Substitute&&) noexcept = default;
Measure::Substitute& Measure::Substitute::operator=(Substitute&&) noexcept = default;

static int FormatValue(WCHAR* buffer, size_t size, double value, int decimals, std::wstring_view suffix = {})
{
	if (size == 0) return 0;

	const auto result = fmt::format_to_n(buffer, size - 1, L"{:.{}f}{}", value, decimals, suffix);
	const size_t length = std::min(result.size, size - 1);
	buffer[length] = L'\0';
	return (int)length;
}

Measure::Measure(Skin* skin, const WCHAR* name) : Section(skin, name),
	m_Value(0.0),
	m_Invert(false),
	m_LogMaxValue(false),
	m_MinValue(0.0),
	m_MaxValue(1.0),
	m_RegExpSubstitute(false),
	m_Disabled(false),
	m_Paused(false),
	m_Initialized(false),
	m_OldValue(),
	m_ValueAssigned(false)
{
}

Measure::~Measure()
{
	delete m_OldValue;
	m_OldValue = nullptr;
}

void Measure::Initialize()
{
	m_Initialized = true;
}

// Read the common options specified in the ini file. The inherited classes must
// call this base implementation if they overwrite this method.
void Measure::ReadOptions(ConfigParser::OptionReader& reader)
{
	auto& parser = m_Skin->GetParser();
	bool oldOnChangeActionEmpty = m_OnChangeAction.empty();

	Section::ReadOptions(reader);

	// Clear substitutes to prevent from being added more than once.
	if (!m_Substitute.empty())
	{
		m_Substitute.clear();
	}

	m_Invert = reader.ReadBool<"InvertMeasure">(false);

	m_Disabled = reader.ReadBool<"Disabled">(false);
	m_Paused = reader.ReadBool<"Paused">(false);

	m_MinValue = reader.ReadFloat<"MinValue">(m_MinValue);
	m_MaxValue = reader.ReadFloat<"MaxValue">(m_MaxValue);

	m_IfActions.ReadOptions(reader);

	// The first time around, we read the conditions here. Subsequent rereads will be done in
	// Update() if needed.
	if (!m_Initialized)
	{
		m_IfActions.ReadConditionOptions(reader);
	}

	reader.ReadString<"OnChangeAction">(m_OnChangeAction, L"", { .sectionVariables = false });

	const UINT averageSize = reader.ReadUInt<"AverageSize">(0);
	if (averageSize == 0)
	{
		m_Average.reset();
	}
	else
	{
		if (!m_Average) m_Average = std::make_unique<AverageData>();
		m_Average->size = averageSize;
	}

	m_RegExpSubstitute = reader.ReadBool<"RegExpSubstitute">(false);
	std::wstring subs = reader.ReadString<"Substitute">(L"");
	if (!subs.empty())
	{
		if (!ParseSubstitute(subs))
		{
			LogErrorF(this, L"Measure: Invalid Substitute=%s", subs.c_str());
		}
	}

	if (m_Initialized &&
		oldOnChangeActionEmpty && !m_OnChangeAction.empty())
	{
		DoChangeAction(false);
	}
}

// "Locale" uses the separators of the user's current locale, "Default" those used by numbers in
// skin files.
LocaleUtil::NumberFormat Measure::ReadNumberFormatOption(ConfigParser::OptionReader& reader)
{
	const std::wstring& option = reader.ReadString<"NumberConversionFormat">(L"");

	if (_wcsicmp(option.c_str(), L"Locale") == 0) return LocaleUtil::NumberFormat::Locale;

	if (!option.empty() && _wcsicmp(option.c_str(), L"Default") != 0)
	{
		LogErrorF(this, L"Measure: Invalid NumberConversionFormat=%s", option.c_str());
	}

	return LocaleUtil::NumberFormat::Default;
}

void Measure::Disable()
{
	m_Disabled = true;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Disabled", L"1");
}

void Measure::Enable()
{
	m_Disabled = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Disabled", L"0");
}

void Measure::Pause()
{
	m_Paused = true;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Paused", L"1");
}

void Measure::Unpause()
{
	m_Paused = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_Skin->GetParser().SetValue(m_Name, L"Paused", L"0");
}

// Substitutes text using a straight find and replace method
void Measure::MakePlainSubstitute(std::wstring& str, const std::wstring& pattern, const std::wstring& replacement)
{
	size_t start = 0;
	while (true)
	{
		const size_t pos = str.find(pattern, start);
		if (pos == std::wstring::npos) break;

		str.replace(pos, pattern.length(), replacement);
		start = pos + replacement.length();
	}
}

// Substitutes part of the text
std::wstring_view Measure::CheckSubstitute(std::wstring_view buffer)
{
	static std::wstring str;

	if (m_Substitute.empty())
	{
		return buffer;
	}

	str = buffer;
	if (!m_RegExpSubstitute)
	{
		for (const auto& substitute : m_Substitute)
		{
			if (!substitute.pattern.empty())
			{
				MakePlainSubstitute(str, substitute.pattern, substitute.replacement);
			}
			else if (str.empty())
			{
				// Empty result and empty substitute -> use second
				str = substitute.replacement;
			}
		}
	}
	else
	{
		int ovector[300];
		for (const auto& substitute : m_Substitute)
		{
			if (!substitute.regexp)
			{
				MakePlainSubstitute(str, substitute.pattern, substitute.replacement);
			}
			else
			{
				Pcre& regexp = *substitute.regexp;
				regexp.SetOffset(0);
				do
				{
					const int options = str.empty() ? 0 : PCRE_NOTEMPTY;
					// Empty string is not a valid match.
					const int rc = regexp.Execute(str, options, ovector, (int)_countof(ovector));
					if (rc <= 0)
					{
						break;
					}

					std::wstring result = substitute.replacement;

					if (rc > 1)
					{
						for (int j = rc - 1 ; j >= 0 ; --j)
						{
							int newStart = ovector[2 * j];
							int inLength = ovector[2 * j + 1] - ovector[2 * j];

							// Match was not found, or length of capture is invalid
							if (newStart < 0 || inLength < 1) continue;

							WCHAR tmpName[64];
							size_t cutLength = _snwprintf_s(tmpName, _TRUNCATE, L"\\%i", j);
							size_t start = 0, pos;
							do
							{
								pos = result.find(tmpName, start, cutLength);
								if (pos != std::string::npos)
								{
									result.replace(pos, cutLength, str, newStart, inLength);
									start = pos + inLength;
								}
							}
							while (pos != std::string::npos);
						}
					}

					const int start = ovector[0];
					const int length = ovector[1] - ovector[0];
					str.replace(start, length, result);
					regexp.SetOffset(start + (int)result.length());
				}
				while (true);
			}
		}
	}

	return str;
}

// Reads the buffer for "Name":"Value"-pairs separated with comma and
// fills the map with the parsed data.
bool Measure::ParseSubstitute(std::wstring_view buffer)
{
	if (buffer.empty()) return true;

	auto isQuote = [](WCHAR ch) { return ch == L'"' || ch == L'\''; };
	auto unquote = [isQuote](std::wstring_view value, bool missingClosingQuote = false) -> std::optional<std::wstring_view>
	{
		if (missingClosingQuote)
		{
			if (value.empty()) return value;
			if (value.front() != L'"') return std::nullopt;
			return value.substr(1);
		}

		if (value.empty()) return value;
		if (!isQuote(value.front()))
		{
			if (isQuote(value.back())) return std::nullopt;
			return value;
		}
		if (value.length() < 2 || value.back() != value.front()) return std::nullopt;
		return value.substr(1, value.length() - 2);
	};

	const bool hasOuterQuotes =
		(buffer.front() == L'"' && buffer.back() == L'\'') ||
		(buffer.front() == L'\'' && buffer.back() == L'"');
	const bool quotesStripped = !hasOuterQuotes;
	std::optional<std::wstring_view> patternValue;
	if (quotesStripped)
	{
		// The INI parser removed the opening quote. Find its closing quote and the pair separator.
		const size_t closingQuote = buffer.find(L'"');
		if (closingQuote == std::wstring_view::npos || closingQuote + 1 >= buffer.length() || buffer[closingQuote + 1] != L':') return false;
		patternValue = buffer.substr(0, closingQuote);
		buffer.remove_prefix(closingQuote + 2);
	}

	StringParser parser(buffer);
	const auto options = StringParser::SkipWhitespace | StringParser::SkipQuoted;
	while (patternValue || !parser.IsConsumed())
	{
		if (!patternValue)
		{
			const std::wstring_view patternToken = parser.ConsumeUntil(L':', options);
			if (patternToken.empty()) return false;
			patternValue = unquote(patternToken);
		}

		const std::wstring_view replacementToken = parser.ConsumeUntilOrRest(L',', options);
		const auto replacementValue = unquote(replacementToken, quotesStripped && parser.IsConsumed());
		if (!patternValue || !replacementValue) return false;

		if (*patternValue != *replacementValue)
		{
			std::wstring pattern(*patternValue);
			if (m_RegExpSubstitute && pattern.empty())
			{
				pattern = L"^$";
			}

			Substitute& substitute = m_Substitute.emplace_back(std::move(pattern), std::wstring(*replacementValue));
			if (m_RegExpSubstitute)
			{
				const char* error;
				substitute.regexp = std::make_unique<Pcre>(substitute.pattern.c_str(), &error);
				if (!*substitute.regexp)
				{
					substitute.regexp.reset();
					LogNoticeF(this, L"Substitute: %S", error);
				}
			}
		}

		patternValue.reset();
	}

	return true;
}

bool Measure::Update(bool rereadOptions)
{
	if (rereadOptions)
	{
		Section::ReadOptions(m_Skin->GetParser(), false);
	}

	// Don't do anything if paused
	if (m_Paused) return false;

	if (!m_Disabled)
	{
		// Only update the counter if the divider
		if (!UpdateCounter()) return false;

		// Call derived method to update value
		UpdateValue();

		if (m_Average)
		{
			auto& average = *m_Average;
			size_t valuesSize = average.values.size();

			if (average.size != valuesSize)
			{
				average.values.resize(average.size, m_Value);
				valuesSize = average.values.size();
				if (average.pos >= valuesSize) average.pos = 0;
			}
			average.values[average.pos] = m_Value;

			++average.pos;
			average.pos %= valuesSize;

			// Calculate the average value
			double value = 0;
			for (size_t i = 0; i < valuesSize; ++i)
			{
				value += average.values[i];
			}
			m_Value = value / (double)valuesSize;
		}

		// If we're logging the maximum value of the measure, check if
		// the new value is greater than the old one, and update if necessary.
		if (m_LogMaxValue)
		{
			m_MaxValue = std::max(m_MaxValue, m_Value);
			m_MinValue = std::min(m_MinValue, m_Value);
		}

		m_ValueAssigned = true;

		// For the conditional options to work with the current measure value when using
		// [MeasureName], we need to read the options after m_Value has been changed.
		if (rereadOptions)
		{
			auto reader = m_Skin->GetParser().GetInheritableOptionReader(m_Name, m_ID);
			m_IfActions.ReadConditionOptions(reader);
		}

		if (m_Skin)
		{
			m_IfActions.DoIfActions(*this, m_Value);
		}

		return true;
	}
	else
	{
		// Disabled measures have 0 as value
		m_Value = 0.0;

		m_IfActions.SetState(m_Value);

		return false;
	}
}

double Measure::GetValue()
{
	// Invert if so requested
	if (m_Invert)
	{
		return m_MaxValue - m_Value + m_MinValue;
	}

	return m_Value;
}

double Measure::GetRelativeValue()
{
	double range = GetValueRange();

	if (range != 0.0)
	{
		double value = GetValue();

		value = std::min(m_MaxValue, value);
		value = std::max(m_MinValue, value);

		value -= m_MinValue;

		return value / range;
	}

	return 1.0;
}

double Measure::GetValueRange()
{
	return m_MaxValue - m_MinValue;
}

// Base implementation. Derivied classes can provide an alternative implementation if they have a
// string value that is not based on m_Value.
std::optional<std::wstring_view> Measure::GetStringValue()
{
	return std::nullopt;
}

std::wstring_view Measure::GetStringOrFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	const std::optional<std::wstring_view> stringValue = GetStringValue();
	if (stringValue) return *stringValue;
	return GetFormattedValue(autoScale, scale, decimals, percentual);
}

// This method returns the value as text string. The actual value is retrieved with GetValue() so
// we don't have to worry about m_Invert here.
//
// autoScale  If true, scale the value automatically to some sensible range.
// scale      The scale to use if autoScale is false.
// decimals   Number of decimals used in the value. If -1, removes ".00000" for dynamic variables.
// percentual Return the value as % from the maximum value.
std::wstring_view Measure::GetFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	static WCHAR buffer[128];

	if (percentual)
	{
		FormatValue(buffer, _countof(buffer), 100.0 * GetRelativeValue(), decimals);
	}
	else if (autoScale != AUTOSCALE_OFF)
	{
		GetScaledValue(autoScale, decimals, GetValue(), buffer, _countof(buffer));
	}
	else
	{
		double val = GetValue() / scale;

		if (decimals == -1)
		{
			const int len = FormatValue(buffer, _countof(buffer), val, 5);
			RemoveTrailingZero(buffer, len);
		}
		else
		{
			FormatValue(buffer, _countof(buffer), val, decimals);
		}
	}

	return CheckSubstitute(buffer);
}

void Measure::GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords)
{
	double value = 0;
	std::wstring_view suffix;

	const double* tblScale =
		g_TblScale[(autoScale == AUTOSCALE_1000 || autoScale == AUTOSCALE_1000K) ? AUTOSCALE_INDEX_1000 : AUTOSCALE_INDEX_1024];

	if (theValue >= tblScale[0])
	{
		value = theValue / tblScale[0];
		suffix = L" T";
	}
	else if (theValue >= tblScale[1])
	{
		value = theValue / tblScale[1];
		suffix = L" G";
	}
	else if (theValue >= tblScale[2])
	{
		value = theValue / tblScale[2];
		suffix = L" M";
	}
	else if (autoScale == AUTOSCALE_1024K || autoScale == AUTOSCALE_1000K || theValue >= tblScale[3])
	{
		value = theValue / tblScale[3];
		suffix = L" k";
	}
	else
	{
		value = theValue;
		suffix = L" ";
	}
	FormatValue(buffer, sizeInWords, value, decimals, suffix);
}

void Measure::RemoveTrailingZero(WCHAR* str, int strLen)
{
	--strLen;
	while (strLen >= 0)
	{
		if (str[strLen] == L'0')
		{
			str[strLen] = L'\0';
			--strLen;
		}
		else
		{
			if (str[strLen] == L'.')
			{
				str[strLen] = L'\0';
			}
			break;
		}
	}
}

// Executes OnChangeAction if action is set.
// If execute parameter is set to false, only updates old value with current value.
void Measure::DoChangeAction(bool execute)
{
	if (!m_OnChangeAction.empty() && m_ValueAssigned)
	{
		double newValue = GetValue();
		const std::wstring_view newStringValue = GetStringValue().value_or(L"");

		if (!m_OldValue)
		{
			m_OldValue = new MeasureValueSet(newValue, newStringValue);
		}
		else if (execute)
		{
			if (m_OldValue->IsChanged(newValue, newStringValue))
			{
				GetRainmeter().ExecuteActionCommand(m_OnChangeAction.c_str(), this);
			}
		}
		else
		{
			m_OldValue->Set(newValue, newStringValue);
		}
	}
}

// Creates the given measure. This is the factory method for the measures.
// If new measures are implemented this method needs to be updated.
Measure* Measure::Create(const WCHAR* measure, Skin* skin, const WCHAR* name)
{
	// Comparison is caseinsensitive

	if (_wcsicmp(L"CPU", measure) == 0)
	{
		return new MeasureCPU(skin, name);
	}
	else if (_wcsicmp(L"CoreTemp", measure) == 0)
	{
		return new MeasureCoreTemp(skin, name);
	}
	else if (_wcsicmp(L"ActionTimer", measure) == 0)
	{
		return new MeasureActionTimer(skin, name);
	}
	else if (_wcsicmp(L"AdvancedCPU", measure) == 0)
	{
		return new MeasureAdvancedCPU(skin, name);
	}
	else if (_wcsicmp(L"Audio", measure) == 0)
	{
		return new MeasureAudio(skin, name);
	}
	else if (_wcsicmp(L"AudioLevel", measure) == 0)
	{
		return new MeasureAudioLevel(skin, name);
	}
	else if (_wcsicmp(L"MediaKey", measure) == 0)
	{
		return new MeasureMediaKey(skin, name);
	}
	else if (_wcsicmp(L"Memory", measure) == 0)
	{
		return new MeasureMemory(skin, name);
	}
	else if (_wcsicmp(L"Mouse", measure) == 0)
	{
		return new MeasureMouse(skin, name);
	}
	else if (_wcsicmp(L"NetIn", measure) == 0)
	{
		return new MeasureNetIn(skin, name);
	}
	else if (_wcsicmp(L"NetOut", measure) == 0)
	{
		return new MeasureNetOut(skin, name);
	}
	else if (_wcsicmp(L"NetTotal", measure) == 0)
	{
		return new MeasureNetTotal(skin, name);
	}
	else if (_wcsicmp(L"NowPlaying", measure) == 0)
	{
		return new MeasureNowPlaying(skin, name);
	}
	else if (_wcsicmp(L"PhysicalMemory", measure) == 0)
	{
		return new MeasurePhysicalMemory(skin, name);
	}
	else if (_wcsicmp(L"PerfMon", measure) == 0)
	{
		return new MeasurePerfMon(skin, name);
	}
	else if (_wcsicmp(L"Ping", measure) == 0)
	{
		return new MeasurePing(skin, name);
	}
	else if (_wcsicmp(L"SwapMemory", measure) == 0)
	{
		return new MeasureVirtualMemory(skin, name);
	}
	else if (_wcsicmp(L"FreeDiskSpace", measure) == 0)
	{
		return new MeasureDiskSpace(skin, name);
	}
	else if (_wcsicmp(L"FolderInfo", measure) == 0)
	{
		return new MeasureFolderInfo(skin, name);
	}
	else if (_wcsicmp(L"FileView", measure) == 0)
	{
		return new MeasureFileView(skin, name);
	}
	else if (_wcsicmp(L"iTunes", measure) == 0)
	{
		return new MeasureiTunes(skin, name);
	}
	else if (_wcsicmp(L"Uptime", measure) == 0)
	{
		return new MeasureUptime(skin, name);
	}
	else if (_wcsicmp(L"Time", measure) == 0)
	{
		return new MeasureTime(skin, name);
	}
	else if (_wcsicmp(L"Plugin", measure) == 0)
	{
		return new MeasurePlugin(skin, name);
	}
	else if (_wcsicmp(L"Power", measure) == 0)
	{
		return new MeasurePower(skin, name);
	}
	else if (_wcsicmp(L"Process", measure) == 0)
	{
		return new MeasureProcess(skin, name);
	}
	else if (_wcsicmp(L"Quote", measure) == 0)
	{
		return new MeasureQuote(skin, name);
	}
	else if (_wcsicmp(L"RecycleManager", measure) == 0)
	{
		return new MeasureRecycleManager(skin, name);
	}
	else if (_wcsicmp(L"Registry", measure) == 0)
	{
		return new MeasureRegistry(skin, name);
	}
	else if (_wcsicmp(L"ResMon", measure) == 0)
	{
		return new MeasureResMon(skin, name);
	}
	else if (_wcsicmp(L"UsageMonitor", measure) == 0)
	{
		return new MeasureUsageMonitor(skin, name);
	}
	else if (_wcsicmp(L"RunCommand", measure) == 0)
	{
		return new MeasureRunCommand(skin, name);
	}
	else if (_wcsicmp(L"Calc", measure) == 0)
	{
		return new MeasureCalc(skin, name);
	}
	else if (_wcsicmp(L"Script", measure) == 0)
	{
		return new MeasureScript(skin, name);
	}
	else if (_wcsicmp(L"String", measure) == 0)
	{
		return new MeasureString(skin, name);
	}
	else if (_wcsicmp(L"InputText", measure) == 0)
	{
		return new MeasureInputText(skin, name);
	}
	else if (_wcsicmp(L"SpeedFan", measure) == 0)
	{
		return new MeasureSpeedFan(skin, name);
	}
	else if (_wcsicmp(L"SysInfo", measure) == 0)
	{
		return new MeasureSysInfo(skin, name);
	}
	else if (_wcsicmp(L"Loop", measure) == 0)
	{
		return new MeasureLoop(skin, name);
	}
	else if (_wcsicmp(L"WebParser", measure) == 0)
	{
		return new MeasureWebParser(skin, name);
	}
	else if (_wcsicmp(L"WifiStatus", measure) == 0)
	{
		return new MeasureWifiStatus(skin, name);
	}
	else if (_wcsicmp(L"WindowMessage", measure) == 0)
	{
		return new MeasureWindowMessage(skin, name);
	}
	else if (_wcsicmp(L"DragDrop", measure) == 0)
	{
		return new MeasureDragDrop(skin, name);
	}

	LogErrorF(skin, L"Measure=%s is not valid in [%s]", measure, name);

	return nullptr;
}

void Measure::Command(const std::wstring& command)
{
	LogWarningF(this, L"!CommandMeasure: Not supported");
}
