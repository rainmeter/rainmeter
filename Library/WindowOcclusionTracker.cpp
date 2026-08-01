/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "WindowOcclusionTracker.h"
#include "Rainmeter.h"
#include "Skin.h"
#include <WtsApi32.h>

namespace {

constexpr UINT INTERVAL_OCCLUSION_MIN = 16;
constexpr UINT INTERVAL_OCCLUSION_MAX = 100;

HWND g_MessageWindow = nullptr;
UINT g_TimerID = 0;

std::vector<HWINEVENTHOOK> g_OcclusionWinEventHooks;
std::map<DWORD, HWINEVENTHOOK> g_ProcessLocationChangeHooks;
std::set<DWORD> g_LocationChangePids;
std::set<HWND> g_TrackedOcclusionWindows;
std::set<HWND> g_OccludingWindows;
HWND g_MovingWindow = nullptr;
UINT g_OcclusionTimerInterval = 0;
bool g_ShowingThumbnails = false;
bool g_ScreenLocked = false;

void ScheduleOcclusionCalculationIfNeeded(HWND messageWindow);
void MarkTrackedWindowsOccluded();
void ComputeWindowOcclusionState();
void CALLBACK OcclusionWinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

bool IsTrackedWindow(HWND hwnd)
{
	return g_TrackedOcclusionWindows.find(hwnd) != g_TrackedOcclusionWindows.end();
}

bool IsWindowCloaked(HWND hwnd)
{
	DWORD cloaked = 0;
	return SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

bool IsWindowFullyOpaque(HWND hwnd)
{
	const LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if ((exStyle & WS_EX_TRANSPARENT) != 0)
	{
		return false;
	}

	if ((exStyle & WS_EX_LAYERED) != 0)
	{
		COLORREF colorKey = 0;
		BYTE alpha = 0;
		DWORD flags = 0;
		if (!GetLayeredWindowAttributes(hwnd, &colorKey, &alpha, &flags))
		{
			return false;
		}

		if ((flags & LWA_COLORKEY) != 0)
		{
			return false;
		}

		if ((flags & LWA_ALPHA) != 0 && alpha < 255)
		{
			return false;
		}
	}

	return true;
}

bool WindowCanOccludeOtherWindows(HWND hwnd, RECT* rect)
{
	if (!IsWindowVisible(hwnd) ||
		IsIconic(hwnd) ||
		IsWindowCloaked(hwnd) ||
		!GetWindowRect(hwnd, rect) ||
		rect->left >= rect->right ||
		rect->top >= rect->bottom)
	{
		return false;
	}

	return IsWindowFullyOpaque(hwnd);
}

bool IsThumbnailPreviewWindow(HWND hwnd)
{
	const int classLen = 64;
	WCHAR className[classLen];
	if (GetClassName(hwnd, className, classLen) <= 0) return false;

	return wcscmp(className, L"MultitaskingViewFrame") == 0 ||
		wcscmp(className, L"TaskListThumbnailWnd") == 0;
}

bool IsTopLevelWindowRelevantForOcclusion(HWND hwnd)
{
	const HWND rootWindow = GetAncestor(hwnd, GA_ROOT);
	return rootWindow != nullptr && rootWindow == hwnd;
}

void SetSkinOcclusionState(HWND hwnd, SkinWindowOcclusionState state)
{
	if (Skin* skin = GetRainmeter().GetSkin(hwnd))
	{
		skin->SetWindowOcclusionState(state);
	}
}

void MarkTrackedWindowsOccluded()
{
	for (HWND hwnd : g_TrackedOcclusionWindows)
	{
		SetSkinOcclusionState(hwnd,
			(!IsWindowVisible(hwnd) || IsIconic(hwnd)) ? SkinWindowOcclusionState::Hidden : SkinWindowOcclusionState::Occluded);
	}
}

void UnregisterProcessLocationChangeHooks()
{
	for (const auto& [_, hook] : g_ProcessLocationChangeHooks)
	{
		UnhookWinEvent(hook);
	}

	g_ProcessLocationChangeHooks.clear();
	g_LocationChangePids.clear();
	g_OccludingWindows.clear();
}

void UnregisterOcclusionHooks()
{
	for (HWINEVENTHOOK hook : g_OcclusionWinEventHooks)
	{
		UnhookWinEvent(hook);
	}

	g_OcclusionWinEventHooks.clear();
	UnregisterProcessLocationChangeHooks();
	g_MovingWindow = nullptr;
	g_ShowingThumbnails = false;
}

void RegisterProcessLocationChangeHook(DWORD pid)
{
	if (g_ProcessLocationChangeHooks.find(pid) != g_ProcessLocationChangeHooks.end()) return;

	HWINEVENTHOOK hook = SetWinEventHook(
		EVENT_OBJECT_LOCATIONCHANGE,
		EVENT_OBJECT_LOCATIONCHANGE,
		nullptr,
		OcclusionWinEventProc,
		pid,
		0,
		WINEVENT_OUTOFCONTEXT);

	if (hook)
	{
		g_ProcessLocationChangeHooks.emplace(pid, hook);
		g_LocationChangePids.insert(pid);
	}
}

void RegisterOcclusionHookRange(UINT eventMin, UINT eventMax)
{
	HWINEVENTHOOK hook = SetWinEventHook(
		eventMin,
		eventMax,
		nullptr,
		OcclusionWinEventProc,
		0,
		0,
		WINEVENT_OUTOFCONTEXT);

	if (hook)
	{
		g_OcclusionWinEventHooks.push_back(hook);
	}
}

void EnsureOcclusionHooksRegistered()
{
	if (!g_TrackedOcclusionWindows.empty() && g_OcclusionWinEventHooks.empty())
	{
		RegisterOcclusionHookRange(EVENT_SYSTEM_CAPTUREEND, EVENT_SYSTEM_CAPTUREEND);
		RegisterOcclusionHookRange(EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZEEND);
		RegisterOcclusionHookRange(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND);
		RegisterOcclusionHookRange(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND);
		RegisterOcclusionHookRange(EVENT_OBJECT_DESTROY, EVENT_OBJECT_HIDE);
		RegisterOcclusionHookRange(EVENT_OBJECT_STATECHANGE, EVENT_OBJECT_STATECHANGE);
		RegisterOcclusionHookRange(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_UNCLOAKED);
	}
}

void ScheduleOcclusionCalculationIfNeeded(HWND messageWindow)
{
	if (g_TrackedOcclusionWindows.empty()) return;

	EnsureOcclusionHooksRegistered();

	// Match Chromium's throttling strategy: ordinary visibility changes should be reflected
	// quickly, but active drags generate a very high rate of location changes so we
	// intentionally back off to a slower timer while moving.
	const UINT interval = g_MovingWindow ? INTERVAL_OCCLUSION_MAX : INTERVAL_OCCLUSION_MIN;
	if (g_OcclusionTimerInterval != 0)
	{
		if (g_OcclusionTimerInterval <= interval) return;

		KillTimer(messageWindow, g_TimerID);
	}

	SetTimer(messageWindow, g_TimerID, interval, nullptr);
	g_OcclusionTimerInterval = interval;
}

struct EnumOcclusionContext
{
	std::map<HWND, SkinWindowOcclusionState> states;
	std::set<DWORD> visiblePids;
	HRGN unoccludedRegion = nullptr;
	size_t unknownCount = 0;
};

BOOL CALLBACK ComputeTrackedWindowOcclusionProc(HWND hwnd, LPARAM lParam)
{
	auto& context = *reinterpret_cast<EnumOcclusionContext*>(lParam);

	RECT windowRect;
	const bool isOccludingWindow = WindowCanOccludeOtherWindows(hwnd, &windowRect);
	if (isOccludingWindow)
	{
		g_OccludingWindows.insert(hwnd);

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid != 0)
		{
			context.visiblePids.insert(pid);
			RegisterProcessLocationChangeHook(pid);
		}
	}

	// Chromium ignores the moving tracked window while it is being dragged so it does not
	// immediately mark itself occluded from transient bounds churn. We only apply that exception
	// to tracked Rainmeter windows; external windows must still participate so drag-over
	// occlusion updates happen live.
	if (hwnd == g_MovingWindow && IsTrackedWindow(hwnd))
	{
		return TRUE;
	}

	if (context.unknownCount == 0)
	{
		if (isOccludingWindow)
		{
			HRGN rectRegion = CreateRectRgnIndirect(&windowRect);
			CombineRgn(context.unoccludedRegion, context.unoccludedRegion, rectRegion, RGN_DIFF);
			DeleteObject(rectRegion);
		}

		return TRUE;
	}

	auto tracked = context.states.find(hwnd);
	if (tracked == context.states.end() || tracked->second != SkinWindowOcclusionState::Unknown)
	{
		if (isOccludingWindow)
		{
			HRGN rectRegion = CreateRectRgnIndirect(&windowRect);
			CombineRgn(context.unoccludedRegion, context.unoccludedRegion, rectRegion, RGN_DIFF);
			DeleteObject(rectRegion);
		}

		return TRUE;
	}

	--context.unknownCount;

	// Snapshot the desktop region that is still visible before this tracked window is processed. If
	// subtracting the window changes nothing, then the window was already fully covered by higher
	// z-order windows.
	HRGN currentUnoccludedRegion = CreateRectRgn(0, 0, 0, 0);
	CombineRgn(currentUnoccludedRegion, context.unoccludedRegion, nullptr, RGN_COPY);

	if (isOccludingWindow)
	{
		HRGN rectRegion = CreateRectRgnIndirect(&windowRect);
		CombineRgn(context.unoccludedRegion, context.unoccludedRegion, rectRegion, RGN_DIFF);
		DeleteObject(rectRegion);
	}
	else if (GetWindowRect(hwnd, &windowRect))
	{
		HRGN windowRegion = CreateRectRgnIndirect(&windowRect);
		CombineRgn(currentUnoccludedRegion, currentUnoccludedRegion, windowRegion, RGN_DIFF);
		DeleteObject(windowRegion);
	}

	tracked->second = EqualRgn(context.unoccludedRegion, currentUnoccludedRegion) ?
		SkinWindowOcclusionState::Occluded :
		SkinWindowOcclusionState::Visible;

	DeleteObject(currentUnoccludedRegion);
	return TRUE;
}

void ComputeWindowOcclusionState()
{
	if (g_TrackedOcclusionWindows.empty()) return;

	if (g_ScreenLocked)
	{
		MarkTrackedWindowsOccluded();
		return;
	}

	EnumOcclusionContext context;
	for (HWND hwnd : g_TrackedOcclusionWindows)
	{
		SkinWindowOcclusionState state = SkinWindowOcclusionState::Unknown;
		if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
		{
			state = SkinWindowOcclusionState::Hidden;
		}
		else if (IsWindowCloaked(hwnd))
		{
			state = SkinWindowOcclusionState::Occluded;
		}
		else if (g_ShowingThumbnails)
		{
			state = SkinWindowOcclusionState::Visible;
		}
		else
		{
			++context.unknownCount;
		}

		context.states.emplace(hwnd, state);
	}

	g_OccludingWindows.clear();
	if (context.unknownCount != 0)
	{
		const int screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
		const int screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
		context.unoccludedRegion = CreateRectRgn(
			screenLeft,
			screenTop,
			screenLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN),
			screenTop + GetSystemMetrics(SM_CYVIRTUALSCREEN));

		// EnumWindows walks top-level windows in z-order, topmost first. That lets us progressively
		// remove opaque rectangles until each tracked skin can be classified as visible or fully
		// occluded.
		EnumWindows(ComputeTrackedWindowOcclusionProc, reinterpret_cast<LPARAM>(&context));
		DeleteObject(context.unoccludedRegion);
	}

	for (const auto& [hwnd, state] : context.states)
	{
		SetSkinOcclusionState(hwnd, state);
	}

	std::set<DWORD> stalePids;
	for (DWORD pid : g_LocationChangePids)
	{
		if (context.visiblePids.find(pid) == context.visiblePids.end())
		{
			stalePids.insert(pid);
		}
	}

	for (DWORD pid : stalePids)
	{
		auto iter = g_ProcessLocationChangeHooks.find(pid);
		if (iter != g_ProcessLocationChangeHooks.end())
		{
			UnhookWinEvent(iter->second);
			g_ProcessLocationChangeHooks.erase(iter);
		}

		g_LocationChangePids.erase(pid);
	}
}

void CALLBACK OcclusionWinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG, DWORD, DWORD)
{
	if (g_TrackedOcclusionWindows.empty() || !hwnd) return;

	if (idObject != OBJID_WINDOW) return;

	if (event == EVENT_OBJECT_DESTROY)
	{
		if (g_OccludingWindows.find(hwnd) == g_OccludingWindows.end()) return;

		g_OccludingWindows.erase(hwnd);
		ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
		return;
	}

	bool calculateOcclusion = IsTopLevelWindowRelevantForOcclusion(hwnd);

	if (event == EVENT_OBJECT_SHOW)
	{
		if (!g_ShowingThumbnails && IsThumbnailPreviewWindow(hwnd))
		{
			g_ShowingThumbnails = true;
			calculateOcclusion = true;
		}
	}
	else if (event == EVENT_OBJECT_HIDE)
	{
		if (g_ShowingThumbnails && IsThumbnailPreviewWindow(hwnd))
		{
			g_ShowingThumbnails = false;
			calculateOcclusion = true;
		}
	}
	else if (event == EVENT_SYSTEM_MOVESIZESTART)
	{
		g_MovingWindow = hwnd;
	}
	else if (event == EVENT_SYSTEM_MOVESIZEEND)
	{
		g_MovingWindow = nullptr;
	}
	else if (g_MovingWindow != nullptr)
	{
		if (event == EVENT_OBJECT_LOCATIONCHANGE || event == EVENT_OBJECT_STATECHANGE)
		{
			// Chromium suppresses redundant recalculations from unrelated tracked windows while one
			// tracked window is being dragged. Keep processing external moving windows so skins update
			// continuously during drag-over occlusion instead of waiting for MOVESIZEEND.
			if (IsTrackedWindow(g_MovingWindow) &&
				(g_TrackedOcclusionWindows.size() <= 1 || !IsTrackedWindow(hwnd)))
			{
				return;
			}
		}
		else
		{
			g_MovingWindow = nullptr;
		}
	}

	if (!calculateOcclusion) return;

	ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
}

}  // namespace

void WindowOcclusionTracker::Initialize(HWND messageWindow, UINT timerID)
{
	g_MessageWindow = messageWindow;
	g_TimerID = timerID;
}

void WindowOcclusionTracker::Finalize()
{
	KillTimer(g_MessageWindow, g_TimerID);

	g_OcclusionTimerInterval = 0;
	UnregisterOcclusionHooks();

	g_MessageWindow = nullptr;
	g_TimerID = 0;
}

void WindowOcclusionTracker::TrackWindow(HWND hwnd)
{
	if (!hwnd) return;

	g_TrackedOcclusionWindows.insert(hwnd);
	ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
}

void WindowOcclusionTracker::UntrackWindow(HWND hwnd)
{
	if (!hwnd) return;

	g_TrackedOcclusionWindows.erase(hwnd);
	if (g_MovingWindow == hwnd)
	{
		g_MovingWindow = nullptr;
	}

	SetSkinOcclusionState(hwnd, SkinWindowOcclusionState::Unknown);

	if (g_TrackedOcclusionWindows.empty())
	{
		KillTimer(g_MessageWindow, g_TimerID);

		g_OcclusionTimerInterval = 0;
		UnregisterOcclusionHooks();
		return;
	}

	ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
}

void WindowOcclusionTracker::HandleTimer()
{
	KillTimer(g_MessageWindow, g_TimerID);
	g_OcclusionTimerInterval = 0;
	ComputeWindowOcclusionState();
}

void WindowOcclusionTracker::HandleShowDesktopChange()
{
	if (g_TrackedOcclusionWindows.empty()) return;

	EnsureOcclusionHooksRegistered();

	if (g_OcclusionTimerInterval != 0)
	{
		KillTimer(g_MessageWindow, g_TimerID);
		g_OcclusionTimerInterval = 0;
	}

	ComputeWindowOcclusionState();
}

void WindowOcclusionTracker::HandleDisplayChange()
{
	ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
}

void WindowOcclusionTracker::HandlePowerResume()
{
	ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
}

void WindowOcclusionTracker::HandleSessionChange(WPARAM sessionEvent)
{
	if (sessionEvent == WTS_SESSION_LOCK)
	{
		g_ScreenLocked = true;
		MarkTrackedWindowsOccluded();
	}
	else if (sessionEvent == WTS_SESSION_UNLOCK)
	{
		g_ScreenLocked = false;
		ScheduleOcclusionCalculationIfNeeded(g_MessageWindow);
	}
}
