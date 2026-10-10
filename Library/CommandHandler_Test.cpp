// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "CommandHandler.h"
#include "ConfigParser.h"
#include "Measure.h"
#include "Meter.h"
#include "Skin.h"
#include "../Common/UnitTest.h"

class BangTestMeasure : public Measure
{
public:
	BangTestMeasure(Skin* skin) : Measure(skin, L"BangTestMeasure") {}
	UINT GetTypeID() override { return TypeID<BangTestMeasure>(); }

	void SetValues(int integer, double number, BangInteger relativeInteger, BangNumber relativeNumber, const WCHAR* text)
	{
		++calls;
		Assert::AreEqual(5, integer);
		Assert::AreEqual(-2.5, number);
		Assert::AreEqual(-3, relativeInteger.value);
		Assert::IsTrue(relativeInteger.relative);
		Assert::AreEqual(1.5, relativeNumber.value);
		Assert::IsTrue(relativeNumber.relative);
		Assert::AreEqual(L"(unparsed text)", text);
	}

	void Run() { ++calls; }
	int calls = 0;

protected:
	void UpdateValue() override {}
};

class BangTestMeter : public Meter
{
public:
	BangTestMeter(Skin* skin) : Meter(skin, L"BangTestMeter") {}
	UINT GetTypeID() override { return TypeID<BangTestMeter>(); }

	void SetValues(int index, int length)
	{
		++calls;
		Assert::AreEqual(3, index);
		Assert::AreEqual(-1, length);
	}

	int calls = 0;
};

TEST_CLASS(Library_CommandHandler_Test)
{
public:
	TEST_METHOD(TestTypedMeasureRegistration)
	{
		Skin skin(L"", L"", false);
		skin.GetParser().Initialize(L"", &skin, nullptr);
		BangTestMeasure measure(&skin);
		CurrentSectionScope currentSection(&skin, &measure);
		CommandHandler::RegisterMeasureBang<BangTestMeasure, &BangTestMeasure::SetValues>(measure.GetTypeID(), L"BangTest:Values");
		CommandHandler::RegisterMeasureBang<BangTestMeasure, &BangTestMeasure::Run>(measure.GetTypeID(), L"BangTest:Run");
		CommandHandler handler;

		std::vector<std::wstring> args = { L"(2 + 3)", L"-2.5", L"-(7 / 2)", L"+(3 / 2)", L"(unparsed text)" };
		handler.ExecuteBang(L"BangTest:Values", args, &skin);
		Assert::AreEqual(1, measure.calls);

		args[1] = L"invalid";
		handler.ExecuteBang(L"BangTest:Values", args, &skin);
		Assert::AreEqual(1, measure.calls);

		args.pop_back();
		handler.ExecuteBang(L"BangTest:Values", args, &skin);
		Assert::AreEqual(1, measure.calls);

		args.clear();
		handler.ExecuteBang(L"BangTest:Run", args, &skin);
		Assert::AreEqual(2, measure.calls);
	}

	TEST_METHOD(TestTypedMeterRegistration)
	{
		Skin skin(L"", L"", false);
		skin.GetParser().Initialize(L"", &skin, nullptr);
		BangTestMeter meter(&skin);
		CurrentSectionScope currentSection(&skin, &meter);
		CommandHandler::RegisterMeterBang<BangTestMeter, &BangTestMeter::SetValues>(meter.GetTypeID(), L"BangTest:Select");
		CommandHandler handler;
		std::vector<std::wstring> args = { L"(6 / 2)", L"-1" };
		handler.ExecuteBang(L"BangTest:Select", args, &skin);
		Assert::AreEqual(1, meter.calls);

		args[1] = L"2147483648";
		handler.ExecuteBang(L"BangTest:Select", args, &skin);
		Assert::AreEqual(1, meter.calls);
	}

	TEST_METHOD(TestNumbersAndFormulas)
	{
		ConfigParser parser;
		const auto literal = CommandHandler::ParseBangNumber(parser, L" 12.5 ");
		Assert::IsTrue(literal.has_value());
		Assert::AreEqual(12.5, literal->value);
		Assert::IsFalse(literal->relative);

		const auto formula = CommandHandler::ParseBangNumber(parser, L" (2 + 3) * 2 ");
		Assert::IsFalse(formula.has_value());
		const auto nested = CommandHandler::ParseBangNumber(parser, L" ((2 + 3) * 2) ");
		Assert::IsTrue(nested.has_value());
		Assert::AreEqual(10.0, nested->value);

		const std::wstring bounded = L"(2 + 3)tail";
		const auto value = CommandHandler::ParseBangNumber(parser, std::wstring_view(bounded).substr(0, 7));
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(5.0, value->value);
	}

	TEST_METHOD(TestRelativeMode)
	{
		ConfigParser parser;
		const auto positive = CommandHandler::ParseBangNumber(parser, L" +(2 + 3) ", true);
		Assert::IsTrue(positive.has_value());
		Assert::AreEqual(5.0, positive->value);
		Assert::IsTrue(positive->relative);

		const auto negative = CommandHandler::ParseBangNumber(parser, L"-(2 + 3)", true);
		Assert::IsTrue(negative.has_value());
		Assert::AreEqual(-5.0, negative->value);
		Assert::IsTrue(negative->relative);

		const auto absolute = CommandHandler::ParseBangNumber(parser, L"(-5)", true);
		Assert::IsTrue(absolute.has_value());
		Assert::AreEqual(-5.0, absolute->value);
		Assert::IsFalse(absolute->relative);

		const auto coordinate = CommandHandler::ParseBangNumber(parser, L"-5");
		Assert::IsTrue(coordinate.has_value());
		Assert::AreEqual(-5.0, coordinate->value);
		Assert::IsFalse(coordinate->relative);
	}

	TEST_METHOD(TestInvalidNumbers)
	{
		ConfigParser parser;
		const WCHAR* invalid[] = { L"", L" ", L"+", L"--1", L"+-1", L"12tail", L"(2 +)", L"(2 + 3)tail", L"1e9999", L"nan", L"inf" };
		for (const WCHAR* argument : invalid)
		{
			Assert::IsFalse(CommandHandler::ParseBangNumber(parser, argument, true).has_value());
		}
	}

	TEST_METHOD(TestIntegerConversion)
	{
		ConfigParser parser;
		const auto fraction = CommandHandler::ParseBangInteger(parser, L"-(7 / 2)", true);
		Assert::IsTrue(fraction.has_value());
		Assert::AreEqual(-3, fraction->value);
		Assert::IsTrue(fraction->relative);

		Assert::IsTrue(CommandHandler::ParseBangInteger(parser, L"2147483647").has_value());
		Assert::IsTrue(CommandHandler::ParseBangInteger(parser, L"-2147483648").has_value());
		Assert::IsFalse(CommandHandler::ParseBangInteger(parser, L"2147483648").has_value());
		Assert::IsFalse(CommandHandler::ParseBangInteger(parser, L"(-2147483649)").has_value());
		Assert::IsFalse(CommandHandler::ParseBangInteger(parser, L"(2147483647 + 1)").has_value());
	}
};
