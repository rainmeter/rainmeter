// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterSvg.h"
#include "ConfigParser.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "../Common/Gfx/Canvas.h"
#include "../Common/Gfx/Svg.h"
#include "../Common/StringParser.h"

MeterSvg::MeterSvg(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_AspectRatioMode(AspectRatioMode::Stretch),
	m_LoadAttempted(false),
	m_NeedsRedraw(false)
{
}

MeterSvg::~MeterSvg()
{
}

void MeterSvg::Initialize()
{
	Meter::Initialize();
	LoadSvg();
}

void MeterSvg::InvalidateDeviceResources()
{
	Meter::InvalidateDeviceResources();
	if (m_Svg) m_Svg->InvalidateDeviceResources();
	m_LoadAttempted = false;
}

void MeterSvg::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	const std::wstring svgImage = parser.ReadString(section, L"SvgImage", L"");
	const bool sourceChanged = svgImage != m_SvgImage;
	m_SvgImage = svgImage;

	std::vector<SvgAttribute> svgAttributes;
	for (size_t i = 1; ; ++i)
	{
		WCHAR option[64];
		_snwprintf_s(option, _TRUNCATE, i == 1 ? L"Attribute" : L"Attribute%zu", i);
		const std::wstring& value = parser.ReadString(section, option, L"");
		if (value.empty()) break;

		StringParser stringParser(value);
		const std::wstring_view selector = stringParser.ConsumeUntil(L'|', StringParser::SkipWhitespace);
		const std::wstring_view attribute = stringParser.ConsumeUntil(L'|', StringParser::SkipWhitespace);
		const std::wstring_view attributeValue = stringParser.ConsumeRest(StringParser::SkipWhitespace);
		if (selector.empty() || attribute.empty())
		{
			LogErrorF(this, L"Invalid parameters: %s", option);
			continue;
		}

		svgAttributes.push_back({ std::wstring(selector), std::wstring(attribute), std::wstring(attributeValue) });
	}

	bool attributesRequireReload = m_SvgAttributes.size() > svgAttributes.size();
	for (size_t i = 0; !attributesRequireReload && i < m_SvgAttributes.size(); ++i)
	{
		attributesRequireReload =
			m_SvgAttributes[i].selector != svgAttributes[i].selector ||
			m_SvgAttributes[i].attribute != svgAttributes[i].attribute;
	}
	m_SvgAttributes = std::move(svgAttributes);

	m_AspectRatioMode = ParseAspectRatioMode(parser.ReadInt(section, L"PreserveAspectRatio", 0));

	if (sourceChanged || attributesRequireReload)
	{
		m_Svg.reset();
		m_LoadAttempted = false;
	}

	if (m_Initialized)
	{
		if (sourceChanged || attributesRequireReload || !m_Svg || !m_Svg->HasDeviceResources())
		{
			LoadSvg();
		}
		else
		{
			ApplySvgAttributes();
			UpdateSize();
		}

		m_NeedsRedraw = true;
	}
}

void MeterSvg::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, true);
}

bool MeterSvg::LoadSvg()
{
	if (m_LoadAttempted) return m_Svg && m_Svg->HasDeviceResources();
	m_LoadAttempted = true;

	if (!m_Svg && !m_SvgImage.empty())
	{
		std::wstring source = m_SvgImage;
		if (!Gfx::Svg::IsInlineData(source))
		{
			m_Skin->MakePathAbsolute(source);
		}
		m_Svg = std::make_unique<Gfx::Svg>(source);
	}

	if (!m_Svg)
	{
		UpdateSize();
		return false;
	}

	HRESULT hr = m_Svg->Load(m_Skin->GetCanvas());
	if (FAILED(hr))
	{
		if (Gfx::Svg::IsInlineData(m_Svg->GetSource()))
		{
			LogErrorF(this, L"Unable to load inline SVG data, error: %s (0x%08x)", _com_error(hr).ErrorMessage(), hr);
		}
		else
		{
			LogErrorF(this, L"Unable to load SVG file: %s, error: %s (0x%08x)",
				m_Svg->GetSource().c_str(), _com_error(hr).ErrorMessage(), hr);
		}
		UpdateSize();
		return false;
	}

	ApplySvgAttributes();

	UpdateSize();
	return true;
}

void MeterSvg::ApplySvgAttributes()
{
	for (const auto& svgAttribute : m_SvgAttributes)
	{
		const HRESULT hr = m_Svg->SetAttribute(svgAttribute.selector, svgAttribute.attribute, svgAttribute.value);
		if (FAILED(hr))
		{
			LogErrorF(this, L"Unable to apply Attribute for selector '%s', attribute '%s', error: %s (0x%08x)",
				svgAttribute.selector.c_str(), svgAttribute.attribute.c_str(), _com_error(hr).ErrorMessage(), hr);
		}
	}
}

void MeterSvg::UpdateSize()
{
	const int svgW = m_Svg ? static_cast<int>(m_Svg->GetWidth()) : 0;
	const int svgH = m_Svg ? static_cast<int>(m_Svg->GetHeight()) : 0;
	if (!m_Svg || !m_Svg->HasDeviceResources() || svgW <= 0 || svgH <= 0)
	{
		if (!m_WDefined) m_W = 0;
		if (!m_HDefined) m_H = 0;
		return;
	}

	if (m_WDefined)
	{
		if (!m_HDefined)
		{
			m_H = m_W * svgH / svgW + GetHeightPadding();
		}
	}
	else if (m_HDefined)
	{
		m_W = m_H * svgW / svgH + GetWidthPadding();
	}
	else
	{
		m_W = svgW + GetWidthPadding();
		m_H = svgH + GetHeightPadding();
	}
}

bool MeterSvg::Update()
{
	if (Meter::Update() && m_NeedsRedraw)
	{
		m_NeedsRedraw = false;
		return true;
	}
	return false;
}

bool MeterSvg::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;
	if ((!m_Svg || !m_Svg->HasDeviceResources()) && !LoadSvg()) return true;

	D2D1_RECT_F meterRect = GetMeterRectPadding();
	FLOAT drawW = meterRect.right - meterRect.left;
	FLOAT drawH = meterRect.bottom - meterRect.top;
	if (drawW <= 0.0f || drawH <= 0.0f || m_Svg->GetWidth() <= 0.0f || m_Svg->GetHeight() <= 0.0f) return true;

	D2D1_RECT_F clipRect = meterRect;
	bool clip = false;
	if (m_AspectRatioMode != AspectRatioMode::Stretch)
	{
		const FLOAT svgRatio = m_Svg->GetWidth() / m_Svg->GetHeight();
		const FLOAT meterRatio = drawW / drawH;
		if (svgRatio != meterRatio)
		{
			if (m_AspectRatioMode == AspectRatioMode::Fit)
			{
				if (svgRatio > meterRatio)
				{
					drawH = drawW / svgRatio;
					meterRect.top += (meterRect.bottom - meterRect.top - drawH) / 2.0f;
				}
				else
				{
					drawW = drawH * svgRatio;
					meterRect.left += (meterRect.right - meterRect.left - drawW) / 2.0f;
				}
			}
			else
			{
				clip = true;
				if (svgRatio > meterRatio)
				{
					drawW = drawH * svgRatio;
					meterRect.left -= (drawW - (meterRect.right - meterRect.left)) / 2.0f;
				}
				else
				{
					drawH = drawW / svgRatio;
					meterRect.top -= (drawH - (meterRect.bottom - meterRect.top)) / 2.0f;
				}
			}
		}
	}

	meterRect.right = meterRect.left + drawW;
	meterRect.bottom = meterRect.top + drawH;
	canvas.DrawSvg(m_Svg.get(), meterRect, clip ? &clipRect : nullptr);
	return true;
}
