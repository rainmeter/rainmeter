/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREITUNES_H_
#define RM_LIBRARY_MEASUREITUNES_H_

#include "Measure.h"

enum COMMAND_TYPE : int;

class MeasureiTunes : public Measure
{
public:
	MeasureiTunes(Skin* skin, const WCHAR* name);
	virtual ~MeasureiTunes();

	MeasureiTunes(const MeasureiTunes& other) = delete;
	MeasureiTunes& operator=(MeasureiTunes other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureiTunes>(); }

	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	COMMAND_TYPE m_Command;
	std::wstring m_BaseDir;
	std::wstring m_DefaultTrackArtworkPath;
	std::wstring m_CurrentTrackArtworkPath;
	std::wstring m_StringValue;
};

#endif
