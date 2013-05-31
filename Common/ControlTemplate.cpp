/*
  Copyright (C) 2012 Birunthan Mohanathas

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