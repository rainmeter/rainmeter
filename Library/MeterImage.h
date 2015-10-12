/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERIMAGE_H__
#define __METERIMAGE_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterImage : public Meter
{
public:
	MeterImage(Skin* skin, const WCHAR* name);
	virtual ~MeterImage();

	MeterImage(const MeterImage& other) = delete;
	MeterImage& operator=(MeterImage other) = delete;

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

	TintedImage m_MaskImage;
	std::wstring m_MaskImageName;

	bool m_NeedsRedraw;
	DRAWMODE m_DrawMode;

	RECT m_ScaleMargins;

	static const WCHAR* c_MaskOptionArray[TintedImage::OptionCount];
};

#endif
