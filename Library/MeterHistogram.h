// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "GeneralImage.h"

class MeterHistogram : public Meter
{
public:
	MeterHistogram(Skin* skin, const WCHAR* name);
	virtual ~MeterHistogram();

	MeterHistogram(const MeterHistogram& other) = delete;
	MeterHistogram& operator=(MeterHistogram other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterHistogram>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return m_PrimaryImageName.empty(); }

private:
	void DisposeBuffer();
	void CreateBuffer();

	D2D1_COLOR_F m_PrimaryColor;
	D2D1_COLOR_F m_SecondaryColor;
	D2D1_COLOR_F m_OverlapColor;

	int m_MeterPos;							// New value placement position
	bool m_Autoscale;
	bool m_Flip;

	std::wstring m_PrimaryImageName;
	std::wstring m_SecondaryImageName;
	std::wstring m_OverlapImageName;

	GeneralImage m_PrimaryImage;
	GeneralImage m_SecondaryImage;
	GeneralImage m_OverlapImage;

	double* m_PrimaryValues;
	double* m_SecondaryValues;

	double m_MaxPrimaryValue;
	double m_MinPrimaryValue;
	double m_MaxSecondaryValue;
	double m_MinSecondaryValue;

	bool m_SizeChanged;

	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;

	static const WCHAR* c_PrimaryOptionArray[GeneralImage::OptionCount];
	static const WCHAR* c_SecondaryOptionArray[GeneralImage::OptionCount];
	static const WCHAR* c_BothOptionArray[GeneralImage::OptionCount];
};
