/*
  Copyright (C) 2001 Kimmo Pekkola

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

#ifndef __MEASURE_H__
#define __MEASURE_H__

#include <windows.h>
#include <vector>
#include <string>
#include "Litestep.h"
#include "Group.h"

enum AUTOSCALE
{
	AUTOSCALE_1024  = 1,    // scales by 1024
	AUTOSCALE_1000  = 2,    // scales by 1000

	AUTOSCALE_1024K = 101,  // scales by 1024, and uses kilo as the lowest unit
	AUTOSCALE_1000K = 102,  // scales by 1000, and uses kilo as the lowest unit

	AUTOSCALE_OFF   = 0,
	AUTOSCALE_ON    = AUTOSCALE_1024
};

class CMeter;
class CMeterWindow;
class CConfigParser;

class CMeasure : public CGroup
{
public:
	CMeasure(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeasure();

	void ReadConfig(CConfigParser& parser) { ReadConfig(parser, GetName()); }

	virtual void Initialize();
	virtual bool Update() = 0;

	const WCHAR* GetName() { return m_Name.c_str(); }
	const std::wstring& GetOriginalName() { return m_Name; }

	void Disable() { m_Disabled = true; }
	void Enable() { m_Disabled = false; }
	bool IsDisabled() { return m_Disabled; }

	bool HasDynamicVariables() { return m_DynamicVariables; }
	void SetDynamicVariables(bool b) { m_DynamicVariables = b; }

	virtual void Command(const std::wstring& command);

	double GetValue();
	double GetRelativeValue();
	double GetValueRange();
	double GetMinValue() { return m_MinValue; }
	double GetMaxValue() { return m_MaxValue; }

	void ResetUpdateCounter() { m_UpdateCounter = m_UpdateDivider; }
	int GetUpdateCounter() { return m_UpdateCounter; }
	int GetUpdateDivider() { return m_UpdateDivider; }

	virtual const WCHAR* GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);
	static void GetScaledValue(AUTOSCALE autoScale, int decimals, double theValue, WCHAR* buffer, size_t sizeInWords);
	static void RemoveTrailingZero(WCHAR* str, int strLen);

	CMeterWindow* GetMeterWindow() { return m_MeterWindow; }

	static CMeasure* Create(const WCHAR* measure, CMeterWindow* meterWindow, const WCHAR* name);

protected:
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);

	virtual bool PreUpdate();
	virtual bool PostUpdate();

	bool ParseSubstitute(std::wstring buffer);
	std::wstring ExtractWord(std::wstring& buffer);
	const WCHAR* CheckSubstitute(const WCHAR* buffer);
	bool MakePlainSubstitute(std::wstring& str, size_t index);

	bool m_DynamicVariables;		// If true, the measure contains dynamic variables
	bool m_Invert;					// If true, the value should be inverted
	bool m_LogMaxValue;				// If true, The maximum & minimum values are logged
	double m_MinValue;				// The minimum value (so far)
	double m_MaxValue;				// The maximum value (so far)
	double m_Value;					// The current value
	const std::wstring m_Name;		// Name of this Measure

	std::vector<std::wstring> m_Substitute;	// Vec of substitute strings
	bool m_RegExpSubstitute;

	std::vector<double> m_MedianMaxValues;	// The values for the median filtering
	std::vector<double> m_MedianMinValues;	// The values for the median filtering
	UINT m_MedianPos;				// Position in the median array, where the new value is placed

	std::vector<double> m_AverageValues;
	UINT m_AveragePos;
	UINT m_AverageSize;

	double m_IfEqualValue;			// The limit for the IfEqual action
	double m_IfAboveValue;			// The limit for the IfAbove action
	double m_IfBelowValue;			// The limit for the IfBelow action
	std::wstring m_IfEqualAction;	// The IfEqual action
	std::wstring m_IfAboveAction;	// The IfAbove action
	std::wstring m_IfBelowAction;	// The IfBelow action
	bool m_IfEqualCommited;			// True when the IfEqual action is executed.
	bool m_IfAboveCommited;			// True when the IfAbove action is executed.
	bool m_IfBelowCommited;			// True when the IfBelow action is executed.
	bool m_Disabled;				// Status of the measure
	int m_UpdateDivider;			// Divider for the update
	int m_UpdateCounter;			// Current update counter
	bool m_Initialized;

	CMeterWindow* m_MeterWindow;
};

#endif
