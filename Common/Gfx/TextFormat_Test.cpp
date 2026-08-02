// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "Canvas.h"
#include "TextFormat.h"
#include "../UnitTest.h"
#include <memory>

namespace Gfx {

TEST_CLASS(Common_Gfx_TextFormat_Test)
{
public:
	std::unique_ptr<Canvas> m_D2D;
	MathParser m_MathParser;

	Common_Gfx_TextFormat_Test() :
		m_D2D(Canvas::Create(Gfx::Renderer::D2D))
	{
		m_D2D->Resize(10, 10);
		m_D2D->SetAntiAliasing(true);
	}

	TEST_METHOD(TestInaccurateText)
	{
		std::unique_ptr<TextFormat> textFormat(m_D2D->CreateTextFormat(m_MathParser));
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
		std::unique_ptr<TextFormat> textFormat(m_D2D->CreateTextFormat(m_MathParser));
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
