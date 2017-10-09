/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Gfx/Util/D2DUtil.h"
#include "TextInlineFormatGradientColor.h"

namespace Gfx {

TextInlineFormat_GradientColor::TextInlineFormat_GradientColor(const std::wstring& pattern, FLOAT angle,
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
		sub.innerRanges.clear();
		sub.brushes.clear();

		if (sub.range.length <= 0) continue;

		std::vector<DWRITE_HIT_TEST_METRICS> metrics;
		HRESULT hr = GetHitTestMetrics(layout, metrics, sub.range);

		if (FAILED(hr)) continue;

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

			if (FAILED(hr)) continue;

			D2D1_POINT_2F start = Util::FindEdgePoint(m_Angle, hit.left, hit.top, hit.width, hit.height);
			D2D1_POINT_2F end = Util::FindEdgePoint(m_Angle + 180.0f, hit.left, hit.top, hit.width, hit.height);

			DWRITE_TEXT_RANGE innerRange = { hit.textPosition, hit.length };

			Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> gradientBrush;
			hr = target->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(start, end),
				collection.Get(),
				gradientBrush.GetAddressOf());

			if (FAILED(hr)) continue;

			sub.innerRanges.push_back(innerRange);
			sub.brushes.push_back(gradientBrush);
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

void TextInlineFormat_GradientColor::ApplyInlineFormat(IDWriteTextLayout* layout, const D2D1_POINT_2F* point, bool beforeDrawing)
{
	if (beforeDrawing && !layout) return;

	// Because the gradient needs to know the drawing position, we need a way to set that position
	// before drawing time, and then remove that same position after drawing time in case the
	// position changes on the next iteration.
	// Note: |layout| should be 'nullptr' when |beforeDrawing| is false (or after drawing time)
	FLOAT sign = beforeDrawing ? 1.0f : -1.0f;

	for (auto& sub : m_SubOptions)
	{
		if (sub.range.length <= 0) continue;

		size_t count = 0;
		for (const auto& range : sub.innerRanges)
		{
			if (sub.brushes[count])
			{
				D2D1_POINT_2F start = sub.brushes[count]->GetStartPoint();
				D2D1_POINT_2F end = sub.brushes[count]->GetEndPoint();

				start.x += sign * point->x;
				start.y += sign * point->y;
				end.x += sign * point->x;
				end.y += sign * point->y;

				sub.brushes[count]->SetStartPoint(start);
				sub.brushes[count]->SetEndPoint(end);

				if (beforeDrawing)
				{
					layout->SetDrawingEffect(sub.brushes[count].Get(), range);
				}
			}

			++count;
		}
	}
}

bool TextInlineFormat_GradientColor::CompareAndUpdateProperties(const std::wstring& pattern, FLOAT angle,
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
