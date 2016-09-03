/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREVIRTUALMEMORY_H__
#define __MEASUREVIRTUALMEMORY_H__

#include "Measure.h"

class MeasureVirtualMemory : public Measure
{
public:
	MeasureVirtualMemory(Skin* skin, const WCHAR* name);
	virtual ~MeasureVirtualMemory();

	MeasureVirtualMemory(const MeasureVirtualMemory& other) = delete;
	MeasureVirtualMemory& operator=(MeasureVirtualMemory other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureVirtualMemory>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_Total;
};

#endif
