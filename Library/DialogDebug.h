/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_DIALOGDEBUG_H_
#define RM_LIBRARY_DIALOGDEBUG_H_

#include "../Common/Dialog.h"
#include "Logger.h"
#include "Skin.h"

class DialogDebug : public Dialog
{
public:
	DialogDebug();
	virtual ~DialogDebug();

	DialogDebug(const DialogDebug& other) = delete;
	DialogDebug& operator=(DialogDebug other) = delete;

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
		~TabLog();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

		void AddItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		void CreateImageList();
		void DestroyImageList();

		bool m_Error;
		bool m_Warning;
		bool m_Notice;
		bool m_Debug;

		HIMAGELIST m_ImageList;
	};

	// Skins tab
	class TabSkins : public Tab
	{
	public:
		enum Id
		{
			Id_SkinsListBox = 200,
			Id_SkinsListView,
			Id_EvaluateGroup,
			Id_EvaluateStringRadio,
			Id_EvaluateNumberRadio,
			Id_EvaluateEdit,
			Id_EvaluateResult
		};

		TabSkins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

		void UpdateSkinList();
		void UpdateMeasureList(Skin* skin);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCustomDraw(WPARAM wParam, LPARAM lParam);

	private:
		static int CALLBACK ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		void UpdateEvaluationResult();

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
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCustomDraw(WPARAM wParam, LPARAM lParam);

	private:
		typedef LPCTSTR (*GETPLUGINAUTHOR)();
		typedef UINT (*GETPLUGINVERSION)();
	};

	enum Id
	{
		Id_CloseButton = IDCLOSE,
		Id_Tab = 500
	};

	TabLog m_TabLog;
	TabSkins m_TabSkins;
	TabPlugins m_TabPlugins;

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogDebug* c_Dialog;
};

#endif
