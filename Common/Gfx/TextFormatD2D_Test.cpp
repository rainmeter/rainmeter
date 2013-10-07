/*
  Copyright (C) 2013 Birunthan Mohanathas

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
