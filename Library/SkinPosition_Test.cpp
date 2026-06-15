/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Skin.h"
#include "System.h"
#include "../Common/UnitTest.h"

TEST_CLASS(Library_SkinPosition_Test)
{
public:
	TEST_METHOD(TestOppositeEdgeOption)
	{
		SkinPosition position(L'R');
		position.option = L"10R";

		const float value = position.ParseWindowOption({});
		position.ComputePosition(value, 100, 1000, USER_DEFAULT_SCREEN_DPI);

		Assert::IsTrue(position.fromOpposite);
		Assert::AreEqual(1090, position.pos);
	}

	TEST_METHOD(TestPercentageOption)
	{
		SkinPosition position(L'R');
		position.option = L"25%";

		const float value = position.ParseWindowOption({});
		position.ComputePosition(value, 100, 800, USER_DEFAULT_SCREEN_DPI);

		Assert::IsTrue(position.percentage);
		Assert::AreEqual(300, position.pos);
	}

	TEST_METHOD(TestBottomEdgeOption)
	{
		SkinPosition position(L'B');
		position.option = L"10B";

		const float value = position.ParseWindowOption({});
		position.ComputePosition(value, 50, 600, USER_DEFAULT_SCREEN_DPI);

		Assert::IsTrue(position.fromOpposite);
		Assert::AreEqual(640, position.pos);
	}

	TEST_METHOD(TestNegativeOption)
	{
		SkinPosition position(L'R');
		position.option = L"-100";

		const float value = position.ParseWindowOption({});
		position.ComputePosition(value, 10, 1000, USER_DEFAULT_SCREEN_DPI);

		Assert::AreEqual(-90, position.pos);
	}

	TEST_METHOD(TestNegativeOptionFromRight)
	{
		SkinPosition position(L'R');
		position.option = L"-100R";

		const float value = position.ParseWindowOption({});
		position.ComputePosition(value, 50, 1000, USER_DEFAULT_SCREEN_DPI);

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
		position.ComputePosition(value, 0, 1000, USER_DEFAULT_SCREEN_DPI);

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
		position.ComputePosition(value, 0, 1000, USER_DEFAULT_SCREEN_DPI);

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
		position.ComputePosition(value, 0, 1000, USER_DEFAULT_SCREEN_DPI);

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
		position.ComputePosition(value, 0, 1000, USER_DEFAULT_SCREEN_DPI);

		Assert::IsTrue(position.anchorPercentage);
		Assert::IsTrue(position.anchorFromOpposite);
		Assert::AreEqual(150, position.anchorPos);
		Assert::AreEqual(-50, position.pos);
	}

	TEST_METHOD(TestMixedDpiMonitor)
	{
		SkinPosition position(L'R');
		position.option = L"100@2";
		position.anchorOption = L"20";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(96, 0, 0, 1920, 1080),
			CreateMonitor(144, 1920, 0, 4320, 1440)
		};

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption(monitors);
		Assert::IsTrue(position.monitor.has_value());
		const MonitorInfo& monitor = monitors[*position.monitor - 1];
		position.ComputePosition(value, monitor.screen.left, monitor.screen.right - monitor.screen.left, monitor.dpi);

		Assert::AreEqual(2, *position.monitor);
		Assert::AreEqual(2040, position.pos);
	}

	TEST_METHOD(TestComputePositionDefaultsToPrimaryMonitor)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100";
		y.option = L"50";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(96, -1600, 0, 0, 900),
			CreateMonitor(144, 0, 0, 1920, 1080)
		};
		const RECT virtualScreen = { -1600, 0, 1920, 1080 };

		const UINT dpi = SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 2, virtualScreen, 96);

		Assert::IsFalse(x.monitor.has_value());
		Assert::IsFalse(y.monitor.has_value());
		Assert::AreEqual(144U, dpi);
		Assert::AreEqual(150, x.pos);
		Assert::AreEqual(75, y.pos);
	}

	TEST_METHOD(TestComputePositionUsesVirtualScreenForMonitorZero)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100@0";
		y.option = L"50";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(120, 0, 0, 1920, 1080),
			CreateMonitor(96, 1920, 0, 3520, 900)
		};
		const RECT virtualScreen = { 0, 0, 3520, 1080 };

		const UINT dpi = SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 1, virtualScreen, 96);

		Assert::AreEqual(0, *x.monitor);
		Assert::AreEqual(0, *y.monitor);
		Assert::AreEqual(96U, dpi);
		Assert::AreEqual(100, x.pos);
		Assert::AreEqual(50, y.pos);
	}

	TEST_METHOD(TestComputePositionInheritsYMonitor)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100";
		y.option = L"50@2";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(96, 0, 0, 1920, 1080),
			CreateMonitor(144, 1920, 0, 4320, 1440)
		};
		const RECT virtualScreen = { 0, 0, 4320, 1440 };

		SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 1, virtualScreen, 96);

		Assert::AreEqual(2, *x.monitor);
		Assert::AreEqual(2, *y.monitor);
		Assert::AreEqual(2070, x.pos);
		Assert::AreEqual(75, y.pos);
	}

	TEST_METHOD(TestComputePositionSelectsDpiFromUnscaledPosition)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"2000";
		y.option = L"100";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(120, 0, 0, 1920, 1080),
			CreateMonitor(96, 1920, 0, 3520, 900)
		};
		const RECT virtualScreen = { 0, 0, 3520, 1080 };

		const UINT dpi = SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 1, virtualScreen, 96);
		Assert::AreEqual(96U, dpi);
		Assert::AreEqual(2000, x.pos);
		Assert::AreEqual(100, y.pos);

		x.option = L"-100R";
		const UINT dpi2 = SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 1, virtualScreen, 96);
		Assert::AreEqual(96U, dpi2);
		Assert::AreEqual(2020, x.pos);
		Assert::AreEqual(100, y.pos);

		x.option = L"100R";
		const UINT dpi3 = SkinPosition::ComputePositionFromOptions(x, y, 200, 100, 1.0f, monitors, 1, virtualScreen, 96);
		Assert::AreEqual(120U, dpi3);
		Assert::AreEqual(1795, x.pos);
		Assert::AreEqual(125, y.pos);
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
		position.ComputePosition(value, 0, 1000, USER_DEFAULT_SCREEN_DPI);

		Assert::AreEqual(20, position.anchorPos);
		Assert::AreEqual(80, position.pos);
	}

	TEST_METHOD(TestComputeWindowOptionRoundTrip)
	{
		SkinPosition position(L'R');
		position.option = L"25.00000%R@2";
		position.anchorOption = L"10R";

		const std::vector<MonitorInfo> monitors = {
			CreateMonitor(96, 0, 0, 1920, 1080),
			CreateMonitor(144, 1920, 0, 4320, 1440)
		};

		position.ParseAnchorOption(200, 1.0f);
		const float value = position.ParseWindowOption(monitors);
		Assert::IsTrue(position.monitor.has_value());

		const MonitorInfo& monitor = monitors[*position.monitor - 1];
		const int origin = monitor.screen.left;
		const int extent = monitor.screen.right - monitor.screen.left;
		position.ComputePosition(value, origin, extent, monitor.dpi);
		position.ComputeWindowOption(origin, extent, monitor.dpi);
		Assert::AreEqual(L"25.00000%R@2", position.option.c_str());
	}

private:
	static MonitorInfo CreateMonitor(UINT dpi, LONG left, LONG top, LONG right, LONG bottom)
	{
		MonitorInfo monitor = {};
		monitor.active = true;
		monitor.dpi = dpi;
		monitor.screen = { left, top, right, bottom };
		return monitor;
	}
};
