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

class DirectoryWatcher;

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
	static void OpenSkin(Skin* skin);
	static void ShowAboutLog();

	static void AddLogItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message);
	static void UpdateSkins();
	static void UpdateMeasures(Skin* skin);
	static void UpdateDisplays();

	static void CloseDialog() { if (c_Dialog) c_Dialog->HandleMessage(WM_CLOSE, 0, 0); }

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	class TabLog : public Tab
	{
	public:
		enum Id
		{
			Id_LogListView = 100,
			Id_TypeMenuButton,
			Id_LogFileMenuButton,
			Id_ErrorMenuItem,
			Id_WarningMenuItem,
			Id_NoticeMenuItem,
			Id_DebugMenuItem,
			Id_DebugModeMenuItem,
			Id_ShowLogFileMenuItem,
			Id_StartLoggingMenuItem,
			Id_StopLoggingMenuItem,
			Id_DeleteLogFileMenuItem,
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

	class TabSkins : public Tab
	{
	public:
		enum Id
		{
			Id_SelectSkinButton = 200,
			Id_SkinsListView,
			Id_SkinMenuButton,
			Id_AutoRefreshCheckBox,
			Id_AddWatchButton,
			Id_JumpMeasures,
			Id_JumpVariables,
			Id_JumpWatches
		};

		TabSkins();
		~TabSkins();

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

		void SelectSkin(Skin* skin);
		void UpdateSkinList();
		void UpdateMeasureList(Skin* skin);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCustomDraw(WPARAM wParam, LPARAM lParam);

	private:
		class PanelWatch;
		struct Watch
		{
			std::wstring text;
			bool formula;
		};

		static int CALLBACK ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		void AddWatch(const std::wstring& text, bool formula);
		void EditWatch(size_t index);
		void SaveWatch(size_t index, const std::wstring& text, bool formula);
		void DeleteWatch(size_t index);
		size_t GetSelectedWatch();
		void EnsureWatchVisible(size_t index);
		void UpdateRangeToolTip(HWND list, POINT point);
		void UpdateDirectoryWatcher();
		static void OnDirectoryChange(const WCHAR* path, void* context);
		static LRESULT CALLBACK SkinsListViewSubclass(HWND hwnd, UINT msg, WPARAM wParam,
			LPARAM lParam, UINT_PTR id, DWORD_PTR data);

		Skin* m_SkinWindow;
		bool m_AutoRefresh;
		HWND m_RangeToolTip;
		int m_RangeToolTipItem;
		std::wstring m_RangeToolTipText;
		std::unique_ptr<DirectoryWatcher> m_DirectoryWatcher;
		std::vector<std::wstring> m_AutoRefreshFiles;
		std::unique_ptr<PanelWatch> m_PanelWatch;
		std::vector<Watch> m_Watches;
	};

	class TabDisplay : public Tab
	{
	public:
		enum Id
		{
			Id_DisplaysListView = 400
		};

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

		void UpdateDisplayList();

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	class TabNetwork : public Tab
	{
	public:
		enum Id
		{
			Id_NetworkListView = 450,
			Id_ShowVirtualInterfacesCheckBox
		};

		void Create(HWND owner);
		virtual void Initialize();
		virtual void Relayout(int w, int h) override;
		virtual void HandleDpiChange() override;

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		void UpdateInterfaceList();
	};

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
		Id_Tab = 500,
		Id_HelpButton
	};

	TabLog m_TabLog;
	TabSkins m_TabSkins;
	TabDisplay m_TabDisplay;
	TabNetwork m_TabNetwork;
	TabPlugins m_TabPlugins;

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogDebug* c_Dialog;
};

#endif
