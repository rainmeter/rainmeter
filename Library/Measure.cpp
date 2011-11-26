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

const int MEDIAN_SIZE = 7;

extern CRainmeter* Rainmeter;

/*
** CMeasure
**
** The constructor
**
*/
CMeasure::CMeasure(CMeterWindow* meterWindow, const WCHAR* name) : m_MeterWindow(meterWindow), m_Name(name), m_AsciiName(ConvertToAscii(name)),
	m_DynamicVariables(false),
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
			m_Disabled = 0!=parser.ParseInt(result.c_str(), 0);
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

	m_RegExpSubstitute = 0!=parser.ReadInt(section, L"RegExpSubstitute", 0);
	std::wstring subs = parser.ReadString(section, L"Substitute", L"");
	if (!subs.empty())
	{
		if ((subs[0] != L'\"' || subs[subs.length() - 1] != L'\'') &&
			(subs[0] != L'\'' || subs[subs.length() - 1] != L'\"'))
		{
			// Add quotes since they are removed by the GetProfileString
			subs.insert(0, L"\"");
			subs.append(L"\"");
		}
		if (!ParseSubstitute(subs))
		{
			LogWithArgs(LOG_ERROR, L"Measure: Invalid Substitute=%s", subs.c_str());
		}
	}

	const std::wstring& group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);
}

/*
** MakePlainSubstitute
**
** Substitues text using a straight find and replace method
*/
bool CMeasure::MakePlainSubstitute(std::wstring& str, size_t index)
{
	size_t start = 0;
	size_t pos = std::wstring::npos;

	do
	{
		pos = str.find(m_Substitute[index].first, start);
		if (pos != std::wstring::npos)
		{
			str.replace(pos, m_Substitute[index].first.length(), m_Substitute[index].second);
			start = pos + m_Substitute[index].second.length();
		}
	}
	while (pos != std::wstring::npos);

	return true;
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
		if (!m_RegExpSubstitute)	// Plain Substitutions only
		{
			str = buffer;

			for (size_t i = 0, isize = m_Substitute.size(); i < isize; ++i)
			{
				if (!m_Substitute[i].first.empty())
				{
					MakePlainSubstitute(str, i);
				}
				else if (str.empty())
				{
					// Empty result and empty substitute -> use second
					str = m_Substitute[i].second;
				}
			}
		}
		else // Contains a RegEx
		{
			std::string utf8str = ConvertToUTF8(buffer);
			int* ovector = new int[OVECCOUNT];

			for (size_t i = 0, isize = m_Substitute.size() ; i < isize ; ++i)
			{
				pcre* re;
				const char* error;
				int erroffset;
				int rc;
				int flags = PCRE_UTF8;
				int offset = 0;

				re = pcre_compile(
					ConvertToUTF8(m_Substitute[i].first.c_str()).c_str(),   // the pattern
					flags,						// default options
					&error,						// for error message
					&erroffset,					// for error offset
					NULL);						// use default character tables

				if (re == NULL)
				{
					MakePlainSubstitute(str, i);
					Log(LOG_NOTICE, ConvertToWide(error).c_str());
				}
				else
				{
					do
					{
						rc = pcre_exec(
							re,						// the compiled pattern
							NULL,					// no extra data - we didn't study the pattern
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
							std::string result = ConvertToUTF8(m_Substitute[i].second.c_str());

							if (rc > 1)
							{
								for (int j = rc - 1 ; j >= 0 ; --j)
								{
									size_t new_start = ovector[2*j];
									size_t in_length = ovector[2*j+1] - ovector[2*j];

									char tmpName[64];
									_snprintf_s(tmpName, _TRUNCATE, "\\%i", j);

									size_t cut_length = strlen(tmpName);
									size_t pos = result.find(tmpName);
									while (pos != std::string::npos)
									{
										result.replace(pos, cut_length, utf8str, new_start, in_length);
										pos = result.find(tmpName, pos + in_length);
									}
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

			str = ConvertUTF8ToWide(utf8str.c_str());
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

	do
	{
		std::wstring word1 = ExtractWord(buffer);
		std::wstring sep = ExtractWord(buffer);
		if (sep.size() != 1 || sep[0] != L':') return false;
		std::wstring word2 = ExtractWord(buffer);

		if (wcscmp(word1.c_str(), word2.c_str()) != 0)
		{
			m_Substitute.push_back(std::pair<std::wstring, std::wstring>(word1, word2));
		}

		std::wstring sep2 = ExtractWord(buffer);
		if (!sep2.empty() && (sep2.size() != 1 || sep2[0] != L',')) return false;
	}
	while (!buffer.empty());

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

		if (buffer[0] == L'\"' || buffer[0] == L'\'')
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
			_itow_s((int)val, buffer, 10);
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
	return GetStringValue(AUTOSCALE_OFF, 1, -1, false);
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

	if (_wcsicmp(L"CPU", measure) == 0)
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
	else if (_wcsicmp(L"Script", measure) == 0)
	{
		return new CMeasureScript(meterWindow, name);
	}

	// Error
	std::wstring error = L"Measure=";
	error += measure;
	error += L" is not valid in [";
	error += name;
	error += L"]";
	throw CError(error);

	return NULL;
}

/*
** ExecuteBang
**
** Executes a custom bang
*/
void CMeasure::ExecuteBang(const WCHAR* args)
{
	LogWithArgs(LOG_WARNING, L"!CommandMeasure: Not supported by [%s]", m_Name.c_str());
}
