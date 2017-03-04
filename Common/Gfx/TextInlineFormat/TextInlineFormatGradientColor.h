/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_GRADIENTCOLOR_H_
#define RM_GFX_TEXTINLINEFORMAT_GRADIENTCOLOR_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace {

// Since the |range| can be spread across multiple lines, we
// need a way for split the range into multiple sections and
// also store a linear gradient brush for each 'inner' range.
struct GradientHelper
{
	DWRITE_TEXT_RANGE range;
	std::vector<DWRITE_TEXT_RANGE> innerRanges;
	std::vector<Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush>> brushes;
};

}

namespace Gfx {

// Sets a color gradient for a select range.
class TextInlineFormat_GradientColor final : public TextInlineFormat
{
public:
	TextInlineFormat_GradientColor(const std::wstring& pattern, FLOAT angle,
		const std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma);
	virtual ~TextInlineFormat_GradientColor();
	virtual InlineType GetType() override { return InlineType::GradientColor; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(IDWriteTextLayout* layout, const D2D1_POINT_2F* point, bool beforeDrawing = true);

	void BuildGradientBrushes(ID2D1RenderTarget* target, IDWriteTextLayout* layout);
	void UpdateSubOptions(const size_t& index, const DWRITE_TEXT_RANGE& range);

	bool CompareAndUpdateProperties(const std::wstring& pattern, FLOAT angle,
		const std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma);

private:
	TextInlineFormat_GradientColor();
	TextInlineFormat_GradientColor(const TextInlineFormat_GradientColor& other) = delete;

	static HRESULT GetHitTestMetrics(IDWriteTextLayout* layout, std::vector<DWRITE_HIT_TEST_METRICS>& metrics, DWRITE_TEXT_RANGE range);

	FLOAT m_Angle;
	bool m_AlternativeGamma;
	std::vector<D2D1_GRADIENT_STOP> m_GradientStops;

	std::vector<GradientHelper> m_SubOptions;
};

}  // namespace Gfx

#endif
