/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREMEDIAKEY_H_
#define RM_LIBRARY_MEASUREMEDIAKEY_H_

#include "Measure.h"

class MeasureMediaKey : public Measure
{
public:
	MeasureMediaKey(Skin* skin, const WCHAR* name);
	virtual ~MeasureMediaKey();

	MeasureMediaKey(const MeasureMediaKey& other) = delete;
	MeasureMediaKey& operator=(MeasureMediaKey other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureMediaKey>(); }

protected:
	void UpdateValue() override {};
	void Command(const std::wstring& command) override;
};

#endif
