// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Measure.h"
#include "../Common/UnitTest.h"

class SubstituteTestMeasure : public Measure
{
public:
	SubstituteTestMeasure() : Measure(nullptr, L"TestMeasure") {}

	bool SetSubstitute(std::wstring_view value, bool regexp = false)
	{
		m_Substitute.clear();
		m_RegExpSubstitute = regexp;
		return ParseSubstitute(value);
	}

	std::wstring ApplySubstitute(std::wstring_view value)
	{
		return std::wstring(CheckSubstitute(value));
	}

	UINT GetTypeID() override { return TypeID<SubstituteTestMeasure>(); }

protected:
	void UpdateValue() override {}
};

TEST_CLASS(Library_Measure_Test)
{
public:
	TEST_METHOD(TestPlainSubstitute)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"first\":\"1\",\"second\":\"2"));
		Assert::AreEqual(L"1 2", measure.ApplySubstitute(L"first second").c_str());
	}

	TEST_METHOD(TestEmptyPatternSubstitute)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"\":\"N/A"));
		Assert::AreEqual(L"N/A", measure.ApplySubstitute(L"").c_str());
	}

	TEST_METHOD(TestQuotedDelimiters)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"a,b\":\"c:d"));
		Assert::AreEqual(L"c:d", measure.ApplySubstitute(L"a,b").c_str());
	}

	TEST_METHOD(TestMixedQuotes)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"\"first\":'second'"));
		Assert::AreEqual(L"second", measure.ApplySubstitute(L"first").c_str());
	}

	TEST_METHOD(TestRegexpSubstituteCanBeReused)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(LR"regexp((\d+)":"<\1>)regexp", true));
		Assert::AreEqual(L"a<12>b", measure.ApplySubstitute(L"a12b").c_str());
		Assert::AreEqual(L"c<34>d", measure.ApplySubstitute(L"c34d").c_str());
	}

	TEST_METHOD(TestQuotedPairSeparator)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(LR"(a"":""","b":"c)", true));
		Assert::AreEqual(L"c", measure.ApplySubstitute(L"ab").c_str());
	}

	TEST_METHOD(TestTrailingWhitespaceReplacement)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"a\":\"  ", true));
		Assert::AreEqual(L"  ", measure.ApplySubstitute(L"a").c_str());
	}

	TEST_METHOD(TestInvalidRegexpUsesPlainSubstitute)
	{
		SubstituteTestMeasure measure;
		Assert::IsTrue(measure.SetSubstitute(L"[\":\"x", true));
		Assert::AreEqual(L"axbx", measure.ApplySubstitute(L"a[b[").c_str());
	}

	TEST_METHOD(TestInvalidSubstitute)
	{
		SubstituteTestMeasure measure;
		Assert::IsFalse(measure.SetSubstitute(L"first\":\"1\",\"broken"));
	}
};
