/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREUPTIME_H__
#define __MEASUREUPTIME_H__

#include "Measure.h"

class MeasureUptime : public Measure
{
public:
	MeasureUptime(Skin* skin, const WCHAR* name);
	virtual ~MeasureUptime();

	MeasureUptime(const MeasureUptime& other) = delete;
	MeasureUptime& operator=(MeasureUptime other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureUptime>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_AddDaysToHours;
	std::wstring m_Format;
	double m_Seconds;
	bool m_SecondsDefined;
};

#endif
