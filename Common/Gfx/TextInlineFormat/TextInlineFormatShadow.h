// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets a color for a select range.
class TextInlineFormat_Shadow final : public TextInlineFormat
{
public:
	TextInlineFormat_Shadow(const std::wstring& pattern, const FLOAT& blur,
		const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color);
	virtual ~TextInlineFormat_Shadow() {}

	virtual InlineType GetType() override { return InlineType::Shadow; }
	virtual void InvalidateDeviceResources() override;
	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }

	void ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout,
		ID2D1SolidColorBrush* solidBrush, const UINT32& strLen, const D2D1_RECT_F& drawRect);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const FLOAT& blur,
		const D2D1_POINT_2F& offset, const D2D1_COLOR_F& color);

private:
	TextInlineFormat_Shadow();
	TextInlineFormat_Shadow(const TextInlineFormat_Shadow& other) = delete;

	FLOAT m_Blur;
	D2D1_POINT_2F m_Offset;
	D2D1_COLOR_F m_Color;

	D2D1_RECT_F m_PreviousPosition;

	Microsoft::WRL::ComPtr<ID2D1Bitmap> m_Bitmap;
	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_BitmapTarget;
};

}  // namespace Gfx
