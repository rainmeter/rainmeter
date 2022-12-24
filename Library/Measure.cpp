/* Copyright (C) 2000 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Measure.h"
#include "MeasureCPU.h"
#include "MeasureMediaKey.h"
#include "MeasureMemory.h"
#include "MeasurePhysicalMemory.h"
#include "MeasureVirtualMemory.h"
#include "MeasureNetIn.h"
#include "MeasureNetOut.h"
#include "MeasureNetTotal.h"
#include "MeasureNowPlaying.h"
#include "MeasureDiskSpace.h"
#include "MeasureUptime.h"
#include "MeasurePlugin.h"
#include "MeasureProcess.h"
#include "MeasureRecycleManager.h"
#include "MeasureRegistry.h"
#include "MeasureString.h"
#include "MeasureTime.h"
#include "MeasureCalc.h"
#include "MeasureScript.h"
#include "MeasureSysInfo.h"
#include "MeasureLoop.h"
#include "MeasureWebParser.h"
#include "MeasureWifiStatus.h"
#include "Rainmeter.h"
#include "Util.h"
#include "pcre/config.h"
#include "pcre/pcre.h"

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

const int MEDIAN_SIZE = 3;

Measure::Measure(Skin* skin, const WCHAR* name) : Section(skin, name),
	m_Value(0.0),
	m_Invert(false),
	m_LogMaxValue(false),
	m_MinValue(0.0),
	m_MaxValue(1.0),
	m_MinValueDefined(false),
	m_MaxValueDefined(false),
	m_RegExpSubstitute(false),
	m_MedianPos(),
	m_AveragePos(),
	m_AverageSize(),
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

/*
** Initializes the measure.
**
*/
void Measure::Initialize()
{
	m_Initialized = true;

	if (GetRainmeter().GetDebug() && (m_MinValueDefined || m_MaxValueDefined))
	{
		if (m_MaxValue == m_MinValue)
		{
			WCHAR buffer[32] = { 0 };
			_snwprintf_s(buffer, _TRUNCATE, L"%f", m_MaxValue);
			RemoveTrailingZero(buffer, (int)wcslen(buffer));
			LogWarningF(this, L"Warning: MaxValue = MinValue: %s", buffer);
		}
		else if (m_MaxValue < m_MinValue)
		{
			WCHAR maxValue[32] = { 0 };
			WCHAR minValue[32] = { 0 };
			_snwprintf_s(maxValue, _TRUNCATE, L"%f", m_MaxValue);
			_snwprintf_s(minValue, _TRUNCATE, L"%f", m_MinValue);
			RemoveTrailingZero(maxValue, (int)wcslen(maxValue));
			RemoveTrailingZero(minValue, (int)wcslen(minValue));
			LogWarningF(this, L"Warning: MaxValue is less than MinValue: MaxValue=%s MinValue=%s", maxValue, minValue);
		}
	}
}

/*
** Read the common options specified in the ini file. The inherited classes must
** call this base implementation if they overwrite this method.
**
*/
void Measure::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	bool oldOnChangeActionEmpty = m_OnChangeAction.empty();

	Section::ReadOptions(parser, section);

	// Clear substitutes to prevent from being added more than once.
	if (!m_Substitute.empty())
	{
		m_Substitute.clear();
	}

	m_Invert = parser.ReadBool(section, L"InvertMeasure", false);

	m_Disabled = parser.ReadBool(section, L"Disabled", false);
	m_Paused = parser.ReadBool(section, L"Paused", false);

	m_MinValue = parser.ReadFloat(section, L"MinValue", m_MinValue);
	m_MaxValueDefined = parser.GetLastValueDefined();

	m_MaxValue = parser.ReadFloat(section, L"MaxValue", m_MaxValue);
	m_MaxValueDefined = parser.GetLastValueDefined();

	m_IfActions.ReadOptions(parser, section);

	// The first time around, we read the conditions here. Subsequent rereads will be done in
	// Update() if needed.
	if (!m_Initialized)
	{
		m_IfActions.ReadConditionOptions(parser, section);
	}

	m_OnChangeAction = parser.ReadString(section, L"OnChangeAction", L"", false);

	m_AverageSize = parser.ReadUInt(section, L"AverageSize", 0U);

	m_RegExpSubstitute = parser.ReadBool(section, L"RegExpSubstitute", false);
	std::wstring subs = parser.ReadString(section, L"Substitute", L"");
	if (!subs.empty())
	{
		if ((subs[0] != L'"' || subs[subs.length() - 1] != L'\'') &&
			(subs[0] != L'\'' || subs[subs.length() - 1] != L'"'))
		{
			// Add quotes since they are removed by the GetProfileString
			subs.insert(0, 1, L'"');
			subs += L'"';
		}
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

/*
** Substitues text using a straight find and replace method
*/
bool Measure::MakePlainSubstitute(std::wstring& str, size_t index)
{
	size_t start = 0, pos;

	do
	{
		pos = str.find(m_Substitute[index], start);
		if (pos != std::wstring::npos)
		{
			str.replace(pos, m_Substitute[index].length(), m_Substitute[index + 1]);
			start = pos + m_Substitute[index + 1].length();
		}
	}
	while (pos != std::wstring::npos);

	return true;
}

/*
** Substitutes part of the text
*/
const WCHAR* Measure::CheckSubstitute(const WCHAR* buffer)
{
	static std::wstring str;

	if (m_Substitute.empty())
	{
		return buffer;
	}

	str = buffer;
	if (!m_RegExpSubstitute)
	{
		for (size_t i = 0, isize = m_Substitute.size(); i < isize; i += 2)
		{
			if (!m_Substitute[i].empty())
			{
				MakePlainSubstitute(str, i);
			}
			else if (str.empty())
			{
				// Empty result and empty substitute -> use second
				str = m_Substitute[i + 1];
			}
		}
	}
	else
	{
		int ovector[300];
		for (size_t i = 0, isize = m_Substitute.size(); i < isize; i += 2)
		{
			const char* error;
			int errorOffset;
			int offset = 0;
			pcre16* re = pcre16_compile(
				(PCRE_SPTR16)m_Substitute[i].c_str(),
				PCRE_UTF16,
				&error,
				&errorOffset,
				nullptr);  // Use default character tables.
			if (!re)
			{
				MakePlainSubstitute(str, i);
				LogNoticeF(this, L"Substitute: %S", error);
			}
			else
			{
				do
				{
					const int options = str.empty() ? 0 : PCRE_NOTEMPTY;
					const int rc = pcre16_exec(
						re,
						nullptr,
						(PCRE_SPTR16)str.c_str(),
						(int)str.length(),
						offset,
						options,               // Empty string is not a valid match
						ovector,
						(int)_countof(ovector));
					if (rc <= 0)
					{
						break;
					}

					std::wstring result = m_Substitute[i + 1];

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
					offset = start + (int)result.length();
				}
				while (true);

				pcre16_free(re);
			}
		}
	}

	return str.c_str();
}

/*
** Reads the buffer for "Name":"Value"-pairs separated with comma and
** fills the map with the parsed data.
*/
bool Measure::ParseSubstitute(std::wstring buffer)
{
	if (buffer.empty()) return true;

	do
	{
		std::wstring word1 = ExtractWord(buffer);
		std::wstring sep = ExtractWord(buffer);
		if (sep.size() != 1 || sep[0] != L':') return false;
		std::wstring word2 = ExtractWord(buffer);

		if (wcscmp(word1.c_str(), word2.c_str()) != 0)
		{
			if (m_RegExpSubstitute && word1.empty())
			{
				word1 = L"^$";
			}

			m_Substitute.push_back(word1);
			m_Substitute.push_back(word2);
		}

		std::wstring sep2 = ExtractWord(buffer);
		if (!sep2.empty() && (sep2.size() != 1 || sep2[0] != L',')) return false;
	}
	while (!buffer.empty());

	return true;
}

/*
** Returns the first word from the buffer. The word can be inside quotes.
** If not, the separators are ' ', '\t', ',' and ':'. Whitespaces are removed
** and buffer _will_ be modified.
*/
std::wstring Measure::ExtractWord(std::wstring& buffer)
{
	std::wstring::size_type end, len = buffer.size();
	std::wstring ret;

	if (len == 0) return ret;

	// Remove whitespaces
	end = 0;
	while (end < len && (buffer[end] == L' ' || buffer[end] == L'\t' || buffer[end] == L'\n')) ++end;
	if (end == len)
	{
		// End of line reached
		end = std::wstring::npos;
	}
	else
	{
		buffer.erase(0, end);
		len = buffer.size();

		if (buffer[0] == L'"' || buffer[0] == L'\'')
		{
			WCHAR quote = buffer[0];

			end = 1;	// Skip the '"'
			// Quotes around the word
			while (end < len && (buffer[end] != quote)) ++end;
			if (end == len) end = std::wstring::npos;

			if (end != std::wstring::npos)
			{
				ret.assign(buffer, 1, end - 1);
				++end;
			}
			else
			{
				// End of string reached - discard result
			}
		}
		else
		{
			end = 0;
			while (end < len && (buffer[end] != L',' && buffer[end] != L':' && buffer[end] != L' ' && buffer[end] != L'\t')) ++end;
			if (end == len) end = std::wstring::npos;

			if (end == std::wstring::npos)
			{
				// End of line reached
				ret = buffer;
			}
			else
			{
				ret.assign(buffer, 0, ++end);	// The separator is also returned!
			}
		}
	}

	buffer.erase(0, end);

	return ret;
}

bool Measure::Update(bool rereadOptions)
{
	if (rereadOptions)
	{
		ReadOptions(m_Skin->GetParser());
	}

	// Don't do anything if paused
	if (m_Paused) return false;

	if (!m_Disabled)
	{
		// Only update the counter if the divider
		if (!UpdateCounter()) return false;

		// Call derived method to update value
		UpdateValue();

		if (m_AverageSize > 0)
		{
			size_t averageValuesSize = m_AverageValues.size();

			if (m_AverageSize != averageValuesSize)
			{
				m_AverageValues.resize(m_AverageSize, m_Value);
				averageValuesSize = m_AverageValues.size();
				if (m_AveragePos >= averageValuesSize) m_AveragePos = 0;
			}
			m_AverageValues[m_AveragePos] = m_Value;

			++m_AveragePos;
			m_AveragePos %= averageValuesSize;

			// Calculate the average value
			double value = 0;
			for (size_t i = 0; i < averageValuesSize; ++i)
			{
				value += m_AverageValues[i];
			}
			m_Value = value / (double)averageValuesSize;
		}

		// If we're logging the maximum value of the measure, check if
		// the new value is greater than the old one, and update if necessary.
		if (m_LogMaxValue)
		{
			if (m_MedianValues.empty())
			{
				m_MedianValues.resize(MEDIAN_SIZE, 0);
			}

			m_MedianValues[m_MedianPos] = m_Value;
			++m_MedianPos;
			m_MedianPos %= MEDIAN_SIZE;

			std::vector<double> medianArray = m_MedianValues;
			std::sort(&medianArray.data()[0], &medianArray.data()[MEDIAN_SIZE]);  // Workaround for "Debug" build mode

			double medianValue = medianArray[MEDIAN_SIZE / 2];
			m_MaxValue = max(m_MaxValue, medianValue);
			m_MinValue = min(m_MinValue, medianValue);
		}

		m_ValueAssigned = true;

		// For the conditional options to work with the current measure value when using
		// [MeasureName], we need to read the options after m_Value has been changed.
		if (rereadOptions)
		{
			m_IfActions.ReadConditionOptions(m_Skin->GetParser(), GetName());
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

/*
** Returns the value of the measure.
**
*/
double Measure::GetValue()
{
	// Invert if so requested
	if (m_Invert)
	{
		return m_MaxValue - m_Value + m_MinValue;
	}

	return m_Value;
}

/*
** Returns the relative value of the measure (0.0 - 1.0).
**
*/
double Measure::GetRelativeValue()
{
	double range = GetValueRange();

	if (range != 0.0)
	{
		double value = GetValue();

		value = min(m_MaxValue, value);
		value = max(m_MinValue, value);

		value -= m_MinValue;

		return value / range;
	}

	return 1.0;
}

/*
** Returns the value range.
**
*/
double Measure::GetValueRange()
{
	return m_MaxValue - m_MinValue;
}

/*
** Base implementation. Derivied classes can provide an alternative implementation if they have a
** string value that is not based on m_Value.
**
*/
const WCHAR* Measure::GetStringValue()
{
	return nullptr;
}

/*
** Returns the unformatted string value if the measure has one or a formatted value otherwise.
**
*/
const WCHAR* Measure::GetStringOrFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	const WCHAR* stringValue = GetStringValue();
	return stringValue ? stringValue : GetFormattedValue(autoScale, scale, decimals, percentual);
}

/*
** This method returns the value as text string. The actual value is
** get with GetValue() so we don't have to worry about m_Invert.
**
** autoScale  If true, scale the value automatically to some sensible range.
** scale      The scale to use if autoScale is false.
** decimals   Number of decimals used in the value. If -1, get rid of ".00000" for dynamic variables.
** percentual Return the value as % from the maximum value.
*/
const WCHAR* Measure::GetFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	static WCHAR buffer[128];
	WCHAR format[32];

	if (percentual)
	{
		double val = 100.0 * GetRelativeValue();

		_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
		_snwprintf_s(buffer, _TRUNCATE, format, val);
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
			int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", val);
			RemoveTrailingZero(buffer, len);
		}
		else
		{
			_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
			_snwprintf_s(buffer, _TRUNCATE, format, val);
		}
	}

	return CheckSubstitute(buffer);
}

void Measure::GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords)
{
	WCHAR format[32] = { 0 };
	double value = 0;

	if (decimals == 0)
	{
		wcsncpy_s(format, L"%.0f", _TRUNCATE);
	}
	else
	{
		_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
	}

	const double* tblScale =
		g_TblScale[(autoScale == AUTOSCALE_1000 || autoScale == AUTOSCALE_1000K) ? AUTOSCALE_INDEX_1000 : AUTOSCALE_INDEX_1024];

	if (theValue >= tblScale[0])
	{
		value = theValue / tblScale[0];
		wcsncat_s(format, L" T", _TRUNCATE);
	}
	else if (theValue >= tblScale[1])
	{
		value = theValue / tblScale[1];
		wcsncat_s(format, L" G", _TRUNCATE);
	}
	else if (theValue >= tblScale[2])
	{
		value = theValue / tblScale[2];
		wcsncat_s(format, L" M", _TRUNCATE);
	}
	else if (autoScale == AUTOSCALE_1024K || autoScale == AUTOSCALE_1000K || theValue >= tblScale[3])
	{
		value = theValue / tblScale[3];
		wcsncat_s(format, L" k", _TRUNCATE);
	}
	else
	{
		value = theValue;
		wcsncat_s(format, L" ", _TRUNCATE);
	}
	_snwprintf_s(buffer, sizeInWords, _TRUNCATE, format, value);
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

/*
** Executes OnChangeAction if action is set.
** If execute parameter is set to false, only updates old value with current value.
**
*/
void Measure::DoChangeAction(bool execute)
{
	if (!m_OnChangeAction.empty() && m_ValueAssigned)
	{
		double newValue = GetValue();
		const WCHAR* newStringValue = GetStringValue();
		if (!newStringValue)
		{
			newStringValue = L"";
		}

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

/*
** Creates the given measure. This is the factory method for the measures.
** If new measures are implemented this method needs to be updated.
**
*/
Measure* Measure::Create(const WCHAR* measure, Skin* skin, const WCHAR* name)
{
	// Comparison is caseinsensitive

	if (_wcsicmp(L"CPU", measure) == 0)
	{
		return new MeasureCPU(skin, name);
	}
	else if (_wcsicmp(L"MediaKey", measure) == 0)
	{
		return new MeasureMediaKey(skin, name);
	}
	else if (_wcsicmp(L"Memory", measure) == 0)
	{
		return new MeasureMemory(skin, name);
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
	else if (_wcsicmp(L"SwapMemory", measure) == 0)
	{
		return new MeasureVirtualMemory(skin, name);
	}
	else if (_wcsicmp(L"FreeDiskSpace", measure) == 0)
	{
		return new MeasureDiskSpace(skin, name);
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
	else if (_wcsicmp(L"Process", measure) == 0)
	{
		return new MeasureProcess(skin, name);
	}
	else if (_wcsicmp(L"RecycleManager", measure) == 0)
	{
		return new MeasureRecycleManager(skin, name);
	}
	else if (_wcsicmp(L"Registry", measure) == 0)
	{
		return new MeasureRegistry(skin, name);
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

	LogErrorF(skin, L"Measure=%s is not valid in [%s]", measure, name);

	return nullptr;
}

/*
** Executes a custom bang.
**
*/
void Measure::Command(const std::wstring& command)
{
	LogWarningF(this, L"!CommandMeasure: Not supported");
}

/*
** Returns the number value of a measure, used by IfCondition's.
**
*/
bool Measure::GetCurrentMeasureValue(const WCHAR* str, int len, double* value, void* context)
{
	auto measure = (Measure*)context;
	const std::vector<Measure*>& measures = measure->m_Skin->GetMeasures();

	for (const auto& iter : measures)
	{
		if (iter->GetOriginalName().length() == len &&
			_wcsnicmp(str, iter->GetName(), len) == 0)
		{
			*value = iter->GetValue();
			return true;
		}
	}

	return false;
}
