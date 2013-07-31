/*
  Copyright (C) 2000 Kimmo Pekkola

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

#include "StdAfx.h"
#include "Measure.h"
#include "MeasureCPU.h"
#include "MeasureMemory.h"
#include "MeasurePhysicalMemory.h"
#include "MeasureVirtualMemory.h"
#include "MeasureNetIn.h"
#include "MeasureNetOut.h"
#include "MeasureNetTotal.h"
#include "MeasureDiskSpace.h"
#include "MeasureUptime.h"
#include "MeasurePlugin.h"
#include "MeasureRegistry.h"
#include "MeasureTime.h"
#include "MeasureCalc.h"
#include "MeasureScript.h"
#include "Rainmeter.h"
#include "Error.h"
#include "Litestep.h"
#include "pcre-8.10/config.h"
#include "pcre-8.10/pcre.h"

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

/*
** The constructor
**
*/
Measure::Measure(MeterWindow* meterWindow, const WCHAR* name) : Section(meterWindow, name),
	m_Invert(false),
	m_LogMaxValue(false),
	m_MinValue(),
	m_MaxValue(1.0),
	m_Value(),
	m_RegExpSubstitute(false),
	m_MedianPos(),
	m_AveragePos(),
	m_AverageSize(),
	m_IfEqualValue(),
	m_IfAboveValue(),
	m_IfBelowValue(),
	m_IfEqualCommitted(false),
	m_IfAboveCommitted(false),
	m_IfBelowCommitted(false),
	m_Disabled(false),
	m_Paused(false),
	m_Initialized(false),
	m_OldValue(),
	m_ValueAssigned(false)
{
}

/*
** The destructor
**
*/
Measure::~Measure()
{
	delete m_OldValue;
}

/*
** Initializes the measure.
**
*/
void Measure::Initialize()
{
	m_Initialized = true;
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
	m_MaxValue = parser.ReadFloat(section, L"MaxValue", m_MaxValue);

	// The ifabove/ifbelow define actions that are ran when the value goes above/below the given number.

	m_IfAboveValue = parser.ReadFloat(section, L"IfAboveValue", 0.0);
	m_IfAboveAction = parser.ReadString(section, L"IfAboveAction", L"", false);

	m_IfBelowValue = parser.ReadFloat(section, L"IfBelowValue", 0.0);
	m_IfBelowAction = parser.ReadString(section, L"IfBelowAction", L"", false);

	m_IfEqualValue = (int64_t)parser.ReadFloat(section, L"IfEqualValue", 0.0);
	m_IfEqualAction = parser.ReadString(section, L"IfEqualAction", L"", false);

	m_OnChangeAction = parser.ReadString(section, L"OnChangeAction", L"", false);

	m_AverageSize = parser.ReadUInt(section, L"AverageSize", 0);

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
	m_MeterWindow->GetParser().SetValue(m_Name, L"Disabled", L"1");
}

void Measure::Enable()
{
	m_Disabled = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_MeterWindow->GetParser().SetValue(m_Name, L"Disabled", L"0");
}

void Measure::Pause()
{
	m_Paused = true;

	// Change the option as well to avoid reset in ReadOptions().
	m_MeterWindow->GetParser().SetValue(m_Name, L"Paused", L"1");
}

void Measure::Unpause()
{
	m_Paused = false;

	// Change the option as well to avoid reset in ReadOptions().
	m_MeterWindow->GetParser().SetValue(m_Name, L"Paused", L"0");
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

	if (!m_Substitute.empty())
	{
		if (!m_RegExpSubstitute)	// Plain Substitutions only
		{
			str = buffer;

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
		else // Contains a RegEx
		{
			std::string utf8str = StringUtil::NarrowUTF8(buffer);
			int* ovector = new int[OVECCOUNT];

			for (size_t i = 0, isize = m_Substitute.size(); i < isize ; i += 2)
			{
				pcre* re;
				const char* error;
				int erroffset;
				int rc;
				int flags = PCRE_UTF8;
				int offset = 0;

				re = pcre_compile(
					StringUtil::NarrowUTF8(m_Substitute[i]).c_str(),   // the pattern
					flags,						// default options
					&error,						// for error message
					&erroffset,					// for error offset
					nullptr);						// use default character tables

				if (re == nullptr)
				{
					MakePlainSubstitute(str, i);
					LogNoticeF(this, L"Substitute: %S", error);
				}
				else
				{
					do
					{
						rc = pcre_exec(
							re,						// the compiled pattern
							nullptr,					// no extra data - we didn't study the pattern
							utf8str.c_str(),		// the subject string
							utf8str.length(),		// the length of the subject
							offset,					// start at offset 0 in the subject
							0,						// default options
							ovector,				// output vector for substring information
							OVECCOUNT);				// number of elements in the output vector

						if (rc <= 0)
						{
							break;
						}
						else
						{
							std::string result = StringUtil::NarrowUTF8(m_Substitute[i + 1]);

							if (rc > 1)
							{
								for (int j = rc - 1 ; j >= 0 ; --j)
								{
									size_t new_start = ovector[2 * j];
									size_t in_length = ovector[2 * j + 1] - ovector[2 * j];

									char tmpName[64];

									size_t cut_length = _snprintf_s(tmpName, _TRUNCATE, "\\%i", j);;
									size_t start = 0, pos;
									do
									{
										pos = result.find(tmpName, start, cut_length);
										if (pos != std::string::npos)
										{
											result.replace(pos, cut_length, utf8str, new_start, in_length);
											start = pos + in_length;
										}
									}
									while (pos != std::string::npos);
								}
							}

							size_t start = ovector[0];
							size_t length = ovector[1] - ovector[0];
							utf8str.replace(start, length, result);
							offset = start + result.length();
						}
					}
					while (true);

					// Release memory used for the compiled pattern
					pcre_free(re);
				}
			}

			delete [] ovector;

			str = StringUtil::WidenUTF8(utf8str);
		}

		return str.c_str();
	}
	else
	{
		return buffer;
	}
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

bool Measure::Update()
{
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

			auto medianArray = m_MedianValues;
			std::sort(&medianArray.data()[0], &medianArray.data()[MEDIAN_SIZE]);  // Workaround for "Debug" build mode

			double medianValue = medianArray[MEDIAN_SIZE / 2];
			m_MaxValue = max(m_MaxValue, medianValue);
			m_MinValue = min(m_MinValue, medianValue);
		}

		m_ValueAssigned = true;

		if (m_MeterWindow)
		{
			if (!m_IfEqualAction.empty())
			{
				if ((int64_t)m_Value == m_IfEqualValue)
				{
					if (!m_IfEqualCommitted)
					{
						m_IfEqualCommitted = true;	// To avoid infinite loop from !Update
						GetRainmeter().ExecuteCommand(m_IfEqualAction.c_str(), m_MeterWindow);
					}
				}
				else
				{
					m_IfEqualCommitted = false;
				}
			}

			if (!m_IfAboveAction.empty())
			{
				if (m_Value > m_IfAboveValue)
				{
					if (!m_IfAboveCommitted)
					{
						m_IfAboveCommitted = true;	// To avoid infinite loop from !Update
						GetRainmeter().ExecuteCommand(m_IfAboveAction.c_str(), m_MeterWindow);
					}
				}
				else
				{
					m_IfAboveCommitted = false;
				}
			}

			if (!m_IfBelowAction.empty())
			{
				if (m_Value < m_IfBelowValue)
				{
					if (!m_IfBelowCommitted)
					{
						m_IfBelowCommitted = true;	// To avoid infinite loop from !Update
						GetRainmeter().ExecuteCommand(m_IfBelowAction.c_str(), m_MeterWindow);
					}
				}
				else
				{
					m_IfBelowCommitted = false;
				}
			}
		}

		return true;
	}
	else
	{
		// Disabled measures have 0 as value
		m_Value = 0.0;

		// Set IfAction committed state to false if condition is not met with value = 0
		if (m_IfEqualValue != 0)
		{
			m_IfEqualCommitted = false;
		}

		if (m_IfAboveValue <= 0.0)
		{
			m_IfAboveCommitted = false;
		}

		if (m_IfBelowValue >= 0.0)
		{
			m_IfBelowCommitted = false;
		}

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
	WCHAR format[32];
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
				GetRainmeter().ExecuteCommand(m_OnChangeAction.c_str(), m_MeterWindow);
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
Measure* Measure::Create(const WCHAR* measure, MeterWindow* meterWindow, const WCHAR* name)
{
	// Comparison is caseinsensitive

	if (_wcsicmp(L"CPU", measure) == 0)
	{
		return new MeasureCPU(meterWindow, name);
	}
	else if (_wcsicmp(L"Memory", measure) == 0)
	{
		return new MeasureMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"NetIn", measure) == 0)
	{
		return new MeasureNetIn(meterWindow, name);
	}
	else if (_wcsicmp(L"NetOut", measure) == 0)
	{
		return new MeasureNetOut(meterWindow, name);
	}
	else if (_wcsicmp(L"NetTotal", measure) == 0)
	{
		return new MeasureNetTotal(meterWindow, name);
	}
	else if (_wcsicmp(L"PhysicalMemory", measure) == 0)
	{
		return new MeasurePhysicalMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"SwapMemory", measure) == 0)
	{
		return new MeasureVirtualMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"FreeDiskSpace", measure) == 0)
	{
		return new MeasureDiskSpace(meterWindow, name);
	}
	else if (_wcsicmp(L"Uptime", measure) == 0)
	{
		return new MeasureUptime(meterWindow, name);
	}
	else if (_wcsicmp(L"Time", measure) == 0)
	{
		return new MeasureTime(meterWindow, name);
	}
	else if (_wcsicmp(L"Plugin", measure) == 0)
	{
		return new MeasurePlugin(meterWindow, name);
	}
	else if (_wcsicmp(L"Registry", measure) == 0)
	{
		return new MeasureRegistry(meterWindow, name);
	}
	else if (_wcsicmp(L"Calc", measure) == 0)
	{
		return new MeasureCalc(meterWindow, name);
	}
	else if (_wcsicmp(L"Script", measure) == 0)
	{
		return new MeasureScript(meterWindow, name);
	}

	LogErrorF(meterWindow, L"Measure=%s is not valid in [%s]", measure, name);

	return nullptr;
}

/*
** Executes a custom bang.
**
*/
void Measure::Command(const std::wstring& command)
{
	LogWarningF(this, L"!CommandMeasure: Not supported by [%s]", m_Name.c_str());
}
