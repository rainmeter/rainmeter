// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "ParseUtil.h"
#include "MathParser.h"
#include "UnitTest.h"

namespace ParseUtil {

TEST_CLASS(Common_ParseUtil_Test)
{
public:
	TEST_METHOD(TestParseColorHex)
	{
		AssertColor(255, 0, 0, 255, L"FF0000");
		AssertColor(255, 0, 0, 128, L"FF000080");
		AssertColor(255, 0, 0, 255, L"0xFF0000");
		AssertColor(17, 34, 51, 68, L"11223344");
	}

	TEST_METHOD(TestParseColorHexPrefixIsCaseSensitive)
	{
		// "0X" is not a prefix, so it is read as a (mostly invalid) value instead.
		AssertColor(0, 255, 255, 255, L"0XFF0000");
	}

	TEST_METHOD(TestParseColorHexAlphaRequiresNoWhitespace)
	{
		// The 7th character being whitespace means the value is not an RGBA hex value.
		AssertColor(255, 0, 0, 255, L"FF0000 80");
	}

	TEST_METHOD(TestParseColorHexTooShort)
	{
		// Anything shorter than 6 characters keeps every component at its default.
		AssertColor(255, 255, 255, 255, L"FF00");
	}

	TEST_METHOD(TestParseColorHexInvalidDigits)
	{
		// Components are read until one is not a hex value, the rest keep their defaults.
		AssertColor(255, 0, 255, 255, L"FF00ZZ");
		AssertColor(255, 255, 255, 255, L"ZZ0000");
	}

	TEST_METHOD(TestParseColorComponents)
	{
		AssertColor(10, 20, 30, 255, L"10,20,30");
		AssertColor(10, 20, 30, 40, L"10,20,30,40");
		AssertColor(10, 20, 30, 255, L" 10 , 20 , 30 ");
	}

	TEST_METHOD(TestParseColorComponentsFormula)
	{
		AssertColor(3, 20, 30, 255, L"(1 + min(2, 3)),20,30");
	}

	TEST_METHOD(TestParseColorComponentsInvalid)
	{
		// An empty component is zero, but a trailing whitespace-only one is not a component
		// at all and leaves the default.
		AssertColor(10, 0, 30, 255, L"10,,30");
		AssertColor(10, 20, 255, 255, L"10,20, ");
		AssertColor(10, 20, 255, 255, L"10,20,\r\n");
		AssertColor(10, 0, 30, 255, L"10,invalid,30");
	}

	TEST_METHOD(TestParseColorComponentsExtra)
	{
		// Anything past the alpha component is ignored.
		AssertColor(10, 20, 30, 40, L"10,20,30,40,50");
	}

	TEST_METHOD(TestParseNumberFromView)
	{
		MathParser mathParser;
		const std::wstring_view str = L"12.5,(1 + 2),abc";

		Assert::AreEqual(12.5, ParseDouble(str.substr(0, 4), 0.0, mathParser), 0.0001);
		Assert::AreEqual(3, ParseInt(str.substr(5, 7), 0, mathParser));

		// A value that is not a number at all uses the default value.
		Assert::AreEqual(-1.0, ParseDouble(str.substr(13), -1.0, mathParser), 0.0001);
		Assert::AreEqual(-1.0, ParseDouble(std::wstring_view(), -1.0, mathParser), 0.0001);
	}

	TEST_METHOD(TestParseNumberFromViewIgnoresTrailingCharacters)
	{
		MathParser mathParser;

		Assert::AreEqual(12.0, ParseDouble(std::wstring_view(L"12tail"), 0.0, mathParser), 0.0001);
	}

	TEST_METHOD(TestParseRect)
	{
		MathParser mathParser;
		const auto rect = ParseRect(L"10,20,30,40", mathParser);

		Assert::AreEqual(10.0f, rect.left, 0.0001f);
		Assert::AreEqual(20.0f, rect.top, 0.0001f);
		Assert::AreEqual(40.0f, rect.right, 0.0001f);   // Left + width
		Assert::AreEqual(60.0f, rect.bottom, 0.0001f);  // Top + height
	}

private:
	static void AssertColor(int r, int g, int b, int a, const WCHAR* str)
	{
		MathParser mathParser;
		const auto color = ParseColor(str, mathParser);

		Assert::AreEqual(r / 255.0f, color.r, 0.0001f, str);
		Assert::AreEqual(g / 255.0f, color.g, 0.0001f, str);
		Assert::AreEqual(b / 255.0f, color.b, 0.0001f, str);
		Assert::AreEqual(a / 255.0f, color.a, 0.0001f, str);
	}
};

}  // namespace ParseUtil
