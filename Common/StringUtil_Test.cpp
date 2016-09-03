/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StringUtil.h"
#include "UnitTest.h"

namespace StringUtil {

TEST_CLASS(Common_StringUtil_Test)
{
public:
	TEST_METHOD(TestWiden)
	{
		Assert::AreEqual(L"test", Widen("test").c_str());
		Assert::AreEqual(L"te", Widen("test", 2).c_str());
		Assert::AreEqual(L"\u0422\u0114st", WidenUTF8("\xd0\xa2\xc4\x94st").c_str());
		Assert::AreEqual(L"\u0422", WidenUTF8("\xd0\xa2\xc4\x94st", 2).c_str());
	}

	TEST_METHOD(TestNarrow)
	{
		Assert::AreEqual("test", Narrow(L"test").c_str());
		Assert::AreEqual("te", Narrow(L"test", 2).c_str());
		Assert::AreEqual("\xd0\xa2\xc4\x94st", NarrowUTF8(L"\u0422\u0114st").c_str());
		Assert::AreEqual("\xd0\xa2", NarrowUTF8(L"\u0422\u0114st", 1).c_str());
	}

	TEST_METHOD(TestEscapeRegExp)
	{
		std::wstring str = L"\\^$|(test)[{. ing+*?";
		EscapeRegExp(str);
		Assert::AreEqual(L"\\\\\\^\\$\\|\\(test\\)\\[\\{\\. ing\\+\\*\\?", str.c_str());
	}

	TEST_METHOD(TestEncodeUrl)
	{
		std::wstring str = L" !*'();:@test&=+$,/?#[ing]";
		EncodeUrl(str);
		Assert::AreEqual(L"%20%21%2A%27%28%29%3B%3A%40test%26%3D%2B%24%2C%2F%3F%23%5Bing%5D", str.c_str());
	}
};

}  // namespace StringUtil
