/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERROUNDLINE_H__
#define __METERROUNDLINE_H__

#include "Meter.h"
#include <memory>

class MeterRoundLine : public Meter
{
public:
	MeterRoundLine(Skin* skin, const WCHAR* name);
	virtual ~MeterRoundLine();

	MeterRoundLine(const MeterRoundLine& other) = delete;
	MeterRoundLine& operator=(MeterRoundLine other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterRoundLine>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	bool m_Solid;
	double m_LineWidth;
	double m_LineLength;
	double m_LineStart;
	double m_StartAngle;
	double m_RotationAngle;
	bool m_CntrlAngle;
	bool m_CntrlLineStart;
	bool m_CntrlLineLength;
	double m_LineStartShift;
	double m_LineLengthShift;
	UINT m_ValueRemainder;
	Gdiplus::Color m_LineColor;
	double m_Value;
};

#endif
