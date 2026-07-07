/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ConfigParser.h"
#include "Meter.h"
#include "MeasureString.h"
#include "Skin.h"
#include "../Common/UnitTest.h"
#include "../Common/Gfx/Util/D2DUtil.h"

class TestMeasureString : public MeasureString
{
public:
	TestMeasureString(const WCHAR* name) : MeasureString(nullptr, name) {}

	void Read(ConfigParser& parser)
	{
		ReadOptions(parser, GetName());
	}
};

class TestMeasure : public Measure
{
public:
	TestMeasure(Skin* skin, const WCHAR* name) : Measure(skin, name) {}

	void Read(ConfigParser& parser)
	{
		ReadOptions(parser, GetName());
	}

	void SetTestValue(double value)
	{
		m_Value = value;
	}

	virtual UINT GetTypeID() { return TypeID<TestMeasure>(); }

protected:
	virtual void UpdateValue() {}
};

class TestMeter : public Meter
{
public:
	TestMeter(Skin* skin, const WCHAR* name) : Meter(skin, name) {}

	virtual UINT GetTypeID() { return TypeID<TestMeter>(); }

	void SetBounds(int x, int y, int w, int h)
	{
		SetX(x);
		SetY(y);
		SetW(w);
		SetH(h);
	}
};

static void AddMeterToSkinForTests(Skin& skin, Meter* meter)
{
	auto& meters = const_cast<std::vector<Meter*>&>(skin.GetMeters());
	meters.push_back(meter);
}

static void AssertSectionVariableSelector(ConfigParser& parser, const WCHAR* section, const WCHAR* selector, const WCHAR* expected)
{
	for (const WCHAR* prefix : { L"", L"&" })
	{
		std::wstring value = L"[";
		value += prefix;
		value += section;
		value += L":";
		value += selector;
		value += L"]";

		Assert::IsTrue(parser.ReplaceMeasures(value));
		Assert::AreEqual(expected, value.c_str());
	}
}

static void AssertInvalidSectionVariableSelector(ConfigParser& parser, const WCHAR* section, const WCHAR* selector)
{
	for (const WCHAR* prefix : { L"", L"&" })
	{
		std::wstring value = L"[";
		value += prefix;
		value += section;
		value += L":";
		value += selector;
		value += L"]";

		const std::wstring expected = value;
		Assert::IsFalse(parser.ReplaceMeasures(value));
		Assert::AreEqual(expected.c_str(), value.c_str());
	}
}

TEST_CLASS(Library_ConfigParser_Test)
{
public:
	TEST_METHOD(TestReadFunctions)
	{
		ConfigParser parser;
		parser.Initialize(L"");  // TODO: Better way to initialize without file.

		parser.SetValue(L"A", L"String", L"abc");
		Assert::AreEqual(parser.ReadString(L"A", L"String", L"").c_str(), L"abc");
		Assert::AreEqual(parser.ReadString(L"A", L"StringNA", L"def").c_str(), L"def");

		parser.SetValue(L"A", L"Number", L"2");
		Assert::AreEqual(parser.ReadInt(L"A", L"Number", 0), 2);

		parser.SetValue(L"A", L"NumberFloat", L"1.23");
		Assert::AreEqual(parser.ReadFloat(L"A", L"NumberFloat", 0.0), 1.23);

		parser.SetValue(L"A", L"NumberU64", L"18446744073709551615");
		Assert::AreEqual(parser.ReadUInt64(L"A", L"NumberU64", 0Ui64), 18446744073709551615);

		parser.SetValue(L"A", L"Formula", L"(1 + 2)");
		Assert::AreEqual(parser.ReadInt(L"A", L"Formula", 0), 3);
		Assert::AreEqual(parser.ReadUInt(L"A", L"Formula", 0), 3U);
		Assert::AreEqual(parser.ReadFloat(L"A", L"Formula", 0.0), 3.0);

		parser.SetValue(L"A", L"Color1", L"AABBCCDD");
		parser.SetValue(L"A", L"Color2", L"170,187,204,221");
		parser.SetValue(L"A", L"Color3", L"AABBCC");
		parser.SetValue(L"A", L"Color4", L"170,187,204");

		const D2D1_COLOR_F& transparent = Gfx::Util::c_Transparent_Color_F;
		const UINT32 abc = 0xAABBCC;

		Assert::IsTrue(Gfx::Util::ColorFEquals(parser.ReadColor(L"A", L"Color1", transparent), D2D1::ColorF(abc, 221.0f / 255.0f)));
		Assert::IsTrue(Gfx::Util::ColorFEquals(parser.ReadColor(L"A", L"Color2", transparent), D2D1::ColorF(abc, 221.0f / 255.0f)));
		Assert::IsTrue(Gfx::Util::ColorFEquals(parser.ReadColor(L"A", L"Color3", transparent), D2D1::ColorF(abc, 1.0f)));
		Assert::IsTrue(Gfx::Util::ColorFEquals(parser.ReadColor(L"A", L"Color4", transparent), D2D1::ColorF(abc, 1.0f)));

		parser.SetValue(L"A", L"Rect", L"1,2,11,22");
		const RECT defRect = {};
		const RECT expRect = {1, 2, 11, 22};
		const RECT readRect = parser.ReadRECT(L"A", L"Rect", defRect);
		Assert::AreEqual(readRect.left, expRect.left);
		Assert::AreEqual(readRect.top, expRect.top);
		Assert::AreEqual(readRect.right, expRect.right);
		Assert::AreEqual(readRect.bottom, expRect.bottom);

		parser.SetValue(L"A", L"Floats", L"1.1;2.1;;3.1");
		std::vector<FLOAT> expFloats;
		expFloats.push_back(1.1f);
		expFloats.push_back(2.1f);
		expFloats.push_back(3.1f);
		Assert::IsTrue(parser.ReadFloats(L"A", L"Floats") == expFloats);
		Assert::IsTrue(parser.ReadFloats(L"A", L"FloatsNA").empty());
	}

	TEST_METHOD(TestVariables)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		parser.SetVariable(L"A", L"abc");
		parser.SetVariable(L"BB", L"def");
		parser.SetVariable(L"CCC", L"#A#");
		parser.SetVariable(L"Var", L"Normal");

		std::wstring string1 = L"A";
		Assert::IsFalse(parser.ReplaceVariables(string1));
		Assert::AreEqual(string1.c_str(), L"A");

		std::wstring string2 = L"#A#";
		Assert::IsTrue(parser.ReplaceVariables(string2));
		Assert::AreEqual(string2.c_str(), L"abc");

		std::wstring string3 = L"#*A*#";
		Assert::IsFalse(parser.ReplaceVariables(string3));
		Assert::AreEqual(string3.c_str(), L"#A#");

		std::wstring string4 = L"#A##a# #*CCC*# #BB##CCC#";
		Assert::IsTrue(parser.ReplaceVariables(string4));
		Assert::AreEqual(string4.c_str(), L"abcabc #CCC# def#A#");

		parser.SetValue(L"A", L"String", L"#A#");
		Assert::AreEqual(parser.ReadString(L"A", L"String", L"").c_str(), L"abc");

		parser.SetValue(L"A", L"String", L"%WINDIR%");
		Assert::AreNotEqual(parser.ReadString(L"A", L"String", L"").c_str(), L"%WINDIR%");

		parser.SetValue(L"A", L"String", L"#Var#");
		Assert::AreNotEqual(parser.ReadString(L"A", L"String", L"").c_str(), L"BuiltIn");
	}

	TEST_METHOD(TestNestedVariables)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		parser.SetVariable(L"Index", L"2");
		parser.SetVariable(L"Var2", L"Second");
		parser.SetVariable(L"Name", L"Var[#Index]");
		parser.SetVariable(L"Command", L"return '[#Var[#Index]]'");

		std::wstring string1 = L"[#Var[#Index]]";
		Assert::IsTrue(parser.ReplaceVariables(string1, true));
		Assert::AreEqual(string1.c_str(), L"Second");

		std::wstring string2 = L"[#[#Name]]";
		Assert::IsTrue(parser.ReplaceVariables(string2, true));
		Assert::AreEqual(string2.c_str(), L"Second");

		std::wstring string3 = L"[#Command]";
		Assert::IsTrue(parser.ReplaceVariables(string3, true));
		Assert::AreEqual(string3.c_str(), L"return 'Second'");

		std::wstring string4 = L"[#*Var[#Index]*]";
		Assert::IsTrue(parser.ReplaceVariables(string4, true));
		Assert::AreEqual(string4.c_str(), L"[#Var2]");
	}

	TEST_METHOD(TestNestedVariablesWithMeasures)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		parser.SetVariable(L"Index", L"2");
		parser.SetVariable(L"Var2", L"Second");

		TestMeasureString measureString2(L"MeasureString2");
		parser.SetValue(L"MeasureString2", L"String", L"Second");
		measureString2.Read(parser);
		parser.AddMeasure(&measureString2);

		TestMeasureString measureIndex(L"MeasureIndex");
		parser.SetValue(L"MeasureIndex", L"String", L"2");
		measureIndex.Read(parser);
		parser.AddMeasure(&measureIndex);

		std::wstring string1 = L"[#Var[&MeasureIndex]]";
		Assert::IsTrue(parser.ReplaceMeasures(string1));
		Assert::AreEqual(string1.c_str(), L"Second");

		std::wstring string2 = L"[&MeasureString[#Index]]";
		Assert::IsTrue(parser.ReplaceMeasures(string2));
		Assert::AreEqual(string2.c_str(), L"Second");

		std::wstring string3 = L"[&MeasureString[&MeasureIndex]]";
		Assert::IsTrue(parser.ReplaceMeasures(string3));
		Assert::AreEqual(string3.c_str(), L"Second");

		std::wstring string4 = L"[&MeasureScript:GetWelcome('[&MeasureString[#Index]]')]";
		Assert::IsTrue(parser.ReplaceMeasures(string4));
		Assert::AreEqual(string4.c_str(), L"[&MeasureScript:GetWelcome('Second')]");

		std::wstring string5 = L"[MeasureString2] [&MeasureString2] [#Var2]";
		Assert::IsTrue(parser.ReplaceMeasures(string5));
		Assert::AreEqual(string5.c_str(), L"Second Second Second");

		std::wstring string6 = L"[*MeasureString2*] [&MeasureString2]";
		Assert::IsTrue(parser.ReplaceMeasures(string6));
		Assert::AreEqual(string6.c_str(), L"[MeasureString2] Second");

		std::wstring string7 = L"[MeasureString[#Index]]";
		Assert::IsTrue(parser.ReplaceMeasures(string7));
		Assert::AreEqual(string7.c_str(), L"Second");

		std::wstring string8 = L"[MeasureString[MeasureIndex]]";
		Assert::IsTrue(parser.ReplaceMeasures(string8));
		Assert::AreEqual(string8.c_str(), L"[MeasureString2]");
	}

	TEST_METHOD(TestSingleLetterSectionVariables)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		TestMeasureString measure(L"M");
		parser.SetValue(L"M", L"String", L"Single");
		measure.Read(parser);
		parser.AddMeasure(&measure);

		std::wstring string1 = L"[M]";
		Assert::IsTrue(parser.ReplaceMeasures(string1));
		Assert::AreEqual(string1.c_str(), L"Single");

		std::wstring string2 = L"[#]";
		Assert::IsFalse(parser.ReplaceVariables(string2, true));
		Assert::AreEqual(string2.c_str(), L"[#]");
	}

	TEST_METHOD(TestMeasureSectionVariableSelectors)
	{
		Skin skin(L"", L"", false);
		ConfigParser& parser = skin.GetParser();
		parser.Initialize(L"", &skin, nullptr);

		TestMeasureString measure(L"MeasureString");
		parser.SetValue(L"MeasureString", L"String", L" !*'();:@test&=+$,/?#[ing]");
		parser.SetValue(L"MeasureString", L"MinValue", L"12");
		parser.SetValue(L"MeasureString", L"MaxValue", L"34");
		measure.Read(parser);
		parser.AddMeasure(&measure);

		AssertSectionVariableSelector(parser, L"MeasureString", L"MaxValue", L"34");
		AssertSectionVariableSelector(parser, L"MeasureString", L"MinValue", L"12");
		AssertSectionVariableSelector(parser, L"MeasureString", L"EncodeUrl", L"%20%21%2A%27%28%29%3B%3A%40test%26%3D%2B%24%2C%2F%3F%23%5Bing%5D");
		AssertSectionVariableSelector(parser, L"MeasureString", L"EscapeRegExp", L" !\\*'\\(\\);:@test&=\\+\\$,/\\?#\\[ing]");
	}

	TEST_METHOD(TestMeasureSectionVariableValueSelectors)
	{
		Skin skin(L"", L"", false);
		ConfigParser& parser = skin.GetParser();
		parser.Initialize(L"", &skin, nullptr);

		TestMeasure measure(&skin, L"Measure");
		parser.SetValue(L"Measure", L"MinValue", L"10");
		parser.SetValue(L"Measure", L"MaxValue", L"50");
		measure.Read(parser);
		measure.SetTestValue(30.0);
		parser.AddMeasure(&measure);

		AssertSectionVariableSelector(parser, L"Measure", L"%", L"50");
		AssertSectionVariableSelector(parser, L"Measure", L"%,1", L"50.0");
		AssertSectionVariableSelector(parser, L"Measure", L"/5", L"6");
		AssertSectionVariableSelector(parser, L"Measure", L"/5, 2", L"6.00");
		AssertSectionVariableSelector(parser, L"Measure", L"MaxValue:/5", L"10");
		AssertInvalidSectionVariableSelector(parser, L"Measure", L"MaxValue:%");
	}

	TEST_METHOD(TestMeterSectionVariableSelectors)
	{
		Skin skin(L"", L"", false);
		ConfigParser& parser = skin.GetParser();
		parser.Initialize(L"", &skin, nullptr);

		auto* meter = new TestMeter(&skin, L"Meter");
		meter->SetBounds(10, 20, 30, 40);
		AddMeterToSkinForTests(skin, meter);

		AssertSectionVariableSelector(parser, L"Meter", L"X", L"10");
		AssertSectionVariableSelector(parser, L"Meter", L"Y", L"20");
		AssertSectionVariableSelector(parser, L"Meter", L"W", L"30");
		AssertSectionVariableSelector(parser, L"Meter", L"H", L"40");
		AssertSectionVariableSelector(parser, L"Meter", L"XW", L"40");
		AssertSectionVariableSelector(parser, L"Meter", L"YH", L"60");
		AssertInvalidSectionVariableSelector(parser, L"Meter", L"Invalid");
	}

	TEST_METHOD(TestNestedVariableChain)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		parser.SetVariable(L"Chain1", L"[#Chain2]");
		parser.SetVariable(L"Chain2", L"[#Chain3]");
		parser.SetVariable(L"Chain3", L"Final");
		std::wstring string1 = L"[#Chain1]";
		Assert::IsTrue(parser.ReplaceVariables(string1, true));
		Assert::AreEqual(string1.c_str(), L"Final");

		parser.SetVariable(L"Deep1", L"[#Deep2]");
		parser.SetVariable(L"Deep2", L"[#Deep3]");
		parser.SetVariable(L"Deep3", L"[#Deep4]");
		parser.SetVariable(L"Deep4", L"[#Deep5]");
		parser.SetVariable(L"Deep5", L"[#Deep6]");
		parser.SetVariable(L"Deep6", L"[#Deep7]");
		parser.SetVariable(L"Deep7", L"Done");

		std::wstring string2 = L"[#Deep1]";
		Assert::IsTrue(parser.ReplaceVariables(string2, true));
		Assert::AreEqual(string2.c_str(), L"[#Deep6]");
	}

	TEST_METHOD(TestSelfReferencingNestedVariable)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		parser.SetVariable(L"SelfRef", L"[#SelfRef]");
		std::wstring string1 = L"[#SelfRef]";
		Assert::IsTrue(parser.ReplaceVariables(string1, true));
		Assert::AreEqual(string1.c_str(), L"[#SelfRef]");
	}

	TEST_METHOD(TestCharacterReferences)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		std::wstring string1 = L"[\\65][\\x42][\\X43]";
		Assert::IsTrue(parser.ReplaceVariables(string1, true));
		Assert::AreEqual(string1.c_str(), L"ABC");

		std::wstring string2 = L"[&MeasureScript:Escape('[\\39]quoted[\\39]')]";
		Assert::IsTrue(parser.ReplaceMeasures(string2));
		Assert::AreEqual(string2.c_str(), L"[&MeasureScript:Escape(''quoted'')]");

		std::wstring string3 = L"[\\0][\\x][\\x110000][\\NotANumber]";
		Assert::IsFalse(parser.ReplaceVariables(string3, true));
		Assert::AreEqual(string3.c_str(), L"[\\0][\\x][\\x110000][\\NotANumber]");

		std::wstring string4 = L"[\\*65*]";
		Assert::IsFalse(parser.ReplaceVariables(string4, true));
		Assert::AreEqual(string4.c_str(), L"[\\65]");
	}

	TEST_METHOD(TestBracketHeavyNonVariableStrings)
	{
		ConfigParser parser;
		parser.Initialize(L"");

		auto makeRegExp = [](size_t count) -> std::wstring
		{
			std::wstring result;
			result.reserve(count * 10);
			for (size_t i = 0; i < count; ++i)
			{
				result += L"(?:[A-Z])";
			}
			return result;
		};

		std::wstring string1 = makeRegExp(999);
		const std::wstring expected1 = string1;
		Assert::IsFalse(parser.ReplaceMeasures(string1));
		Assert::AreEqual(string1.c_str(), expected1.c_str());

		std::wstring string2 = makeRegExp(1000);
		const std::wstring expected2 = string2;
		Assert::IsFalse(parser.ReplaceMeasures(string2));
		Assert::AreEqual(string2.c_str(), expected2.c_str());
	}
};
