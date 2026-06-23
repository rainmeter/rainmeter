/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinPosition.h"
#include "MonitorUtil.h"
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
		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo(monitors, 1);
		const MonitorInfo& monitor = monitorInfo.monitors[*position.monitor - 1];
		position.pos = position.ResolveLogicalPosition(value, monitor.logicalScreen.left, monitor.logicalScreen.right - monitor.logicalScreen.left);

		Assert::AreEqual(2, *position.monitor);
		Assert::AreEqual(2000, position.pos);
	}

	TEST_METHOD(TestResolvePhysicalPositionDefaultsToPrimaryMonitor)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100";
		y.option = L"50";

		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo({
			CreateMonitor(96, -1600, 0, 0, 900),
			CreateMonitor(144, 0, 0, 1920, 1080)
		}, 2);

		const UINT dpi = SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);

		Assert::IsFalse(x.monitor.has_value());
		Assert::IsFalse(y.monitor.has_value());
		Assert::AreEqual(144U, dpi);
		Assert::AreEqual(150, x.pos);
		Assert::AreEqual(75, y.pos);
	}

	TEST_METHOD(TestResolvePhysicalPositionUsesVirtualScreenForMonitorZero)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100@0";
		y.option = L"50";

		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo({
			CreateMonitor(120, 0, 0, 1920, 1080),
			CreateMonitor(96, 1920, 0, 3520, 900)
		}, 1);

		const UINT dpi = SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);

		Assert::AreEqual(0, *x.monitor);
		Assert::AreEqual(0, *y.monitor);
		Assert::AreEqual(120U, dpi);
		Assert::AreEqual(125, x.pos);
		Assert::AreEqual(63, y.pos);
	}

	TEST_METHOD(TestResolvePhysicalPositionInheritsYMonitor)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"100";
		y.option = L"50@2";

		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo({
			CreateMonitor(96, 0, 0, 1920, 1080),
			CreateMonitor(144, 1920, 0, 4320, 1440)
		}, 1);

		SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);

		Assert::AreEqual(2, *x.monitor);
		Assert::AreEqual(2, *y.monitor);
		Assert::AreEqual(2070, x.pos);
		Assert::AreEqual(75, y.pos);
	}

	TEST_METHOD(TestResolvePhysicalPositionSelectsDpiFromConvertedPosition)
	{
		SkinPosition x(L'R');
		SkinPosition y(L'B');
		x.option = L"2000";
		y.option = L"100";

		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo({
			CreateMonitor(120, 0, 0, 1920, 1080),
			CreateMonitor(96, 1920, 0, 3520, 900)
		}, 1);

		const UINT dpi = SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);
		Assert::AreEqual(96U, dpi);
		Assert::AreEqual(2384, x.pos);
		Assert::AreEqual(100, y.pos);

		x.option = L"-100R";
		const UINT dpi2 = SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);
		Assert::AreEqual(96U, dpi2);
		Assert::AreEqual(2020, x.pos);
		Assert::AreEqual(100, y.pos);

		x.option = L"100R";
		const UINT dpi3 = SkinPosition::ResolvePhysicalPosition(x, y, 200, 100, 1.0f, monitorInfo);
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
		position.pos = position.ResolveLogicalPosition(value, 0, 1000);

		Assert::AreEqual(20, position.anchorPos);
		Assert::AreEqual(80, position.pos);
	}

	TEST_METHOD(TestUpdateOptionValueRoundTrip)
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

		MultiMonitorInfo monitorInfo = CreateMultiMonitorInfo(monitors, 1);
		const MonitorInfo& monitor = monitorInfo.monitors[*position.monitor - 1];
		const int referenceOrigin = monitor.logicalScreen.left;
		const int referenceExtent = monitor.logicalScreen.right - monitor.logicalScreen.left;
		const int logicalPos = position.ResolveLogicalPosition(value, referenceOrigin, referenceExtent);
		position.UpdateOptionValue(logicalPos, referenceOrigin, referenceExtent);
		Assert::AreEqual(L"25.00000%R@2", position.option.c_str());
	}

private:
	static MultiMonitorInfo CreateMultiMonitorInfo(std::vector<MonitorInfo> monitors, int primary)
	{
		MultiMonitorInfo monitorInfo = {};
		monitorInfo.primary = primary;
		monitorInfo.monitors = std::move(monitors);
		monitorInfo.UpdateLogicalMonitorInfo();
		return monitorInfo;
	}

	static MonitorInfo CreateMonitor(UINT dpi, LONG left, LONG top, LONG right, LONG bottom)
	{
		MonitorInfo monitor = {};
		monitor.active = true;
		monitor.dpi = dpi;
		monitor.screen = { left, top, right, bottom };
		return monitor;
	}
};
