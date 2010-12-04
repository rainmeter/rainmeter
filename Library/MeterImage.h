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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERIMAGE_H__
#define __METERIMAGE_H__

#include "Meter.h"
#include "TintedImage.h"
#include "MeterWindow.h"

class CMeterImage : public CMeter
{
public:
	CMeterImage(CMeterWindow* meterWindow);
	virtual ~CMeterImage();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(const std::list<CMeasure*>& measures);

protected:
	void LoadImage(bool bLoadAlways);

	CTintedImage m_Image;
	std::wstring m_ImageName;			// Name of the image
	std::wstring m_Path;

	bool m_NeedsReload;
	bool m_WidthDefined;
	bool m_HeightDefined;
	bool m_PreserveAspectRatio;			// If true, aspect ratio of the image is preserved when the image is scaled
	bool m_Tile;
};

#endif
