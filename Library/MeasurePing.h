/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREPING_H_
#define RM_LIBRARY_MEASUREPING_H_

#include "Measure.h"

struct PingData;

class MeasurePing : public Measure
{
public:
	MeasurePing(Skin* skin, const WCHAR* name);
	virtual ~MeasurePing();

	MeasurePing(const MeasurePing& other) = delete;
	MeasurePing& operator=(MeasurePing other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePing>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	PingData* m_Data;
};

#endif
