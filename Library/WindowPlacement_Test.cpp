/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "WindowPlacement.h"
#include "../Common/UnitTest.h"

TEST_CLASS(Library_WindowPlacement_Test)
{
public:
	TEST_METHOD(TestPixelPlacement)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(100, result.x.coordinate);
		Assert::AreEqual(50, result.y.coordinate);
		Assert::AreEqual(0, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
		Assert::IsFalse(result.x.percentage);
		Assert::IsFalse(result.y.percentage);
	}

	TEST_METHOD(TestCenteredPercentageAnchor)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50%", L"50%", L"50%", L"50%", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(860, result.x.coordinate);
		Assert::AreEqual(490, result.y.coordinate);
		Assert::AreEqual(100, result.x.anchorScreen);
		Assert::AreEqual(50, result.y.anchorScreen);
		Assert::IsTrue(result.x.percentage);
		Assert::IsTrue(result.y.percentage);
		Assert::IsTrue(result.x.anchorPercentage);
		Assert::IsTrue(result.y.anchorPercentage);
	}

	TEST_METHOD(TestAnchorUsesDevicePixelScale)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"50", L"20", 200, 100, 1.5f },
			CreateMonitors());

		Assert::AreEqual(25, result.x.coordinate);
		Assert::AreEqual(20, result.y.coordinate);
		Assert::AreEqual(50, result.x.anchorScreen);
		Assert::AreEqual(20, result.y.anchorScreen);
	}

	TEST_METHOD(TestWindowPositionUsesLogicalCoordinates)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"0", L"0", 200, 100, 1.5f, 0.0f, true, true },
			CreateMonitors());

		Assert::AreEqual(150, result.x.coordinate);
		Assert::AreEqual(75, result.y.coordinate);
	}

	TEST_METHOD(TestPercentageWindowPositionDoesNotUseLogicalScale)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50%", L"50%", L"0", L"0", 200, 100, 1.5f, 0.0f, true, true },
			CreateMonitors());

		Assert::AreEqual(960, result.x.coordinate);
		Assert::AreEqual(540, result.y.coordinate);
	}

	TEST_METHOD(TestAutoAnchorStartStartWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"0", L"0", 0, 0, 0, 0);
	}

	TEST_METHOD(TestAutoAnchorCenterStartWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"860", L"0", 810, 0, 100, 0);
	}

	TEST_METHOD(TestAutoAnchorEndStartWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"1720", L"0", 1620, 0, 200, 0);
	}

	TEST_METHOD(TestAutoAnchorStartCenterWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"0", L"490", 0, 465, 0, 50);
	}

	TEST_METHOD(TestAutoAnchorCenterCenterWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"860", L"490", 810, 465, 100, 50);
	}

	TEST_METHOD(TestAutoAnchorEndCenterWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"1720", L"490", 1620, 465, 200, 50);
	}

	TEST_METHOD(TestAutoAnchorStartEndWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"0", L"980", 0, 930, 0, 100);
	}

	TEST_METHOD(TestAutoAnchorCenterEndWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"860", L"980", 810, 930, 100, 100);
	}

	TEST_METHOD(TestAutoAnchorEndEndWhenScaleIncreases)
	{
		AssertAutoDpiPlacement(L"1720", L"980", 1620, 930, 200, 100);
	}

	TEST_METHOD(TestAutoAnchorFirstBoundaryUsesCenter)
	{
		AssertAutoDpiPlacement(L"540", L"310", 490, 285, 100, 50);
	}

	TEST_METHOD(TestAutoAnchorSecondBoundaryUsesEnd)
	{
		AssertAutoDpiPlacement(L"1180", L"670", 1080, 620, 200, 100);
	}

	TEST_METHOD(TestAutoAnchorStartStartWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"0", L"0", 0, 0, 0, 0, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorCenterStartWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"810", L"0", 860, 0, 100, 0, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorEndStartWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"1620", L"0", 1720, 0, 200, 0, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorStartCenterWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"0", L"465", 0, 490, 0, 50, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorCenterCenterWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"810", L"465", 860, 490, 100, 50, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorEndCenterWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"1620", L"465", 1720, 490, 200, 50, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorStartEndWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"0", L"930", 0, 980, 0, 100, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorCenterEndWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"810", L"930", 860, 980, 100, 100, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorEndEndWhenScaleDecreases)
	{
		AssertAutoDpiPlacement(L"1620", L"930", 1720, 980, 200, 100, 1.0f, 1.5f);
	}

	TEST_METHOD(TestAutoAnchorUsesSelectedMonitor)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1080@2", L"0", L"0", L"0", 200, 100, 1.5f, 1.0f, false, false },
			CreateMonitors());

		Assert::AreEqual(2900, result.x.coordinate);
		Assert::AreEqual(0, result.y.coordinate);
		Assert::AreEqual(200, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
	}

	TEST_METHOD(TestAutoAnchorUsesSelectedMonitorCenter)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"540@2", L"462", L"0", L"0", 200, 100, 1.5f, 1.0f, false, false },
			CreateMonitors());

		Assert::AreEqual(2410, result.x.coordinate);
		Assert::AreEqual(437, result.y.coordinate);
		Assert::AreEqual(100, result.x.anchorScreen);
		Assert::AreEqual(50, result.y.anchorScreen);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
	}

	TEST_METHOD(TestAutoAnchorDoesNotRunWhenScaleIsUnchanged)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1720", L"980", L"0", L"0", 200, 100, 1.5f, 1.5f, false, false },
			CreateSingleMonitor());

		Assert::AreEqual(1720, result.x.coordinate);
		Assert::AreEqual(980, result.y.coordinate);
		Assert::AreEqual(0, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
	}

	TEST_METHOD(TestExplicitAnchorPreventsAutoAnchor)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1720", L"980", L"0", L"0", 200, 100, 1.5f, 1.0f, true, true },
			CreateSingleMonitor());

		Assert::AreEqual(1720, result.x.coordinate);
		Assert::AreEqual(980, result.y.coordinate);
		Assert::AreEqual(0, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
	}

	TEST_METHOD(TestExplicitAnchorCanStillStickToEndDuringScaleChange)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1920", L"1080", L"100%", L"100%", 200, 100, 1.5f, 1.0f, true, true },
			CreateSingleMonitor());

		Assert::AreEqual(1620, result.x.coordinate);
		Assert::AreEqual(930, result.y.coordinate);
		Assert::AreEqual(200, result.x.anchorScreen);
		Assert::AreEqual(100, result.y.anchorScreen);
		Assert::IsTrue(result.x.anchorPercentage);
		Assert::IsTrue(result.y.anchorPercentage);
	}

	TEST_METHOD(TestAutoAnchorCanRunOnOnlyOneAxis)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1720", L"540", L"0", L"50%", 200, 100, 1.5f, 1.0f, false, true },
			CreateSingleMonitor());

		Assert::AreEqual(1620, result.x.coordinate);
		Assert::AreEqual(465, result.y.coordinate);
		Assert::AreEqual(200, result.x.anchorScreen);
		Assert::AreEqual(50, result.y.anchorScreen);
		Assert::IsFalse(result.x.anchorPercentage);
		Assert::IsTrue(result.y.anchorPercentage);
	}

	TEST_METHOD(TestNegativeWindowCoordinates)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"-25", L"-10", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(-25, result.x.coordinate);
		Assert::AreEqual(-10, result.y.coordinate);
		Assert::IsFalse(result.x.percentage);
		Assert::IsFalse(result.y.percentage);
	}

	TEST_METHOD(TestSingleMonitorPlacementCanExceedScreenBounds)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"2000", L"1200", L"0", L"0", 200, 100, 1.0f },
			CreateSingleMonitor());

		Assert::AreEqual(2000, result.x.coordinate);
		Assert::AreEqual(1200, result.y.coordinate);
		Assert::AreEqual(1, result.x.screen);
		Assert::AreEqual(1, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestSingleMonitorPercentagePlacementCanExceedScreenBounds)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"125%", L"150%", L"0", L"0", 200, 100, 1.0f },
			CreateSingleMonitor());

		Assert::AreEqual(2400, result.x.coordinate);
		Assert::AreEqual(1620, result.y.coordinate);
		Assert::IsTrue(result.x.percentage);
		Assert::IsTrue(result.y.percentage);
	}

	TEST_METHOD(TestFractionalPixelsAreTruncated)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"10.9", L"-10.9", L"1.9", L"2.9", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(9, result.x.coordinate);
		Assert::AreEqual(-12, result.y.coordinate);
		Assert::AreEqual(1, result.x.anchorScreen);
		Assert::AreEqual(2, result.y.anchorScreen);
	}

	TEST_METHOD(TestLogicalFractionalPixelsUseDevicePixelRounding)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"10.9", L"-10.9", L"0", L"0", 200, 100, 1.5f, 0.0f, true, true },
			CreateMonitors());

		Assert::AreEqual(17, result.x.coordinate);
		Assert::AreEqual(-17, result.y.coordinate);
	}

	TEST_METHOD(TestFarEdgeModifiers)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100R", L"80B", L"20%R", L"10B", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1660, result.x.coordinate);
		Assert::AreEqual(910, result.y.coordinate);
		Assert::AreEqual(160, result.x.anchorScreen);
		Assert::AreEqual(90, result.y.anchorScreen);
		Assert::IsTrue(result.x.fromFarEdge);
		Assert::IsTrue(result.y.fromFarEdge);
		Assert::IsTrue(result.x.anchorFromFarEdge);
		Assert::IsTrue(result.y.anchorFromFarEdge);
	}

	TEST_METHOD(TestPercentageFarEdgeWindowPosition)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"25%R", L"10%B", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1440, result.x.coordinate);
		Assert::AreEqual(972, result.y.coordinate);
		Assert::IsTrue(result.x.percentage);
		Assert::IsTrue(result.y.percentage);
		Assert::IsTrue(result.x.fromFarEdge);
		Assert::IsTrue(result.y.fromFarEdge);
	}

	TEST_METHOD(TestZeroFarEdgeAnchorUsesWindowSize)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"0R", L"0B", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(-100, result.x.coordinate);
		Assert::AreEqual(-50, result.y.coordinate);
		Assert::AreEqual(200, result.x.anchorScreen);
		Assert::AreEqual(100, result.y.anchorScreen);
		Assert::IsTrue(result.x.anchorFromFarEdge);
		Assert::IsTrue(result.y.anchorFromFarEdge);
	}

	TEST_METHOD(TestHundredPercentFarEdgeWindowPositionUsesNearEdge)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100%R", L"100%B", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(0, result.x.coordinate);
		Assert::AreEqual(0, result.y.coordinate);
		Assert::IsTrue(result.x.percentage);
		Assert::IsTrue(result.y.percentage);
		Assert::IsTrue(result.x.fromFarEdge);
		Assert::IsTrue(result.y.fromFarEdge);
	}

	TEST_METHOD(TestFarEdgeAnchorCanResolveBeforeWindowStart)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"250R", L"150B", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(150, result.x.coordinate);
		Assert::AreEqual(100, result.y.coordinate);
		Assert::AreEqual(-50, result.x.anchorScreen);
		Assert::AreEqual(-50, result.y.anchorScreen);
	}

	TEST_METHOD(TestNegativeAnchorValuesAreTreatedAsZero)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100", L"50", L"-10", L"-20", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(100, result.x.coordinate);
		Assert::AreEqual(50, result.y.coordinate);
		Assert::AreEqual(0, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
	}

	TEST_METHOD(TestScreenSelectionAppliesToBothAxes)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50", L"500@2", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1970, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestXScreenSelectionAppliesToBothAxes)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@2", L"500", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1970, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestYScreenSelectionOverridesOnlyY)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@2", L"25@0", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1970, result.x.coordinate);
		Assert::AreEqual(-25, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(0, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestDefaultPrimaryScreenCanBeNonFirstMonitor)
	{
		MultiMonitorInfo monitorsInfo = CreateMonitors();
		monitorsInfo.primary = 2;

		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50", L"500", L"0", L"0", 200, 100, 1.0f },
			monitorsInfo);

		Assert::AreEqual(1970, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestExplicitMonitorPlacementCanExceedMonitorBounds)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"1400@2", L"1100", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(3320, result.x.coordinate);
		Assert::AreEqual(1100, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestExplicitMonitorPercentagePlacementCanExceedMonitorBounds)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"125%@2", L"150%", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(3520, result.x.coordinate);
		Assert::AreEqual(1536, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsTrue(result.x.percentage);
		Assert::IsTrue(result.y.percentage);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestVirtualScreenSelection)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"100@0", L"50", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(0, result.x.coordinate);
		Assert::AreEqual(0, result.y.coordinate);
		Assert::AreEqual(0, result.x.screen);
		Assert::AreEqual(0, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestInvalidScreenSelectionFallsBackToPrimary)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@3", L"500", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(50, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(1, result.x.screen);
		Assert::AreEqual(1, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestMissingScreenDigitsFallsBackToPrimary)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@", L"500", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(50, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(1, result.x.screen);
		Assert::AreEqual(1, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestInactiveScreenSelectionFallsBackToPrimary)
	{
		MultiMonitorInfo monitorsInfo = CreateMonitors();
		monitorsInfo.monitors.push_back(CreateMonitor(3200, 0, 4000, 600, false));

		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@3", L"500", L"0", L"0", 200, 100, 1.0f },
			monitorsInfo);

		Assert::AreEqual(50, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(1, result.x.screen);
		Assert::AreEqual(1, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestScreenSelectionUsesLeadingDigitsOnly)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"50@2abc", L"500", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(1970, result.x.coordinate);
		Assert::AreEqual(500, result.y.coordinate);
		Assert::AreEqual(2, result.x.screen);
		Assert::AreEqual(2, result.y.screen);
		Assert::IsTrue(result.x.screenDefined);
		Assert::IsTrue(result.y.screenDefined);
	}

	TEST_METHOD(TestUnreplacedVariablesDoNotSetModifiers)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"#WORKAREAX@n#", L"#WORKAREAY@n#", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(0, result.x.coordinate);
		Assert::AreEqual(0, result.y.coordinate);
		Assert::AreEqual(1, result.x.screen);
		Assert::AreEqual(1, result.y.screen);
		Assert::IsFalse(result.x.screenDefined);
		Assert::IsFalse(result.y.screenDefined);
	}

	TEST_METHOD(TestModifiersBeforeUnreplacedVariablesAreIgnored)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"10%R#VAR#", L"20%B#VAR#", L"0", L"0", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(10, result.x.coordinate);
		Assert::AreEqual(20, result.y.coordinate);
		Assert::IsFalse(result.x.percentage);
		Assert::IsFalse(result.y.percentage);
		Assert::IsFalse(result.x.fromFarEdge);
		Assert::IsFalse(result.y.fromFarEdge);
	}

	TEST_METHOD(TestEmptyAndNonNumericValuesAreZero)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ L"", L"abc", L"abc", L"", 200, 100, 1.0f },
			CreateMonitors());

		Assert::AreEqual(0, result.x.coordinate);
		Assert::AreEqual(0, result.y.coordinate);
		Assert::AreEqual(0, result.x.anchorScreen);
		Assert::AreEqual(0, result.y.anchorScreen);
	}

private:
	static void AssertAutoDpiPlacement(
		LPCWSTR windowX,
		LPCWSTR windowY,
		int expectedX,
		int expectedY,
		int expectedAnchorX,
		int expectedAnchorY,
		float scale = 1.5f,
		float oldScale = 1.0f)
	{
		const WindowPlacement::Result result = WindowPlacement::WindowToScreen(
			{ windowX, windowY, L"0", L"0", 200, 100, scale, oldScale, false, false },
			CreateSingleMonitor());

		Assert::AreEqual(expectedX, result.x.coordinate);
		Assert::AreEqual(expectedY, result.y.coordinate);
		Assert::AreEqual(expectedAnchorX, result.x.anchorScreen);
		Assert::AreEqual(expectedAnchorY, result.y.anchorScreen);
		Assert::IsFalse(result.x.anchorFromFarEdge);
		Assert::IsFalse(result.y.anchorFromFarEdge);
		Assert::IsFalse(result.x.anchorPercentage);
		Assert::IsFalse(result.y.anchorPercentage);
	}

	static MultiMonitorInfo CreateSingleMonitor()
	{
		MultiMonitorInfo monitorsInfo = {};
		monitorsInfo.vsL = 0;
		monitorsInfo.vsT = 0;
		monitorsInfo.vsW = 1920;
		monitorsInfo.vsH = 1080;
		monitorsInfo.primary = 1;
		monitorsInfo.monitors.push_back(CreateMonitor(0, 0, 1920, 1080));
		return monitorsInfo;
	}

	static MultiMonitorInfo CreateMonitors()
	{
		MultiMonitorInfo monitorsInfo = {};
		monitorsInfo.vsL = -100;
		monitorsInfo.vsT = -50;
		monitorsInfo.vsW = 3300;
		monitorsInfo.vsH = 1200;
		monitorsInfo.primary = 1;
		monitorsInfo.monitors.push_back(CreateMonitor(0, 0, 1920, 1080));
		monitorsInfo.monitors.push_back(CreateMonitor(1920, 0, 3200, 1024));
		return monitorsInfo;
	}

	static MonitorInfo CreateMonitor(int left, int top, int right, int bottom, bool active = true)
	{
		MonitorInfo monitor = {};
		monitor.active = active;
		monitor.screen = { left, top, right, bottom };
		return monitor;
	}
};
