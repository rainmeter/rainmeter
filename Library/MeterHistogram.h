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

class CMeterHistogram : public CMeter
{
public:
	CMeterHistogram(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeterHistogram();

	virtual UINT GetTypeID() { return TypeID<CMeterHistogram>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(const std::list<CMeasure*>& measures);

protected:
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);

private:
	void DisposeBuffer();

	std::wstring m_SecondaryMeasureName;
	CMeasure* m_SecondaryMeasure;
	Gdiplus::Color m_PrimaryColor;
	Gdiplus::Color m_SecondaryColor;
	Gdiplus::Color m_OverlapColor;

	int m_MeterPos;							// New value placement position
	bool m_Autoscale;
	bool m_Flip;

	std::wstring m_PrimaryImageName;
	std::wstring m_SecondaryImageName;
	std::wstring m_OverlapImageName;

	CTintedImage m_PrimaryImage;
	CTintedImage m_SecondaryImage;
	CTintedImage m_OverlapImage;

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

	static const WCHAR* c_PrimaryOptionArray[CTintedImage::ConfigCount];
	static const WCHAR* c_SecondaryOptionArray[CTintedImage::ConfigCount];
	static const WCHAR* c_BothOptionArray[CTintedImage::ConfigCount];
};

#endif
