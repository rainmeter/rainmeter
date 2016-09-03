/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ControlTemplate.h"

namespace ControlTemplate
{

void CreateControls(const Control* cts, UINT ctCount, HWND parent, HFONT font, GetStringFunc getString)
{
	for (UINT i = 0; i < ctCount; ++i)
	{
		const Control& ct = cts[i];

		WCHAR* text = ct.textId ? getString(ct.textId) : nullptr;

		RECT r = { ct.x, ct.y, ct.w, ct.h };
		MapDialogRect(parent, &r);

		HWND wnd = CreateWindowEx(
			ct.exStyle,
			ct.name,
			text,
			ct.style,
			r.left, r.top, r.right, r.bottom,
			parent,
			(HMENU)ct.id,
			nullptr,
			nullptr);
		SendMessage(wnd, WM_SETFONT, (WPARAM)font, FALSE);
	}
}

}  // namespace ControlTemplate
