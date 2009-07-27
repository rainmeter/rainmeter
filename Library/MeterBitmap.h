/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERBITMAP_H__
#define __METERBITMAP_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterBitmap : public CMeter
{
public:
	CMeterBitmap(CMeterWindow* meterWindow);
	virtual ~CMeterBitmap();

	virtual bool HitTest(int x, int y);

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual bool HasActiveTransition();

private:
	bool m_ZeroFrame;			// If true, the first frame is only shown when the measured value is zero
	int m_FrameCount;			// Number of frames in the bitmap
	int m_TransitionFrameCount;	// Number of transition frames (per one frame) in the bitmap
	Gdiplus::Bitmap* m_Bitmap;	// Handle to the bitmap
	std::wstring m_ImageName;	// Name of the image
	METER_ALIGNMENT m_Align;	// Alignment of the bitmaps
	bool m_Extend;				// If true, bitmaps extend horizontally and are used like numbers
	int m_Separation;
	int m_Digits;
	double m_Value;

	DWORD m_TransitionStartTicks;
	double m_TransitionStartValue;
};

#endif
