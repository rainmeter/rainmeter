/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURERESMON_H_
#define RM_LIBRARY_MEASURERESMON_H_

#include "Measure.h"

class MeasureResMon : public Measure
{
public:
	MeasureResMon(Skin* skin, const WCHAR* name);
	virtual ~MeasureResMon();

	MeasureResMon(const MeasureResMon& other) = delete;
	MeasureResMon& operator=(MeasureResMon other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureResMon>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class Type
	{
		GDI,
		USER,
		HANDLE,
		WINDOW
	};

	Type m_Type;
	std::wstring m_ProcessName;
};

#endif
