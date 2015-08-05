/*
  Copyright (C) 2015 Brian Ferguson

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

#include "StdAfx.h"
#include "TextInlineFormatGradientColor.h"

namespace {

static const double M_PI = 3.14159265358979323846;

static D2D1_POINT_2F FindEdgePoint(const UINT32 theta, const float left, const float top, const float width, const float height)
{
	double theta1 = theta * (M_PI / 180.0f);

	while (theta1 < -M_PI) theta1 += (2 * M_PI);
	while (theta1 > M_PI) theta1 -= (2 * M_PI);

	const float recttan = atan2f(top + height, left + width);
	const float thetatan = (float)tan(theta1);

	enum Region
	{
		One,        // 315 - 45
		Two,        // 45  - 135
		Three,      // 135 - 225
		Four        // 225 - 315
	} region;

	if (theta1 > -recttan && theta1 <= recttan)
	{
		region = One;
	}
	else if (theta1 > recttan && theta1 <= (M_PI - recttan))
	{
		region = Two;
	}
	else if (theta1 > (M_PI - recttan) || theta1 <= -(M_PI - recttan))
	{
		region = Three;
	}
	else
	{
		region = Four;
	}

	float xfactor = 1.0f;
	float yfactor = -1.0f;
	switch (region)
	{
	case One: yfactor = -yfactor; break;
	case Two: yfactor = -yfactor; break;
	case Three: xfactor = -xfactor; break;
	case Four: xfactor = -xfactor; break;
	}

	D2D1_POINT_2F point = { left + (width / 2.0f), top + (height / 2.0f) };
	if (region == One || region == Three)
	{
		point.x += xfactor * (width / 2.0f);
		point.y += yfactor * (width / 2.0f) * thetatan;
	}
	else
	{
		point.x += xfactor * (height / (2.0f * thetatan));
		point.y += yfactor * (height / 2.0f);
	}

	return point;
}

}  // namespace

namespace Gfx {

TextInlineFormat_GradientColor::TextInlineFormat_GradientColor(const std::wstring& pattern, UINT32 angle,
	const std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma) :
		TextInlineFormat(pattern),
		m_Angle(angle),
		m_GradientStops(stops),
		m_AlternativeGamma(altGamma)
{
}

TextInlineFormat_GradientColor::~TextInlineFormat_GradientColor()
{
}

void TextInlineFormat_GradientColor::BuildGradientBrushes(ID2D1RenderTarget* target, IDWriteTextLayout* layout)
{
	for (auto& sub : m_SubOptions)
	{
		// Since we are building the sub options (brushes, inner ranges) again,
		// we need to destroy any previous sub options prior to building them.
		for (size_t i = 0; i < sub.brushes.size(); ++i)
		{
			sub.brushes[i]->Release();
			sub.brushes[i] = nullptr;
		}
		sub.inner_ranges.clear();
		sub.brushes.clear();

		if (sub.range.length > 0)
		{
			std::vector<DWRITE_HIT_TEST_METRICS> metrics;
			HRESULT hr = GetHitTestMetrics(layout, metrics, sub.range);

			if (SUCCEEDED(hr))
			{
				size_t count = 0;
				for (const auto& hit : metrics)
				{
					Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> collection;
					hr = target->CreateGradientStopCollection(
						&m_GradientStops[0],
						(UINT32)m_GradientStops.size(),
						m_AlternativeGamma ? D2D1_GAMMA_1_0 : D2D1_GAMMA_2_2,
						D2D1_EXTEND_MODE_CLAMP,
						collection.GetAddressOf());

					if (SUCCEEDED(hr))
					{
						D2D1_POINT_2F start = FindEdgePoint(m_Angle, hit.left, hit.top, hit.width, hit.height);
						D2D1_POINT_2F end = FindEdgePoint(m_Angle + 180, hit.left, hit.top, hit.width, hit.height);

						DWRITE_TEXT_RANGE innerRange = { hit.textPosition, hit.length };

						ID2D1LinearGradientBrush* gradientBrush;
						hr = target->CreateLinearGradientBrush(
							D2D1::LinearGradientBrushProperties(start, end),
							collection.Get(),
							&gradientBrush);

						if (SUCCEEDED(hr))
						{
							sub.inner_ranges.push_back(innerRange);
							sub.brushes.push_back(gradientBrush);
						}
					}
				}
			}
		}
	}
}

void TextInlineFormat_GradientColor::UpdateSubOptions(const size_t& index, const DWRITE_TEXT_RANGE& range)
{
	if (index >= m_SubOptions.size())
	{
		GradientHelper sub;
		sub.range = range;
		m_SubOptions.push_back(sub);
	}
	else
	{
		m_SubOptions[index].range = range;
	}
}

void TextInlineFormat_GradientColor::ApplyInlineFormat(IDWriteTextLayout* layout,
	const D2D1_POINT_2F* point, bool hasChanged)
{
	if (!layout) return;

	for (const auto& sub : m_SubOptions)
	{
		if (sub.range.length > 0)
		{
			size_t count = 0;
			for (const auto& range : sub.inner_ranges)
			{
				if (sub.brushes[count])
				{
					D2D1_POINT_2F start = sub.brushes[count]->GetStartPoint();
					D2D1_POINT_2F end = sub.brushes[count]->GetEndPoint();

					if (hasChanged)
					{
						start.x += point->x;
						start.y += point->y;

						end.x += point->x;
						end.y += point->y;
					}

					sub.brushes[count]->SetStartPoint(start);
					sub.brushes[count]->SetEndPoint(end);

					layout->SetDrawingEffect(sub.brushes[count], range);
				}

				++count;
			}
		}
	}
}

bool TextInlineFormat_GradientColor::CompareAndUpdateProperties(const std::wstring& pattern, UINT32 angle,
	const std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma)
{
	// Check most options (and size of 'stops').
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Angle != angle ||
		m_AlternativeGamma != altGamma || m_GradientStops.size() != stops.size())
	{
		SetPattern(pattern);
		m_Angle = angle;
		m_AlternativeGamma = altGamma;
		m_GradientStops = stops;
		m_GradientStops.shrink_to_fit();
		return true;
	}
	else
	{
		// In case the 'color' or its 'position' changed (and nothing else).
		bool hasChanged = false;
		for (size_t i = 0; i < stops.size(); ++i)
		{
			if (m_GradientStops[i].color.r != stops[i].color.r ||
				m_GradientStops[i].color.g != stops[i].color.g ||
				m_GradientStops[i].color.b != stops[i].color.b ||
				m_GradientStops[i].color.a != stops[i].color.a ||
				m_GradientStops[i].position != stops[i].position)
			{
				m_GradientStops = stops;
				m_GradientStops.shrink_to_fit();
				hasChanged = true;
			}
		}

		if (hasChanged) return true;
	}

	return false;
}

HRESULT TextInlineFormat_GradientColor::GetHitTestMetrics(IDWriteTextLayout* layout,
	std::vector<DWRITE_HIT_TEST_METRICS>& metrics, DWRITE_TEXT_RANGE range)
{
	UINT32 count = 0;
	HRESULT hr = layout->HitTestTextRange(range.startPosition, range.length, 0, 0, nullptr, 0, &count);
	if (FAILED(hr))
	{
		metrics.resize(count);
		hr = layout->HitTestTextRange(range.startPosition, range.length, 0, 0, &metrics[0],
			static_cast<UINT32>(metrics.size()), &count);
	}

	return hr;
}

}  // namespace Gfx
