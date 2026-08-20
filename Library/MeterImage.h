// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "GeneralImage.h"
#include "AspectRatioMode.h"

class MeterImage : public Meter
{
public:
	MeterImage(Skin* skin, const WCHAR* name);
	virtual ~MeterImage();

	MeterImage(const MeterImage& other) = delete;
	MeterImage& operator=(MeterImage other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterImage>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void BindMeasures(ConfigParser& parser, std::wstring_view section) override;

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite ? true : m_ImageName.empty(); }

private:
	void LoadImage(const std::wstring& imageName, bool bLoadAlways);

	GeneralImage m_Image;
	std::wstring m_ImageName;
	std::wstring m_ImageNameResult;

	GeneralImage m_MaskImage;
	std::wstring m_MaskImageName;

	bool m_NeedsRedraw;
	bool m_Tile;
	AspectRatioMode m_AspectRatioMode;

	RECT m_ScaleMargins;

	static const WCHAR* c_MaskOptionArray[GeneralImage::OptionCount];
};
