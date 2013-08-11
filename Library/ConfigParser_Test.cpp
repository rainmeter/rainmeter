/*
  Copyright (C) 2013 Rainmeter Team

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

#include "ConfigParser.h"
#include "../Common/UnitTest.h"

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
		Assert::AreEqual(parser.ReadUInt64(L"A", L"NumberU64", 0), 18446744073709551615);

		parser.SetValue(L"A", L"Formula", L"(1 + 2)");
		Assert::AreEqual(parser.ReadInt(L"A", L"Formula", 0), 3);
		Assert::AreEqual(parser.ReadUInt(L"A", L"Formula", 0), 3U);
		Assert::AreEqual(parser.ReadFloat(L"A", L"Formula", 0.0), 3.0);

		parser.SetValue(L"A", L"Color1", L"AABBCCDD");
		parser.SetValue(L"A", L"Color2", L"170,187,204,221");
		parser.SetValue(L"A", L"Color3", L"AABBCC");
		parser.SetValue(L"A", L"Color4", L"170,187,204");
		Assert::AreEqual(parser.ReadColor(L"A", L"Color1", 0x00000000), (Gdiplus::ARGB)0xDDAABBCC);
		Assert::AreEqual(parser.ReadColor(L"A", L"Color2", 0x00000000), (Gdiplus::ARGB)0xDDAABBCC);
		Assert::AreEqual(parser.ReadColor(L"A", L"Color3", 0x00000000), (Gdiplus::ARGB)0xFFAABBCC);
		Assert::AreEqual(parser.ReadColor(L"A", L"Color4", 0x00000000), (Gdiplus::ARGB)0xFFAABBCC);
		Assert::AreEqual(parser.ReadColor(L"A", L"ColorNA", 0x00000000), (Gdiplus::ARGB)0x00000000);

		parser.SetValue(L"A", L"Rect", L"1,2,11,22");
		const RECT defRect = {};
		const RECT expRect = {1, 2, 11, 22};
		const RECT readRect = parser.ReadRECT(L"A", L"Rect", defRect);
		Assert::AreEqual(readRect.left, expRect.left);
		Assert::AreEqual(readRect.top, expRect.top);
		Assert::AreEqual(readRect.right, expRect.right);
		Assert::AreEqual(readRect.bottom, expRect.bottom);

		parser.SetValue(L"A", L"Floats", L"1.1;2.1;;3.1");
		std::vector<Gdiplus::REAL> expFloats;
		expFloats.push_back(1.1f);
		expFloats.push_back(2.1f);
		expFloats.push_back(3.1f);
		Assert::IsTrue(parser.ReadFloats(L"A", L"Floats") == expFloats);
		Assert::IsTrue(parser.ReadFloats(L"A", L"FloatsNA").empty());
	}

	TEST_METHOD(TestVariables)
	{
		ConfigParser parser;
		parser.Initialize(L"");  // TODO: Better way to initialize without file.

		parser.SetVariable(L"A", L"abc");
		parser.SetVariable(L"BB", L"def");
		parser.SetVariable(L"CCC", L"#A#");
		parser.SetVariable(L"Var", L"Normal");
		parser.SetBuiltInVariable(L"Var", L"BuiltIn");

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
};
