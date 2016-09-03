/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURECALC_H__
#define __MEASURECALC_H__

#include "Measure.h"

class MeasureCalc : public Measure
{
public:
	MeasureCalc(Skin* skin, const WCHAR* name);
	virtual ~MeasureCalc();

	MeasureCalc(const MeasureCalc& other) = delete;
	MeasureCalc& operator=(MeasureCalc other) = delete;

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
};

#endif
