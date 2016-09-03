/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Canvas.h"
#include "TextFormatD2D.h"
#include "../UnitTest.h"
#include <memory>

namespace Gfx {

TEST_CLASS(Common_Gfx_TextFormatD2D_Test)
{
public:
	std::unique_ptr<Canvas> m_D2D;

	Common_Gfx_TextFormatD2D_Test() :
		m_D2D(Canvas::Create(Gfx::Renderer::D2D))
	{
		// TODO: Handle this in CanvasD2D.
		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

		m_D2D->Resize(10, 10);
		m_D2D->SetAntiAliasing(true);
	}

	TEST_METHOD(TestInaccurateText)
	{
		std::unique_ptr<TextFormatD2D> textFormat((TextFormatD2D*)m_D2D->CreateTextFormat());
		textFormat->SetProperties(L"Arial", 10, false, false, nullptr);

		DWRITE_TEXT_METRICS metrics;

		metrics = textFormat->GetMetrics(L"test", 4, true);
		Assert::AreEqual(26, (int)metrics.width);
		Assert::AreEqual(16, (int)metrics.height);

		metrics = textFormat->GetMetrics(L"test", 4, false);
		Assert::AreEqual(21, (int)metrics.width);
		Assert::AreEqual(14, (int)metrics.height);
	}

	TEST_METHOD(TestTrailingNewlineGdipCompatibility)
	{
		std::unique_ptr<TextFormatD2D> textFormat((TextFormatD2D*)m_D2D->CreateTextFormat());
		textFormat->SetProperties(L"Arial", 10, false, false, nullptr);

		DWRITE_TEXT_METRICS metrics;
		
		metrics = textFormat->GetMetrics(L"test\n", 5, false);
		Assert::AreEqual(15, (int)metrics.height);
		metrics = textFormat->GetMetrics(L"test\r\n", 6, false);
		Assert::AreEqual(15, (int)metrics.height);

		metrics = textFormat->GetMetrics(L"test\n ", 6, false);
		Assert::AreEqual(30, (int)metrics.height);
		metrics = textFormat->GetMetrics(L"test\r\n ", 7, false);
		Assert::AreEqual(30, (int)metrics.height);

		metrics = textFormat->GetMetrics(L"test\n\n", 6, false);
		Assert::AreEqual(30, (int)metrics.height);
		metrics = textFormat->GetMetrics(L"test\r\n\r\n", 8, false);
		Assert::AreEqual(30, (int)metrics.height);
	}
};

}  // namespace Gfx
