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
		SkinPosition position(L'R');
		position.option = L"10R";

		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 100, 1000);

		Assert::IsTrue(position.fromOpposite);
		Assert::AreEqual(1090, position.pos);
	}

	TEST_METHOD(TestPercentageOption)
	{
		SkinPosition position(L'R');
		position.option = L"25%";

		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 100, 800);

		Assert::IsTrue(position.percentage);
		Assert::AreEqual(300, position.pos);
	}

	TEST_METHOD(TestBottomEdgeOption)
	{
		SkinPosition position(L'B');
		position.option = L"10B";

		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 50, 600);

		Assert::IsTrue(position.fromOpposite);
		Assert::AreEqual(640, position.pos);
	}

	TEST_METHOD(TestNegativeOption)
	{
		SkinPosition position(L'R');
		position.option = L"-100";

		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 10, 1000);

		Assert::AreEqual(-90, position.pos);
	}

	TEST_METHOD(TestNegativeOptionFromRight)
	{
		SkinPosition position(L'R');
		position.option = L"-100R";

		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 50, 1000);

		Assert::IsTrue(position.fromOpposite);
		Assert::AreEqual(1150, position.pos);
	}

	TEST_METHOD(TestAnchorOption)
	{
		SkinPosition position(L'R');
		position.option = L"100";
		position.anchorOption = L"10";

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::AreEqual(10, position.anchorPos);
		Assert::AreEqual(90, position.pos);
	}

	TEST_METHOD(TestOppositeAnchorOption)
	{
		SkinPosition position(L'R');
		position.option = L"100";
		position.anchorOption = L"10R";

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(position.anchorFromOpposite);
		Assert::AreEqual(190, position.anchorPos);
		Assert::AreEqual(-90, position.pos);
	}

	TEST_METHOD(TestPercentageAnchorOption)
	{
		SkinPosition position(L'R');
		position.option = L"100";
		position.anchorOption = L"25%";

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(position.anchorPercentage);
		Assert::AreEqual(50, position.anchorPos);
		Assert::AreEqual(50, position.pos);
	}

	TEST_METHOD(TestOppositePercentageAnchorOption)
	{
		SkinPosition position(L'R');
		position.option = L"100";
		position.anchorOption = L"25%R";

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::IsTrue(position.anchorPercentage);
		Assert::IsTrue(position.anchorFromOpposite);
		Assert::AreEqual(150, position.anchorPos);
		Assert::AreEqual(-50, position.pos);
	}
	TEST_METHOD(TestZoomChangesAnchorPosition)
	{
		SkinPosition position(L'R');
		position.option = L"100";
		position.anchorOption = L"10";

		position.ParseAnchorOption(200, 1.0f);
		Assert::AreEqual(10, position.anchorPos);

		position.ParseAnchorOption(200, 2.0f);
		const float value = position.ParseWindowOption({});
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::AreEqual(20, position.anchorPos);
		Assert::AreEqual(80, position.pos);
	}
};
