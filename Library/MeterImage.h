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

class CMeterImage : public CMeter
{
public:
	CMeterImage(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeterImage();

	virtual UINT GetTypeID() { return TypeID<CMeterImage>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(const std::list<CMeasure*>& measures);

protected:
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);

private:
	void LoadImage(const std::wstring& imageName, bool bLoadAlways);

	CTintedImage m_Image;
	std::wstring m_ImageName;
	std::wstring m_ImageNameResult;		// Image name as absolute path
	std::wstring m_Path;

	bool m_NeedsRedraw;
	bool m_PreserveAspectRatio;
	bool m_Tile;

	RECT m_ScaleMargins;

	std::vector<std::wstring> m_MeasureNames;
	std::vector<CMeasure*> m_Measures;
};

#endif
