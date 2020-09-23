/* Copyright (C) 2020 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREPROCESS_H_
#define RM_LIBRARY_MEASUREPROCESS_H_

#include "Measure.h"

class MeasureProcess : public Measure
{
public:
	MeasureProcess(Skin* skin, const WCHAR* name);
	virtual ~MeasureProcess();

	MeasureProcess(const MeasureProcess& other) = delete;
	MeasureProcess& operator=(MeasureProcess other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureProcess>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	std::wstring m_ProcessNameLowercase;
};

#endif
