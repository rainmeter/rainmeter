/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREFILEVIEW_H_
#define RM_LIBRARY_MEASUREFILEVIEW_H_

#include "Measure.h"

struct ChildMeasure;

class MeasureFileView : public Measure
{
public:
	MeasureFileView(Skin* skin, const WCHAR* name);
	virtual ~MeasureFileView();

	MeasureFileView(const MeasureFileView& other) = delete;
	MeasureFileView& operator=(MeasureFileView other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureFileView>(); }
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ChildMeasure* m_Child;
};

#endif
