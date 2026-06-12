/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "WindowPlacement.h"

namespace WindowPlacement {

namespace {

enum class Axis
{
	X,
	Y
};

enum class AutoAnchor
{
	Start,
	Center,
	End
};

struct ParsedWindowOption
{
	float amount = 0.0f;
	bool percentage = false;
	bool fromFarEdge = false;
	bool explicitScreen = false;
	int screen = 1;
};

struct ParsedAnchorOption
{
	float amount = 0.0f;
	bool percentage = false;
	bool fromFarEdge = false;
};

struct ScreenSpan
{
	int start;
	int size;
};

int ScaleToDevicePixels(int value, float scale)
{
	const float scaled = (float)value * scale;
	return (int)((value >= 0) ? ceilf(scaled) : floorf(scaled));
}

bool HasModifier(const std::wstring& value, WCHAR modifier)
{
	const std::wstring::size_type modifierIndex = value.find_last_of(modifier);
	if (modifierIndex == std::wstring::npos)
	{
		return false;
	}

	// Variables that survived parsing can still contain characters that look like position
	// modifiers, e.g. "#WORKAREAX@n#". A modifier is only real when it appears after the last '#',
	// which matches the legacy parsing behavior in Skin::WindowToScreen().
	const std::wstring::size_type lastVariableDelimiter = value.find_last_of(L'#');
	return lastVariableDelimiter == std::wstring::npos || lastVariableDelimiter < modifierIndex;
}

float ParseAmount(const std::wstring& value, LPCWSTR acceptedCharacters)
{
	const std::wstring::size_type end = value.find_first_not_of(acceptedCharacters);
	return (float)_wtof(end == std::wstring::npos ? value.c_str() : value.substr(0, end).c_str());
}

bool IsValidScreen(int screen, const MultiMonitorInfo& monitorsInfo)
{
	if (screen == 0)
	{
		return true;
	}

	const size_t monitorIndex = (size_t)(screen - 1);
	return screen > 0 && monitorIndex < monitorsInfo.monitors.size() &&
		monitorsInfo.monitors[monitorIndex].active;
}

ParsedAnchorOption ParseAnchorOption(const std::wstring& value, Axis axis)
{
	// AnchorX/AnchorY are measured inside the skin window. "50%" means the middle of the
	// unscaled window, while "50%R" or "50%B" means halfway back from the right/bottom edge.
	ParsedAnchorOption option;
	option.amount = ParseAmount(value, L"0123456789.");
	option.percentage = HasModifier(value, L'%');
	option.fromFarEdge = HasModifier(value, axis == Axis::X ? L'R' : L'B');
	return option;
}

ParsedWindowOption ParseWindowOption(const std::wstring& value, Axis axis, const MultiMonitorInfo& monitorsInfo)
{
	// WindowX/WindowY are screen positions for the anchor point. Their suffix modifiers are
	// independent: "20%R@2" means 20% of monitor 2's width, measured from that monitor's right
	// edge. The @N monitor selector is validated here, but the cross-axis defaulting rule is
	// applied later once both axes have been parsed.
	ParsedWindowOption option;
	option.amount = ParseAmount(value, L"-0123456789.");
	option.percentage = HasModifier(value, L'%');
	option.fromFarEdge = HasModifier(value, axis == Axis::X ? L'R' : L'B');

	const std::wstring::size_type screenMarker = value.find_last_of(L'@');
	if (screenMarker != std::wstring::npos && HasModifier(value, L'@'))
	{
		const std::wstring::size_type screenStart = screenMarker + 1;
		const std::wstring::size_type screenEnd = value.find_first_not_of(L"0123456789", screenStart);
		const std::wstring screenString = value.substr(
			screenStart,
			(screenEnd != std::wstring::npos) ? screenEnd - screenStart : std::wstring::npos);

		if (!screenString.empty())
		{
			const int screen = _wtoi(screenString.c_str());
			if (IsValidScreen(screen, monitorsInfo))
			{
				option.explicitScreen = true;
				option.screen = screen;
			}
		}
	}

	return option;
}

int ResolveDistance(float amount, int size, bool percentage)
{
	return percentage ? (int)((float)size * amount / 100.0f) : (int)amount;
}

int ResolveWindowDistance(float amount, int size, bool percentage, float scale)
{
	if (percentage) return ResolveDistance(amount, size, true);
	if (fabsf(scale - 1.0f) <= 0.0001f) return (int)amount;

	const float scaled = amount * scale;
	return (int)((amount >= 0.0f) ? ceilf(scaled) : floorf(scaled));
}

int ResolveAnchor(const ParsedAnchorOption& option, int windowSize)
{
	// Anchor values remain in logical skin pixels. Skin::WindowToScreen() stores these logical
	// offsets for later save/drag calculations, and only scales them when converting to screen
	// coordinates.
	const int distance = ResolveDistance(option.amount, windowSize, option.percentage);
	return option.fromFarEdge ? windowSize - distance : distance;
}

int ResolveAutoAnchor(AutoAnchor anchor, int windowSize)
{
	switch (anchor)
	{
	case AutoAnchor::Center:
		return windowSize / 2;

	case AutoAnchor::End:
		return windowSize;
	}

	return 0;
}

AutoAnchor SelectAutoAnchor(int oldCoordinate, int oldWindowSize, const ScreenSpan& screen)
{
	// When DPI changes and the user did not define an anchor, split the selected screen axis
	// into three bands. A skin in the first band sticks to the start edge, one in the middle band
	// sticks to its center, and one in the last band sticks to the end edge.
	const int oldWindowCenter = oldCoordinate + oldWindowSize / 2;
	const int firstBandEnd = screen.start + screen.size / 3;
	const int secondBandEnd = screen.start + (screen.size * 2) / 3;

	if (oldWindowCenter < firstBandEnd)
	{
		return AutoAnchor::Start;
	}

	if (oldWindowCenter < secondBandEnd)
	{
		return AutoAnchor::Center;
	}

	return AutoAnchor::End;
}

ScreenSpan GetScreenSpan(int screen, Axis axis, const MultiMonitorInfo& monitorsInfo)
{
	if (screen == 0)
	{
		return {
			axis == Axis::X ? monitorsInfo.vsL : monitorsInfo.vsT,
			axis == Axis::X ? monitorsInfo.vsW : monitorsInfo.vsH
		};
	}

	const RECT& monitor = monitorsInfo.monitors[(size_t)(screen - 1)].screen;
	if (axis == Axis::X)
	{
		return { monitor.left, monitor.right - monitor.left };
	}

	return { monitor.top, monitor.bottom - monitor.top };
}

AxisResult ResolveAxis(
	const ParsedWindowOption& window,
	const ParsedAnchorOption& anchor,
	Axis axis,
	int windowSize,
	float scale,
	float oldScale,
	bool anchorDefined,
	const MultiMonitorInfo& monitorsInfo)
{
	const ScreenSpan screen = GetScreenSpan(window.screen, axis, monitorsInfo);
	const float previousScale = (oldScale > 0.0f) ? oldScale : scale;
	const bool scaleChanged = fabsf(previousScale - scale) > 0.0001f;

	// WindowX and WindowY describe the on-screen location of the selected anchor point, not
	// necessarily the upper-left corner. First resolve that anchor point against the selected
	// screen, then subtract the scaled anchor offset to get the window's top-left screen position.
	const int distance = ResolveWindowDistance(window.amount, screen.size, window.percentage, scale);
	const int anchorPoint = window.fromFarEdge ?
		screen.start + (screen.size - distance) :
		screen.start + distance;
	const int anchorScreen = (scaleChanged && !anchorDefined) ?
		ResolveAutoAnchor(SelectAutoAnchor(anchorPoint, ScaleToDevicePixels(windowSize, previousScale), screen), windowSize) :
		ResolveAnchor(anchor, windowSize);

	return {
		window.screen,
		window.explicitScreen,
		window.fromFarEdge,
		window.percentage,
		anchorScreen,
		anchorDefined && anchor.fromFarEdge,
		anchorDefined && anchor.percentage,
		(scaleChanged && !anchorDefined) ?
			anchorPoint + ScaleToDevicePixels(anchorScreen, previousScale) - ScaleToDevicePixels(anchorScreen, scale) :
			anchorPoint - ScaleToDevicePixels(anchorScreen, scale)
	};
}

}  // namespace

Result WindowToScreen(const Input& input, const MultiMonitorInfo& monitorsInfo)
{
	ParsedWindowOption windowX = ParseWindowOption(input.windowX, Axis::X, monitorsInfo);
	ParsedWindowOption windowY = ParseWindowOption(input.windowY, Axis::Y, monitorsInfo);
	const ParsedAnchorOption anchorX = ParseAnchorOption(input.anchorX, Axis::X);
	const ParsedAnchorOption anchorY = ParseAnchorOption(input.anchorY, Axis::Y);
	const bool windowXHasScreen = windowX.explicitScreen;
	const bool windowYHasScreen = windowY.explicitScreen;
	const int windowXScreen = windowX.screen;
	const int windowYScreen = windowY.screen;

	// Screens default to the primary monitor. If either WindowX or WindowY contains a valid @N,
	// that screen applies to both axes; a second @N on the other axis overrides only that axis.
	windowX.screen = windowY.screen = monitorsInfo.primary;
	windowX.explicitScreen = windowY.explicitScreen = false;

	if (windowXHasScreen)
	{
		windowX.screen = windowY.screen = windowXScreen;
		windowX.explicitScreen = windowY.explicitScreen = true;
	}

	if (windowYHasScreen)
	{
		windowY.screen = windowYScreen;
		windowY.explicitScreen = true;

		if (!windowX.explicitScreen)
		{
			windowX.screen = windowYScreen;
			windowX.explicitScreen = true;
		}
	}

	return {
		ResolveAxis(windowX, anchorX, Axis::X, input.windowW, input.scale, input.oldScale, input.anchorXDefined, monitorsInfo),
		ResolveAxis(windowY, anchorY, Axis::Y, input.windowH, input.scale, input.oldScale, input.anchorYDefined, monitorsInfo)
	};
}

}  // namespace WindowPlacement
