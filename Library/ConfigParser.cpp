/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MathParser.h"
#include "../Common/ParseUtil.h"
#include "../Common/PathUtil.h"
#include "ConfigParser.h"
#include "Util.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "System.h"
#include "MonitorUtil.h"
#include "Measure.h"
#include "MeasurePlugin.h"
#include "MeasureScript.h"
#include "MeasureTime.h"
#include "Meter.h"
#include "resource.h"

namespace {

enum class MonitorArea
{
	Screen,
	Work,
	VirtualScreen
};

enum class MonitorComponent
{
	X,
	Y,
	Width,
	Height
};

void LogFormulaError(const WCHAR* error, const WCHAR* formula)
{
	LogErrorF(L"Formula: %s: %s", error, formula);
}

template<size_t N>
bool MatchRange(const WCHAR* start, const WCHAR* end, const WCHAR (&value)[N])
{
	constexpr size_t len = N - 1;
	return (size_t)(end - start) == len && _wcsnicmp(start, value, len) == 0;
}

bool ParseMonitorComponent(const WCHAR* start, const WCHAR* end, MonitorComponent& component)
{
	if (MatchRange(start, end, L"X"))
	{
		component = MonitorComponent::X;
		return true;
	}
	else if (MatchRange(start, end, L"Y"))
	{
		component = MonitorComponent::Y;
		return true;
	}
	else if (MatchRange(start, end, L"WIDTH"))
	{
		component = MonitorComponent::Width;
		return true;
	}
	else if (MatchRange(start, end, L"HEIGHT"))
	{
		component = MonitorComponent::Height;
		return true;
	}

	return false;
}

bool ParseMonitorNumber(const WCHAR* start, const WCHAR* end, int& monitorNumber)
{
	if (start == end) return false;

	monitorNumber = 0;
	for (const WCHAR* ch = start; ch != end; ++ch)
	{
		if (!iswdigit(*ch)) return false;
		monitorNumber = monitorNumber * 10 + (*ch - L'0');
	}

	return monitorNumber > 0;
}

bool ParseMonitorSelector(const WCHAR* start, const WCHAR* end, bool& autoSelect, int& monitorNumber)
{
	if (MatchRange(start, end, L"AUTO"))
	{
		autoSelect = true;
		monitorNumber = -1;
		return true;
	}

	return ParseMonitorNumber(start, end, monitorNumber);
}

bool ParseMonitorVariable(const WCHAR* str, bool& physical, MonitorArea& area, MonitorComponent& component, bool& primary, bool& autoSelect, int& monitorNumber)
{
	const WCHAR* start = str;
	const WCHAR* end = str + wcslen(str);
	physical = false;
	primary = false;
	autoSelect = false;
	monitorNumber = -1;

	if (end - start >= 3 && _wcsicmp(end - 3, L":PX") == 0)
	{
		physical = true;
		end -= 3;
	}

	if (StringUtil::MatchAndSkipPrefix(&start, end, L"PWORKAREA"))
	{
		primary = true;
		area = MonitorArea::Work;
	}
	else if (StringUtil::MatchAndSkipPrefix(&start, end, L"PSCREENAREA"))
	{
		primary = true;
		area = MonitorArea::Screen;
	}
	else if (StringUtil::MatchAndSkipPrefix(&start, end, L"VSCREENAREA"))
	{
		area = MonitorArea::VirtualScreen;
	}
	else if (StringUtil::MatchAndSkipPrefix(&start, end, L"WORKAREA"))
	{
		area = MonitorArea::Work;
	}
	else if (StringUtil::MatchAndSkipPrefix(&start, end, L"SCREENAREA"))
	{
		area = MonitorArea::Screen;
	}
	else
	{
		return false;
	}

	const WCHAR* componentEnd = end;
	for (const WCHAR* ch = start; ch != end; ++ch)
	{
		if (*ch == L'@')
		{
			if (primary || area == MonitorArea::VirtualScreen) return false;

			componentEnd = ch;
			if (!ParseMonitorSelector(ch + 1, end, autoSelect, monitorNumber)) return false;
			break;
		}
	}

	return ParseMonitorComponent(start, componentEnd, component);
}

int GetMonitorRectValue(const RECT& rect, MonitorComponent component)
{
	switch (component)
	{
	case MonitorComponent::X:
		return rect.left;

	case MonitorComponent::Y:
		return rect.top;

	case MonitorComponent::Width:
		return rect.right - rect.left;

	case MonitorComponent::Height:
		return rect.bottom - rect.top;
	}

	return 0;
}

}  // namespace

ankerl::unordered_dense::map<ConfigParser::VariableType, WCHAR> ConfigParser::c_VariableMap;

ConfigParser::ConfigParser() :
	m_LastReplaced(false),
	m_LastDefaultUsed(false),
	m_LastValueDefined(false),
	m_MonitorVariableMode(MonitorVariableMode::DEFAULT_LOGICAL),
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
	m_MonitorVariableMode = MonitorVariableMode::DEFAULT_LOGICAL;

	m_CurrentSection.clear();
	m_SectionInsertPos = m_Sections.end();

	// Set the built-in variables. Do this before the ini file is read so that the paths can be used with @include
	SetBuiltInVariables(filename, resourcePath, skin);

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
** Gets a value for the variable. Returns false if not found.
**
*/
bool ConfigParser::GetVariable(const std::wstring& strVariable, std::wstring& strValue)
{
	const std::wstring strTmp = StrToUpper(strVariable);

	// #1: Built-in variables
	auto iter = m_BuiltInVariables.find(strTmp);
	if (iter != m_BuiltInVariables.end())
	{
		strValue = (*iter).second;
		return true;
	}

	static const std::wstring s_CurrentSectionStr = L"CURRENTSECTION";
	if (strTmp == s_CurrentSectionStr)
	{
		strValue = m_CurrentSection;
		return true;
	}

	// #2: Current config variables
	if (GetCurrentConfigVariable(strTmp, strValue))
	{
		return true;
	}

	// #3: Monitor variables
	if (GetMonitorVariable(strTmp, strValue))
	{
		return true;
	}

	// #4: User-defined variables
	iter = m_Variables.find(strTmp);
	if (iter != m_Variables.end())
	{
		strValue = (*iter).second;
		return true;
	}

	return false;
}

const std::wstring* ConfigParser::GetVariableOriginalName(const std::wstring& strVariable)
{
	const std::wstring strTmp = StrToUpper(strVariable);

	// User-defined variables
	auto iter = m_OriginalVariableNames.find(strTmp);
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
		// [Meter:X], [Meter:Y], [Meter:W], [Meter:H]
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
			// Check if calling a Script/Plugin measure
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
			}
			while (0);
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

bool ConfigParser::GetCurrentConfigVariable(const std::wstring& strVariable, std::wstring& strValue)
{
	if (!m_Skin) return false;

	constexpr WCHAR currentConfig[] = L"CURRENTCONFIG";
	const WCHAR* start = strVariable.c_str();
	const WCHAR* end = start + strVariable.length();
	const WCHAR* componentStart = start + (_countof(currentConfig) - 1);
	if (end < componentStart || !MatchRange(start, componentStart, currentConfig))
	{
		return false;
	}

	MonitorComponent component = MonitorComponent::X;
	if (!ParseMonitorComponent(componentStart, end, component))
	{
		return false;
	}

	int value = 0;
	switch (component)
	{
	case MonitorComponent::X:
		value = m_Skin->GetLogicalWindowPosition().x;
		break;

	case MonitorComponent::Y:
		value = m_Skin->GetLogicalWindowPosition().y;
		break;

	case MonitorComponent::Width:
		value = m_Skin->GetCurrentConfigW();
		break;

	case MonitorComponent::Height:
		value = m_Skin->GetCurrentConfigH();
		break;
	}

	WCHAR buffer[16] = { 0 };
	_itow_s(value, buffer, 10);
	strValue = buffer;
	return true;
}

bool ConfigParser::GetMonitorVariable(const std::wstring& strVariable, std::wstring& strValue)
{
	bool physical = false;
	bool primary = false;
	bool autoSelect = false;
	int monitorNumber = -1;
	MonitorArea area = MonitorArea::Screen;
	MonitorComponent component = MonitorComponent::X;
	if (!ParseMonitorVariable(strVariable.c_str(), physical, area, component, primary, autoSelect, monitorNumber))
	{
		return false;
	}
	if (m_MonitorVariableMode == MonitorVariableMode::FORCE_PHYSICAL)
	{
		physical = true;
	}

	const auto& monitorsInfo = MonitorUtil::GetMultiMonitorInfo();
	const auto& monitors = monitorsInfo.monitors;
	if (monitors.empty())
	{
		return false;
	}

	RECT rect = {};
	if (area == MonitorArea::VirtualScreen)
	{
		rect = physical ? monitorsInfo.virtualScreen : monitorsInfo.logicalVirtualScreen;
	}
	else
	{
		int screenIndex = monitorsInfo.primary;
		if (monitorNumber > 0)
		{
			screenIndex = monitorNumber;
		}
		else if (autoSelect)
		{
			if (!m_Skin) return false;
			screenIndex = monitorsInfo.MonitorIndexForWindow(m_Skin->GetWindow());
		}
		else if (!primary && m_Skin)
		{
			const bool horizontal = component == MonitorComponent::X || component == MonitorComponent::Width;
			if (horizontal && m_Skin->GetX().monitor)
			{
				const int i = *m_Skin->GetX().monitor;
				if (i >= 0 && (i == 0 || i <= (int)monitors.size() && monitors[i - 1].active))
				{
					screenIndex = i;
				}
			}
			else if (!horizontal && m_Skin->GetY().monitor)
			{
				const int i = *m_Skin->GetY().monitor;
				if (i >= 0 && (i == 0 || i <= (int)monitors.size() && monitors[i - 1].active))
				{
					screenIndex = i;
				}
			}
		}

		if (screenIndex == 0)
		{
			rect = physical ? monitorsInfo.virtualScreen : monitorsInfo.logicalVirtualScreen;
		}
		else
		{
			int monitorIndex = screenIndex - 1;
			const int primaryIndex = monitorsInfo.primary - 1;
			if (monitorIndex < 0 || monitorIndex >= (int)monitors.size())
			{
				return false;
			}

			if (!monitors[monitorIndex].active)
			{
				if (primaryIndex < 0 || primaryIndex >= (int)monitors.size())
				{
					return false;
				}

				monitorIndex = primaryIndex;
			}

			const auto& monitor = monitors[monitorIndex];
			rect =
				physical ?
				((area == MonitorArea::Work) ? monitor.work : monitor.screen) :
				((area == MonitorArea::Work) ? monitor.logicalWork : monitor.logicalScreen);
		}
	}

	WCHAR buffer[32] = { 0 };
	_itow_s(GetMonitorRectValue(rect, component), buffer, 10);
	strValue = buffer;
	return true;
}

/*
** Replaces environment and internal variables in the given string.
**
*/
bool ConfigParser::ReplaceVariables(std::wstring& result, bool isNewStyle)
{
	bool replaced = false;

	PathUtil::ExpandEnvironmentVariables(result);

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
				std::wstring value;
				if (GetVariable(L"CURRENTSECTION", value))
				{
					// Variable found, replace it with the value
					result.replace(start, length, value);
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
					std::wstring value;
					if (GetVariable(strVariable, value))
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1ULL, value);
						start += value.length();
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
	if (m_Skin && (m_CurrentSection.empty() || meter))
	{
		Section* section = m_Skin->GetCurrentActionSection();
		if (section || meter)
		{
			m_CurrentSection.assign(meter ? meter->GetName() : section->GetName());
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
			LogErrorSF(m_Skin, m_CurrentSection.c_str(),
				L"Parsing Error: Maximum number of variable replacements reached (%llu) in string: %s", maxReplacements, str.c_str());
			if (GetRainmeter().GetDebug())
			{
				LogDebugSF(m_Skin, m_CurrentSection.c_str(), L"Parsing Error: Result: %s", result.c_str());
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
				LogErrorSF(m_Skin, m_CurrentSection.c_str(),
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
						std::wstring value;
						if (GetVariable(variable, value))
						{
							foundValue.assign(value);
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
		LogWarningSF(m_Skin, m_CurrentSection.c_str(), L"Warning: Potential self-referenced variable: %s", selfReferencedVariable.c_str());
		if (GetRainmeter().GetDebug())
		{
			LogDebugSF(m_Skin, m_CurrentSection.c_str(), L"Original string: %s", str.c_str());
			LogDebugSF(m_Skin, m_CurrentSection.c_str(), L"Replaced string: %s", result.c_str());
		}
	}

	// Reset the current section
	if (hasCurrentAction)
	{
		m_CurrentSection.clear();
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
	if (m_Skin)
	{
		pt = m_Skin->PhysicalToRelativeLogical(pt);
	}

	if (_wcsnicmp(var, L"MOUSEX", 6) == 0)
	{
		var += 6;
		int xOffset = meter ? meter->GetX() : 0;
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
		int yOffset = meter ? meter->GetY() : 0;
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
		m_CurrentSection.assign(strSection);  // Set temporarily
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
		m_CurrentSection.clear();  // Reset
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
	auto iter = m_Measures.find(StrToUpper(name));
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

std::vector<std::wstring> ConfigParser::Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	return ParseUtil::Tokenize(str, delimiters);
}

std::vector<std::wstring> ConfigParser::TokenizeWithPairedPunctuation(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct)
{
	return ParseUtil::TokenizeWithPairedPunctuation(str, delimiter, punct);
}

double ConfigParser::ParseDouble(LPCTSTR str, double defValue)
{
	return ParseUtil::ParseDouble(str, defValue, LogFormulaError);
}

int ConfigParser::ParseInt(LPCTSTR str, int defValue)
{
	return ParseUtil::ParseInt(str, defValue, LogFormulaError);
}

uint32_t ConfigParser::ParseUInt(LPCTSTR str, uint32_t defValue)
{
	return ParseUtil::ParseUInt(str, defValue, LogFormulaError);
}

uint64_t ConfigParser::ParseUInt64(LPCTSTR str, uint64_t defValue)
{
	return ParseUtil::ParseUInt64(str, defValue, LogFormulaError);
}

D2D1_COLOR_F ConfigParser::ParseColor(LPCTSTR str)
{
	return ParseUtil::ParseColor(str, LogFormulaError);
}

D2D1_RECT_F ConfigParser::ParseRect(LPCTSTR str)
{
	return ParseUtil::ParseRect(str, LogFormulaError);
}

RECT ConfigParser::ParseRECT(LPCTSTR str)
{
	return ParseUtil::ParseRECT(str, LogFormulaError);
}

/*
** Reads the given ini file and fills the m_Values and m_Keys maps.
**
*/
void ConfigParser::ReadIniFile(const std::wstring& iniFile, LPCTSTR skinSection, int depth)
{
	if (depth > 100)	// Is 100 enough to assume the include loop never ends?
	{
		GetRainmeter().ShowMessage(nullptr, GetString(IDS_IncludeInfiniteLoop), MB_OK | MB_ICONERROR);
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
	ankerl::unordered_dense::set<std::wstring> unique;
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

	auto iter = m_Values.find(StrToUpperC(strTmp));
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

	auto iter = m_Values.find(StrToUpperC(strTmp));
	return (iter != m_Values.end()) ? (*iter).second : strDefault;
}
