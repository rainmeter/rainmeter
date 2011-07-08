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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

const int MEDIAN_SIZE = 7;

extern CRainmeter* Rainmeter;

/*
** CMeasure
**
** The constructor
**
*/
CMeasure::CMeasure(CMeterWindow* meterWindow, const WCHAR* name) : m_MeterWindow(meterWindow), m_Name(name), m_ANSIName(ConvertToAscii(name)),
	m_DynamicVariables(false),
	m_Invert(false),
	m_LogMaxValue(false),
	m_MinValue(),
	m_MaxValue(1.0),
	m_Value(),
	m_MedianPos(),
	m_AveragePos(),
	m_AverageSize(),
	m_IfEqualValue(),
	m_IfAboveValue(),
	m_IfBelowValue(),
	m_IfEqualCommited(false),
	m_IfAboveCommited(false),
	m_IfBelowCommited(false),
	m_Disabled(false),
	m_UpdateDivider(1),
	m_UpdateCounter(1),
	m_Initialized(false)
{
}

/*
** ~CMeasure
**
** The destructor
**
*/
CMeasure::~CMeasure()
{
}

/*
** Initialize
**
** Initializes the measure.
**
*/
void CMeasure::Initialize()
{
	m_Initialized = true;
}

/*
** ReadConfig
**
** Reads the common configs for all Measures. The inherited classes
** must call the base implementation if they overwrite this method.
**
*/
void CMeasure::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Clear substitutes to prevent from being added more than once.
	if (!m_Substitute.empty())
	{
		m_Substitute.clear();
	}

	m_Invert = 0!=parser.ReadInt(section, L"InvertMeasure", 0);

	if (!m_Initialized)
	{
		m_Disabled = 0!=parser.ReadInt(section, L"Disabled", 0);
	}
	else
	{
		const std::wstring& result = parser.ReadString(section, L"Disabled", L"0");
		if (parser.GetLastReplaced())
		{
			m_Disabled = 0!=(int)parser.ParseDouble(result, 0.0, true);
		}
	}

	int updateDivider = parser.ReadInt(section, L"UpdateDivider", 1);
	if (updateDivider != m_UpdateDivider)
	{
		m_UpdateCounter = m_UpdateDivider = updateDivider;
	}

	m_MinValue = parser.ReadFloat(section, L"MinValue", m_MinValue);
	m_MaxValue = parser.ReadFloat(section, L"MaxValue", m_MaxValue);

	// The ifabove/ifbelow define actions that are ran when the value goes above/below the given number.

	m_IfAboveValue = parser.ReadFloat(section, L"IfAboveValue", 0.0);
	m_IfAboveAction = parser.ReadString(section, L"IfAboveAction", L"", false);

	m_IfBelowValue = parser.ReadFloat(section, L"IfBelowValue", 0.0);
	m_IfBelowAction = parser.ReadString(section, L"IfBelowAction", L"", false);

	m_IfEqualValue = parser.ReadFloat(section, L"IfEqualValue", 0.0);
	m_IfEqualAction = parser.ReadString(section, L"IfEqualAction", L"", false);

	m_AverageSize = parser.ReadUInt(section, L"AverageSize", 0);

	m_DynamicVariables = 0!=parser.ReadInt(section, L"DynamicVariables", 0);

	std::wstring subs = parser.ReadString(section, L"Substitute", L"");
	if (!subs.empty() &&
		(subs[0] != L'\"' || subs[subs.length() - 1] != L'\'') &&
		(subs[0] != L'\'' || subs[subs.length() - 1] != L'\"'))
	{
		// Add quotes since they are removed by the GetProfileString
		subs.insert(0, L"\"");
		subs.append(L"\"");
	}
	if (!ParseSubstitute(subs))
	{
		LogWithArgs(LOG_WARNING, L"Incorrect substitute string: %s", subs.c_str());
	}

	const std::wstring& group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);
}

/*
** CheckSubstitute
**
** Substitutes part of the text
*/
const WCHAR* CMeasure::CheckSubstitute(const WCHAR* buffer)
{
	static std::wstring str;

	if (!m_Substitute.empty())
	{
		str = buffer;

		for (size_t i = 0, isize = m_Substitute.size(); i < isize; i += 2)
		{
			if (str.empty() && m_Substitute[i].empty())
			{
				// Empty result and empty substitute -> use second
				str = m_Substitute[i + 1];
			}
			else if (!m_Substitute[i].empty())
			{
				size_t start = 0;
				size_t pos = std::wstring::npos;

				do
				{
					pos = str.find(m_Substitute[i], start);
					if (pos != std::wstring::npos)
					{
						str.replace(str.begin() + pos, str.begin() + pos + m_Substitute[i].size(), m_Substitute[i + 1]);
						start = pos + m_Substitute[i + 1].size();
					}
				} while(pos != std::wstring::npos);
			}
		}

		return str.c_str();
	}
	else
	{
		return buffer;
	}
}

/*
** ParseSubstitute
**
** Reads the buffer for "Name":"Value"-pairs separated with comma and
** fills the map with the parsed data.
*/
bool CMeasure::ParseSubstitute(std::wstring buffer)
{
	if (buffer.empty()) return true;

	while (!buffer.empty())
	{
		std::wstring word1 = ExtractWord(buffer);
		std::wstring sep = ExtractWord(buffer);
		if (sep != L":") return false;
		std::wstring word2 = ExtractWord(buffer);

		if (word1 != word2)
		{
			m_Substitute.push_back(word1);
			m_Substitute.push_back(word2);
		}

		sep = ExtractWord(buffer);
		if (!sep.empty() && sep != L",") return false;
	}

	return true;
}

/*
** ExtractWord
**
** Returns the first word from the buffer. The word can be inside quotes.
** If not, the separators are ' ', '\t', ',' and ':'. Whitespaces are removed
** and buffer _will_ be modified.
*/
std::wstring CMeasure::ExtractWord(std::wstring& buffer)
{
	std::wstring::size_type end, len;
	std::wstring ret;

	if (buffer.empty()) return ret;

	len = buffer.size();

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

		if (buffer[0] == L'\"' || buffer[0] == L'\'')
		{
			WCHAR quote = buffer[0];

			end = 1;	// Skip the '"'
			// Quotes around the word
			while (end < len && (buffer[end] != quote)) ++end;
			if (end == len) end = std::wstring::npos;

			if (end != std::wstring::npos)
			{
				ret = buffer.substr(1, end - 1);
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
				ret = buffer.substr(0, ++end);	// The separator is also returned!
			}
		}
	}

	buffer.erase(0, end);

	return ret;
}


/*
** PreUpdate
**
** The base implementation of the update method. This includes the code
** that is common for all measures. This is called every time the measure
** is updated. The inherited classes must call the base implementation if
** they overwrite this method. If this method returns false, the update
** needs not to be done.
**
*/
bool CMeasure::PreUpdate()
{
	if (IsDisabled())
	{
		m_Value = 0.0;	// Disable measures return 0 as value
		return false;
	}

	// Only update the counter if the divider
	++m_UpdateCounter;
	if (m_UpdateCounter < m_UpdateDivider) return false;
	m_UpdateCounter = 0;

	// If we're logging the maximum value of the measure, check if
	// the new value is greater than the old one, and update if necessary.
	if (m_LogMaxValue)
	{
		if (m_MedianMaxValues.empty())
		{
			m_MedianMaxValues.resize(MEDIAN_SIZE, 0);
			m_MedianMinValues.resize(MEDIAN_SIZE, 0);
		}

		m_MedianMaxValues[m_MedianPos] = m_Value;
		m_MedianMinValues[m_MedianPos] = m_Value;
		++m_MedianPos;
		m_MedianPos %= MEDIAN_SIZE;

		std::vector<double> medianArray;

		medianArray = m_MedianMaxValues;
		std::sort(medianArray.begin(), medianArray.end());
		m_MaxValue = max(m_MaxValue, medianArray[MEDIAN_SIZE / 2]);

		medianArray = m_MedianMinValues;
		std::sort(medianArray.begin(), medianArray.end());
		m_MinValue = min(m_MinValue, medianArray[MEDIAN_SIZE / 2]);
	}

	if (m_MeterWindow)
	{
		// Check the IfEqualValue
		if (!m_IfEqualAction.empty())
		{
			if ((int)m_Value == (int)m_IfEqualValue)
			{
				if (!m_IfEqualCommited)
				{
					m_IfEqualCommited = true;  // To avoid crashing by !RainmeterUpdate due to infinite loop
					Rainmeter->ExecuteCommand(m_IfEqualAction.c_str(), m_MeterWindow);
				}
			}
			else
			{
				m_IfEqualCommited = false;
			}
		}

		// Check the IfAboveValue
		if (!m_IfAboveAction.empty())
		{
			if (m_Value > m_IfAboveValue)
			{
				if (!m_IfAboveCommited)
				{
					m_IfAboveCommited = true;  // To avoid crashing by !RainmeterUpdate due to infinite loop
					Rainmeter->ExecuteCommand(m_IfAboveAction.c_str(), m_MeterWindow);
				}
			}
			else
			{
				m_IfAboveCommited = false;
			}
		}

		// Check the IfBelowValue
		if (!m_IfBelowAction.empty())
		{
			if (m_Value < m_IfBelowValue)
			{
				if (!m_IfBelowCommited)
				{
					m_IfBelowCommited = true;  // To avoid crashing by !RainmeterUpdate due to infinite loop
					Rainmeter->ExecuteCommand(m_IfBelowAction.c_str(), m_MeterWindow);
				}
			}
			else
			{
				m_IfBelowCommited = false;
			}
		}
	}

	return true;
}

/*
** PostUpdate
**
** Does post measuring things to the value. All measures must call this
** after they have set the m_Value.
**
*/
bool CMeasure::PostUpdate()
{
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
		m_Value = 0;
		for (size_t i = 0; i < averageValuesSize; ++i)
		{
			m_Value += m_AverageValues[i];
		}
		m_Value /= (double)averageValuesSize;
	}
	return true;
}

/*
** GetValue
**
** Returns the value of the measure.
**
*/
double CMeasure::GetValue()
{
	// Invert if so requested
	if (m_Invert)
	{
		return m_MaxValue - m_Value + m_MinValue;
	}

	return m_Value;
}

/*
** GetRelativeValue
**
** Returns the relative value of the measure (0.0 - 1.0).
**
*/
double CMeasure::GetRelativeValue()
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
** GetValueRange
**
** Returns the value range.
**
*/
double CMeasure::GetValueRange()
{
	return m_MaxValue - m_MinValue;
}

/*
** GetStringValue
**
** This method returns the value as text string. The actual value is
** get with GetValue() so we don't have to worry about m_Invert.
**
** autoScale  If true, scale the value automatically to some sensible range.
** scale      The scale to use if autoScale is false.
** decimals   Number of decimals used in the value. If -1, get rid of ".00000" for dynamic variables.
** percentual Return the value as % from the maximum value.
*/
const WCHAR* CMeasure::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	static WCHAR buffer[MAX_LINE_LENGTH];
	WCHAR format[32];

	if (percentual)
	{
		double val = 100.0 * GetRelativeValue();

		if (decimals == 0)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%i", (int)val);
		}
		else
		{
			_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
			_snwprintf_s(buffer, _TRUNCATE, format, val);
		}
	}
	else if (autoScale != AUTOSCALE_OFF)
	{
		GetScaledValue(autoScale, decimals, GetValue(), buffer, _countof(buffer));
	}
	else
	{
		double val = GetValue() / scale;

		if (decimals == 0)
		{
			val += (val >= 0) ? 0.5 : -0.5;
			_snwprintf_s(buffer, _TRUNCATE, L"%lli", (LONGLONG)val);
		}
		else if (decimals == -1)
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

void CMeasure::GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords)
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

	int index = (autoScale == AUTOSCALE_1000 || autoScale == AUTOSCALE_1000K) ? AUTOSCALE_INDEX_1000 : AUTOSCALE_INDEX_1024;

	if (theValue > (g_TblScale[index][0] * 0.99))
	{
		wcsncat_s(format, L" T", _TRUNCATE);
		value = theValue / g_TblScale[index][0];
	}
	else if (theValue > (g_TblScale[index][1] * 0.99))
	{
		wcsncat_s(format, L" G", _TRUNCATE);
		value = theValue / g_TblScale[index][1];
	}
	else if (theValue > (g_TblScale[index][2] * 0.99))
	{
		wcsncat_s(format, L" M", _TRUNCATE);
		value = theValue / g_TblScale[index][2];
	}
	else if (autoScale == AUTOSCALE_1024K || autoScale == AUTOSCALE_1000K || theValue > (g_TblScale[index][3] * 0.99))
	{
		wcsncat_s(format, L" k", _TRUNCATE);
		value = theValue / g_TblScale[index][3];
	}
	else
	{
		wcsncat_s(format, L" ", _TRUNCATE);
		value = theValue;
	}
	_snwprintf_s(buffer, sizeInWords, _TRUNCATE, format, value);
}

void CMeasure::RemoveTrailingZero(WCHAR* str, int strLen)
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
** GetStats
**
** Returns the stats as string. The stats are shown in the About dialog.
*/
const WCHAR* CMeasure::GetStats()
{
	static std::wstring value;

	value = GetStringValue(AUTOSCALE_ON, 1, 1, false);

	return value.c_str();
}

/*
** Create
**
** Creates the given measure. This is the factory method for the measures.
** If new measures are implemented this method needs to be updated.
**
*/
CMeasure* CMeasure::Create(const WCHAR* measure, CMeterWindow* meterWindow, const WCHAR* name)
{
	// Comparison is caseinsensitive

	if (*measure == L'\0')
	{
		return NULL;
	}
	else if (_wcsicmp(L"CPU", measure) == 0)
	{
		return new CMeasureCPU(meterWindow, name);
	}
	else if (_wcsicmp(L"Memory", measure) == 0)
	{
		return new CMeasureMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"NetIn", measure) == 0)
	{
		return new CMeasureNetIn(meterWindow, name);
	}
	else if (_wcsicmp(L"NetOut", measure) == 0)
	{
		return new CMeasureNetOut(meterWindow, name);
	}
	else if (_wcsicmp(L"NetTotal", measure) == 0)
	{
		return new CMeasureNetTotal(meterWindow, name);
	}
	else if (_wcsicmp(L"PhysicalMemory", measure) == 0)
	{
		return new CMeasurePhysicalMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"SwapMemory", measure) == 0)
	{
		return new CMeasureVirtualMemory(meterWindow, name);
	}
	else if (_wcsicmp(L"FreeDiskSpace", measure) == 0)
	{
		return new CMeasureDiskSpace(meterWindow, name);
	}
	else if (_wcsicmp(L"Uptime", measure) == 0)
	{
		return new CMeasureUptime(meterWindow, name);
	}
	else if (_wcsicmp(L"Time", measure) == 0)
	{
		return new CMeasureTime(meterWindow, name);
	}
	else if (_wcsicmp(L"Plugin", measure) == 0)
	{
		return new CMeasurePlugin(meterWindow, name);
	}
	else if (_wcsicmp(L"Registry", measure) == 0)
	{
		return new CMeasureRegistry(meterWindow, name);
	}
	else if (_wcsicmp(L"Calc", measure) == 0)
	{
		return new CMeasureCalc(meterWindow, name);
	}
	else if (_wcsicmp(L"script", measure) == 0)
	{
		return new CMeasureScript(meterWindow, name);
	}

	// Error
	std::wstring error = L"Measure=";
	error += measure;
	error += L" is not valid in section [";
	error += name;
	error += L"].";
	throw CError(error, __LINE__, __FILE__);

	return NULL;
}

/*
** ExecuteBang
**
** Executes a custom bang
*/
void CMeasure::ExecuteBang(const WCHAR* args)
{
	LogWithArgs(LOG_WARNING, L"[%s] doesn't support this bang: %s", m_Name.c_str(), args);
}
