/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURELOOP_H__
#define __MEASURELOOP_H__

#include "Measure.h"

class MeasureLoop : public Measure
{
public:
	MeasureLoop(Skin* skin, const WCHAR* name);
	virtual ~MeasureLoop();

	MeasureLoop(const MeasureLoop& other) = delete;
	MeasureLoop& operator=(MeasureLoop other) = delete;

	virtual void Command(const std::wstring& command);

	virtual UINT GetTypeID() { return TypeID<MeasureLoop>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	void Reset();

	int m_StartValue;
	int m_EndValue;
	int m_Increment;

	int m_LoopCount;
	int m_LoopCounter;

	bool m_SkipFirst;
	bool m_HasOverRun;
};

#endif
