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
#include "MeasurePlugin.h"
#include "MeasureScript.h"
#include "MeasureTime.h"
#include "Meter.h"
#include "resource.h"

namespace {

struct PairInfo
{
	const WCHAR begin;
	const WCHAR end;
};

const std::unordered_map<PairedPunctuation, PairInfo> s_PairedPunct =
{
	{ PairedPunctuation::SingleQuote, { L'\'', L'\'' } },
	{ PairedPunctuation::DoubleQuote, { L'"', L'"' } },
	{ PairedPunctuation::BothQuotes,  { L'"', L'\'' } },
	{ PairedPunctuation::Parentheses, { L'(', L')' } },
	{ PairedPunctuation::Brackets,    { L'[', L']' } },
	{ PairedPunctuation::Braces,      { L'{', L'}' } },
	{ PairedPunctuation::Guillemet,   { L'<', L'>' } }
};

}  // namespace

std::unordered_map<std::wstring, std::wstring> ConfigParser::c_MonitorVariables;
std::unordered_map<ConfigParser::VariableType, WCHAR> ConfigParser::c_VariableMap;

ConfigParser::ConfigParser() :
	m_LastReplaced(false),
	m_LastDefaultUsed(false),
	m_LastValueDefined(false),
	m_CurrentSection(),
	m_Skin()
{
	if (c_VariableMap.empty())
	{
		c_VariableMap.emplace(VariableType::Section, L'&');
		c_VariableMap.emplace(VariableType::Variable, L'#');
		c_VariableMap.emplace(VariableType::Mouse, L'$');
		c_VariableMap.emplace(VariableType::CharacterReference, L'\\');
	}
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
	m_OriginalVariableNames.clear();

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
		return m_BuiltInVariables.emplace(name, value);
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
	std::wstring original = strVariable;

	StrToUpperC(strVariable);
	m_Variables[strVariable] = strValue;

	if (m_OriginalVariableNames.find(strVariable) == m_OriginalVariableNames.end())
	{
		m_OriginalVariableNames[strVariable] = original;
	}
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

const std::wstring* ConfigParser::GetVariableOriginalName(const std::wstring& strVariable)
{
	const std::wstring strTmp = StrToUpper(strVariable);

	// User-defined variables
	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_OriginalVariableNames.find(strTmp);
	if (iter != m_OriginalVariableNames.end())
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
bool ConfigParser::GetSectionVariable(std::wstring& strVariable, std::wstring& strValue, void* logEntry)
{
	if (!m_Skin) return false;

	const size_t firstParens = strVariable.find_first_of(L'(');  // Assume section names do not have a left parenthesis?
	size_t colonPos = strVariable.find_last_of(L':', firstParens);
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
		// [Meter:X], [Meter:Y], [Meter:W], [Meter:H], etc.
		Meter* meter = m_Skin->GetMeter(strVariable);
		if (meter)
		{
			WCHAR buffer[32] = { 0 };
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
			else if (_wcsicmp(selectorSz, L"XW") == 0)
			{
				_itow_s(meter->GetX() + meter->GetW(), buffer, 10);
			}
			else if (_wcsicmp(selectorSz, L"YH") == 0)
			{
				_itow_s(meter->GetY() + meter->GetH(), buffer, 10);
			}
			else
			{
				// Fallback for meter: use ReadString for non-numeric properties.
				const std::wstring optionValue = m_Skin->GetParser().ReadString(strVariable.c_str(), selector.c_str(), L"");
				if (!optionValue.empty())
				{
					strValue = optionValue;
					return true;
				}
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
	//
	// Script: [ScriptMeasure:SomeFunction()], [ScriptMeasure:Something('Something')]
	// NOTE: Parenthesis are required. Arguments enclosed in single or double quotes are treated as strings, otherwise
	//   they are treated as numbers. If the lua function returns a number, it will be converted to a string.
	enum class ValueType
	{
		Raw,
		Percentual,
		Max,
		Min,
		EscapeRegExp,
		EncodeUrl,
		TimeStamp,
		Script,
		Plugin
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
			// Check if calling a Script/Plugin measure, or fallback to ReadString.
			Measure* measure = m_Skin->GetMeasure(strVariable);
			if (!measure) return false;

			// Lua (and possibly plugins) can reset the style template when
			// reading values, so save the style template here and reset it
			// back after the lua/plugin has returned.
			std::vector<std::wstring> meterStyle = m_StyleTemplate;

			bool retValue = false;
			const auto type = measure->GetTypeID();
			if (type == TypeID<MeasureScript>())
			{
				valueType = ValueType::Script;  // Needed?
				MeasureScript* script = (MeasureScript*)measure;
				retValue = script->CommandWithReturn(selectorSz, strValue, logEntry);
			}
			else if (type == TypeID<MeasurePlugin>())
			{
				valueType = ValueType::Plugin;  // Needed?
				MeasurePlugin* plugin = (MeasurePlugin*)measure;
				retValue = plugin->CommandWithReturn(selectorSz, strValue, logEntry);
			}
			else
			{
				// Fallback for measures: use ReadString for non-command properties.
				const std::wstring optionValue = m_Skin->GetParser().ReadString(strVariable.c_str(), selector.c_str(), L"");
				if (!optionValue.empty())
				{
					strValue = optionValue;
					retValue = true;
				}
				else
				{
					retValue = false;
				}
			}

			m_StyleTemplate = meterStyle;
			return retValue;
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
				const WCHAR* keySelectorSz = strVariable.c_str() + colonPos + 1ULL;

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
			} while (0);
		}
	}

	Measure* measure = m_Skin->GetMeasure(strVariable);
	if (measure)
	{
		if (valueType == ValueType::EscapeRegExp)
		{
			const WCHAR* tmp = measure->GetStringValue();
			strValue = tmp ? tmp : L"";
			StringUtil::EscapeRegExp(strValue);
			return true;
		}
		else if (valueType == ValueType::EncodeUrl)
		{
			const WCHAR* tmp = measure->GetStringValue();
			strValue = tmp ? tmp : L"";
			StringUtil::EncodeUrl(strValue);
			return true;
		}
		else if (measure->GetTypeID() == TypeID<MeasureTime>() && valueType == ValueType::TimeStamp)
		{
			MeasureTime* time = (MeasureTime*)measure;
			strValue = std::to_wstring(time->GetTimeStamp().QuadPart / 10000000LL);
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
			(valueType == ValueType::Max) ? measure->GetMaxValue() / scale :
			(valueType == ValueType::Min) ? measure->GetMinValue() / scale :
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

		WCHAR format[32] = { 0 };
		WCHAR buffer[128] = { 0 };
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

	WCHAR buffer[32] = { 0 };
	const int monitorIndex = monitorsInfo.primary - 1;
	const RECT workArea = monitors[monitorIndex].work;
	const RECT scrArea = monitors[monitorIndex].screen;

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
		WCHAR buffer2[64] = { 0 };

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

		WCHAR buffer[32] = { 0 };
		int w1 = 0, w2 = 0, s1 = 0, s2 = 0;
		int screenIndex = 0;

		// Set X / WIDTH
		screenIndex = monitorsInfo.primary;
		if (skin->GetXScreenDefined())
		{
			int i = skin->GetXScreen();
			const int index = i - 1;
			if (i >= 0 && (i == 0 || i <= numOfMonitors && monitors[index].active))
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
			const int monitorIndex = screenIndex - 1;
			w1 = monitors[monitorIndex].work.left;
			w2 = monitors[monitorIndex].work.right - monitors[monitorIndex].work.left;
			s1 = monitors[monitorIndex].screen.left;
			s2 = monitors[monitorIndex].screen.right - monitors[monitorIndex].screen.left;
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
			const int i = skin->GetYScreen();
			const int index = i - 1;
			if (i >= 0 && (i == 0 || i <= numOfMonitors && monitors[index].active))
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
			const int monitorIndex = screenIndex - 1;
			w1 = monitors[monitorIndex].work.top;
			w2 = monitors[monitorIndex].work.bottom - monitors[monitorIndex].work.top;
			s1 = monitors[monitorIndex].screen.top;
			s2 = monitors[monitorIndex].screen.bottom - monitors[monitorIndex].screen.top;
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
bool ConfigParser::ReplaceVariables(std::wstring& result, bool isNewStyle)
{
	bool replaced = false;

	PathUtil::ExpandEnvironmentVariables(result);

	if (c_MonitorVariables.empty())
	{
		SetMultiMonitorVariables(true);
	}

	// Check for new-style variables ([#VAR])
	// Note: Most new-style variables are parsed later (when section variables are parsed),
	//   however, there are a few places where we just want to parse only variables (without
	//   section variables).
	if (isNewStyle)
	{
		replaced = ParseVariables(result, VariableType::Variable);
	}
	else
	{
		// Special parsing for [#CURRENTSECTION] for use in actions
		size_t start = 0ULL;
		bool loop = true;
		const std::wstring strVariable = L"[#CURRENTSECTION]";
		const size_t length = strVariable.length();
		do
		{
			start = result.find(strVariable, start);
			if (start != std::wstring::npos)
			{
				const std::wstring* value = GetVariable(L"CURRENTSECTION");
				if (value)
				{
					// Variable found, replace it with the value
					result.replace(start, length, *value);
					start += length;
					replaced = true;
				}
			}
			else
			{
				loop = false;
			}
		}
		while (loop);
	}

	// Check for old-style variables (#VAR#)
	size_t start = 0ULL, end = 0ULL;
	bool loop = true;

	do
	{
		start = result.find(L'#', start);
		if (start != std::wstring::npos)
		{
			size_t si = start + 1ULL;
			end = result.find(L'#', si);
			if (end != std::wstring::npos)
			{
				size_t ei = end - 1ULL;
				if (si != ei && result[si] == L'*' && result[ei] == L'*')
				{
					result.erase(ei, 1ULL);
					result.erase(si, 1ULL);
					start = ei;
				}
				else
				{
					std::wstring strVariable = result.substr(si, end - si);
					const std::wstring* value = GetVariable(strVariable);
					if (value)
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1ULL, *value);
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
	// Check for new-style measures (and section variables) [&Measure], [&Meter]
	// Note: This also parses regular variables as well (in case of nested variable types) eg. [#Var[&Measure]]
	bool replaced = ParseVariables(result, VariableType::Section);

	// Check for old-style measures and section variables. [Measure], [Meter:X], etc.
	size_t start = 0ULL;
	while ((start = result.find(L'[', start)) != std::wstring::npos)
	{
		size_t si = start + 1ULL;
		size_t end = result.find(L']', si);
		if (end == std::wstring::npos)
		{
			break;
		}

		size_t next = result.find(L'[', si);
		if (next == std::wstring::npos || end < next)
		{
			size_t ei = end - 1ULL;
			if (si != ei && result[si] == L'*' && result[ei] == L'*')
			{
				result.erase(ei, 1ULL);
				result.erase(si, 1ULL);
				start = ei;
			}
			else
			{
				std::wstring var = result.substr(si, end - si);

				Measure* measure = GetMeasure(var);
				if (measure)
				{
					const WCHAR* value = measure->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, -1, false);
					size_t valueLen = wcslen(value);

					// Measure found, replace it with the value
					result.replace(start, end - start + 1, value, valueLen);
					start += valueLen;
					replaced = true;
				}
				else
				{
					// It is possible for a variable to be reset when calling a custom function in a plugin or lua.
					// Copy the result here, and replace it before returning.
					std::wstring str = result;

					std::wstring value;
					if (GetSectionVariable(var, value))
					{
						// Replace section variable with the value.
						str.replace(start, end - start + 1, value);
						start += value.length();
						replaced = true;
						result = str;
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

/*
** Replaces nested measure/section variables, regular variables, and mouse variables in the given string.
**
*/
bool ConfigParser::ParseVariables(std::wstring& str, const VariableType type, Meter* meter)
{
	// Since actions are parsed when executed, get the current active
	// section in case the current section variable is used.
	bool hasCurrentAction = false;
	if (m_Skin && (m_CurrentSection->empty() || meter))
	{
		Section* section = m_Skin->GetCurrentActionSection();
		if (section || meter)
		{
			m_CurrentSection->assign(meter ? meter->GetName() : section->GetName());
			hasCurrentAction = true;
		}
	}

	// It is possible for a variable to be reset when calling a custom function in a plugin or lua.
	// Copy the result here, and replace it before returning.
	std::wstring result = str;
	bool replaced = false;

	size_t previousStart = 0ULL;
	std::wstring previousVariable;

	Logger::Entry delayedLogEntry = { Logger::Level::Debug, L"", L"", L"" };

	// Because each nested variable needs to be re-parsed from the beginning of the replaced string,
	// self-references can be detected multiple times during the variable replacement process.
	// In these cases, provide a warning to the user before returning.
	std::wstring selfReferencedVariable;

	// Max number of variable replacements for |str|
	static const size_t maxReplacements = 1000ULL;

	// Find the innermost section variable(s) first, then move outward (working left to right)
	size_t end = 0ULL;
	size_t counter = 0ULL;
	while ((end = result.find(L']', end)) != std::wstring::npos)
	{
		// Restrict the number of variable replacements to a reseasonable amount
		if (++counter >= maxReplacements)
		{
			LogErrorSF(m_Skin, m_CurrentSection->c_str(),
				L"Parsing Error: Maximum number of variable replacements reached (%llu) in string: %s", maxReplacements, str.c_str());
			if (GetRainmeter().GetDebug())
			{
				LogDebugSF(m_Skin, m_CurrentSection->c_str(), L"Parsing Error: Result: %s", result.c_str());
			}
			break;
		}

		bool found = false;

		const size_t ei = end - 1ULL;
		size_t start = ei;

		while ((start = result.rfind(L'[', start)) != std::wstring::npos)
		{
			found = false;
			size_t si = start + 2ULL;  // Start index where escaped variable "should" be: [ *   *]

			// Check for escaped variables first, if found, skip to the next variable
			if (si != ei && result[si] == L'*' && result[ei] == L'*')
			{
				// Normally we remove the *'s for escaped variable names here, however mouse actions
				// are parsed before being sent to the command handler where the rest of the variables
				// are parsed. So we need to leave the escape *'s when called from the mouse parser.
				if (type != VariableType::Mouse)
				{
					result.erase(ei, 1ULL);
					result.erase(si, 1ULL);
				}
				break;		// Break out of inner "start" loop and continue to the next nested variable
			}

			--si;  // Move index to the "key" character (if it exists)

			// Avoid empty commands
			std::wstring original = result.substr(si, end - si);
			if (original.empty())
			{
				break;		// Break out of inner "start" loop and continue to the next nested variable
			}

			// Avoid self references
			if (previousStart == start && _wcsicmp(original.c_str(), previousVariable.c_str()) == 0)
			{
				LogErrorSF(m_Skin, m_CurrentSection->c_str(),
					L"Cannot replace variable with itself: \"%s\"", original.c_str());
				break;		// Break out of inner "start" loop and continue to the next nested variable
			}

			previousVariable = original;
			previousStart = start;

			// Separate "key" character from variable
			const WCHAR key = result.substr(si, 1ULL).c_str()[0];
			std::wstring variable = result.substr(si + 1ULL, end - si - 1ULL);
			if (variable.empty())
			{
				break; // Break out of inner "start" loop and continue to the next nested variable
			}

			// Find "type" of key
			bool isValid = false;
			VariableType kType = VariableType::Section;
			for (const auto& t : c_VariableMap)
			{
				if (t.second == key)
				{
					kType = t.first;
					isValid = true;
					break;
				}
			}

			// |key| is invalid or variable name is empty ([#], [&], [$], [\])
			if (!isValid)
			{
				if (start == 0ULL) break;	// Already at beginning of string, try next ending bracket

				--start;		// Check for any "starting" brackets in string prior to the current starting position
				continue;		// This is not a valid nested variable, check the next starting bracket
			}

			// Since regular variables are replaced just before section variables in most cases, we replace
			// both types at the same time in case nesting of the different types occurs. The only side effect
			// is new-style regular variables located in an action will now be "dynamic" just like section
			// variables.
			//  Special case 1: Mouse variables cannot be used in the outer part of a nested variable. This is
			//    because mouse variables are parsed and replaced before the other new-style variables.
			//  Special case 2: Places where regular variables need to be parsed without any section variables
			//    parsed afterward. One example is when "@Include" is parsed.
			//  Special case 3: Always process escaped character references.

			std::wstring foundValue;

			if ((key == c_VariableMap.find(type)->second) ||										// Special cases 1, 2
				(kType == VariableType::CharacterReference) ||										// Special case 3
				(type == VariableType::Section && key == c_VariableMap[VariableType::Variable]))	// Most cases
			{
				switch (kType)
				{
				case VariableType::Section:
					{
						Measure* measure = GetMeasure(variable);
						if (measure)
						{
							const WCHAR* value = measure->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, -1, false);
							foundValue.assign(value, wcslen(value));
							found = true;
							break;
						}
						found = GetSectionVariable(variable, foundValue, &delayedLogEntry);
					}
					break;

				case VariableType::Variable:
					{
						const std::wstring* value = GetVariable(variable);
						if (value)
						{
							foundValue.assign(*value);
							found = true;
						}
					}
					break;

				case VariableType::Mouse:
					{
						foundValue = GetMouseVariable(variable, meter);
						found = !foundValue.empty();
					}
					break;

				case VariableType::CharacterReference:
					{
						int base = 10;
						if (variable[0] == L'x' || variable[0] == L'X')
						{
							base = 16;
							variable.erase(0ULL, 1ULL);  // remove 'x' or 'X'

							if (variable.empty())
							{
								break;  // Invalid escape sequence [\x]
							}
						}

						WCHAR* pch = nullptr;
						errno = 0;
						long ch = wcstol(variable.c_str(), &pch, base);
						if (pch == nullptr || *pch != L'\0' || errno == ERANGE || ch <= 0L || ch >= 0xFFFE)
						{
							break;  // Invalid character
						}

						foundValue.assign(1ULL, (WCHAR)ch);
						found = true;
					}
					break;
				}
			}

			if (found)
			{
				// Look for any potential self-references in the "found" value
				auto findVariable = [&](WCHAR postfix) -> void
				{
					// Only check for self-references if none have been found
					if (selfReferencedVariable.empty())
					{
						const std::wstring var = L"[" + original + postfix;
						if (StringUtil::CaseInsensitiveFind(foundValue, var) != -1)
						{
							selfReferencedVariable = original;  // Reports only the first self-reference
						}
					}
				};

				findVariable(L']');  // Look for any nested variables.  ex. [#Variable]
				findVariable(L':');  // Look for any section variables with parameters.  ex. [&Measure:

				result.replace(start, end - start + 1ULL, foundValue);
				replaced = true;

				end = start - 1ULL;
				break;		// Break out of inner "start" loop and continue to the next nested variable
			}

			// No variable found

			if (start == 0ULL) break;	// Already at beginning of string, try next ending bracket

			--start;		// Check for any "starting" brackets in string prior to the current starting position
		}

		if (!delayedLogEntry.message.empty() && found && start == previousStart)
		{
			// Since custom script/plugin functions can accept single brackets as parameters, it is possible that
			// the nested variable parser can produce errors when determining function names. Reset any delayed
			// messages if the variable at the starting position was found.
			delayedLogEntry = { Logger::Level::Debug, L"", L"", L"" }; 
		}

		++end;	// Check for the next "end" bracket after the current ending bracket
	}

	if (!delayedLogEntry.message.empty())
	{
		GetLogger().Log(&delayedLogEntry);
	}

	// Log the self reference warning(s)
	if (!selfReferencedVariable.empty())
	{
		LogWarningSF(m_Skin, m_CurrentSection->c_str(), L"Warning: Potential self-referenced variable: %s", selfReferencedVariable.c_str());
		if (GetRainmeter().GetDebug())
		{
			LogDebugSF(m_Skin, m_CurrentSection->c_str(), L"Original string: %s", str.c_str());
			LogDebugSF(m_Skin, m_CurrentSection->c_str(), L"Replaced string: %s", result.c_str());
		}
	}

	// Reset the current section
	if (hasCurrentAction)
	{
		m_CurrentSection->clear();
	}

	str = result;
	return replaced;
}

bool ConfigParser::ContainsNewStyleVariable(const std::wstring& str)
{
	if (str.find(L'[') == std::wstring::npos) return false;

	for (const auto& key : c_VariableMap)
	{
		std::wstring var = L"[";
		var += key.second;

		if (str.find(var) != std::wstring::npos) return true;
	}

	return false;
}

std::wstring ConfigParser::GetMouseVariable(const std::wstring& variable, Meter* meter)
{
	std::wstring result;
	LPCWSTR var = variable.c_str();
	WCHAR buffer[32] = { 0 };

	POINT pt = { 0 };
	GetCursorPos(&pt);

	if (_wcsnicmp(var, L"MOUSEX", 6) == 0)
	{
		var += 6;
		int xOffset = m_Skin->GetX() + (meter ? meter->GetX() : 0);
		if (wcscmp(var, L":%") == 0)  // $MOUSEX:%$ or [$MOUSEX:%]
		{
			double width = (meter ? meter->GetW() : m_Skin->GetW());
			xOffset = (int)(((pt.x - xOffset + 1) / width) * 100.0);
			_itow_s(xOffset, buffer, 10);
			result = buffer;
		}
		else if (*var == L'\0')  // $MOUSEX$ or [$MOUSEX]
		{
			_itow_s(pt.x - xOffset, buffer, 10);
			result = buffer;
		}
	}
	else if (_wcsnicmp(var, L"MOUSEY", 6) == 0)
	{
		var += 6;
		int yOffset = m_Skin->GetY() + (meter ? meter->GetY() : 0);
		if (wcscmp(var, L":%") == 0)  // $MOUSEY:%$ or [$MOUSEX:%]
		{
			double width = (meter ? meter->GetH() : m_Skin->GetH());
			yOffset = (int)(((pt.y - yOffset + 1) / width) * 100.0);
			_itow_s(yOffset, buffer, 10);
			result = buffer;
		}
		else if (*var == L'\0')  // $MOUSEY$ or [$MOUSEY]
		{
			_itow_s(pt.y - yOffset, buffer, 10);
			result = buffer;
		}
	}

	return result;
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
		m_CurrentSection->assign(strSection);  // Set temporarily
		m_LastValueDefined = true;

		if (result.size() >= 3)
		{
			if (result.find(L'#') != std::wstring::npos)
			{
				// Make sure new-style variables are processed for the [Variables] section
				bool runNewStyle = strSection == L"Variables" ? true : false;
				if (ReplaceVariables(result, runNewStyle))
				{
					m_LastReplaced = true;
				}
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
		m_CurrentSection->clear();  // Reset
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

std::vector<FLOAT> ConfigParser::ReadFloats(LPCTSTR section, LPCTSTR key)
{
	std::vector<FLOAT> result;
	const std::wstring& str = ReadString(section, key, L"");
	if (!str.empty())
	{
		// Tokenize and parse the floats
		const WCHAR delimiter = L';';
		size_t lastPos = 0ULL, pos = 0ULL;
		do
		{
			lastPos = str.find_first_not_of(delimiter, pos);
			if (lastPos == std::wstring::npos) break;

			pos = str.find_first_of(delimiter, lastPos + 1ULL);

			result.push_back((FLOAT)ParseDouble(str.substr(lastPos, pos - lastPos).c_str(), 0.0));  // (pos != std::wstring::npos) ? pos - lastPos : pos
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
		const WCHAR* str = result.c_str();
		if (*str == L'(')
		{
			double dblValue = 0.0;
			const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
			if (!errMsg)
			{
				return (int)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*str)
		{
			errno = 0;
			int intValue = wcstol(str, nullptr, 10);
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
		const WCHAR* str = result.c_str();
		if (*str == L'(')
		{
			double dblValue = 0.0;
			const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
			if (!errMsg)
			{
				return (uint32_t)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*str)
		{
			errno = 0;
			uint32_t uintValue = wcstoul(str, nullptr, 10);
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
		const WCHAR* str = result.c_str();
		if (*str == L'(')
		{
			double dblValue = 0.0;
			const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
			if (!errMsg)
			{
				return (uint64_t)dblValue;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*str)
		{
			errno = 0;
			uint64_t uint64Value = _wcstoui64(str, nullptr, 10);
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
		double value = 0.0;
		const WCHAR* str = result.c_str();
		if (*str == L'(')
		{
			const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
			if (!errMsg)
			{
				return value;
			}

			LogErrorF(m_Skin, L"Formula: %s in key \"%s\" in [%s]", errMsg, key, section);
		}
		else if (*str)
		{
			errno = 0;
			value = wcstod(str, nullptr);
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
	if (!formula.empty() && formula[0] == L'(' && formula[formula.size() - 1ULL] == L')')
	{
		const WCHAR* str = formula.c_str();
		const WCHAR* errMsg = MathParser::CheckedParse(str, resultValue);
		if (errMsg != nullptr)
		{
			LogErrorF(m_Skin, L"Formula: %s: %s", errMsg, str);
			return false;
		}

		return true;
	}

	return false;
}

// Strips any trailing modifiers before evaluating a formula, then
//  appends the trailing modifiers for further processing.
std::wstring ConfigParser::ParseFormulaWithModifiers(const std::wstring& formula)
{
	std::wstring modifiers;
	double value = 0.0;

	const size_t pos = formula.find_last_of(L')');
	if (pos != std::wstring::npos)
	{
		modifiers = formula.substr(pos + 1);  // can be empty!
		const std::wstring newFormula(formula, 0, pos + 1ULL);
		if (ParseFormula(newFormula, &value))
		{
			WCHAR buffer[128] = { 0 };
			int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%lf", value);
			Measure::RemoveTrailingZero(buffer, bufferLen);
			modifiers.insert(0, buffer);  // Insert the "value" in front of the modifiers.
			return modifiers;
		}
	}

	return formula;
}

D2D1_COLOR_F ConfigParser::ReadColor(LPCTSTR section, LPCTSTR key, const D2D1_COLOR_F& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed || result.empty()) ? defValue : ParseColor(result.c_str());
}

D2D1_RECT_F ConfigParser::ReadRect(LPCTSTR section, LPCTSTR key, const D2D1_RECT_F& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	return (m_LastDefaultUsed) ? defValue : ParseRect(result.c_str());
}

RECT ConfigParser::ReadRECT(LPCTSTR section, LPCTSTR key, const RECT& defValue)
{
	const std::wstring& result = ReadString(section, key, L"");

	RECT r = { 0 };
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

	size_t lastPos = 0ULL, pos = 0ULL;
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
	size_t start = 0ULL;
	size_t end = 0ULL;

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

	if (punct == PairedPunctuation::SingleQuote ||
		punct == PairedPunctuation::DoubleQuote)
	{
		bool found = false;
		for (auto& iter : str)
		{
			if (iter == s_PairedPunct.at(punct).begin) found = !found;
			else if (iter == delimiter && !found)
			{
				getToken();
				start = end + 1;  // skip delimiter
			}
			++end;
		}
	}
	else if (punct == PairedPunctuation::BothQuotes)
	{
		// Skip delimiters if inside either a pair of single quotes, or a pair of double quotes
		bool found = false;
		WCHAR current = L'\0';
		for (auto& iter : str)
		{
			if (!current &&
				(iter == s_PairedPunct.at(punct).begin ||	// single quote
				 iter == s_PairedPunct.at(punct).end))		// double quote
			{
				current = iter;
				found = true;
			}
			else if (iter == current)
			{
				current = L'\0';
				found = false;
			}
			else if (iter == delimiter && !found)
			{
				getToken();
				start = end + 1;  // skip delimiter
			}
			++end;
		}
	}
	else
	{
		int pairs = 0;
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
double ConfigParser::ParseDouble(LPCTSTR str, double defValue)
{
	assert(str);

	double value = 0.0;
	if (*str == L'(')
	{
		const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
		if (!errMsg)
		{
			return value;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		double value = wcstod(str, nullptr);
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
int ConfigParser::ParseInt(LPCTSTR str, int defValue)
{
	assert(str);

	if (*str == L'(')
	{
		double dblValue = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
		if (!errMsg)
		{
			return (int)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		int intValue = wcstol(str, nullptr, 10);
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
uint32_t ConfigParser::ParseUInt(LPCTSTR str, uint32_t defValue)
{
	assert(str);

	if (*str == L'(')
	{
		double dblValue = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
		if (!errMsg)
		{
			return (uint32_t)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		uint32_t uintValue = wcstoul(str, nullptr, 10);
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
uint64_t ConfigParser::ParseUInt64(LPCTSTR str, uint64_t defValue)
{
	assert(str);

	if (*str == L'(')
	{
		double dblValue = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &dblValue);
		if (!errMsg)
		{
			return (uint64_t)dblValue;
		}

		LogErrorF(L"Formula: %s: %s", errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		uint64_t uint64Value = _wcstoui64(str, nullptr, 10);
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
bool ParseInt4(LPCTSTR s, T& v1, T& v2, T& v3, T& v4)
{
	if (wcschr(s, L','))
	{
		std::wstring str = s;
		std::vector<T> tokens;
		size_t start = 0ULL;
		size_t end = 0ULL;
		int parens = 0;

		auto getToken = [&]() -> void
		{
			start = str.find_first_not_of(L" \t", start); // skip any leading whitespace
			if (start <= end)
			{
				tokens.push_back((T)ConfigParser::ParseInt(str.substr(start, end - start).c_str(), 0));
			}
		};

		for (auto& iter : str)
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
						start = end + 1ULL; // skip comma
						break;
					}
					//else multi arg function ?
				}
				break;
			}
			++end;
		}

		// read last token
		getToken();

		size_t size = tokens.size();
		if (size > 0ULL) v1 = tokens[0];
		if (size > 1ULL) v2 = tokens[1];
		if (size > 2ULL) v3 = tokens[2];
		if (size > 3ULL) v4 = tokens[3];

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
D2D1_COLOR_F ConfigParser::ParseColor(LPCTSTR str)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (!ParseInt4(str, R, G, B, A))
	{
		if (wcsncmp(str, L"0x", 2ULL) == 0)
		{
			str += 2;  // skip prefix
		}

		size_t len = wcslen(str);
		if (len >= 8 && !iswspace(str[6]))
		{
			swscanf_s(str, L"%02x%02x%02x%02x", &R, &G, &B, &A);
		}
		else if (len >= 6ULL)
		{
			swscanf_s(str, L"%02x%02x%02x", &R, &G, &B);
		}
	}

	return D2D1::ColorF(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
}

/*
** Helper method that parses the D2D1::RectF values from the given string.
** The rect can be supplied as four comma separated values (X/Y/Width/Height).
**
*/
D2D1_RECT_F ConfigParser::ParseRect(LPCTSTR str)
{
	D2D1_RECT_F r = D2D1::RectF();
	ParseInt4(str, r.left, r.top, r.right, r.bottom);
	r.right += r.left;
	r.bottom += r.top;
	return r;
}

/*
** Helper method that parses the RECT values from the given string.
** The rect can be supplied as four comma separated values (left/top/right/bottom).
**
*/
RECT ConfigParser::ParseRECT(LPCTSTR str)
{
	RECT r = { 0 };
	ParseInt4(str, r.left, r.top, r.right, r.bottom);
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
	if (_waccess_s(iniFile.c_str(), 0) != 0)
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
				items = nullptr;
				if (temporary) System::RemoveFile(iniRead);
				return;
			}
			if (res < itemsSize - 2)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			items = nullptr;
			itemsSize *= 2UL;
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
			if (res < itemsSize - 2UL)		// Fits in the buffer
			{
				epos = items + res;
				break;
			}

			delete [] items;
			items = nullptr;
			itemsSize *= 2UL;
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

					key.assign(pos, clen);
					std::wstring original = key;
					StrToUpperC(key);
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
								ReplaceVariables(value, true);
								if (!PathUtil::IsAbsolute(value))
								{
									// Relative to the ini folder
									value.insert(0, PathUtil::GetFolderFromFilePath(iniFile));
								}

								if (resetInsertPos)
								{
									std::list<std::wstring>::const_iterator jt = it;
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

								// Save the section insertion position in case the included file also uses an @Include
								std::list<std::wstring>::const_iterator prevInsertPos = m_SectionInsertPos;

								ReadIniFile(value, skinSection, depth + 1);

								// Reset the section insertion position to previous position
								m_SectionInsertPos = prevInsertPos;
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
									m_OriginalVariableNames[key] = original;
								}
							}
						}
					}
				}
				pos += len + 1ULL;
			}
			else  // Empty string
			{
				++pos;
			}
		}
	}

	delete [] items;
	items = nullptr;
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
	strTmp.reserve(strSection.size() + 1ULL + strKey.size());
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
	strTmp.reserve(strSection.size() + 1ULL + strKey.size());
	strTmp = strSection;
	strTmp += L'~';
	strTmp += strKey;

	std::unordered_map<std::wstring, std::wstring>::const_iterator iter = m_Values.find(StrToUpperC(strTmp));
	return (iter != m_Values.end()) ? (*iter).second : strDefault;
}
