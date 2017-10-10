/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __DIALOGABOUT_H__
#define __DIALOGABOUT_H__

#include "../Common/Dialog.h"
#include "Logger.h"
#include "Skin.h"

class DialogAbout : public Dialog
{
public:
	DialogAbout();
	virtual ~DialogAbout();

	DialogAbout(const DialogAbout& other) = delete;
	DialogAbout& operator=(DialogAbout other) = delete;

	static Dialog* GetDialog() { return c_Dialog; }

	static void Open(int tab = 0);
	static void Open(const WCHAR* name);
	static void ShowAboutLog();

	static void AddLogItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);
	static void UpdateSkins();
	static void UpdateMeasures(Skin* skin);

	static void CloseDialog() { if (c_Dialog) c_Dialog->HandleMessage(WM_CLOSE, 0, 0); }

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	// Log tab
	class TabLog : public Tab
	{
	public:
		enum Id
		{
			Id_LogListView = 100,
			Id_ErrorCheckBox,
			Id_WarningCheckBox,
			Id_NoticeCheckBox,
			Id_DebugCheckBox,
			Id_ClearButton
		};

		TabLog();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

		void AddItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		bool m_Error;
		bool m_Warning;
		bool m_Notice;
		bool m_Debug;
	};

	// Skins tab
	class TabSkins : public Tab
	{
	public:
		enum Id
		{
			Id_SkinsListBox = 200,
			Id_SkinsListView
		};

		TabSkins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);
	
		void UpdateSkinList();
		void UpdateMeasureList(Skin* skin);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCustomDraw(WPARAM wParam, LPARAM lParam);

	private:
		static int CALLBACK ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

		Skin* m_SkinWindow;
	};

	// Plugins tab
	class TabPlugins : public Tab
	{
	public:
		enum Id
		{
			Id_PluginsListView = 300
		};

		TabPlugins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCustomDraw(WPARAM wParam, LPARAM lParam);

	private:
		typedef LPCTSTR (*GETPLUGINAUTHOR)();
		typedef UINT (*GETPLUGINVERSION)();
	};

	// Version tab
	class TabVersion : public Tab
	{
	public:
		enum Id
		{
			Id_AppIcon = 400,
			Id_VersionLabel,
			Id_HomeLink,
			Id_LicenseLink,
			Id_WinVerLabel,
			Id_PathLabel,
			Id_IniFileLabel,
			Id_SkinPathLabel,
			Id_CopyButton
		};

		TabVersion();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Resize(int w, int h);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	enum Id
	{
		Id_CloseButton = IDCLOSE,
		Id_Tab = 500
	};

	Tab& GetActiveTab();

	TabLog m_TabLog;
	TabSkins m_TabSkins;
	TabPlugins m_TabPlugins;
	TabVersion m_TabVersion;

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogAbout* c_Dialog;
};

#endif
