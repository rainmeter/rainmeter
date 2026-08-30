// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

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

	TEST_METHOD(TestWidenToBuffer)
	{
		// A small inline capacity so that the heap spill is easy to exercise.
		StringBuffer<WCHAR, 4> buffer;

		Widen("test", -1, CP_ACP, buffer);
		Assert::AreEqual(L"test", buffer.c_str());
		Assert::AreEqual((size_t)4, buffer.length());
		Assert::AreEqual((size_t)4, buffer.capacity());  // Fits inline exactly.

		Widen("testing", -1, CP_ACP, buffer);
		Assert::AreEqual(L"testing", buffer.c_str());
		Assert::AreEqual((size_t)7, buffer.length());
		Assert::AreEqual((size_t)7, buffer.capacity());  // Spilled to the heap.

		// A buffer that has spilled is reused for a result that would have fit inline.
		Widen("te", -1, CP_ACP, buffer);
		Assert::AreEqual(L"te", buffer.c_str());
		Assert::AreEqual((size_t)2, buffer.length());

		Widen("test", 2, CP_ACP, buffer);
		Assert::AreEqual(L"te", buffer.c_str());

		Widen("", -1, CP_ACP, buffer);
		Assert::AreEqual(L"", buffer.c_str());
		Assert::IsTrue(buffer.empty());

		Widen("test", 0, CP_ACP, buffer);
		Assert::AreEqual(L"", buffer.c_str());

		Widen(nullptr, -1, CP_ACP, buffer);
		Assert::AreEqual(L"", buffer.c_str());

		// A surrogate pair straddling the inline/heap boundary.
		WidenUTF8("abc\xf0\x9f\x92\xa9", -1, buffer);
		Assert::AreEqual(L"abc\U0001F4A9", buffer.c_str());
		Assert::AreEqual((size_t)5, buffer.length());

		// A buffer filled by its constructor, as returned by value from LuaHelper::ToWide().
		const StringBuffer<WCHAR, 4> constructed([](StringBuffer<WCHAR, 4>& b)
		{
			Widen("filled", -1, CP_ACP, b);
		});
		Assert::AreEqual(L"filled", constructed.c_str());
		Assert::AreEqual((size_t)6, constructed.length());
	}

	TEST_METHOD(TestNarrowToBuffer)
	{
		StringBuffer<char, 4> buffer;

		Narrow(L"test", -1, CP_ACP, buffer);
		Assert::AreEqual("test", buffer.c_str());
		Assert::AreEqual((size_t)4, buffer.length());
		Assert::AreEqual((size_t)4, buffer.capacity());

		Narrow(L"testing", -1, CP_ACP, buffer);
		Assert::AreEqual("testing", buffer.c_str());
		Assert::AreEqual((size_t)7, buffer.length());
		Assert::AreEqual((size_t)7, buffer.capacity());

		Narrow(L"test", 2, CP_ACP, buffer);
		Assert::AreEqual("te", buffer.c_str());

		Narrow(L"", -1, CP_ACP, buffer);
		Assert::AreEqual("", buffer.c_str());
		Assert::IsTrue(buffer.empty());

		Narrow(nullptr, -1, CP_ACP, buffer);
		Assert::AreEqual("", buffer.c_str());

		// Three UTF-8 bytes out of one wide character, so the result spills where the input did not.
		NarrowUTF8(L"\u0422\u0114st", -1, buffer);
		Assert::AreEqual("\xd0\xa2\xc4\x94st", buffer.c_str());
		Assert::AreEqual((size_t)6, buffer.length());

		NarrowUTF8(L"\u0422\u0114st", 1, buffer);
		Assert::AreEqual("\xd0\xa2", buffer.c_str());

		// The conversion result is a view over the buffer.
		const std::string_view view = buffer;
		Assert::AreEqual((size_t)2, view.length());
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

	TEST_METHOD(TestTruncateWithEllipsis)
	{
		Assert::AreEqual(L"test", TruncateWithEllipsis(L"test", 4).c_str());
		Assert::AreEqual(L"tes\u2026", TruncateWithEllipsis(L"testing", 4).c_str());
		Assert::AreEqual(L"", TruncateWithEllipsis(L"test", 0).c_str());
	}
};

}  // namespace StringUtil
