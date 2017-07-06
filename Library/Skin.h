/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKIN_H_
#define RM_LIBRARY_SKIN_H_

#include <windows.h>
#include <dwmapi.h>
#include <string>
#include <list>
#include "CommandHandler.h"
#include "ConfigParser.h"
#include "Group.h"
#include "Mouse.h"
#include "../Common/Gfx/Canvas.h"

#define BEGIN_MESSAGEPROC switch (uMsg) {
#define MESSAGE(handler, msg) case msg: return skin->handler(uMsg, wParam, lParam);
#define REJECT_MESSAGE(msg) case msg: return 0;
#define END_MESSAGEPROC } return DefWindowProc(hWnd, uMsg, wParam, lParam);

#define WM_METERWINDOW_DELAYED_REFRESH WM_APP + 1
#define WM_METERWINDOW_DELAYED_MOVE    WM_APP + 3

#define METERWINDOW_CLASS_NAME	L"RainmeterMeterWindow"

#define RI_MOUSE_HORIZONTAL_WHEEL 0x0800

enum BUTTONPROC
{
	BUTTONPROC_DOWN,
	BUTTONPROC_UP,
	BUTTONPROC_MOVE
};

enum ZPOSITION
{
	ZPOSITION_ONDESKTOP = -2,
	ZPOSITION_ONBOTTOM = -1,
	ZPOSITION_NORMAL = 0,
	ZPOSITION_ONTOP = 1,
	ZPOSITION_ONTOPMOST = 2
};

enum BLURMODE
{
	BLURMODE_NONE = 0,
	BLURMODE_FULL,
	BLURMODE_REGION
};

enum BGMODE
{
	BGMODE_IMAGE = 0,
	BGMODE_COPY,
	BGMODE_SOLID,
	BGMODE_SCALED_IMAGE,
	BGMODE_TILED_IMAGE
};

enum HIDEMODE
{
	HIDEMODE_NONE = 0,
	HIDEMODE_HIDE,
	HIDEMODE_FADEIN,
	HIDEMODE_FADEOUT
};

enum BEVELTYPE 
{
	BEVELTYPE_NONE,
	BEVELTYPE_UP,
	BEVELTYPE_DOWN
};

enum RESIZEMODE
{
	RESIZEMODE_NONE = 0,
	RESIZEMODE_CHECK,
	RESIZEMODE_RESET
};

class Rainmeter;
class Measure;
class Meter;

namespace Gfx {
class FontCollection;
class TextFormat;
}

class Skin : public Group
{
public:
	Skin(const std::wstring& folderPath, const std::wstring& file);
	~Skin();

	Skin(const Skin& other) = delete;
	Skin& operator=(Skin other) = delete;

	void Initialize();

	void DoBang(Bang bang, const std::vector<std::wstring>& args);
	void DoDelayedCommand(const WCHAR* command, UINT delay);

	void HideMeter(const std::wstring& name, bool group = false);
	void ShowMeter(const std::wstring& name, bool group = false);
	void ToggleMeter(const std::wstring& name, bool group = false);
	void MoveMeter(const std::wstring& name, int x, int y);
	void UpdateMeter(const std::wstring& name, bool group = false);
	void DisableMeasure(const std::wstring& name, bool group = false);
	void EnableMeasure(const std::wstring& name, bool group = false);
	void ToggleMeasure(const std::wstring& name, bool group = false);
	void PauseMeasure(const std::wstring& name, bool group = false);
	void UnpauseMeasure(const std::wstring& name, bool group = false);
	void TogglePauseMeasure(const std::wstring& name, bool group = false);
	void UpdateMeasure(const std::wstring& name, bool group = false);
	void Deactivate();
	void Refresh(bool init, bool all = false);
	void Redraw();
	void RedrawWindow() { UpdateWindow(m_TransparencyValue); }
	void SetVariable(const std::wstring& variable, const std::wstring& value);
	void SetOption(const std::wstring& section, const std::wstring& option, const std::wstring& value, bool group);

	void SetMouseLeaveEvent(bool cancel);
	void SetHasMouseScrollAction() { m_HasMouseScrollAction = true; }

	void MoveWindow(int x, int y);
	void MoveSelectedWindow(int dx, int dy);
	bool IsSelected() { return m_Selected; }
	void SelectSkinsGroup(std::unordered_set<std::wstring> groups);
	void Select();
	void Deselect();

	void ChangeZPos(ZPOSITION zPos, bool all = false);
	void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
	void FadeWindow(int from, int to);
	void HideFade();
	void ShowFade();

	void ResizeBlur(const std::wstring& arg, int mode);
	bool IsBlur() { return m_Blur; }
	void SetBlur(bool b) { m_Blur = b; }

	void SetResizeWindowMode(RESIZEMODE mode) { if (m_ResizeWindow != RESIZEMODE_RESET || mode != RESIZEMODE_CHECK) m_ResizeWindow = mode; }

	Gfx::Canvas& GetCanvas() { return m_Canvas; }
	HWND GetWindow() { return m_Window; }

	ConfigParser& GetParser() { return m_Parser; }

	const std::wstring& GetFolderPath() { return m_FolderPath; }
	const std::wstring& GetFileName() { return m_FileName; }
	std::wstring GetFilePath();
	std::wstring GetRootName();
	std::wstring GetRootPath();
	std::wstring GetResourcesPath();
	std::wstring GetSkinPath();

	const std::vector<Measure*>& GetMeasures() { return m_Measures; }
	const std::vector<Meter*>& GetMeters() { return m_Meters; }

	ZPOSITION GetWindowZPosition() { return m_WindowZPosition; }
	bool GetXPercentage() { return m_WindowXPercentage; }
	bool GetYPercentage() { return m_WindowYPercentage; }
	bool GetXFromRight() { return m_WindowXFromRight; }
	bool GetYFromBottom() { return m_WindowYFromBottom; }

	int GetW() { return m_WindowW; }
	int GetH() { return m_WindowH; }
	int GetX() { return m_ScreenX; }
	int GetY() { return m_ScreenY; }

	bool GetXScreenDefined() { return m_WindowXScreenDefined; }
	bool GetYScreenDefined() { return m_WindowYScreenDefined; }
	int GetXScreen() { return m_WindowXScreen; }
	int GetYScreen() { return m_WindowYScreen; }

	bool GetClickThrough() { return m_ClickThrough; }
	bool GetKeepOnScreen() { return m_KeepOnScreen; }
	bool GetAutoSelectScreen() { return m_AutoSelectScreen; }
	bool GetWindowDraggable() { return m_WindowDraggable; }
	bool GetSavePosition() { return m_SavePosition; }
	bool GetSnapEdges() { return m_SnapEdges; }
	HIDEMODE GetWindowHide() { return m_WindowHide; }
	int GetAlphaValue() { return m_AlphaValue; }
	int GetUpdateCounter() { return m_UpdateCounter; }
	int GetTransitionUpdate() { return m_TransitionUpdate; }
	int GetDefaultUpdateDivider() { return m_DefaultUpdateDivider; }

	bool GetMeterToolTipHidden() { return m_ToolTipHidden; }

	bool GetFavorite() { return m_Favorite; }

	bool IsClosing() { return m_State == STATE_CLOSING; }

	const Mouse& GetMouse() { return m_Mouse; }

	void MakePathAbsolute(std::wstring& path);

	Gfx::FontCollection* GetFontCollection() { return m_FontCollection; }

	Meter* GetMeter(const std::wstring& meterName);
	Measure* GetMeasure(const std::wstring& measureName) { return m_Parser.GetMeasure(measureName); }

	friend class DialogManage;

protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK InitialWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT OnMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnEnterMenuLoop(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseHScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnRightButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMiddleButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnRightButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMiddleButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnRightButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMiddleButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnXButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnXButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnXButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedRefresh(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDwmColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDwmCompositionChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSetWindowFocus(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimeChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPowerBroadcast(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	enum STATE
	{
		STATE_INITIALIZING,
		STATE_REFRESHING,
		STATE_RUNNING,
		STATE_CLOSING
	};

	enum OPTION
	{
		OPTION_POSITION         = 0x00000001,
		OPTION_ALPHAVALUE       = 0x00000002,
		OPTION_FADEDURATION     = 0x00000004,
		OPTION_CLICKTHROUGH     = 0x00000008,
		OPTION_DRAGGABLE        = 0x00000010,
		OPTION_HIDEONMOUSEOVER  = 0x00000020,
		OPTION_SAVEPOSITION     = 0x00000040,
		OPTION_SNAPEDGES        = 0x00000080,
		OPTION_KEEPONSCREEN     = 0x00000100,
		OPTION_AUTOSELECTSCREEN = 0x00000200,
		OPTION_ALWAYSONTOP      = 0x00000400,

		OPTION_ALL              = 0xFFFFFFFF
	};

	bool HitTest(int x, int y);

	void SnapToWindow(Skin* skin, LPWINDOWPOS wp);
	void MapCoordsToScreen(int& x, int& y, int w, int h);
	void WindowToScreen();
	void ScreenToWindow();
	void PostUpdate(bool bActiveTransition);
	bool UpdateMeasure(Measure* measure, bool force);
	bool UpdateMeter(Meter* meter, bool& bActiveTransition, bool force);
	void Update(bool refresh);
	void UpdateWindow(int alpha, bool canvasBeginDrawCalled = false);
	void UpdateWindowTransparency(int alpha);
	void ReadOptions();
	void WriteOptions(INT setting = OPTION_ALL);
	bool ReadSkin();
	void ShowWindowIfAppropriate();
	HWND GetWindowFromPoint(POINT pos);
	void HandleButtons(POINT pos, BUTTONPROC proc, bool execute = true);
	void SetClickThrough(bool b);
	void SetKeepOnScreen(bool b);
	void SetAutoSelectScreen(bool b);
	void SetWindowDraggable(bool b);
	void SetSavePosition(bool b);
	void SavePositionIfAppropriate();
	void SetSnapEdges(bool b);
	void UpdateFadeDuration();
	void SetWindowHide(HIDEMODE hide);
	void SetWindowZPosition(ZPOSITION zpos);
	bool DoAction(int x, int y, MOUSEACTION action, bool test);
	bool DoMoveAction(int x, int y, MOUSEACTION action);
	bool ResizeWindow(bool reset);
	void IgnoreAeroPeek();
	void RegisterMouseInput();
	void UnregisterMouseInput();
	void AddWindowExStyle(LONG_PTR flag);
	void RemoveWindowExStyle(LONG_PTR flag);
	void BlurBehindWindow(BOOL fEnable);
	void SetWindowPositionVariables(int x, int y);
	void SetWindowSizeVariables(int w, int h);
	void SetFavorite(bool favorite);
	void DeselectSkinsIfAppropriate(HWND hwnd);

	void ShowBlur();
	void HideBlur();

	void Dispose(bool refresh);
	void CreateDoubleBuffer(int cx, int cy);

	Gfx::Canvas m_Canvas;

	ConfigParser m_Parser;

	Gdiplus::Bitmap* m_Background;
	SIZE m_BackgroundSize;

	HWND m_Window;

	Mouse m_Mouse;
	bool m_MouseOver;
	bool m_MouseInputRegistered;
	bool m_HasMouseScrollAction;

	std::wstring m_OnRefreshAction;
	std::wstring m_OnCloseAction;
	std::wstring m_OnFocusAction;
	std::wstring m_OnUnfocusAction;
	std::wstring m_OnUpdateAction;
	std::wstring m_OnWakeAction;

	std::wstring m_SkinGroup;
	std::wstring m_BackgroundName;
	RECT m_BackgroundMargins;
	RECT m_DragMargins;
	std::wstring m_WindowX;
	std::wstring m_WindowY;
	std::wstring m_AnchorX;
	std::wstring m_AnchorY;
	int m_WindowXScreen;
	int m_WindowYScreen;
	bool m_WindowXScreenDefined;
	bool m_WindowYScreenDefined;
	bool m_WindowXFromRight;
	bool m_WindowYFromBottom;
	bool m_WindowXPercentage;
	bool m_WindowYPercentage;
	int m_WindowW;
	int m_WindowH;
	int m_ScreenX;								// X-postion on the virtual screen 
	int m_ScreenY;								// Y-postion on the virtual screen
	int m_SkinW;								// User defined width of skin
	int m_SkinH;								// User defined height of skin
	bool m_AnchorXFromRight;
	bool m_AnchorYFromBottom;
	bool m_AnchorXPercentage;
	bool m_AnchorYPercentage;
	int m_AnchorScreenX;
	int m_AnchorScreenY;
	bool m_WindowDraggable;
	int m_WindowUpdate;
	int m_TransitionUpdate;
	int m_DefaultUpdateDivider;
	bool m_ActiveTransition;
	bool m_HasNetMeasures;
	bool m_HasButtons;
	HIDEMODE m_WindowHide;
	bool m_WindowStartHidden;
	bool m_SavePosition;
	bool m_SnapEdges;
	int m_AlphaValue;
	int m_FadeDuration;
	int m_NewFadeDuration;
	ZPOSITION m_WindowZPosition;
	bool m_DynamicWindowSize;
	bool m_ClickThrough;
	bool m_KeepOnScreen;
	bool m_AutoSelectScreen;
	bool m_Dragging;
	bool m_Dragged;
	BGMODE m_BackgroundMode;
	Gdiplus::Color m_SolidColor;
	Gdiplus::Color m_SolidColor2;
	Gdiplus::REAL m_SolidAngle;
	BEVELTYPE m_SolidBevel;

	bool m_OldWindowDraggable;
	bool m_OldKeepOnScreen;
	bool m_OldClickThrough;

	bool m_Selected;
	Gdiplus::Color m_SelectedColor;

	Group m_DragGroup;

	bool m_Blur;
	BLURMODE m_BlurMode;
	HRGN m_BlurRegion;

	ULONGLONG m_FadeStartTime;
	int m_FadeStartValue;
	int m_FadeEndValue;
	bool m_ActiveFade;

	int m_TransparencyValue;

	STATE m_State;

	bool m_Hidden;
	RESIZEMODE m_ResizeWindow;

	std::unordered_map<UINT_PTR, std::wstring> m_DelayedCommands;

	std::vector<Measure*> m_Measures;
	std::vector<Meter*> m_Meters;

	const std::wstring m_FolderPath;
	const std::wstring m_FileName;

	int m_UpdateCounter;
	UINT m_MouseMoveCounter;

	Gfx::FontCollection* m_FontCollection;

	bool m_ToolTipHidden;

	bool m_Favorite;

	static int c_InstanceCount;
	static bool c_IsInSelectionMode;
};

#endif
