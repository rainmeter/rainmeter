/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "MathParser.h"
#include "UnitTest.h"

namespace MathParser {

TEST_CLASS(Common_MathParser_Test)
{
public:
	void ParseAssert(double expected, const WCHAR* formula)
	{
		double value = 0.0;
		Assert::IsNull(Parse(formula, &value));
		Assert::AreEqual(expected, value);
	}

	TEST_METHOD(TestParse)
	{
		ParseAssert(5.0, L"5");
		ParseAssert(5.0, L"      5      ");
		ParseAssert(5.0, L"((((5))))");
		ParseAssert(10.0, L"(5 + 5)");
		ParseAssert(-10.0, L"(-5 - 5)");
		ParseAssert(10.0, L"-(-5-5)");
		ParseAssert(10.0, L"-(-5+-5)");
		ParseAssert(1.0, L"1 && 1");
		ParseAssert(0.0, L"1 && 0");
		ParseAssert(1.0, L"1 || 1");
		ParseAssert(1.0, L"1 || -1");
		ParseAssert(0.0, L"1 < 2 && 2 > 1");
		ParseAssert(1.0, L"(1 < 2) && (2 > 1)");
		ParseAssert(sin(50.0), L"sin(50)");
		ParseAssert(-sin(100.0), L"-sin((25 + 25) * 2)");
		ParseAssert(1.0, L"01");
		ParseAssert(11.0, L"011");
		ParseAssert(3.0, L"0b11");
		ParseAssert(10.0, L"0xA");
		ParseAssert(10.0, L"0o12");
		ParseAssert(2.0, L"1 ? 2 : 3");
		ParseAssert(3.0, L"0 ? 2 : 3");
		ParseAssert(5.0, L"1 ? (2 + 3) : 3");
		ParseAssert(2.0, L"1 ? 2 : 3");
		ParseAssert(4.0, L"1 ? 2 : 0 ? 4 : 5");
		ParseAssert(4.0, L"1 ? 2 : 1 ? 4 : 5");
		ParseAssert(2.0, L"1 ? 2 : (1 ? 4 : 5)");
		ParseAssert(5.0, L"0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:(0?1:5)))))))))))))))))))))))))))))");
		ParseAssert(1.0, L"trunc(1.5)");
		ParseAssert(-1.0, L"trunc(-1.5)");
		ParseAssert(2.0, L"round(1.555)");
		ParseAssert(1.56, L"round(1.555, 2)");
		ParseAssert(-2.0, L"min(1, -2)");
		ParseAssert(3.0, L"max(3, -2)");
		ParseAssert(3.0, L"clamp(6, -2, 3)");

		double value;

		Assert::IsNotNull(Parse(L"1 ? 0 ? 4 : 5 : 3", &value));
		Assert::IsNotNull(Parse(L"++1", &value));
		Assert::IsNotNull(Parse(L"((1)", &value));
		Assert::IsNotNull(Parse(L"1 / 0", &value));
		Assert::IsNotNull(Parse(L"1 &&", &value));
		Assert::IsNotNull(Parse(L"a", &value));

		Assert::IsNull(Parse(L"e", &value));
		Assert::AreEqual(2, (int)value);

		Assert::IsNull(Parse(L"pi", &value));
		Assert::AreEqual(3, (int)value);

		Assert::IsNull(Parse(L"pi", &value, GetValueHelper));
		Assert::AreEqual(3, (int)value);

		Assert::IsNull(Parse(L"a + 5", &value, GetValueHelper));
		Assert::AreEqual(15.0, value);

		Assert::IsNull(Parse(L"bbb", &value, GetValueHelper));
		Assert::AreEqual(20.0, value);

		Assert::IsNull(Parse(L"(ccc_)", &value, GetValueHelper, (void*)1));
		Assert::AreEqual(30.0, value);
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
