// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "ClickCapture.h"
#include "MeasureMouse.h"
#include "MonitorUtil.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

namespace {

class ClickCaptureController
{
public:
	ClickCaptureController() = default;
	~ClickCaptureController();

	ClickCaptureController(const ClickCaptureController&) = delete;
	ClickCaptureController& operator=(const ClickCaptureController&) = delete;

	void Initialize();
	bool Start(MeasureMouse* owner, const D2D1_COLOR_F& color);
	void Stop(MeasureMouse* owner);
	void Reset();
	void Finalize();
	bool IsOwner(const MeasureMouse* owner) const { return m_Owner == owner; }
	bool IsOverlayWindow(HWND window) const;

private:
	// WaitingForClear ignores buttons held before capture. Draining swallows releases after
	// cancellation, when there is no longer an owner to notify.
	enum class State { Idle, WaitingForClear, Armed, Pressed, Completing, Draining };

	struct Overlay
	{
		HWND window = nullptr;
		Microsoft::WRL::ComPtr<IDCompositionTarget> target;
	};

	static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	HRESULT CreateOverlays(const D2D1_COLOR_F& color);
	void DestroyOverlays();
	void Cancel();
	void Tick();
	void Complete();
	void HandleButton(UINT button, bool down, POINT screenPos);

	HINSTANCE m_Instance = nullptr;
	HWND m_ControlWindow = nullptr;
	MeasureMouse* m_Owner = nullptr;
	State m_State = State::Idle;
	UINT m_PressedButtons = 0;
	UINT m_Button = 0;
	POINT m_Position = {};
	bool m_EscapeDown = false;

	// Posted completions must not finish a request that has been replaced or canceled.
	UINT_PTR m_Generation = 0;
	std::vector<Overlay> m_Overlays;
	Microsoft::WRL::ComPtr<IDCompositionDevice> m_Device;
};

ClickCaptureController& GetController()
{
	static ClickCaptureController s_Controller;
	return s_Controller;
}

constexpr WCHAR g_ClassName[] = L"RainmeterClickCapture";
constexpr UINT g_CompleteMessage = WM_APP + 1;
constexpr UINT_PTR g_Timer = 1;
constexpr int g_Buttons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
constexpr MOUSEACTION g_Actions[] = { MOUSE_LMB_UP, MOUSE_RMB_UP, MOUSE_MMB_UP, MOUSE_X1MB_UP, MOUSE_X2MB_UP };

UINT GetPressedButtons()
{
	UINT buttons = 0;
	for (UINT i = 0; i < _countof(g_Buttons); ++i)
	{
		if (GetAsyncKeyState(g_Buttons[i]) < 0) buttons |= 1 << i;
	}
	return buttons;
}

ClickCaptureController::~ClickCaptureController()
{
	Finalize();
}

void ClickCaptureController::Finalize()
{
	Reset();
	if (m_ControlWindow)
	{
		DestroyWindow(m_ControlWindow);
		m_ControlWindow = nullptr;
	}
	if (m_Instance)
	{
		UnregisterClass(g_ClassName, m_Instance);
		m_Instance = nullptr;
	}
}

void ClickCaptureController::Initialize()
{
	WNDCLASS wc = {};
	m_Instance = GetRainmeter().GetModuleInstance();
	wc.hInstance = m_Instance;
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = g_ClassName;
	RegisterClass(&wc);
}

bool ClickCaptureController::Start(MeasureMouse* owner, const D2D1_COLOR_F& color)
{
	// Completion goes through a window that survives overlay destruction and replacement.
	if (!m_ControlWindow)
	{
		m_ControlWindow = CreateWindowEx(0, g_ClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, m_Instance, this);
	}

	const HRESULT result = CreateOverlays(color);
	if (!m_ControlWindow || FAILED(result))
	{
		if (m_Overlays.empty())
		{
			Reset();
		}
		else
		{
			Cancel();
		}

		LogErrorF(owner, L"Mouse: Unable to create the screen capture overlay (0x%08X)", (UINT)result);
		return false;
	}

	++m_Generation;
	m_Owner = owner;
	m_PressedButtons = 0;

	// A mouse action may start capture while its triggering button is still held.
	m_State = GetPressedButtons() ? State::WaitingForClear : State::Armed;
	m_EscapeDown = GetAsyncKeyState(VK_ESCAPE) < 0;

	SetTimer(m_ControlWindow, g_Timer, 30, nullptr);

	// A skin's drag capture would otherwise divert input away from the overlays.
	if (GetCapture()) ReleaseCapture();

	for (const auto& overlay : m_Overlays)
	{
		SetWindowPos(overlay.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}

	return true;
}

HRESULT ClickCaptureController::CreateOverlays(const D2D1_COLOR_F& color)
{
	// Keep the old input shield until all replacement windows are ready.
	Microsoft::WRL::ComPtr<IDCompositionDevice> device;
	HRESULT result = Gfx::Canvas::CreateCompositionDevice(device.GetAddressOf());
	if (FAILED(result)) return result;

	// Every monitor can stretch the same solid pixel instead of allocating a full-screen bitmap.
	Microsoft::WRL::ComPtr<IDCompositionSurface> surface;
	result = device->CreateSurface(1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, surface.GetAddressOf());
	if (FAILED(result)) return result;

	Microsoft::WRL::ComPtr<ID2D1DeviceContext> context;
	POINT offset = {};
	result = surface->BeginDraw(nullptr, __uuidof(ID2D1DeviceContext), reinterpret_cast<void**>(context.GetAddressOf()), &offset);
	if (FAILED(result)) return result;

	// DirectComposition supplies the drawing session and may place our pixel inside a larger
	// texture. Clip Clear to its returned offset; do not call BeginDraw or EndDraw on the context.
	context->SetDpi(96.0f, 96.0f);
	context->SetTransform(D2D1::Matrix3x2F::Identity());
	context->PushAxisAlignedClip(D2D1::RectF((float)offset.x, (float)offset.y, (float)offset.x + 1.0f, (float)offset.y + 1.0f), D2D1_ANTIALIAS_MODE_ALIASED);
	context->Clear(color);
	context->PopAxisAlignedClip();
	result = surface->EndDraw();
	if (FAILED(result)) return result;

	std::vector<Overlay> overlays;
	for (const auto& monitor : MonitorUtil::GetMultiMonitorInfo().monitors)
	{
		if (!monitor.active) continue;

		const RECT& rect = monitor.screen;
		const int width = rect.right - rect.left;
		const int height = rect.bottom - rect.top;
		if (width <= 0 || height <= 0) continue;

		Overlay overlay;
		// A layered window passes clicks through fully transparent pixels. This window uses
		// composition for its color while retaining normal rectangular hit testing at alpha zero.
		overlay.window = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP, g_ClassName, nullptr, WS_POPUP, rect.left, rect.top, width, height, nullptr, nullptr, m_Instance, this);
		if (!overlay.window)
		{
			result = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		overlays.push_back(std::move(overlay));
		auto& target = overlays.back().target;
		result = device->CreateTargetForHwnd(overlays.back().window, TRUE, target.GetAddressOf());
		if (FAILED(result)) break;

		Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
		result = device->CreateVisual(visual.GetAddressOf());
		if (FAILED(result)) break;
		result = visual->SetContent(surface.Get());
		if (FAILED(result)) break;

		Microsoft::WRL::ComPtr<IDCompositionScaleTransform> transform;
		result = device->CreateScaleTransform(transform.GetAddressOf());
		if (FAILED(result)) break;
		result = transform->SetScaleX((float)width);
		if (FAILED(result)) break;
		result = transform->SetScaleY((float)height);
		if (FAILED(result)) break;
		result = visual->SetTransform(transform.Get());
		if (FAILED(result)) break;
		result = target->SetRoot(visual.Get());
		if (FAILED(result)) break;
	}

	if (SUCCEEDED(result) && overlays.empty()) result = E_FAIL;
	if (SUCCEEDED(result)) result = device->Commit();
	if (SUCCEEDED(result)) result = device->WaitForCommitCompletion();
	if (FAILED(result))
	{
		for (const auto& overlay : overlays) DestroyWindow(overlay.window);
		return result;
	}

	// Show replacements before removing old windows so a held button stays intercepted.
	if (!m_Overlays.empty())
	{
		for (const auto& overlay : overlays) ShowWindow(overlay.window, SW_SHOWNOACTIVATE);
	}

	DestroyOverlays();
	m_Overlays = std::move(overlays);
	m_Device = std::move(device);
	return S_OK;
}

bool ClickCaptureController::IsOverlayWindow(HWND window) const
{
	return std::any_of(m_Overlays.begin(), m_Overlays.end(), [window](const Overlay& overlay)
	{
		return overlay.window == window;
	});
}

void ClickCaptureController::DestroyOverlays()
{
	for (const auto& overlay : m_Overlays) DestroyWindow(overlay.window);
	m_Overlays.clear();
	m_Device.Reset();
}

void ClickCaptureController::Reset()
{
	++m_Generation;
	m_Owner = nullptr;
	m_State = State::Idle;
	m_PressedButtons = 0;
	if (m_ControlWindow) KillTimer(m_ControlWindow, g_Timer);
	DestroyOverlays();
}

void ClickCaptureController::Cancel()
{
	++m_Generation;

	// The owner may be in its destructor. Drop it now, but keep intercepting input until release
	// so the application underneath cannot receive a button-up without the matching button-down.
	m_Owner = nullptr;
	m_PressedButtons = GetPressedButtons();
	if (m_PressedButtons)
	{
		m_State = State::Draining;
	}
	else
	{
		Reset();
	}
}

void ClickCaptureController::Stop(MeasureMouse* owner)
{
	if (IsOwner(owner)) Cancel();
}

void ClickCaptureController::Tick()
{
	if (m_State == State::Idle) return;

	if (m_Owner && (m_Owner->IsDisabled() || m_Owner->IsPaused())) Cancel();

	// The overlays never take keyboard focus, so Escape must be checked outside WM_KEYDOWN.
	const bool escapeDown = GetAsyncKeyState(VK_ESCAPE) < 0;
	if (escapeDown && !m_EscapeDown) Cancel();
	m_EscapeDown = escapeDown;

	if (m_State == State::WaitingForClear && !GetPressedButtons()) m_State = State::Armed;
	if (m_State == State::Draining && !GetPressedButtons()) Reset();
	if (m_State == State::Completing && !GetPressedButtons())
	{
		if (!PostMessage(m_ControlWindow, g_CompleteMessage, m_Generation, 0)) Reset();
	}

	if (m_Device)
	{
		BOOL valid = FALSE;
		if (FAILED(m_Device->CheckDeviceState(&valid)) || !valid)
		{
			LogError(L"Mouse: Screen capture composition device was lost");
			Reset();
			return;
		}
	}

	for (const auto& overlay : m_Overlays)
	{
		SetWindowPos(overlay.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void ClickCaptureController::HandleButton(UINT button, bool down, POINT screenPos)
{
	if (m_State == State::Draining)
	{
		if (!down && !GetPressedButtons()) Reset();
		return;
	}

	if (m_State == State::WaitingForClear)
	{
		if (!down && !GetPressedButtons()) m_State = State::Armed;
		return;
	}

	if (m_State == State::Armed && down)
	{
		m_Button = button;
		m_State = State::Pressed;
	}

	if (m_State != State::Pressed && m_State != State::Completing) return;

	if (down)
	{
		m_PressedButtons |= 1 << button;
		m_State = State::Pressed;
		// Another press can arrive before a posted completion is handled. Wait for its release too.
		++m_Generation;
	}
	else
	{
		m_PressedButtons &= ~(1 << button);
		if (button == m_Button) m_Position = screenPos;
		if (!m_PressedButtons)
		{
			m_State = State::Completing;
			// Defer the skin action until this window procedure has finished handling the release.
			if (!PostMessage(m_ControlWindow, g_CompleteMessage, m_Generation, 0)) Reset();
		}
	}
}

void ClickCaptureController::Complete()
{
	if (m_State != State::Completing || !m_Owner) return;
	if (m_Owner->IsDisabled() || m_Owner->IsPaused())
	{
		Cancel();
		return;
	}

	if (GetPressedButtons()) return;

	MeasureMouse* owner = m_Owner;
	const MOUSEACTION action = g_Actions[m_Button];
	const POINT position = m_Position;
	// The action may destroy its measure or start a new request. Clear this request first and
	// leave the callback last so neither the owner nor the new request is touched afterward.
	Reset();
	owner->CompleteScreenCapture(action, position);
}

LRESULT CALLBACK ClickCaptureController::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto instance = reinterpret_cast<ClickCaptureController*>(GetWindowLongPtr(window, GWLP_USERDATA));
	if (message == WM_NCCREATE)
	{
		instance = static_cast<ClickCaptureController*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instance));
	}
	if (instance) return instance->HandleMessage(window, message, wParam, lParam);
	return DefWindowProc(window, message, wParam, lParam);
}

LRESULT ClickCaptureController::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (window == m_ControlWindow)
	{
		if (message == WM_TIMER && wParam == g_Timer)
		{
			Tick();
			return 0;
		}
		if (message == g_CompleteMessage)
		{
			if (wParam == m_Generation) Complete();
			return 0;
		}
	}

	switch (message)
	{
	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		ValidateRect(window, nullptr);
		return 0;

	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_CONTEXTMENU:
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		{
			UINT button = 0;
			if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) button = 1;
			if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP) button = 2;
			if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP) button = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? 3 : 4;
			const bool down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;
			POINT position = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(window, &position);
			HandleButton(button, down, position);
			return button >= 3 ? TRUE : 0;
		}
	}

	return DefWindowProc(window, message, wParam, lParam);
}

}  // namespace

namespace ClickCapture {

void Initialize()
{
	GetController().Initialize();
}

bool Start(MeasureMouse* owner, const D2D1_COLOR_F& color)
{
	return GetController().Start(owner, color);
}

void Stop(MeasureMouse* owner)
{
	GetController().Stop(owner);
}

bool IsOwner(const MeasureMouse* owner)
{
	return GetController().IsOwner(owner);
}

bool IsOverlayWindow(HWND window)
{
	return GetController().IsOverlayWindow(window);
}

void Reset()
{
	GetController().Reset();
}

void Finalize()
{
	GetController().Finalize();
}

}  // namespace ClickCapture
