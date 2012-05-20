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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERBITMAP_H__
#define __METERBITMAP_H__

#include "Meter.h"
#include "TintedImage.h"

class CMeterBitmap : public CMeter
{
public:
	CMeterBitmap(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeterBitmap();

	virtual UINT GetTypeID() { return TypeID<CMeterBitmap>(); }

	virtual bool HitTest(int x, int y);

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual bool HasActiveTransition();

protected:
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);

private:
	CTintedImage m_Image;
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
