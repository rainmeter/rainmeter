/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURENOWPLAYING_
#define RM_LIBRARY_MEASURENOWPLAYING_

#include "Measure.h"

enum MeasureType;
struct ParentMeasure;
class Player;

class MeasureNowPlaying : public Measure
{
public:
	MeasureNowPlaying(Skin* skin, const WCHAR* name);
	virtual ~MeasureNowPlaying();

	MeasureNowPlaying(const MeasureNowPlaying& other) = delete;
	MeasureNowPlaying& operator=(MeasureNowPlaying other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureNowPlaying>(); }

	const WCHAR* GetStringValue() override;

	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ParentMeasure* m_Parent;
	MeasureType m_Type;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);

#endif
