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

#include "MathParser.h"
#include "UnitTest.h"

namespace MathParser {

TEST_CLASS(Common_MathParser_Test)
{
public:
	void ParseAssert(const WCHAR* formula, double expected)
	{
		double value = 0.0;
		Assert::IsNull(Parse(formula, &value));
		Assert::AreEqual(value, expected);
	}

	TEST_METHOD(TestParse)
	{
		ParseAssert(L"5", 5.0);
		ParseAssert(L"      5      ", 5.0);
		ParseAssert(L"((((5))))", 5.0);
		ParseAssert(L"(5 + 5)", 10.0);
		ParseAssert(L"(-5 - 5)", -10.0);
		ParseAssert(L"-(-5-5)", 10.0);
		ParseAssert(L"-(-5+-5)", 10.0);
		ParseAssert(L"1 && 1", 1.0);
		ParseAssert(L"1 && 0", 0.0);
		ParseAssert(L"1 || 1", 1.0);
		ParseAssert(L"1 || -1", 1.0);
		ParseAssert(L"1 < 2 && 2 > 1", 0.0);
		ParseAssert(L"(1 < 2) && (2 > 1)", 1.0);
		ParseAssert(L"sin(50)", sin(50.0));
		ParseAssert(L"-sin((25 + 25) * 2)", -sin(100.0));
		ParseAssert(L"01", 1.0);
		ParseAssert(L"011", 11.0);
		ParseAssert(L"0b11", 3.0);
		ParseAssert(L"0xA", 10.0);
		ParseAssert(L"0o12", 10.0);
		ParseAssert(L"1 ? 2 : 3", 2.0);
		ParseAssert(L"0 ? 2 : 3", 3.0);
		ParseAssert(L"1 ? (2 + 3) : 3", 5.0);
		ParseAssert(L"1 ? 2 : 3", 2.0);
		ParseAssert(L"1 ? 2 : 0 ? 4 : 5", 4.0);
		ParseAssert(L"1 ? 2 : 1 ? 4 : 5", 4.0);
		ParseAssert(L"1 ? 2 : (1 ? 4 : 5)", 2.0);
		ParseAssert(L"0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:5)))))))))))))))))))))))))))))", 5.0);

		double value;

		Assert::IsNotNull(Parse(L"1 ? 0 ? 4 : 5 : 3", &value));
		Assert::IsNotNull(Parse(L"++1", &value));
		Assert::IsNotNull(Parse(L"((1)", &value));
		Assert::IsNotNull(Parse(L"1 / 0", &value));
		Assert::IsNotNull(Parse(L"1 &&", &value));
		Assert::IsNotNull(Parse(L"a", &value));

		Assert::IsNull(Parse(L"e", &value));
		Assert::AreEqual((int)value, 2);

		Assert::IsNull(Parse(L"pi", &value));
		Assert::AreEqual((int)value, 3);

		Assert::IsNull(Parse(L"pi", &value, GetValueHelper));
		Assert::AreEqual((int)value, 3);

		Assert::IsNull(Parse(L"a + 5", &value, GetValueHelper));
		Assert::AreEqual(value, 15.0);

		Assert::IsNull(Parse(L"bbb", &value, GetValueHelper));
		Assert::AreEqual(value, 20.0);

		Assert::IsNull(Parse(L"(ccc_)", &value, GetValueHelper, (void*)1));
		Assert::AreEqual(value, 30.0);
	}

	static bool GetValueHelper(const WCHAR* str, int len, double* value, void* context)
	{
		if (wcsncmp(str, L"a", len) == 0)
		{
			*value = 10.0;
			return true;
		}
		else if (wcsncmp(str, L"bbb", len) == 0)
		{
			*value = 20.0;
			return true;
		}
		else if (wcsncmp(str, L"ccc_", len) == 0 && context == (void*)1)
		{
			*value = 30.0;
			return true;
		}
		else if (wcsncmp(str, L"pi", len) == 0)
		{
			*value = 40.0;
			return true;
		}

		return false;
	}
};

}  // namespace StringUtil
