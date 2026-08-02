/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

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
