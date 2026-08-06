// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StringParser.h"
#include "MathParser.h"
#include "UnitTest.h"

TEST_CLASS(Common_StringParser_Test)
{
public:
	TEST_METHOD(TestConsumeString)
	{
		StringParser parser(L"PrefixSuffix");

		Assert::IsTrue(parser.Consume(L"Prefix"));
		Assert::IsFalse(parser.IsConsumed());
		Assert::IsTrue(parser.ConsumeRest(L"Suffix"));
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeRestStringRollback)
	{
		StringParser parser(L"PrefixSuffix");

		Assert::IsFalse(parser.ConsumeRest(L"Prefix"));
		Assert::IsTrue(parser.ConsumeRest(L"PrefixSuffix"));
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeChar)
	{
		StringParser parser(L"xY");

		Assert::IsTrue(parser.Consume(L'X'));
		Assert::IsTrue(parser.ConsumeRest(L'y'));
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeWhitespace)
	{
		StringParser parser(L" \t\r\nValue");

		parser.ConsumeWhitespace();
		Assert::IsTrue(parser.ConsumeRest(L"Value"));
	}

	TEST_METHOD(TestConsumeInt)
	{
		StringParser parser(L"-123rest");

		const auto value = parser.ConsumeInt();
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(-123, *value);
		Assert::IsTrue(parser.ConsumeRest(L"rest"));
	}

	TEST_METHOD(TestConsumeUInt)
	{
		StringParser parser(L"42tail");

		const auto value = parser.ConsumeUInt();
		Assert::IsTrue(value.has_value());
		Assert::AreEqual((UINT)42, *value);
		Assert::IsTrue(parser.ConsumeRest(L"tail"));
	}

	TEST_METHOD(TestConsumeDouble)
	{
		StringParser parser(L"-12.5tail");

		const auto value = parser.ConsumeDouble();
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(-12.5, *value, 0.0001);
		Assert::IsTrue(parser.ConsumeRest(L"tail"));
	}

	TEST_METHOD(TestConsumeNumbersRejectLeadingWhitespaceByDefault)
	{
		StringParser parser(L" 42");

		Assert::IsFalse(parser.ConsumeInt().has_value());
		Assert::IsFalse(parser.IsConsumed());
		Assert::IsTrue(parser.ConsumeRest(L" 42"));
	}

	TEST_METHOD(TestConsumeNumbersSkipWhitespaceOption)
	{
		StringParser parser(L" \t42\r\n");

		const auto value = parser.ConsumeRestInt(StringParser::SkipWhitespace);
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(42, *value);
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeStringSkipWhitespaceOption)
	{
		StringParser parser(L" \tValue\r\n");

		Assert::IsTrue(parser.ConsumeRest(L"Value", StringParser::SkipWhitespace));
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeRestNumberRollback)
	{
		StringParser parser(L"42tail");

		Assert::IsFalse(parser.ConsumeRestInt().has_value());
		Assert::IsTrue(parser.ConsumeRest(L"42tail"));
	}

	TEST_METHOD(TestConsumeIntOrFormula)
	{
		MathParser mathParser;
		StringParser parser(L"(1 + min(2, 3))tail");

		const auto value = parser.ConsumeIntOrFormula(mathParser);
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(3, *value);
		Assert::IsTrue(parser.ConsumeRest(L"tail"));
	}

	TEST_METHOD(TestConsumeRestDoubleOrFormulaSkipWhitespace)
	{
		MathParser mathParser;
		StringParser parser(L" \t(5 / 2)\r\n");

		const auto value = parser.ConsumeRestDoubleOrFormula(mathParser, StringParser::SkipWhitespace);
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(2.5, *value, 0.0001);
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeRestFormulaRollback)
	{
		MathParser mathParser;
		StringParser parser(L"(1 + 2)tail");

		Assert::IsFalse(parser.ConsumeRestIntOrFormula(mathParser).has_value());
		Assert::IsTrue(parser.ConsumeRest(L"(1 + 2)tail"));
	}

	TEST_METHOD(TestConsumeUntil)
	{
		StringParser parser(L"first|second|third");

		AssertValue(L"first", parser.ConsumeUntil(L'|'));
		AssertValue(L"second", parser.ConsumeUntil(L'|'));
		AssertValue(L"", parser.ConsumeUntil(L'|'));
		Assert::IsTrue(parser.IsConsumed());
	}

	TEST_METHOD(TestConsumeUntilSkipWhitespaceOption)
	{
		StringParser parser(L"  first \t| second");

		AssertValue(L"first", parser.ConsumeUntil(L'|', StringParser::SkipWhitespace));
		Assert::IsTrue(parser.ConsumeRest(L"second", StringParser::SkipWhitespace));
	}

	TEST_METHOD(TestConsumeUntilIgnoresParenthesesByDefault)
	{
		StringParser parser(L"(1,2),3");

		AssertValue(L"(1", parser.ConsumeUntil(L','));
		Assert::IsTrue(parser.ConsumeRest(L"2),3"));
	}

	TEST_METHOD(TestConsumeUntilSkipNestedParenthesesOption)
	{
		StringParser parser(L"(min(1,2)),3");

		AssertValue(L"(min(1,2))", parser.ConsumeUntil(L',', StringParser::SkipNestedParentheses));
		Assert::IsTrue(parser.ConsumeRest(L"3"));
	}

	TEST_METHOD(TestConsumeUntilCombinedOptions)
	{
		StringParser parser(L" (1 + 2) , tail");
		const auto option = StringParser::SkipWhitespace | StringParser::SkipNestedParentheses;

		AssertValue(L"(1 + 2)", parser.ConsumeUntil(L',', option));
		Assert::IsTrue(parser.ConsumeRest(L"tail", StringParser::SkipWhitespace));
	}

private:
	static void AssertValue(const WCHAR* expected, std::wstring_view actual)
	{
		Assert::AreEqual(expected, std::wstring(actual).c_str());
	}
};
