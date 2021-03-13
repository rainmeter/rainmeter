/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatShadow.h"
#include "Gfx/Util/D2DUtil.h"

namespace Gfx {

D2D1_VECTOR_4F ToVector4F(D2D1_COLOR_F color)
{
	return D2D1::Vector4F(color.r, color.g, color.b, color.a);
}

TextInlineFormat_Shadow::TextInlineFormat_Shadow(const std::wstring& pattern, const FLOAT& blur,
	const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color) :
		TextInlineFormat(pattern),
		m_Offset(offset),
		m_Blur(blur),
		m_Color(color),
		m_PreviousPosition(D2D1::RectF(-1.0f, -1.0f, -1.0f, -1.0f))
{
}

TextInlineFormat_Shadow::~TextInlineFormat_Shadow()
{
	if (m_Bitmap) m_Bitmap.Reset();
	if (m_BitmapTarget) m_BitmapTarget.Reset();
}

void TextInlineFormat_Shadow::ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout,
	ID2D1SolidColorBrush* solidBrush, const UINT32& strLen, const D2D1_POINT_2F& drawPosition)
{
	if (!target || !layout) return;

	// In order to make a shadow effect using the built-in D2D effect, we first need to make
	// certain parts of the string transparent. We then draw only the parts of the string we
	// we want a shadow for onto a memory bitmap. From this bitmap we can create the shadow
	// effect and draw it.

	const D2D1_COLOR_F& color = Util::c_Transparent_Color_F;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> transparent;
	HRESULT hr = target->CreateSolidColorBrush(color, transparent.GetAddressOf());
	if (FAILED(hr)) return;

	// Only change characters outside of the range(s) transparent
	for (UINT32 i = 0; i < strLen; ++i)
	{
		bool found = false;
		for (const auto& range : GetRanges())
		{
			if (range.length <= 0) continue;

			if (i >= range.startPosition && i < (range.startPosition + range.length))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			DWRITE_TEXT_RANGE temp = { i, 1 };
			layout->SetDrawingEffect(transparent.Get(), temp);
		}
	}

	// Reset the shadow bitmap if the drawing position or size of target has changed.
	if (m_BitmapTarget)
	{
		const auto tSize = target->GetSize();
		const D2D1_RECT_F position = D2D1::RectF(drawPosition.x, drawPosition.y, tSize.width, tSize.height);

		if (position.left != m_PreviousPosition.left ||
			position.top != m_PreviousPosition.top ||
			position.right != m_PreviousPosition.right ||
			position.bottom != m_PreviousPosition.bottom)
		{
			m_BitmapTarget.Reset();
			m_PreviousPosition = position;
		}
	}

	m_Bitmap.Reset();

	if (!m_BitmapTarget)
	{
		m_BitmapTarget.Reset();

		hr = target->CreateCompatibleRenderTarget(m_BitmapTarget.GetAddressOf());
		if (FAILED(hr)) return;
	}

	// Draw onto memory bitmap target
	// Note: Hardware acceleration seems to keep the bitmap render target in memory
	// even though it is cleared, so manually "Clear" with a transparent color.
	m_BitmapTarget->BeginDraw();
	m_BitmapTarget->Clear(color);
	m_BitmapTarget->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), layout, solidBrush);
	m_BitmapTarget->EndDraw();

	hr = m_BitmapTarget->GetBitmap(m_Bitmap.GetAddressOf());
	if (FAILED(hr)) return;

	// Create shadow effect
	Microsoft::WRL::ComPtr<ID2D1Effect> shadow;
	hr = target->CreateEffect(CLSID_D2D1Shadow, shadow.GetAddressOf());
	if (FAILED(hr)) return;

	// Load shadow options to effect
	shadow->SetInput(0U, m_Bitmap.Get());
	shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, m_Blur);
	shadow->SetValue(D2D1_SHADOW_PROP_COLOR, ToVector4F(m_Color));
	shadow->SetValue(D2D1_SHADOW_PROP_OPTIMIZATION, D2D1_SHADOW_OPTIMIZATION_SPEED);

	// Draw effect
	target->DrawImage(shadow.Get(), Util::AddPoint2F(drawPosition, m_Offset));
}

bool TextInlineFormat_Shadow::CompareAndUpdateProperties(const std::wstring& pattern, const FLOAT& blur,
	const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || !Util::ColorFEquals(m_Color, color) ||
		blur != m_Blur || offset.x != m_Offset.x || offset.y != m_Offset.y)
	{
		SetPattern(pattern);
		m_Offset = offset;
		m_Color = color;
		m_Blur = blur;
		return true;
	}

	return false;
}

}  // namespace Gfx
