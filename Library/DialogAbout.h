// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Common/Dialog.h"

class DialogAbout : public Dialog
{
public:
	DialogAbout();
	virtual ~DialogAbout();

	DialogAbout(const DialogAbout& other) = delete;
	DialogAbout& operator=(DialogAbout other) = delete;

	static void Open();
	static void CloseDialog() { if (c_Dialog) c_Dialog->HandleMessage(WM_CLOSE, 0, 0); }

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

private:
	enum Id
	{
		Id_AppIcon = 400,
		Id_VersionLabel,
		Id_BuildLink,
		Id_HomeLink,
		Id_LicenseLink,
		Id_WinVerLabel,
		Id_PathLink,
		Id_SkinPathLink,
		Id_SettingsPathLink,
		Id_IniFileLink,
		Id_CopyButton,
		Id_CloseButton = IDCLOSE
	};

	void Initialize();
	void Relayout(int w, int h);

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogAbout* c_Dialog;
};
