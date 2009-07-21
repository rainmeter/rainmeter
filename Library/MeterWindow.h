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
#include "Export.h"

#define BEGIN_MESSAGEPROC switch(uMsg) {
#define MESSAGE(handler, msg) case msg: return Window?Window->handler(wParam, lParam):DefWindowProc(hWnd, uMsg, wParam, lParam);
#define REJECT_MESSAGE(msg) case msg: return 0;
#define END_MESSAGEPROC } return DefWindowProc(hWnd, uMsg, wParam, lParam);

#define WM_DELAYED_EXECUTE WM_APP

enum MOUSE
{
	MOUSE_LMB_DOWN,
	MOUSE_LMB_UP,
	MOUSE_RMB_DOWN,
	MOUSE_RMB_UP,
	MOUSE_OVER,
	MOUSE_LEAVE
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
	BANG_TOGGLEMEASURE,
	BANG_ENABLEMEASURE,
	BANG_DISABLEMEASURE,
	BANG_SHOW,
	BANG_HIDE,
	BANG_TOGGLE,
	BANG_MOVE,
	BANG_ZPOS,
	BANG_LSHOOK,
	BANG_ABOUT,
	BANG_MOVEMETER,
	BANG_PLUGIN
};

enum PATH_FOLDER
{
	PATH_FOLDER_INI,
	PATH_FOLDER_SKINS,
	PATH_FOLDER_CURRENT_SKIN,
	PATH_FOLDER_PLUGIN
};

typedef struct 
{
	int count;						//Number of monitors
	HMONITOR m_Monitors[32];		//Monitor info
	RECT m_MonitorRect[32];		//Monitor rect on virtual screen
	MONITORINFO m_MonitorInfo[32];	//Monitor information
	//int index;					//Utility for enumeration
	int vsT, vsL, vsH, vsW;
} MULTIMONITOR_INFO;

class CRainmeter;
class CMeasure;
class CMeter;

class CMeterWindow
{
public:
	CMeterWindow(std::wstring& config, std::wstring& iniFile);
	~CMeterWindow();

	int Initialize(CRainmeter& Rainmeter);

	void RunBang(BANGCOMMAND bang, const WCHAR* arg);

	void MoveMeter(int x, int y, const WCHAR* name);
	void HideMeter(const WCHAR* name);
	void ShowMeter(const WCHAR* name);
	void ToggleMeter(const WCHAR* name);
	void DisableMeasure(const WCHAR* name);
	void EnableMeasure(const WCHAR* name);
	void ToggleMeasure(const WCHAR* name);
	void Refresh(bool init);
	void Redraw();

	void MoveWindow(int x, int y);
	void ChangeZPos(ZPOSITION zPos);
	
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

	bool GetNativeTransparency() { return m_NativeTransparency; }
	bool GetClickThrough() { return m_ClickThrough; }
	bool GetKeepOnScreen() { return m_KeepOnScreen; }
	bool GetWindowDraggable() { return m_WindowDraggable; }
	bool GetSavePosition() { return m_SavePosition; }
	bool GetSnapEdges() { return m_SnapEdges; }
	HIDEMODE GetWindowHide() { return m_WindowHide; }
	int GetAlphaValue() { return m_AlphaValue; }
	int GetUpdateCounter() { return m_UpdateCounter; }
	int GetTransitionUpdate() { return m_TransitionUpdate; }

	void AddMeasureBang(const WCHAR* bang, int index, CMeasure* measure);

	LRESULT OnCopyData(WPARAM wParam, LPARAM lParam);

protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
	LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnNcHitTest(WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowPosChanging(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnRightButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnRightButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedExecute(WPARAM wParam, LPARAM lParam);
	LRESULT OnSettingChange(WPARAM wParam, LPARAM lParam); 


private:
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
	void ReadSkin();
	void InitializeMeters();
	void ShowWindowIfAppropriate();
	bool DoAction(int x, int y, MOUSE mouse, bool test);
	bool ResizeWindow(bool reset);
	void FadeWindow(int from, int to);

	CConfigParser m_Parser;

	Gdiplus::Bitmap* m_DoubleBuffer;
	Gdiplus::Bitmap* m_Background;				// The background bitmap
	SIZE m_BackgroundSize;

	HWND m_Window;								// Handle to the Rainmeter window
	HINSTANCE m_User32Library;
	BOOL m_ChildWindow;

	std::wstring m_RightMouseDownAction;		// Action to run when right mouse is pressed
	std::wstring m_LeftMouseDownAction;			// Action to run when left mouse is pressed
	std::wstring m_RightMouseUpAction;			// Action to run when right mouse is released
	std::wstring m_LeftMouseUpAction;			// Action to run when left mouse is released
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
	bool m_WindowXFromRight;
	bool m_WindowYFromBottom;
	bool m_WindowXPercentage;
	bool m_WindowYPercentage;
	float m_WindowXNumber;						// Store the number portion from the config
	float m_WindowYNumber;						// Store the number portion from the config
	int m_WindowW;								// Window's Width
	int m_WindowH;								// Window's Height
	int m_ScreenX;								// Window's X-postion on the virtual screen 
	int m_ScreenY;								// Window's Y-postion on the virtual screen
	bool m_AnchorXFromRight;
	bool m_AnchorYFromBottom;
	bool m_AnchorXPercentage;
	bool m_AnchorYPercentage;
	float m_AnchorXNumber;						// Store the number portion from the config
	float m_AnchorYNumber;						// Store the number portion from the config
	int m_AnchorScreenX;						// Window's anchor X-postion 
	int m_AnchorScreenY;						// Window's anchor Y-postion
	static MULTIMONITOR_INFO m_Monitors;		// Multi-Monitor info
	bool m_WindowDraggable;						// True, if window can be moved
	int m_WindowUpdate;							// Measure update frequency
	int m_TransitionUpdate;						// Transition redraw frequency
	bool m_ActiveTransition;
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
	bool m_Dragging;							//
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

	std::wstring m_SkinName;						// Name of the current skin folder
	std::wstring m_SkinIniFile;					// Name of the current skin iniFile

	std::wstring m_ConfigEditor;

	UINT m_Message;								// The current window message

	int m_UpdateCounter;

	CRainmeter* m_Rainmeter;					// Pointer to the main object

	static int m_InstanceCount;
};

#endif
