/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MonitorUtil.h"
#include "../Common/UnitTest.h"

TEST_CLASS(Library_MonitorUtil_Test)
{
public:
	TEST_METHOD(TestMultiMonitorInfoConvertsPhysicalToLogicalAcrossDpiSpans)
	{
		MultiMonitorInfo monitorInfo = {};
		monitorInfo.primary = 1;
		monitorInfo.monitors = {
			CreateMonitor(96, 0, 0, 1920, 1080),
			CreateMonitor(144, 1920, 0, 4320, 1440)
		};
		monitorInfo.UpdateLogicalMonitorInfo();

		const POINT logical = monitorInfo.PhysicalToLogical({ 2040, 300 });
		Assert::AreEqual(2000L, logical.x);
		Assert::AreEqual(200L, logical.y);

		UINT dpi = 0;
		const POINT physical = monitorInfo.LogicalToPhysical(logical, &dpi);
		Assert::AreEqual(2040L, physical.x);
		Assert::AreEqual(300L, physical.y);
		Assert::AreEqual(144U, dpi);

		const RECT logicalVirtualScreen = monitorInfo.GetLogicalVirtualScreenRect();
		Assert::AreEqual(0L, logicalVirtualScreen.left);
		Assert::AreEqual(0L, logicalVirtualScreen.top);
		Assert::AreEqual(3520L, logicalVirtualScreen.right);
		Assert::AreEqual(1080L, logicalVirtualScreen.bottom);

		const POINT logicalSecondaryOrigin = monitorInfo.PhysicalToLogical({ 1920, 0 });
		Assert::AreEqual(1920L, logicalSecondaryOrigin.x);
		Assert::AreEqual(0L, logicalSecondaryOrigin.y);
	}

	TEST_METHOD(TestMultiMonitorInfoConvertsLeftOfPrimaryAcrossDpiSpans)
	{
		MultiMonitorInfo monitorInfo = {};
		monitorInfo.primary = 2;
		monitorInfo.monitors = {
			CreateMonitor(144, -2400, 0, 0, 1440),
			CreateMonitor(96, 0, 0, 1920, 1080)
		};
		monitorInfo.UpdateLogicalMonitorInfo();

		const POINT logical = monitorInfo.PhysicalToLogical({ -1200, 300 });
		Assert::AreEqual(-800L, logical.x);
		Assert::AreEqual(200L, logical.y);

		UINT dpi = 0;
		const POINT physical = monitorInfo.LogicalToPhysical(logical, &dpi);
		Assert::AreEqual(-1200L, physical.x);
		Assert::AreEqual(300L, physical.y);
		Assert::AreEqual(144U, dpi);
	}

	TEST_METHOD(TestMultiMonitorInfoConvertsPointAcross150And300DpiSpans)
	{
		MultiMonitorInfo monitorInfo = {};
		monitorInfo.primary = 1;
		monitorInfo.monitors = {
			CreateMonitor(96, 0, 0, 1000, 1000),
			CreateMonitor(144, 1000, 0, 2500, 1000),
			CreateMonitor(288, 2500, 0, 5500, 1000)
		};
		monitorInfo.UpdateLogicalMonitorInfo();

		const POINT logical = monitorInfo.PhysicalToLogical({ 3100, 600 });
		Assert::AreEqual(2200L, logical.x);
		Assert::AreEqual(200L, logical.y);

		UINT dpi = 0;
		const POINT physical = monitorInfo.LogicalToPhysical(logical, &dpi);
		Assert::AreEqual(3100L, physical.x);
		Assert::AreEqual(600L, physical.y);
		Assert::AreEqual(288U, dpi);
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
