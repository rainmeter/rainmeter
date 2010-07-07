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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __MEASURE_H__
#define __MEASURE_H__

#include "MeterWindow.h"
#include "Litestep.h"
#include "Group.h"

class CMeter;

class CMeasure : public CGroup
{
public:
	CMeasure(CMeterWindow* meterWindow);
	virtual ~CMeasure();
	
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual void Initialize();
	virtual bool Update() = 0;

	virtual const WCHAR* GetStats();

	void SetName(const WCHAR* name) { m_Name = name; m_ANSIName = ConvertToAscii(name); };
	const WCHAR* GetName() { return m_Name.c_str(); };
	const char* GetANSIName() { return m_ANSIName.c_str(); };

	void Disable() { m_Disabled = true; };
	void Enable() { m_Disabled = false; };
	bool IsDisabled() { return m_Disabled; };

	bool HasDynamicVariables() { return m_DynamicVariables; }

	virtual void ExecuteBang(const WCHAR* args);

	double GetValue();
	double GetRelativeValue();
	double GetValueRange();
	double GetMinValue() { return m_MinValue; };
	double GetMaxValue() { return m_MaxValue; };

	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);
	static void GetScaledValue(int decimals, double theValue, WCHAR* buffer);

	static CMeasure* Create(const WCHAR* measure, CMeterWindow* meterWindow);

protected:
	virtual bool PreUpdate();
	virtual bool PostUpdate();

	bool ParseSubstitute(std::wstring buffer);
	std::wstring ExtractWord(std::wstring& buffer);
	const WCHAR* CheckSubstitute(const WCHAR* buffer);

	bool m_DynamicVariables;		// If true, the measure contains dynamic variables
	bool m_Invert;					// If true, the value should be inverted
	bool m_LogMaxValue;				// If true, The maximum & minimum values are logged
	double m_MinValue;				// The minimum value (so far)
	double m_MaxValue;				// The maximum value (so far)
	double m_Value;					// The current value
	std::wstring m_Name;				// Name of this Measure
	std::string m_ANSIName;				// Name of this Measure in ANSI

	std::vector<std::wstring> m_Substitute;	// Vec of substitute strings

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
	UINT m_UpdateDivider;			// Divider for the update
	UINT m_UpdateCounter;			// Current update counter
	bool m_Initialized;

	CMeterWindow* m_MeterWindow;
};

#endif
