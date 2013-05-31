/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

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
