/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinPosition.h"
#include "../Common/UnitTest.h"

TEST_CLASS(Library_SkinPosition_Test)
{
public:
	TEST_METHOD(TestOppositeEdgeOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"10R";

		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 100, 1000);

		Assert::IsTrue(option.fromOpposite);
		Assert::AreEqual(1090, resolvedPos);
	}

	TEST_METHOD(TestPercentageOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"25%";

		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 100, 800);

		Assert::IsTrue(option.percentage);
		Assert::AreEqual(300, resolvedPos);
	}

	TEST_METHOD(TestBottomEdgeOption)
	{
		SkinPositionOption option(L'B');
		option.windowOption = L"10B";

		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 50, 600);

		Assert::IsTrue(option.fromOpposite);
		Assert::AreEqual(640, resolvedPos);
	}

	TEST_METHOD(TestNegativeOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"-100";

		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 10, 1000);

		Assert::AreEqual(-90, resolvedPos);
	}

	TEST_METHOD(TestNegativeOptionFromRight)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"-100R";

		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 50, 1000);

		Assert::IsTrue(option.fromOpposite);
		Assert::AreEqual(1150, resolvedPos);
	}

	TEST_METHOD(TestAnchorOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"100";
		option.anchorOption = L"10";

		option.ParseAnchorOption(200, 1.0f);
		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 0, 1000);

		Assert::AreEqual(10, option.anchorPos);
		Assert::AreEqual(90, resolvedPos);
	}

	TEST_METHOD(TestOppositeAnchorOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"100";
		option.anchorOption = L"10R";

		option.ParseAnchorOption(200, 1.0f);
		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(option.anchorFromOpposite);
		Assert::AreEqual(190, option.anchorPos);
		Assert::AreEqual(-90, resolvedPos);
	}

	TEST_METHOD(TestPercentageAnchorOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"100";
		option.anchorOption = L"25%";

		option.ParseAnchorOption(200, 1.0f);
		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(option.anchorPercentage);
		Assert::AreEqual(50, option.anchorPos);
		Assert::AreEqual(50, resolvedPos);
	}

	TEST_METHOD(TestOppositePercentageAnchorOption)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"100";
		option.anchorOption = L"25%R";

		option.ParseAnchorOption(200, 1.0f);
		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(option.anchorPercentage);
		Assert::IsTrue(option.anchorFromOpposite);
		Assert::AreEqual(150, option.anchorPos);
		Assert::AreEqual(-50, resolvedPos);
	}
	TEST_METHOD(TestZoomChangesAnchorPosition)
	{
		SkinPositionOption option(L'R');
		option.windowOption = L"100";
		option.anchorOption = L"10";

		option.ParseAnchorOption(200, 1.0f);
		Assert::AreEqual(10, option.anchorPos);

		option.ParseAnchorOption(200, 2.0f);
		const float value = option.ParseWindowOption({});
		const int resolvedPos = option.ResolveLogicalPosition(value, 0, 1000);

		Assert::AreEqual(20, option.anchorPos);
		Assert::AreEqual(80, resolvedPos);
	}
};
