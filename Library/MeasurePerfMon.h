/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREPERFMON_H_
#define RM_LIBRARY_MEASUREPERFMON_H_

#include "Measure.h"

class MeasurePerfMon : public Measure
{
public:
	MeasurePerfMon(Skin* skin, const WCHAR* name);
	virtual ~MeasurePerfMon();

	MeasurePerfMon(const MeasurePerfMon& other) = delete;
	MeasurePerfMon& operator=(MeasurePerfMon other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePerfMon>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ULONGLONG GetPerfData(const WCHAR* objectName, const WCHAR* instanceName, const WCHAR* counterName);

	std::wstring m_ObjectName;
	std::wstring m_CounterName;
	std::wstring m_InstanceName;
	ULONGLONG m_OldValue;
	bool m_Difference;
	bool m_FirstTime;
};

#endif
