/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURETIME_H__
#define __MEASURETIME_H__

#include "Measure.h"

class MeasureTime : public Measure
{
public:
	MeasureTime(Skin* skin, const WCHAR* name);
	virtual ~MeasureTime();

	MeasureTime(const MeasureTime& other) = delete;
	MeasureTime& operator=(MeasureTime other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureTime>(); }

	virtual const WCHAR* GetStringValue();

	void UpdateDelta();

	LARGE_INTEGER GetTimeStamp() { return m_Time; }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	enum TIMESTAMP_TYPE
	{
		FIXED,
		DST_END,
		DST_START,
		DST_NEXT_END,
		DST_NEXT_START,
		INVALID
	};

	void TimeToString(WCHAR* buf, size_t bufLen, const WCHAR* format, const tm* time);
	void FillCurrentTime();
	void FreeLocale();

	std::wstring m_Format;
	_locale_t m_FormatLocale;

	LARGE_INTEGER m_Delta;
	LARGE_INTEGER m_Time;

	double m_TimeStamp;
	TIMESTAMP_TYPE m_TimeStampType;

	double m_TimeZone;
	bool m_DaylightSavingTime;
};

#endif
