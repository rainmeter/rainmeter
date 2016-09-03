/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURENETTOTAL_H__
#define __MEASURENETTOTAL_H__

#include "MeasureNet.h"

class MeasureNetTotal : public MeasureNet
{
public:
	MeasureNetTotal(Skin* skin, const WCHAR* name);
	virtual ~MeasureNetTotal();

	MeasureNetTotal(const MeasureNetTotal& other) = delete;
	MeasureNetTotal& operator=(MeasureNetTotal other) = delete;
};

#endif
