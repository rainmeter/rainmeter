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

#ifndef __METERHISTOGRAM_H__
#define __METERHISTOGRAM_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterHistogram : public Meter
{
public:
	MeterHistogram(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterHistogram();

	virtual UINT GetTypeID() { return TypeID<MeterHistogram>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return m_PrimaryImageName.empty(); }

private:
	void DisposeBuffer();
	void CreateBuffer();

	Gdiplus::Color m_PrimaryColor;
	Gdiplus::Color m_SecondaryColor;
	Gdiplus::Color m_OverlapColor;

	int m_MeterPos;							// New value placement position
	bool m_Autoscale;
	bool m_Flip;

	std::wstring m_PrimaryImageName;
	std::wstring m_SecondaryImageName;
	std::wstring m_OverlapImageName;

	TintedImage m_PrimaryImage;
	TintedImage m_SecondaryImage;
	TintedImage m_OverlapImage;

	bool m_PrimaryNeedsReload;
	bool m_SecondaryNeedsReload;
	bool m_OverlapNeedsReload;

	double* m_PrimaryValues;
	double* m_SecondaryValues;

	double m_MaxPrimaryValue;
	double m_MinPrimaryValue;
	double m_MaxSecondaryValue;
	double m_MinSecondaryValue;

	bool m_SizeChanged;
	
	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;

	static const WCHAR* c_PrimaryOptionArray[TintedImage::OptionCount];
	static const WCHAR* c_SecondaryOptionArray[TintedImage::OptionCount];
	static const WCHAR* c_BothOptionArray[TintedImage::OptionCount];
};

#endif
