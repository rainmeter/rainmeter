/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOWPLAYING_H__
#define __NOWPLAYING_H__

#include "Player.h"

struct ParentMeasure
{
	ParentMeasure() :
		player(),
		data(),
		skin(),
		ownerName(),
		measureCount(1),
		trackCount(0),
		disableLeadingZero(false)
	{}

	Player* player;
	void* data;
	void* skin;
	LPCWSTR ownerName;
	std::wstring trackChangeAction;
	std::wstring playerPath;
	UINT measureCount;
	UINT trackCount;
	bool disableLeadingZero;
};

struct Measure
{
	Measure() :
		parent(),
		type(MEASURE_NONE)
	{}

	ParentMeasure* parent;
	MeasureType type;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);

#endif
