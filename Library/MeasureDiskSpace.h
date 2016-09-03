/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREDISKSPACE_H__
#define __MEASUREDISKSPACE_H__

#include "Measure.h"

class MeasureDiskSpace : public Measure
{
public:
	MeasureDiskSpace(Skin* skin, const WCHAR* name);
	virtual ~MeasureDiskSpace();

	MeasureDiskSpace(const MeasureDiskSpace& other) = delete;
	MeasureDiskSpace& operator=(MeasureDiskSpace other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureDiskSpace>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_Drive;
	std::wstring m_StringValue;
	bool m_Type;
	bool m_Total;
	bool m_Label;
	bool m_IgnoreRemovable;
	bool m_DiskQuota;

	ULONGLONG m_OldTotalBytes;
};

#endif
