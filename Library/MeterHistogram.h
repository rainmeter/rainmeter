/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERHISTOGRAM_H__
#define __METERHISTOGRAM_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterHistogram : public Meter
{
public:
	MeterHistogram(Skin* skin, const WCHAR* name);
	virtual ~MeterHistogram();

	MeterHistogram(const MeterHistogram& other) = delete;
	MeterHistogram& operator=(MeterHistogram other) = delete;

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
