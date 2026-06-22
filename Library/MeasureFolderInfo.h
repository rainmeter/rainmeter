/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREFOLDERINFO_H_
#define RM_LIBRARY_MEASUREFOLDERINFO_H_

#include "Measure.h"

struct FolderInfoParentMeasure;

class MeasureFolderInfo : public Measure
{
public:
	MeasureFolderInfo(Skin* skin, const WCHAR* name);
	virtual ~MeasureFolderInfo();

	MeasureFolderInfo(const MeasureFolderInfo& other) = delete;
	MeasureFolderInfo& operator=(MeasureFolderInfo other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureFolderInfo>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class Type;

	FolderInfoParentMeasure* m_Parent;
	Type m_Type;
};

#endif
