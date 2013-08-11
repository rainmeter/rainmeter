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

#include "StringUtil.h"
#include "UnitTest.h"

namespace StringUtil {

TEST_CLASS(Common_StringUtil_Test)
{
public:
	TEST_METHOD(TestWiden)
	{
		Assert::AreEqual(Widen("test").c_str(), L"test");
		Assert::AreEqual(Widen("test", 2).c_str(), L"te");
		Assert::AreEqual(WidenUTF8("\xd0\xa2\xc4\x94st").c_str(), L"\u0422\u0114st");
		Assert::AreEqual(WidenUTF8("\xd0\xa2\xc4\x94st", 2).c_str(), L"\u0422");
	}

	TEST_METHOD(TestNarrow)
	{
		Assert::AreEqual(Narrow(L"test").c_str(), "test");
		Assert::AreEqual(Narrow(L"test", 2).c_str(), "te");
		Assert::AreEqual(NarrowUTF8(L"\u0422\u0114st").c_str(), "\xd0\xa2\xc4\x94st");
		Assert::AreEqual(NarrowUTF8(L"\u0422\u0114st", 1).c_str(), "\xd0\xa2");
	}
};

}  // namespace StringUtil
