/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURE_H__
#define __MEASURE_H__

#include <windows.h>
#include <vector>
#include <string>
#include "IfActions.h"
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
	MeasureValueSet(double val, const WCHAR* str) : m_Value(val), m_StringValue(str) {}
	void Set(double val, const WCHAR* str) { m_Value = val; m_StringValue = str; }
	bool IsChanged(double val, const WCHAR* str) { if (m_Value != val || wcscmp(m_StringValue.c_str(), str) != 0) { Set(val, str); return true; } return false; }
private:
	double m_Value;
	std::wstring m_StringValue;
};

class Meter;
class Skin;
class ConfigParser;

class __declspec(novtable) Measure : public Section
{
public:
	virtual ~Measure();

	Measure(const Measure& other) = delete;

	void ReadOptions(ConfigParser& parser) { ReadOptions(parser, GetName()); }

	virtual void Initialize();
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

	virtual const WCHAR* GetStringValue();
	const WCHAR* GetStringOrFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);
	const WCHAR* GetFormattedValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);

	static void GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords);
	static void RemoveTrailingZero(WCHAR* str, int strLen);

	const std::wstring& GetOnChangeAction() { return m_OnChangeAction; }
	void DoChangeAction(bool execute = true);

	static Measure* Create(const WCHAR* measure, Skin* skin, const WCHAR* name);
	static bool GetCurrentMeasureValue(const WCHAR* str, int len, double* value, void* context);

protected:
	Measure(Skin* skin, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue() = 0;

	bool ParseSubstitute(std::wstring buffer);
	std::wstring ExtractWord(std::wstring& buffer);
	const WCHAR* CheckSubstitute(const WCHAR* buffer);
	bool MakePlainSubstitute(std::wstring& str, size_t index);

	double m_Value;

	bool m_Invert;
	
	bool m_LogMaxValue;
	double m_MinValue;
	double m_MaxValue;
	bool m_MinValueDefined;
	bool m_MaxValueDefined;

	std::vector<std::wstring> m_Substitute;
	bool m_RegExpSubstitute;

	std::vector<double> m_MedianValues;	// The values for the median filtering
	UINT m_MedianPos;					// Position in the median array, where the new value is placed

	std::vector<double> m_AverageValues;
	UINT m_AveragePos;
	UINT m_AverageSize;

	IfActions m_IfActions;

	bool m_Disabled;
	bool m_Paused;
	bool m_Initialized;

	std::wstring m_OnChangeAction;
	MeasureValueSet* m_OldValue;
	bool m_ValueAssigned;
};

#endif
