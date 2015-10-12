/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERROTATOR_H__
#define __METERROTATOR_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterRotator : public Meter
{
public:
	MeterRotator(Skin* skin, const WCHAR* name);
	virtual ~MeterRotator();

	MeterRotator(const MeterRotator& other) = delete;
	MeterRotator& operator=(MeterRotator other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterRotator>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

private:
	TintedImage m_Image;
	std::wstring m_ImageName;
	bool m_NeedsReload;

	double m_OffsetX;
	double m_OffsetY;
	double m_StartAngle;
	double m_RotationAngle;
	UINT m_ValueRemainder;
	double m_Value;
};

#endif
