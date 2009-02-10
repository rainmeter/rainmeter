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
/*
  $Header: /home/cvsroot/Rainmeter/Library/Measure.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Measure.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.13  2004/07/11 17:14:20  rainy
  Fixed string generation when num of decimals is 0.

  Revision 1.12  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.11  2003/02/10 18:13:49  rainy
  Added median filter to max value.

  Revision 1.10  2002/12/23 14:26:21  rainy
  Stats are gathered a bit different way now.

  Revision 1.9  2002/05/04 08:12:32  rainy
  Measure update is not tied to the update rate directly anymore.

  Revision 1.8  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.7  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.6  2001/10/28 10:24:06  rainy
  GetStringValue uses consts.
  Added IfAbove/Below actions.
  Added Plugin and Registry Measures.

  Revision 1.5  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.4  2001/09/01 13:00:41  rainy
  Slight changes in the interface. The value is now measured only once if possible.
  Added support for logging the max value.

  Revision 1.3  2001/08/19 09:15:41  rainy
  Invert was moved here from the meter.

  Revision 1.2  2001/08/12 15:47:00  Rainy
  Adjusted Update()'s interface.
  Added GetStringValue() method.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/

#ifndef __MEASURE_H__
#define __MEASURE_H__

#include "MeterWindow.h"
#include "Litestep.h"

class CMeter;

class CMeasure
{
public:
	CMeasure(CMeterWindow* meterWindow);
	virtual ~CMeasure();
	
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update() = 0;

	virtual const WCHAR* GetStats();

	void SetName(const WCHAR* name) { m_Name = name; m_ANSIName = ConvertToAscii(name); };
	const WCHAR* GetName() { return m_Name.c_str(); };
	const char* GetANSIName() { return m_ANSIName.c_str(); };

	void Disable() { m_Disabled = true; };
	void Enable() { m_Disabled = false; };
	bool IsDisabled() { return m_Disabled; };

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

	CMeterWindow* m_MeterWindow;
};

#endif
