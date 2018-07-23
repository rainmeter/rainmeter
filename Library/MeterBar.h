/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERBAR_H__
#define __METERBAR_H__

#include "Meter.h"
#include "GeneralImage.h"

class MeterBar : public Meter
{
public:
	MeterBar(Skin* skin, const WCHAR* name);
	virtual ~MeterBar();

	MeterBar(const MeterBar& other) = delete;
	MeterBar& operator=(MeterBar other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterBar>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return !m_Image.IsLoaded(); }

private:
	enum ORIENTATION
	{
		HORIZONTAL,
		VERTICAL
	};

	GeneralImage m_Image;
	std::wstring m_ImageName;

	D2D1_COLOR_F m_Color;
	ORIENTATION m_Orientation;	// Growth direction
	double m_Value;
	int m_Border;
	bool m_Flip;
};

#endif
