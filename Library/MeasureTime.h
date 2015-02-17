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
