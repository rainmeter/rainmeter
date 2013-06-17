/*
  Copyright (C) 2002 Kimmo Pekkola

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

#ifndef __METERIMAGE_H__
#define __METERIMAGE_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterImage : public Meter
{
public:
	MeterImage(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterImage();

	virtual UINT GetTypeID() { return TypeID<MeterImage>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);
	
	virtual bool IsFixedSize(bool overwrite = false) { return overwrite ? true : m_ImageName.empty(); }

private:
	enum DRAWMODE
	{
		DRAWMODE_NONE = 0,
		DRAWMODE_TILE,
		DRAWMODE_KEEPRATIO,
		DRAWMODE_KEEPRATIOANDCROP
	};

	void LoadImage(const std::wstring& imageName, bool bLoadAlways);

	TintedImage m_Image;
	std::wstring m_ImageName;
	std::wstring m_ImageNameResult;

	bool m_NeedsRedraw;
	DRAWMODE m_DrawMode;

	RECT m_ScaleMargins;
};

#endif
