/*
  Copyright (C) 2011 Birunthan Mohanathas

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

#ifndef _DIALOGABOUT_H_
#define _DIALOGABOUT_H_

#include "MeterWindow.h"
#include "TrayWindow.h"
#include "Dialog.h"

class CDialogAbout : public CDialog
{
public:
	CDialogAbout(HWND window);
	virtual ~CDialogAbout();

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	static void Open(int tab = 0);

	static void AddLogItem(int level, LPCWSTR time, LPCWSTR message);
	static void UpdateSkins();
	static void UpdateMeasures(LPCTSTR entryName = NULL);

	static WINDOWPLACEMENT c_WindowPlacement;
	static CDialogAbout* c_Dialog;

private:
	// Log tab
	class CTabLog : public CTab
	{
	public:
		CTabLog(HWND window);

		virtual void Initialize();
		virtual void Resize(int w, int h);

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

		void AddItem(int level, LPCWSTR time, LPCWSTR message);

	private:
		bool m_Error;
		bool m_Warning;
		bool m_Notice;
		bool m_Debug;
	};

	// Measures tab
	class CTabMeasures : public CTab
	{
	public:
		CTabMeasures(HWND window);

		virtual void Initialize();
		virtual void Resize(int w, int h);

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	
		void UpdateSkinList();
		void UpdateMeasureList(LPCTSTR entryName);
	};

	// Plugins tab
	class CTabPlugins : public CTab
	{
	public:
		CTabPlugins(HWND window);

		virtual void Initialize();
		virtual void Resize(int w, int h);

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		typedef LPCTSTR (*GETPLUGINAUTHOR)();
		typedef UINT (*GETPLUGINVERSION)();
	};

	// Version tab
	class CTabVersion : public CTab
	{
	public:
		CTabVersion(HWND window);

		virtual void Initialize();
		virtual void Resize(int w, int h);

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};
	
	CTabLog* m_TabLog;
	CTabMeasures* m_TabMeasures;
	CTabPlugins* m_TabPlugins;
	CTabVersion* m_TabVersion;
};

#endif
