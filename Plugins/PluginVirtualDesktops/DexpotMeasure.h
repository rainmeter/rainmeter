/*
  Copyright (C) 2010-2011 Patrick Dubbert

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once
#include "VDMeasure.h"

#include <windows.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#define STRINGBUFFER_SIZE MAX_PATH

class DexpotMeasure : public VDMeasure
{
public:
	DexpotMeasure(HMODULE instance, UINT id);
	virtual ~DexpotMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual void Finalize();
	virtual UINT Update();
	virtual LPCTSTR GetString(UINT flags);
	virtual void ExecuteBang(LPCTSTR args) {};

	static DexpotMeasure* CreateMeasure(HMODULE instance, UINT id, LPCTSTR iniFile, LPCTSTR section);

protected:
	virtual void InitializeData() {};
	virtual void OnSwitched(int FromDesktop, int ToDesktop, WORD Flags, WORD Trigger) {};
	virtual void OnDesktopCountChanged(int) {};
	virtual void OnShutdown() {};
	virtual void OnDexpotStarted();
	virtual void OnDesktopConfigurationChanged() {};

	static void SendBang(std::wstring&);

	static HWND hWndDexpot;
	static HWND hWndMessageWindow;
	static TCHAR StringBuffer[STRINGBUFFER_SIZE];
	static BOOL PluginRegistered;
	static int CurrentDesktop;

private:
	static BOOL FindDexpotWindow();
	static HWND CreateMessageWindow();
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static int InstanceCount;
	static HWND hWndRainmeterControl;
	static std::set<DexpotMeasure*> DexpotMeasures;
	static UINT WM_DEXPOTSTARTED;
};


class DexpotDesktopCountMeasure : public DexpotMeasure
{
public:
	DexpotDesktopCountMeasure(HMODULE instance, UINT id);
	virtual ~DexpotDesktopCountMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual UINT Update();
	virtual void InitializeData();
	virtual void OnDesktopCountChanged(int);

private:
	enum DesktopCountType
	{
		Total,
		Columns,
		Rows,
	};

	int DesktopCount;
	std::wstring OnChange;
	DesktopCountType CountType;
};


class DexpotCurrentDesktopMeasure : public DexpotMeasure
{
public:
	DexpotCurrentDesktopMeasure(HMODULE instance, UINT id);
	virtual ~DexpotCurrentDesktopMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual UINT Update();
	virtual void OnSwitched(int, int, WORD, WORD);

private:
	std::wstring OnChange;
};


class DexpotVDMActiveMeasure : public DexpotMeasure
{
public:
	DexpotVDMActiveMeasure(HMODULE instance, UINT id);
	virtual ~DexpotVDMActiveMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual UINT Update();
	virtual void OnShutdown();
	virtual void OnDexpotStarted();

private:
	std::wstring OnActivate;
	std::wstring OnDeactivate;
};


class DexpotSwitchDesktopMeasure : public DexpotMeasure
{
public:
	DexpotSwitchDesktopMeasure(HMODULE instance, UINT id);
	virtual ~DexpotSwitchDesktopMeasure(void) {};

	virtual void ExecuteBang(LPCTSTR args);
};


class DexpotScreenshotMeasure : public DexpotMeasure
{
public:
	DexpotScreenshotMeasure(HMODULE instance, UINT id);
	virtual ~DexpotScreenshotMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual UINT Update();
	virtual LPCTSTR GetString(UINT flags);
	virtual void InitializeData();
	virtual void OnSwitched(int, int, WORD, WORD);

private:
	void UpdateScreenshot();

	std::wstring OutputFile;
	int DesktopNumber;
	int Width;
	int Height;
	BOOL RefreshOnUpdate;
};


class DexpotDesktopNameMeasure : public DexpotMeasure
{
public:
	DexpotDesktopNameMeasure(HMODULE instance, UINT id);
	virtual ~DexpotDesktopNameMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual LPCTSTR GetString(UINT flags);
	virtual void InitializeData();
	virtual void OnDesktopConfigurationChanged();
	virtual void OnDesktopCountChanged(int NewCount);

	static void SetDesktopName(UINT Desktop, std::wstring &Name);

private:
	int DesktopNumber;
	static std::vector<std::wstring> DesktopNames;
};


class DexpotDesktopWallpaperMeasure : public DexpotMeasure
{
public:
	DexpotDesktopWallpaperMeasure(HMODULE instance, UINT id);
	virtual ~DexpotDesktopWallpaperMeasure(void) {};

	virtual UINT Initialize(LPCTSTR iniFile, LPCTSTR section);
	virtual LPCTSTR GetString(UINT flags);
	virtual void InitializeData();
	virtual void OnSwitched(int FromDesktop, int ToDesktop, WORD Flags, WORD Trigger);
	virtual void OnDesktopConfigurationChanged();
	virtual void OnDesktopCountChanged(int NewCount);

	static void SetDesktopWallpaper(UINT Desktop, std::wstring &Wallpaper);

private:
	int DesktopNumber;
	static std::vector<std::wstring> DesktopWallpapers;
};


class DexpotCommandMeasure : public DexpotMeasure
{
public:
	DexpotCommandMeasure(HMODULE instance, UINT id);
	virtual ~DexpotCommandMeasure(void) {};

	virtual void ExecuteBang(LPCTSTR args);
};
