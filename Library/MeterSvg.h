/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Meter.h"
#include "AspectRatioMode.h"
#include <memory>

namespace Gfx {
class D2DSvg;
}

class MeterSvg : public Meter
{
public:
	MeterSvg(Skin* skin, const WCHAR* name);
	virtual ~MeterSvg();

	MeterSvg(const MeterSvg& other) = delete;
	MeterSvg& operator=(MeterSvg other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterSvg>(); }

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual bool IsFixedSize(bool overwrite = false)
	{
		return overwrite ? true : m_SvgImage.empty();
	}

private:
	bool LoadSvg();
	void UpdateSize();

	std::wstring m_SvgImage;
	std::unique_ptr<Gfx::D2DSvg> m_Svg;
	AspectRatioMode m_AspectRatioMode;
	bool m_LoadAttempted;
	bool m_NeedsRedraw;
};
