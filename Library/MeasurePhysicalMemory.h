/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREPHYSICALMEMORY_H__
#define __MEASUREPHYSICALMEMORY_H__

#include "Measure.h"

class MeasurePhysicalMemory : public Measure
{
public:
	MeasurePhysicalMemory(Skin* skin, const WCHAR* name);
	virtual ~MeasurePhysicalMemory();

	MeasurePhysicalMemory(const MeasurePhysicalMemory& other) = delete;
	MeasurePhysicalMemory& operator=(MeasurePhysicalMemory other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasurePhysicalMemory>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_Total;
};

#endif
