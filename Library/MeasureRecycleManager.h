/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURERECYCLEMANAGER_H_
#define RM_LIBRARY_MEASURERECYCLEMANAGER_H_

#include "Measure.h"

class MeasureRecycleManager : public Measure
{
public:
	MeasureRecycleManager(Skin* skin, const WCHAR* name);
	virtual ~MeasureRecycleManager();

	MeasureRecycleManager(const MeasureRecycleManager& other) = delete;
	MeasureRecycleManager& operator=(MeasureRecycleManager other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureRecycleManager>(); }

	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class Type;
	Type m_Type;
};

#endif
