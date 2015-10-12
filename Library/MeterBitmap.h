/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERBITMAP_H__
#define __METERBITMAP_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterBitmap : public Meter
{
public:
	MeterBitmap(Skin* skin, const WCHAR* name);
	virtual ~MeterBitmap();

	MeterBitmap(const MeterBitmap& other) = delete;
	MeterBitmap& operator=(MeterBitmap other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterBitmap>(); }

	virtual bool HitTest(int x, int y);

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);
	virtual bool HasActiveTransition();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

private:
	TintedImage m_Image;
	std::wstring m_ImageName;
	bool m_NeedsReload;

	bool m_ZeroFrame;			// If true, the first frame is only shown when the measured value is zero
	int m_FrameCount;
	int m_TransitionFrameCount;	// Number of transition frames (per one frame) in the bitmap
	METER_ALIGNMENT m_Align;
	bool m_Extend;				// If true, bitmaps extend horizontally and are used like numbers
	int m_Separation;
	int m_Digits;
	double m_Value;

	ULONGLONG m_TransitionStartTicks;
	double m_TransitionStartValue;
};

#endif
