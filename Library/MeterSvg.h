// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "AspectRatioMode.h"
#include <memory>

namespace Gfx {
class Svg;
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
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);
	virtual bool IsFixedSize(bool overwrite = false)
	{
		return overwrite ? true : m_SvgImage.empty();
	}

private:
	bool LoadSvg();
	void UpdateSize();

	std::wstring m_SvgImage;
	std::unique_ptr<Gfx::Svg> m_Svg;
	AspectRatioMode m_AspectRatioMode;
	bool m_LoadAttempted;
	bool m_NeedsRedraw;
};
