/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERWINDOW_H__
#define __METERWINDOW_H__

#pragma warning(disable: 4786)

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <list>
#include <vector>
#include "ConfigParser.h"
#include "Group.h"
#include "Export.h"

#define BEGIN_MESSAGEPROC switch(uMsg) {
#define MESSAGE(handler, msg) case msg: return Window?Window->handler(uMsg, wParam, lParam):DefWindowProc(hWnd, uMsg, wParam, lParam);
#define REJECT_MESSAGE(msg) case msg: return 0;
#define END_MESSAGEPROC } return DefWindowProc(hWnd, uMsg, wParam, lParam);

#define WM_DELAYED_EXECUTE WM_APP + 0
#define WM_DELAYED_REFRESH WM_APP + 1
#define WM_DELAYED_MOVE    WM_APP + 3

#define METERWINDOW_CLASS_NAME	L"RainmeterMeterWindow"

enum MOUSE
{
	MOUSE_LMB_DOWN,
	MOUSE_LMB_UP,
	MOUSE_LMB_DBLCLK,
	MOUSE_RMB_DOWN,
	MOUSE_RMB_UP,
	MOUSE_RMB_DBLCLK,
	MOUSE_MMB_DOWN,
	MOUSE_MMB_UP,
	MOUSE_MMB_DBLCLK,
	MOUSE_OVER,
	MOUSE_LEAVE
};

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

enum BGMODE
{
	BGMODE_IMAGE = 0,
	BGMODE_COPY,
	BGMODE_SOLID,
	BGMODE_SCALED_IMAGE
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

enum BANGCOMMAND
{
	BANG_REFRESH,
	BANG_REDRAW,
	BANG_TOGGLEMETER,
	BANG_SHOWMETER,
	BANG_HIDEMETER,
	BANG_MOVEMETER,
	BANG_TOGGLEMEASURE,
	BANG_ENABLEMEASURE,
	BANG_DISABLEMEASURE,
	BANG_SHOW,
	BANG_HIDE,
	BANG_TOGGLE,
	BANG_SHOWFADE,
	BANG_HIDEFADE,
	BANG_TOGGLEFADE,
	BANG_MOVE,
	BANG_ZPOS,
	BANG_SETTRANSPARENCY,
	BANG_CLICKTHROUGH,
	BANG_DRAGGABLE,
	BANG_SNAPEDGES,
	BANG_KEEPONSCREEN,

	BANG_TOGGLEMETERGROUP,
	BANG_SHOWMETERGROUP,
	BANG_HIDEMETERGROUP,
	BANG_TOGGLEMEASUREGROUP,
	BANG_ENABLEMEASUREGROUP,
	BANG_DISABLEMEASUREGROUP,

	BANG_LSHOOK,
	BANG_PLUGIN,
	BANG_SETVARIABLE
};

class CRainmeter;
class CMeasure;
class CMeter;

class CMeterWindow : public CGroup
{
public:
	CMeterWindow(std::wstring& path, std::wstring& config, std::wstring& iniFile);
	~CMeterWindow();

	int Initialize(CRainmeter& Rainmeter);

	CRainmeter* GetMainObject() { return m_Rainmeter; };

	void RunBang(BANGCOMMAND bang, const WCHAR* arg);

	void MoveMeter(int x, int y, const WCHAR* name);
	void HideMeter(const WCHAR* name, bool group = false);
	void ShowMeter(const WCHAR* name, bool group = false);
	void ToggleMeter(const WCHAR* name, bool group = false);
	void DisableMeasure(const WCHAR* name, bool group = false);
	void EnableMeasure(const WCHAR* name, bool group = false);
	void ToggleMeasure(const WCHAR* name, bool group = false);
	void Refresh(bool init, bool all = false);
	void Redraw();

	void SetMouseLeaveEvent(bool cancel);

	void MoveWindow(int x, int y);
	void ChangeZPos(ZPOSITION zPos, bool all = false);
	void FadeWindow(int from, int to);

	Gdiplus::Bitmap* GetDoubleBuffer() { return m_DoubleBuffer; };
	HWND GetWindow() { return m_Window; };

	CConfigParser& GetParser() { return m_Parser; };

	const std::wstring& GetSkinAuthor() { return m_Author; };
	const std::wstring& GetSkinName() { return m_SkinName; };
	const std::wstring& GetSkinIniFile() { return m_SkinIniFile; };

	std::list<CMeasure*>& GetMeasures() { return m_Measures; };
	std::list<CMeter*>& GetMeters() { return m_Meters; };

	ZPOSITION GetWindowZPosition() { return m_WindowZPosition; }
	bool GetXPercentage() { return m_WindowXPercentage; }
	bool GetYPercentage() { return m_WindowYPercentage; }
	bool GetXFromRight() { return m_WindowXFromRight; }
	bool GetYFromBottom() { return m_WindowYFromBottom; }

	bool GetXScreenDefined() { return m_WindowXScreenDefined; }
	bool GetYScreenDefined() { return m_WindowYScreenDefined; }
	int GetXScreen() { return m_WindowXScreen; }
	int GetYScreen() { return m_WindowYScreen; }

	bool GetNativeTransparency() { return m_NativeTransparency; }
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

	void AddMeasureBang(const WCHAR* bang, int index, CMeasure* measure);

	LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam);

	std::wstring MakePathAbsolute(std::wstring path);

	Gdiplus::PrivateFontCollection* GetPrivateFontCollection(){ return m_FontCollection; }

protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam);
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
	LRESULT OnDelayedExecute(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedRefresh(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam);  
	LRESULT OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam);  

private:
	bool HitTest(int x, int y);

	void CreateRegion(bool clear);
	void GetSkinFolders(const std::wstring& folder);
	Gdiplus::Bitmap* GrabDesktop(int x, int y, int w, int h);
	void SnapToWindow(CMeterWindow* window, LPWINDOWPOS wp);
	void MapCoordsToScreen(int& x, int& y, int w, int h);
	void WindowToScreen();
	void ScreenToWindow();
	void Update(bool nodraw);
	void UpdateTransparency(int alpha, bool reset);
	void ReadConfig();
	void WriteConfig();
	bool ReadSkin();
	void InitializeMeasures();
	void InitializeMeters();
	void ShowWindowIfAppropriate();
	HWND GetWindowFromPoint(POINT pos);
	void HandleButtons(POINT pos, BUTTONPROC proc, CMeterWindow* meterWindow, bool changeCursor);
	void SetClickThrough(bool b);
	void SetKeepOnScreen(bool b);
	void SetWindowDraggable(bool b);
	void SetSavePosition(bool b);
	void SetSnapEdges(bool b);
	bool DoAction(int x, int y, MOUSE mouse, bool test);
	bool DoMoveAction(int x, int y, MOUSE mouse);
	bool ResizeWindow(bool reset);
	void IgnoreAeroPeek();

	CConfigParser m_Parser;

	Gdiplus::Bitmap* m_DoubleBuffer;
	Gdiplus::Bitmap* m_Background;				// The background bitmap
	SIZE m_BackgroundSize;

	HWND m_Window;								// Handle to the Rainmeter window
	HINSTANCE m_User32Library;
	BOOL m_ChildWindow;

	std::wstring m_RightMouseDownAction;		// Action to run when right mouse is pressed
	std::wstring m_LeftMouseDownAction;			// Action to run when left mouse is pressed
	std::wstring m_MiddleMouseDownAction;		// Action to run when middle mouse is pressed
	std::wstring m_RightMouseUpAction;			// Action to run when right mouse is released
	std::wstring m_LeftMouseUpAction;			// Action to run when left mouse is released
	std::wstring m_MiddleMouseUpAction;			// Action to run when middle mouse is released
	std::wstring m_RightMouseDoubleClickAction;	// Action to run when right mouse is double-clicked
	std::wstring m_LeftMouseDoubleClickAction;	// Action to run when left mouse is double-clicked
	std::wstring m_MiddleMouseDoubleClickAction;	// Action to run when middle mouse is double-clicked
	std::wstring m_MouseOverAction;				// Action to run when mouse goes over the window
	std::wstring m_MouseLeaveAction;			// Action to run when mouse leaves the window
	std::wstring m_OnRefreshAction;				// Action to run when window is initialized

	bool m_MouseOver;

	std::wstring m_Author;						// Skin's author
	std::wstring m_BackgroundName;				// Name of the background image
	Gdiplus::Rect m_BackgroundMargins;
	Gdiplus::Rect m_DragMargins;
	std::wstring m_WindowX;						// Window's X-position in config file
	std::wstring m_WindowY;						// Window's Y-position in config file
	std::wstring m_AnchorX;						// Anchor's X-position in config file
	std::wstring m_AnchorY;						// Anchor's Y-position in config file
	int m_WindowXScreen;
	int m_WindowYScreen;
	bool m_WindowXScreenDefined;
	bool m_WindowYScreenDefined;
	bool m_WindowXFromRight;
	bool m_WindowYFromBottom;
	bool m_WindowXPercentage;
	bool m_WindowYPercentage;
	int m_WindowW;								// Window's Width
	int m_WindowH;								// Window's Height
	int m_ScreenX;								// Window's X-postion on the virtual screen 
	int m_ScreenY;								// Window's Y-postion on the virtual screen
	bool m_AnchorXFromRight;
	bool m_AnchorYFromBottom;
	bool m_AnchorXPercentage;
	bool m_AnchorYPercentage;
	int m_AnchorScreenX;						// Window's anchor X-postion 
	int m_AnchorScreenY;						// Window's anchor Y-postion
	bool m_WindowDraggable;						// True, if window can be moved
	int m_WindowUpdate;							// Measure update frequency
	int m_TransitionUpdate;						// Transition redraw frequency
	bool m_ActiveTransition;
	bool m_HasNetMeasures;
	bool m_HasButtons;
	HIDEMODE m_WindowHide;						// If true, the window is hidden when mouse is over it
	bool m_WindowStartHidden;					// If true, the window is hidden at startup
	bool m_SavePosition;						// If true, the window's position is saved
	bool m_SnapEdges;							// If true, the window snaps to the edges of the screen when moved
	bool m_NativeTransparency;					// If true, use the W2k/XP native transparency
	int m_AlphaValue;							// The 'from' transparency value 0 - 255
	int m_FadeDuration;							// Time it takes to fade the window
	bool m_MeasuresToVariables;					// If true, Measured values are transformed to Litestep's eVars
	ZPOSITION m_WindowZPosition;				// Window's Z-position
	bool m_DynamicWindowSize;					// 
	bool m_ClickThrough;						// 
	bool m_KeepOnScreen;						// 
	bool m_AutoSelectScreen;					//
	bool m_Dragging;							//
	bool m_Dragged;								//
	BGMODE m_BackgroundMode;					// The background mode
	Gdiplus::Color m_SolidColor;				// Color of the solid background
	Gdiplus::Color m_SolidColor2;				// Color of the solid background
	Gdiplus::REAL m_SolidAngle;				// 
	BEVELTYPE m_SolidBevel;						// The type of the bevel

	DWORD m_FadeStartTime;
	int m_FadeStartValue;
	int m_FadeEndValue;
	int m_TransparencyValue;

	bool m_Refreshing;							// This is true, when the meter is refreshing

	bool m_Hidden;								// True, if Rainmeter is hidden
	bool m_ResetRegion;							// If true, the window region is recalculated during the next update

	std::list<CMeasure*> m_Measures;			// All the measures
	std::list<CMeter*> m_Meters;				// All the meters

	std::wstring m_SkinPath;					// Path of the skin folder
	std::wstring m_SkinName;					// Name of the current skin folder
	std::wstring m_SkinIniFile;					// Name of the current skin iniFile

	int m_UpdateCounter;

	CRainmeter* m_Rainmeter;					// Pointer to the main object

	static int c_InstanceCount;

	Gdiplus::PrivateFontCollection* m_FontCollection;

	bool m_MouseActionCursor;
};

#endif
