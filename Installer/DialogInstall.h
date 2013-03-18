/*
  Copyright (C) 2013 Birunthan Mohanathas

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

#ifndef RM_INSTALLER_DIALOGINSTALL_H_
#define RM_INSTALLER_DIALOGINSTALL_H_

#include "../Common/Dialog.h"

class CDialogInstall : public CDialog
{
public:
	CDialogInstall();
	virtual ~CDialogInstall();

	static CDialogInstall* CDialogInstall::Create();

	static void Open(int tab = 0);
	static void Open(const WCHAR* name);
	static void ShowAboutLog();

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	enum Id
	{
		Id_CloseButton = IDCLOSE
	};
};

#endif
