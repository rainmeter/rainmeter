/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_DIALOGABOUT_H_
#define RM_LIBRARY_DIALOGABOUT_H_

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

#endif
