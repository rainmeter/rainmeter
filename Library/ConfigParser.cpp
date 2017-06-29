/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MathParser.h"
#include "../Common/PathUtil.h"
#include "ConfigParser.h"
#include "Util.h"
#include "Rainmeter.h"
#include "System.h"
#include "Measure.h"
#include "MeasureTime.h"
#include "Meter.h"
#include "resource.h"

using namespace Gdiplus;

namespace {

struct PairInfo
{
	const WCHAR begin;
	const WCHAR end;
};

const std::unordered_map<PairedPunctuation, PairInfo> s_PairedPunct =
{
	{ PairedPunctuation::SingleQuote, { L'\'', L'\'' } },
	{ PairedPunctuation::DoubleQuote, { L'\"', L'\"' } },
	{ PairedPunctuation::Parentheses, { L'(', L')' } },
	{ PairedPunctuation::Brackets,    { L'[', L']' } },
	{ PairedPunctuation::Braces,      { L'{', L'}' } },
	{ PairedPunctuation::Guillemet,   { L'<', L'>' } }
};

}  // namespace

std::unordered_map<std::wstring, std::wstring> ConfigParser::c_MonitorVariables;

ConfigParser::ConfigParser() :
	m_LastReplaced(false),
	m_LastDefaultUsed(false),
	m_LastValueDefined(false),
	m_CurrentSection(),
	m_Skin()
{
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::Initialize(const std::wstring& filename, Skin* skin, LPCTSTR skinSection, const std::wstring* resourcePath)
{
	m_Skin = skin;

	m_Measures.clear();
	m_Sections.clear();
	m_Values.clear();
	m_BuiltInVariables.clear();
	m_Variables.clear();

	m_StyleTemplate.clear();
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	m_CurrentSection = nullptr;
	m_SectionInsertPos = m_Sections.end();

	// Set the built-in variables. Do this before the ini file is read so that the paths can be used with @include
	SetBuiltInVariables(filename, resourcePath, skin);
	ResetMonitorVariables(skin);

	System::UpdateIniFileMappingList();

	ReadIniFile(filename, skinSection);
	ReadVariables();

	// Clear and minimize
	m_FoundSections.clear();
	m_ListVariables.clear();
	m_SectionInsertPos = m_Sections.end();
}

void ConfigParser::SetBuiltInVariables(const std::wstring& filename, const std::wstring* resourcePath, Skin* skin)
{
	auto insertVariable = [&](const WCHAR* name, std::wstring value)
	{
		return m_BuiltInVariables.insert(std::make_pair(name, value));
	};

	insertVariable(L"PROGRAMPATH", GetRainmeter().GetPath());
	insertVariable(L"PROGRAMDRIVE", GetRainmeter().GetDrive());
	insertVariable(L"SETTINGSPATH", GetRainmeter().GetSettingsPath());
	insertVariable(L"SKINSPATH", GetRainmeter().GetSkinPath());
	insertVariable(L"PLUGINSPATH", GetRainmeter().GetPluginPath());
	insertVariable(L"CURRENTPATH", PathUtil::GetFolderFromFilePath(filename));
	insertVariable(L"ADDONSPATH", GetRainmeter().GetAddonPath());

	insertVariable(L"CONFIGEDITOR", GetRainmeter().GetSkinEditor());

	if (skin)
	{
		insertVariable(L"CURRENTFILE", skin->GetFileName());
		insertVariable(L"CURRENTCONFIG", skin->GetFolderPath());
		insertVariable(L"ROOTCONFIG", skin->GetRootName());
		insertVariable(L"ROOTCONFIGPATH", skin->GetRootPath());
	}

	insertVariable(L"CRLF", L"\n");

	m_CurrentSection = &(insertVariable(L"CURRENTSECTION", L"").first->second);	// shortcut

	if (resourcePath)
	{
		SetVariable(L"@", *resourcePath);
	}
}

/*
** Sets all user-defined variables.
**
*/
void ConfigParser::ReadVariables()
{
	std::list<std::wstring>::const_iterator iter = m_ListVariables.begin();
	for ( ; iter != m_ListVariables.end(); ++iter)
	{
		SetVariable((*iter), ReadString(L"Variables", (*iter).c_str(), L"", false));
	}
}

void ConfigParser::SetVariable(std::wstring strVariable, const std::wstring& strValue)
{
	StrToUpperC(strVariable);
	m_Variables[strVariable] = strValue;
}

void ConfigParser::SetBuiltInVariable(const std::wstring& strVariable, const std::wstring& strValue)
{
	m_BuiltInVariables[strVariable] = strValue;
}

/*
** Gets a value for the variable. Returns nullptr if not found.
**
*/
const std::wstring* ConfigParser::GetVariable(const std::wstring& strVariable)
{
	const std::wstring strTmp = StrToUpper(strVariable);

	// #1: Built-in variables
	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_BuiltInVariables.find(strTmp);
	if (iter != m_BuiltInVariables.end())
	{
		return &(*iter).second;
	}

	// #2: Monitor variables
	iter = c_MonitorVariables.find(strTmp);
	if (iter != c_MonitorVariables.end())
	{
		return &(*iter).second;
	}

	// #3: User-defined variables
	iter = m_Variables.find(strTmp);
	if (iter != m_Variables.end())
	{
		return &(*iter).second;
	}

	return nullptr;
}

/*
** Gets the value of a section variable. Returns true if strValue is set.
** The selector is stripped from strVariable.
**
*/
bool ConfigParser::GetSectionVariable(std::wstring& strVariable, std::wstring& strValue)
{
	size_t colonPos = strVariable.find_last_of(L':');
	if (colonPos == std::wstring::npos)
	{
		return false;
	}

	const std::wstring selector = strVariable.substr(colonPos + 1);
	const WCHAR* selectorSz = selector.c_str();
	strVariable.resize(colonPos);

	bool isKeySelector = (!selector.empty() && iswalpha(selectorSz[0]));

	if (isKeySelector)
	{
		// [Meter:X], [Meter:Y], [Meter:W], [Meter:H]
		Meter* meter = m_Skin->GetMeter(strVariable);
		if (meter)
		{
			WCHAR buffer[32];
			if (_wcsicmp(selectorSz, L"X") == 0)
			{
				_itow_s(meter->GetX(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"Y") == 0)
			{
				_itow_s(meter->GetY(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"W") == 0)
			{
				_itow_s(meter->GetW(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"H") == 0)
			{
				_itow_s(meter->GetH(), buffer, 10);
			}
			else
			{
				return false;
			}

			strValue = buffer;
			return true;
		}
	}

	// Number: [Measure:], [Measure:dec]
	// Percentual: [Measure:%], [Measure:%, dec]
	// Scale: [Measure:/scale], [Measure:/scale, dec]
	// Max/Min: [Measure:MaxValue], [Measure:MaxValue:/scale, dec] ('%' cannot be used)
	// EscapeRegExp: [Measure:EscapeRegExp] (Escapes regular expression syntax, used for 'IfMatch')
	// EncodeUrl: [Measure:EncodeUrl] (Escapes URL reserved characters)
	// TimeStamp: [TimeMeasure:TimeStamp] (ONLY for Time measures, returns the Windows timestamp of the measure)
	enum class ValueType
	{
		Raw,
		Percentual,
		Max,
		Min,
		EscapeRegExp,
		EncodeUrl,
		TimeStamp
	} valueType = ValueType::Raw;

	if (isKeySelector)
	{
		if (_wcsicmp(selectorSz, L"MaxValue") == 0)
		{
			valueType = ValueType::Max;
		}
		else if (_wcsicmp(selectorSz, L"MinValue") == 0)
		{
			valueType = ValueType::Min;
		}
		else if (_wcsicmp(selectorSz, L"EscapeRegExp") == 0)
		{
			valueType = ValueType::EscapeRegExp;
		}
		else if (_wcsicmp(selectorSz, L"EncodeUrl") == 0)
		{
			valueType = ValueType::EncodeUrl;
		}
		else if (_wcsicmp(selectorSz, L"TimeStamp") == 0)
		{
			valueType = ValueType::TimeStamp;
		}
		else
		{
			return false;
		}

		selectorSz = L"";
	}
	else
	{
		colonPos = strVariable.find_last_of(L':');
		if (colonPos != std::wstring::npos)
		{
			do
			{
				const WCHAR* keySelectorSz = strVariable.c_str() + colonPos + 1;

				if (_wcsicmp(keySelectorSz, L"MaxValue") == 0)
				{
					valueType = ValueType::Max;
				}
				else if (_wcsicmp(keySelectorSz, L"MinValue") == 0)
				{
					valueType = ValueType::Min;
				}
				else
				{
					// Section name contains ':' ?
					break;
				}

				strVariable.resize(colonPos);
			}
			while (0);
		}
	}

	Measure* measure = m_Skin->GetMeasure(strVariable);
	if (measure)
	{
		if (valueType == ValueType::EscapeRegExp)
		{
			strValue = measure->GetStringValue();
			StringUtil::EscapeRegExp(strValue);
			return true;
		}
		else if (valueType == ValueType::EncodeUrl)
		{
			strValue = measure->GetStringValue();
			StringUtil::EncodeUrl(strValue);
			return true;
		}
		else if (measure->GetTypeID() == TypeID<MeasureTime>() && valueType == ValueType::TimeStamp)
		{
			MeasureTime* time = (MeasureTime*)measure;
			strValue = std::to_wstring(time->GetTimeStamp().QuadPart / 10000000);
			return true;
		}

		int scale = 1;

		const WCHAR* decimalsSz = wcschr(selectorSz, L',');
		if (decimalsSz)
		{
			++decimalsSz;
		}

		if (*selectorSz == L'%')  // Percentual
		{
			if (valueType == ValueType::Max || valueType == ValueType::Min)
			{
				// '%' cannot be used with Max/Min values.
				return false;
			}

			valueType = ValueType::Percentual;
		}
		else if (*selectorSz == L'/')  // Scale
		{
			errno = 0;
			scale = _wtoi(selectorSz + 1);
			if (errno == EINVAL || scale == 0)
			{
				// Invalid scale value.
				return false;
			}
		}
		else
		{
			if (decimalsSz)
			{
				return false;
			}

			decimalsSz = selectorSz;
		}

		const double value =
			(valueType == ValueType::Percentual) ? measure->GetRelativeValue() * 100.0 :
			(valueType == ValueType::Max)        ? measure->GetMaxValue() / scale :
			(valueType == ValueType::Min)        ? measure->GetMinValue() / scale :
			                                       measure->GetValue() / scale;
		int decimals = 10;
		if (decimalsSz)
		{
			while (iswspace(*decimalsSz)) ++decimalsSz;

			if (*decimalsSz)
			{
				decimals = _wtoi(decimalsSz);
				decimals = max(0, decimals);
				decimals = min(32, decimals);
			}
			else
			{
				decimalsSz = nullptr;
			}
		}

		WCHAR format[32];
		WCHAR buffer[128];
		_snwprintf_s(format, _TRUNCATE, L"%%.%if", decimals);
		int bufferLen = _snwprintf_s(buffer, _TRUNCATE, format, value);
			
		if (!decimalsSz)
		{
			// Remove trailing zeros if decimal count was not specified.
			measure->RemoveTrailingZero(buffer, bufferLen);
			bufferLen = (int)wcslen(buffer);
		}

		strValue.assign(buffer, bufferLen);
		return true;
	}
	
	return false;
}

void ConfigParser::ResetMonitorVariables(Skin* skin)
{
	// Set the SCREENAREA/WORKAREA variables
	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Set the SCREENAREA/WORKAREA variables for present monitor
	SetAutoSelectedMonitorVariables(skin);
}

/*
** Sets new values for the SCREENAREA/WORKAREA variables.
**
*/
void ConfigParser::SetMultiMonitorVariables(bool reset)
{
	auto setMonitorVariable = [&](const WCHAR* variable, const WCHAR* value)
	{
		c_MonitorVariables[variable] = value;
	};

	if (!reset && c_MonitorVariables.empty())
	{
		reset = true;  // Set all variables
	}

	const size_t numOfMonitors = System::GetMonitorCount();  // intentional
	const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
	const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

	WCHAR buffer[32];
	const RECT workArea = monitors[monitorsInfo.primary - 1].work;
	const RECT scrArea = monitors[monitorsInfo.primary - 1].screen;

	_itow_s(workArea.left, buffer, 10);
	setMonitorVariable(L"WORKAREAX", buffer);
	setMonitorVariable(L"PWORKAREAX", buffer);
	_itow_s(workArea.top, buffer, 10);
	setMonitorVariable(L"WORKAREAY", buffer);
	setMonitorVariable(L"PWORKAREAY", buffer);
	_itow_s(workArea.right - workArea.left, buffer, 10);
	setMonitorVariable(L"WORKAREAWIDTH", buffer);
	setMonitorVariable(L"PWORKAREAWIDTH", buffer);
	_itow_s(workArea.bottom - workArea.top, buffer, 10);
	setMonitorVariable(L"WORKAREAHEIGHT", buffer);
	setMonitorVariable(L"PWORKAREAHEIGHT", buffer);

	if (reset)
	{
		_itow_s(scrArea.left, buffer, 10);
		setMonitorVariable(L"SCREENAREAX", buffer);
		setMonitorVariable(L"PSCREENAREAX", buffer);
		_itow_s(scrArea.top, buffer, 10);
		setMonitorVariable(L"SCREENAREAY", buffer);
		setMonitorVariable(L"PSCREENAREAY", buffer);
		_itow_s(scrArea.right - scrArea.left, buffer, 10);
		setMonitorVariable(L"SCREENAREAWIDTH", buffer);
		setMonitorVariable(L"PSCREENAREAWIDTH", buffer);
		_itow_s(scrArea.bottom - scrArea.top, buffer, 10);
		setMonitorVariable(L"SCREENAREAHEIGHT", buffer);
		setMonitorVariable(L"PSCREENAREAHEIGHT", buffer);

		_itow_s(monitorsInfo.vsL, buffer, 10);
		setMonitorVariable(L"VSCREENAREAX", buffer);
		_itow_s(monitorsInfo.vsT, buffer, 10);
		setMonitorVariable(L"VSCREENAREAY", buffer);
		_itow_s(monitorsInfo.vsW, buffer, 10);
		setMonitorVariable(L"VSCREENAREAWIDTH", buffer);
		_itow_s(monitorsInfo.vsH, buffer, 10);
		setMonitorVariable(L"VSCREENAREAHEIGHT", buffer);
	}

	int i = 1;
	for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++i)
	{
		WCHAR buffer2[64];

		const RECT work = ((*iter).active) ? (*iter).work : workArea;

		_itow_s(work.left, buffer, 10);
		_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAX@%i", i);
		setMonitorVariable(buffer2, buffer);
		_itow_s(work.top, buffer, 10);
		_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAY@%i", i);
		setMonitorVariable(buffer2, buffer);
		_itow_s(work.right - work.left, buffer, 10);
		_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAWIDTH@%i", i);
		setMonitorVariable(buffer2, buffer);
		_itow_s(work.bottom - work.top, buffer, 10);
		_snwprintf_s(buffer2, _TRUNCATE, L"WORKAREAHEIGHT@%i", i);
		setMonitorVariable(buffer2, buffer);

		if (reset)
		{
			const RECT screen = ((*iter).active) ? (*iter).screen : scrArea;

			_itow_s(screen.left, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAX@%i", i);
			setMonitorVariable(buffer2, buffer);
			_itow_s(screen.top, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAY@%i", i);
			setMonitorVariable(buffer2, buffer);
			_itow_s(screen.right - screen.left, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAWIDTH@%i", i);
			setMonitorVariable(buffer2, buffer);
			_itow_s(screen.bottom - screen.top, buffer, 10);
			_snwprintf_s(buffer2, _TRUNCATE, L"SCREENAREAHEIGHT@%i", i);
			setMonitorVariable(buffer2, buffer);
		}
	}
}

/*
** Sets new SCREENAREA/WORKAREA variables for present monitor.
**
*/
void ConfigParser::SetAutoSelectedMonitorVariables(Skin* skin)
{
	if (skin)
	{
		const int numOfMonitors = (int)System::GetMonitorCount();
		const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

		WCHAR buffer[32];
		int w1, w2, s1, s2;
		int screenIndex;

		// Set X / WIDTH
		screenIndex = monitorsInfo.primary;
		if (skin->GetXScreenDefined())
		{
			int i = skin->GetXScreen();
			if (i >= 0 && (i == 0 || i <= numOfMonitors && monitors[i - 1].active))
			{
				screenIndex = i;
			}
		}

		if (screenIndex == 0)
		{
			s1 = w1 = monitorsInfo.vsL;
			s2 = w2 = monitorsInfo.vsW;
		}
		else
		{
			w1 = monitors[screenIndex - 1].work.left;
			w2 = monitors[screenIndex - 1].work.right - monitors[screenIndex - 1].work.left;
			s1 = monitors[screenIndex - 1].screen.left;
			s2 = monitors[screenIndex - 1].screen.right - monitors[screenIndex - 1].screen.left;
		}

		_itow_s(w1, buffer, 10);
		SetBuiltInVariable(L"WORKAREAX", buffer);
		_itow_s(w2, buffer, 10);
		SetBuiltInVariable(L"WORKAREAWIDTH", buffer);
		_itow_s(s1, buffer, 10);
		SetBuiltInVariable(L"SCREENAREAX", buffer);
		_itow_s(s2, buffer, 10);
		SetBuiltInVariable(L"SCREENAREAWIDTH", buffer);

		// Set Y / HEIGHT
		screenIndex = monitorsInfo.primary;
		if (skin->GetYScreenDefined())
		{
			int i = skin->GetYScreen();
			if (i >= 0 && (i == 0 || i <= numOfMonitors && monitors[i - 1].active))
			{
				screenIndex = i;
			}
		}

		if (screenIndex == 0)
		{
			s1 = w1 = monitorsInfo.vsL;
			s2 = w2 = monitorsInfo.vsW;
		}
		else
		{
			w1 = monitors[screenIndex - 1].work.top;
			w2 = monitors[screenIndex - 1].work.bottom - monitors[screenIndex - 1].work.top;
			s1 = monitors[screenIndex - 1].screen.top;
			s2 = monitors[screenIndex - 1].screen.bottom - monitors[screenIndex - 1].screen.top;
		}

		_itow_s(w1, buffer, 10);
		SetBuiltInVariable(L"WORKAREAY", buffer);
		_itow_s(w2, buffer, 10);
		SetBuiltInVariable(L"WORKAREAHEIGHT", buffer);
		_itow_s(s1, buffer, 10);
		SetBuiltInVariable(L"SCREENAREAY", buffer);
		_itow_s(s2, buffer, 10);
		SetBuiltInVariable(L"SCREENAREAHEIGHT", buffer);
	}
}

/*
** Replaces environment and internal variables in the given string.
**
*/
bool ConfigParser::ReplaceVariables(std::wstring& result)
{
	bool replaced = false;

	PathUtil::ExpandEnvironmentVariables(result);

	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Check for variables (#VAR#)
	size_t start = 0, end;
	bool loop = true;

	do
	{
		start = result.find(L'#', start);
		if (start != std::wstring::npos)
		{
			size_t si = start + 1;
			end = result.find(L'#', si);
			if (end != std::wstring::npos)
			{
				size_t ei = end - 1;
				if (si != ei && result[si] == L'*' && result[ei] == L'*')
				{
					result.erase(ei, 1);
					result.erase(si, 1);
					start = ei;
				}
				else
				{
					std::wstring strVariable = result.substr(si, end - si);
					const std::wstring* value = GetVariable(strVariable);
					if (value)
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1, *value);
						start += (*value).length();
						replaced = true;
					}
					else
					{
						start = end;
					}
				}
			}
			else
			{
				loop = false;
			}
		}
		else
		{
			loop = false;
		}
	}
	while (loop);

	return replaced;
}

/*
** Replaces measures in the given string.
**
*/
bool ConfigParser::ReplaceMeasures(std::wstring& result)
{
	bool replaced = false;

	size_t start = 0;
	while ((start = result.find(L'[', start)) != std::wstring::npos)
	{
		size_t si = start + 1;
		size_t end = result.find(L']', si);
		if (end == std::wstring::npos)
		{
			break;
		}

		size_t next = result.find(L'[', si);
		if (next == std::wstring::npos || end < next)
		{
			size_t ei = end - 1;
			if (si != ei && result[si] == L'*' && result[ei] == L'*')
			{
				result.erase(ei, 1);
				result.erase(si, 1);
				start = ei;
			}
			else
			{
				std::wstring var = result.substr(si, end - si);

				Measure* measure = GetMeasure(var);
				if (measure)
				{
					const WCHAR* value = measure->GetStringOrFormattedValue(AUTOSCALE_OFF, 1, -1, false);
					size_t valueLen = wcslen(value);

					// Measure found, replace it with the value
					result.replace(start, end - start + 1, value, valueLen);
					start += valueLen;
					replaced = true;
				}
				else
				{
					std::wstring value;
					if (GetSectionVariable(var, value))
					{
						// Replace section variable with the value.
						result.replace(start, end - start + 1, value);
						start += value.length();
						replaced = true;
					}
					else
					{
						start = end;
					}
				}
			}
		}
		else
		{
			start = next;
		}
	}
	
	return replaced;
}

const std::wstring& ConfigParser::ReadString(LPCTSTR section, LPCTSTR key, LPCTSTR defValue, bool bReplaceMeasures)
{
	static std::wstring result;

	// Clear last status
	m_LastReplaced = false;
	m_LastDefaultUsed = false;
	m_LastValueDefined = false;

	const std::wstring strSection = section;
	const std::wstring strKey = key;
	const std::wstring strDefault = defValue;

	const std::wstring& strValue = GetValue(strSection, strKey, strDefault);
	if (&strValue == &strDefault)
	{
		bool foundStyleValue = false;

		// If the template is defined read the value from there.
		std::vector<std::wstring>::const_reverse_iterator iter = m_StyleTemplate.rbegin();
		for ( ; iter != m_StyleTemplate.rend(); ++iter)
		{
			const std::wstring& strStyleValue = GetValue((*iter), strKey, strDefault);

			//LogDebugF(L"StyleTemplate: [%s] %s (from [%s]) : strDefault=%s (0x%p), strStyleValue=%s (0x%p)",
			//	section, key, (*iter).c_str(), strDefault.c_str(), &strDefault, strStyleValue.c_str(), &strStyleValue);

			if (&strStyleValue != &strDefault)
			{
				result = strStyleValue;
				foundStyleValue = true;
				break;
			}
		}

		if (!foundStyleValue)
		{
			result = strDefault;
			m_LastDefaultUsed = true;
			return result;
		}
	}
	else
	{
		result = strValue;
	}

	if (!result.empty())
	{
		m_LastValueDefined = true;

		if (result.size() >= 3)
		{
			if (result.find(L'#') != std::wstring::npos)
			{
				m_CurrentSection->assign(strSection);  // Set temporarily

				if (ReplaceVariables(result))
				{
					m_LastReplaced = true;
				}

				m_CurrentSection->clear();  // Reset
			}
			else
			{
				PathUtil::ExpandEnvironmentVariables(result);
			}

			if (bReplaceMeasures && ReplaceMeasures(result))
			{
				m_LastReplaced = true;
			}
		}
	}

	return result;
}

bool ConfigParser::IsKeyDefined(LPCTSTR section, LPCTSTR key)
{
	ReadString(section, key, L"", false);
	return !m_LastDefaultUsed;
}

bool ConfigParser::IsValueDefined(LPCTSTR section, LPCTSTR key)
{
	ReadString(section, key, L"", false);
	return m_LastValueDefined;
}

void ConfigParser::AddMeasure(Measure* pMeasure)
{
	if (pMeasure)
	{
		m_Measures[StrToUpper(pMeasure->GetOriginalName())] = pMeasure;
	}
}

Measure* ConfigParser::GetMeasure(const std::wstring& name)
{
	std::unordered_map<std::wstring, Measure*>::const_iterator iter = m_Measures.find(StrToUpper(name));
	if (iter != m_Measures.end())
	{
		return (*iter).second;
	}

	return nullptr;
}

std::vector<Gdiplus::REAL> ConfigParser::ReadFloats(LPCTSTR section, LPCTSTR key)
{
	std::vector<Gdiplus::REAL> result;
	const std::wstring& str = ReadString(section, key, L"");
	if (!str.empty())
	{
		// Tokenize and parse the floats
		const WCHAR delimiter = L';';
		size_t lastPos, pos = 0;
		do
		{
			lastPos = str.find_first_not_of(delimiter, pos);
			if (lastPos == std::wstring::npos) break;

			pos = str.find_first_of(delimiter, lastPos + 1);

			result.push_back((Gdiplus::REAL)ParseDouble(str.substr(lastPos, pos - lastPos).c_str(), 0.0));  // (pos != std::wstring::npos) ? pos - lastPos : pos
			if (pos == std::wstring::npos) break;

			++pos;
		}
		while (true);
	}
	return result;
}

int ConfigParser::ReadInt(LPCTSTR section, LPCTSTR key, int defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (int)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			int intValue = wcstol(string, nullptr, 10);
			if (errno != ERANGE)
			{
				return intValue;
			}
		}
	}

	return defValue;
}

uint32_t ConfigParser::ReadUInt(LPCTSTR section, LPCTSTR key, uint32_t defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (uint32_t)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			uint32_t uintValue = wcstoul(string, nullptr, 10);
			if (errno != ERANGE)
			{
				return uintValue;
			}
		}
	}

	return defValue;
}

uint64_t ConfigParser::ReadUInt64(LPCTSTR section, LPCTSTR key, uint64_t defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			double dblValue;
			const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
			if (!errMsg)
			{
				return (uint64_t)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			uint64_t uint64Value = _wcstoui64(string, nullptr, 10);
			if (errno != ERANGE)
			{
				return uint64Value;
			}
		}
	}

	return defValue;
}

double ConfigParser::ReadFloat(LPCTSTR section, LPCTSTR key, double defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	if (!m_LastDefaultUsed)
	{
		double value;
		const WCHAR* string = result.c_str();
		if (*string == L'(')
		{
			const WCHAR* errMsg = MathParser::CheckedParse(string, &value);
			if (!errMsg)
			{
				return value;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*string)
		{
			errno = 0;
			value = wcstod(string, nullptr);
			if (errno != ERANGE)
			{
				return value;
			}
		}
	}

	return defValue;
}

// Returns true if the formula was read successfully, false for failure.
bool ConfigParser::ParseFormula(const std::wstring& formula, double* resultValue)
{
	// Formulas must be surrounded by parenthesis
	if (!formula.empty() && formula[0] == L'(' && formula[formula.size() - 1] == L')')
	{
		const WCHAR* string = formula.c_str();
		const WCHAR* errMsg = MathParser::CheckedParse(string, resultValue);
		if (errMsg != nullptr)
		{
			LogErrorF(m_Skin, L"Formula: %s: %s", errMsg, string);
			return false;
		}

		return true;
	}

	return false;
}

ARGB ConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, ARGB defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseColor(result.c_str());
}

Rect ConfigParser::ReadRect(LPCTSTR section, LPCTSTR key, const Rect& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseRect(result.c_str());
}

RECT ConfigParser::ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	RECT r;
	if (m_LastDefaultUsed)
	{
		r = defValue;
	}
	else
	{
		r = ParseRECT(result.c_str());
	}
	return r;
}

/*
** Splits the string from the delimiters.
** Now trims empty element in vector and white-space in each string.
**
** Modified from http://www.digitalpeer.com/id/simple
*/
std::vector<std::wstring> ConfigParser::Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	size_t lastPos, pos = 0;
	do
	{
		lastPos = str.find_first_not_of(delimiters, pos);
		if (lastPos == std::wstring::npos) break;

		pos = str.find_first_of(delimiters, lastPos + 1);
		std::wstring token = str.substr(lastPos, pos - lastPos);  // len = (pos != std::wstring::npos) ? pos - lastPos : pos

		size_t pos2 = token.find_first_not_of(L" \t\r\n");
		if (pos2 != std::wstring::npos)
		{
			size_t lastPos2 = token.find_last_not_of(L" \t\r\n");
			if (pos2 != 0 || lastPos2 != (token.size() - 1))
			{
				// Trim white-space
				token.assign(token, pos2, lastPos2 - pos2 + 1);
			}
			tokens.push_back(token);
		}

		if (pos == std::wstring::npos) break;
		++pos;
	}
	while (true);

	return tokens;
}

/*
** Splits the string from a delimiter, but skips delimiters inside of the defined paired punctuation
**
*/
std::vector<std::wstring> ConfigParser::Tokenize2(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct)
{
	std::vector<std::wstring> tokens;
	
	int pairs = 0;
	size_t start = 0;
	size_t end = 0;

	auto getToken = [&]() -> void
	{
		start = str.find_first_not_of(L" \t\r\n", start); // skip any leading whitespace
		if (start <= end)
		{
			std::wstring temp = str.substr(start, end - start);
			temp.erase(temp.find_last_not_of(L" \t\r\n") + 1); // remove any trailing whitespace
			tokens.push_back(temp);
		}
	};

	for (auto& iter : str)
	{
		if (iter == s_PairedPunct.at(punct).begin) ++pairs;
		else if (iter == s_PairedPunct.at(punct).end) --pairs;
		else if (iter == delimiter && pairs == 0)
		{
			getToken();
			start = end + 1;  // skip delimiter
		}

		++end;
	}

	// Get last token
	getToken();

	return tokens;
}

/*
** Helper method that parses the floating-point value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
double ConfigParser::ParseDouble(LPCTSTR string, double defValue)
{
	assert(string);

	double value;
	if (*string == L'(')
	{
		const WCHAR* errMsg = MathParser::CheckedParse(string, &value);
		if (!errMsg)
		{
			return value;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		double value = wcstod(string, nullptr);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

/*
** Helper method that parses the integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
int ConfigParser::ParseInt(LPCTSTR string, int defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (int)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		int intValue = wcstol(string, nullptr, 10);
		if (errno != ERANGE)
		{
			return intValue;
		}
	}

	return defValue;
}

/*
** Helper method that parses the unsigned integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
uint32_t ConfigParser::ParseUInt(LPCTSTR string, uint32_t defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (uint32_t)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		uint32_t uintValue = wcstoul(string, nullptr, 10);
		if (errno != ERANGE)
		{
			return uintValue;
		}
	}

	return defValue;
}

/*
** Helper method that parses the 64bit unsigned integer value from the given string.
** If the given string is invalid format or causes overflow/underflow, returns given default value.
**
*/
uint64_t ConfigParser::ParseUInt64(LPCTSTR string, uint64_t defValue)
{
	assert(string);

	if (*string == L'(')
	{
		double dblValue;
		const WCHAR* errMsg = MathParser::CheckedParse(string, &dblValue);
		if (!errMsg)
		{
			return (uint64_t)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, string);
	}
	else if (*string)
	{
		errno = 0;
		uint64_t uint64Value = _wcstoui64(string, nullptr, 10);
		if (errno != ERANGE)
		{
			return uint64Value;
		}
	}

	return defValue;
}

/*
** Helper template that parses four comma separated values from the given string.
**
*/
template <typename T>
bool ParseInt4(LPCTSTR string, T& v1, T& v2, T& v3, T& v4)
{
	if (wcschr(string, L','))
	{
		std::wstring str = string;
		std::vector<T> tokens;
		size_t start = 0;
		size_t end = 0;
		int parens = 0;

		auto getToken = [&]() -> void
		{
			start = str.find_first_not_of(L" \t", start); // skip any leading whitespace
			if (start <= end)
			{
				tokens.push_back(ConfigParser::ParseInt(str.substr(start, end - start).c_str(), 0));
			}
		};

		for (auto iter : str)
		{
			switch (iter)
			{
			case '(': ++parens; break;
			case ')': --parens; break;
			case ',':
				{
					if (parens == 0)
					{
						getToken();
						start = end + 1; // skip comma
						break;
					}
					//else multi arg function ?
				}
			}
			++end;
		}

		// read last token
		getToken();

		size_t size = tokens.size();
		if (size > 0) v1 = tokens[0];
		if (size > 1) v2 = tokens[1];
		if (size > 2) v3 = tokens[2];
		if (size > 3) v4 = tokens[3];

		return true;
	}

	return false;
}

/*
** Helper method that parses the color values from the given string.
** The color can be supplied as three/four comma separated values or as one
** hex-value.
**
*/
ARGB ConfigParser::ParseColor(LPCTSTR string)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (!ParseInt4(string, R, G, B, A))
	{
		if (wcsncmp(string, L"0x", 2) == 0)
		{
			string += 2;  // skip prefix
		}

		size_t len = wcslen(string);
		if (len >= 8 && !iswspace(string[6]))
		{
			swscanf(string, L"%02x%02x%02x%02x", &R, &G, &B, &A);
		}
		else if (len >= 6)
		{
			swscanf(string, L"%02x%02x%02x", &R, &G, &B);
		}
	}

	return Color::MakeARGB(A, R, G, B);
}

/*
** Helper method that parses the Gdiplus::Rect values from the given string.
** The rect can be supplied as four comma separated values (X/Y/Width/Height).
**
*/
Rect ConfigParser::ParseRect(LPCTSTR string)
{
	Rect r;
	ParseInt4(string, r.X, r.Y, r.Width, r.Height);
	return r;
}

/*
** Helper method that parses the RECT values from the given string.
** The rect can be supplied as four comma separated values (left/top/right/bottom).
**
*/
RECT ConfigParser::ParseRECT(LPCTSTR string)
{
	RECT r = {0};
	ParseInt4(string, r.left, r.top, r.right, r.bottom);
	return r;
}

/*
** Reads the given ini file and fills the m_Values and m_Keys maps.
**
*/
void ConfigParser::ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection, int depth)
{
	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		GetRainmeter().ShowMessage(nullptr, GetString(ID_STR_INCLUDEINFINITELOOP), MB_OK | MB_ICONERROR);
		return;
	}

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		LogErrorF(m_Skin, L"Unable to read file: %s", iniFile.c_str());
		return;
	}

	// Avoid "IniFileMapping"
	std::wstring iniRead = System::GetTemporaryFile(iniFile);
	bool temporary = (!iniRead.empty() && (iniRead.size() != 1 || iniRead[0] != L'?'));

	if (temporary)
	{
		if (GetRainmeter().GetDebug()) LogDebugF(m_Skin, L"Reading file: %s (Temp: %s)", iniFile.c_str(), iniRead.c_str());
	}
	else
	{
		if (GetRainmeter().GetDebug()) LogDebugF(m_Skin, L"Reading file: %s", iniFile.c_str());
		iniRead = iniFile;
	}

	// Get all the sections (i.e. different meters)
	std::list<std::wstring> sections;
	std::unordered_set<std::wstring> unique;
	std::wstring key, value;  // buffer

	DWORD itemsSize = MAX_LINE_LENGTH;
	WCHAR* items = new WCHAR[itemsSize];
	WCHAR* pos = nullptr;
	WCHAR* epos = nullptr;

	if (skinSection == nullptr)
	{
		// Get all the sections
		do
		{
			items[0] = 0;
			DWORD res = GetPrivateProfileSectionNames(items, itemsSize, iniRead.c_str());
			if (res == 0)		// File not found
			{
				delete [] items;
				if (temporary) System::RemoveFile(iniRead);
				return;
			}
			if (res < itemsSize - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			itemsSize *= 2;
			items = new WCHAR[itemsSize];
		}
		while (true);

		// Read the sections
		pos = items;
		while (pos < epos)
		{
			if (*pos)
			{
				value = pos;  // section name
				StrToUpperC(key.assign(value));
				if (unique.insert(key).second)
				{
					if (m_FoundSections.insert(key).second)
					{
						m_Sections.insert(m_SectionInsertPos, value);
					}
					sections.push_back(value);
				}
				pos += value.size() + 1;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}
	else
	{
		// Special case: Read only "Rainmeter" and specified section from "Rainmeter.ini"
		const std::wstring strRainmeter = L"Rainmeter";
		const std::wstring strFolder = skinSection;

		sections.push_back(strRainmeter);
		sections.push_back(strFolder);

		if (depth == 0)  // Add once
		{
			m_Sections.push_back(strRainmeter);
			m_Sections.push_back(strFolder);
		}
	}

	// Read the keys and values
	for (auto it = sections.cbegin(); it != sections.cend(); ++it)
	{
		unique.clear();

		const WCHAR* sectionName = (*it).c_str();
		bool isVariables = (_wcsicmp(sectionName, L"Variables") == 0);
		bool isMetadata = (skinSection == nullptr && !isVariables && _wcsicmp(sectionName, L"Metadata") == 0);
		bool resetInsertPos = true;

		// Read all "key=value" from the section
		do
		{
			items[0] = 0;
			DWORD res = GetPrivateProfileSection(sectionName, items, itemsSize, iniRead.c_str());
			if (res < itemsSize - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			itemsSize *= 2;
			items = new WCHAR[itemsSize];
		}
		while (true);

		pos = items;
		while (pos < epos)
		{
			if (*pos)
			{
				size_t len = wcslen(pos);
				WCHAR* sep = wmemchr(pos, L'=', len);
				if (sep != nullptr && sep != pos)
				{
					size_t clen = sep - pos;  // key's length

					StrToUpperC(key.assign(pos, clen));
					if (unique.insert(key).second)
					{
						++sep;
						clen = len - (clen + 1);  // value's length

						// Trim surrounded quotes from value
						if (clen >= 2 && (sep[0] == L'"' || sep[0] == L'\'') && sep[clen - 1] == sep[0])
						{
							clen -= 2;
							++sep;
						}

						if (wcsncmp(key.c_str(), L"@INCLUDE", 8) == 0)
						{
							if (clen > 0)
							{
								value.assign(sep, clen);
								ReadVariables();
								ReplaceVariables(value);
								if (!PathUtil::IsAbsolute(value))
								{
									// Relative to the ini folder
									value.insert(0, PathUtil::GetFolderFromFilePath(iniFile));
								}

								if (resetInsertPos)
								{
									auto jt = it;
									if (++jt == sections.end())  // Special case: @include was used in the last section of the current file
									{
										// Set the insertion place to the last
										m_SectionInsertPos = m_Sections.end();
										resetInsertPos = false;
									}
									else
									{
										// Find the appropriate insertion place
										for (jt = m_Sections.cbegin(); jt != m_Sections.cend(); ++jt)
										{
											if (_wcsicmp((*jt).c_str(), sectionName) == 0)
											{
												m_SectionInsertPos = ++jt;
												resetInsertPos = false;
												break;
											}
										}
									}
								}

								ReadIniFile(value, skinSection, depth + 1);
							}
						}
						else
						{
							if (!isMetadata)  // Uncache Metadata's key-value pair in the skin
							{
								value.assign(sep, clen);
								SetValue((*it), key, value);

								if (isVariables)
								{
									m_ListVariables.push_back(key);
								}
							}
						}
					}
				}
				pos += len + 1;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}

	delete [] items;
	if (temporary) System::RemoveFile(iniRead);
}

/*
** Sets the value for the key under the given section.
**
*/
void ConfigParser::SetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strValue)
{
	// LogDebugF(L"[%s] %s=%s (size: %i)", strSection.c_str(), strKey.c_str(), strValue.c_str(), (int)m_Values.size());

	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	m_Values[StrToUpperC(strTmp)] = strValue;
}

/*
** Deletes the value for the key under the given section.
**
*/
void ConfigParser::DeleteValue(const std::wstring& strSection, const std::wstring& strKey)
{
	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToUpperC(strTmp));
	if (iter != m_Values.end())
	{
		m_Values.erase(iter);
	}
}

/*
** Returns the value for the key under the given section.
**
*/
const std::wstring& ConfigParser::GetValue(const std::wstring& strSection, const std::wstring& strKey, const std::wstring& strDefault)
{
	std::wstring strTmp;
	strTmp.reserve(strSection.size() + 1 + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToUpperC(strTmp));
	return (iter != m_Values.end()) ? (*iter).second : strDefault;
}
