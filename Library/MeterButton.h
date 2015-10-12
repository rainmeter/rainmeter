/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERBUTTON_H__
#define __METERBUTTON_H__

#include "Meter.h"
#include "TintedImage.h"

#define BUTTON_FRAMES 3

class MeterButton : public Meter
{
public:
	MeterButton(Skin* skin, const WCHAR* name);
	virtual ~MeterButton();

	MeterButton(const MeterButton& other) = delete;
	MeterButton& operator=(MeterButton other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterButton>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	bool MouseMove(POINT pos);
	bool MouseUp(POINT pos, bool execute);
	bool MouseDown(POINT pos);

	void SetFocus(bool f) { m_Focus = f; }

	bool HitTest2(int px, int py);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);
	
	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

private:
	TintedImage m_Image;
	std::wstring m_ImageName;
	bool m_NeedsReload;

	Gdiplus::CachedBitmap* m_Bitmaps[BUTTON_FRAMES];	// Cached bitmaps
	std::wstring m_Command;
	int m_State;
	bool m_Clicked;
	bool m_Focus;
};

#endif
