/*
  Copyright (C) 2004 Kimmo Pekkola

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

#ifndef __MEASURECALC_H__
#define __MEASURECALC_H__

#include "Measure.h"
#include <random>

class MeasureCalc : public Measure
{
public:
	MeasureCalc(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureCalc();

	virtual UINT GetTypeID() { return TypeID<MeasureCalc>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	static bool GetMeasureValue(const WCHAR* str, int len, double* value, void* context);

	void FormulaReplace();
	int GetRandom();

	std::wstring m_Formula;
	bool m_ParseError;

	int m_LowBound;
	int m_HighBound;

	bool m_UpdateRandom;
	bool m_UniqueRandom;

	std::vector<int> m_UniqueNumbers;
	void UpdateUniqueNumberList();

	// Uniform Random Number Generator
	std::default_random_engine m_Engine;
	std::uniform_int_distribution<int> m_Distrubtion;
};

#endif
