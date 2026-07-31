/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "LuaSection.h"
#include "../LuaHelper.h"
#include "../../Section.h"
#include "../../Skin.h"

namespace LuaSection {

int SetOption(lua_State* L, Section* section)
{
	Skin* skin = section->GetSkin();
	const WCHAR* sectionName = section->GetName();

	if (lua_istable(L, 2))
	{
		lua_pushnil(L);
		while (lua_next(L, 2) != 0)
		{
			if (lua_type(L, -2) == LUA_TSTRING && lua_isstring(L, -1))
			{
				const std::wstring value = LuaHelper::ToWide(-1);
				const std::wstring option = LuaHelper::ToWide(-2);
				skin->SetOption(sectionName, option, value, false);
			}

			lua_pop(L, 1);
		}
	}
	else if (lua_isstring(L, 2) && lua_isstring(L, 3))
	{
		const std::wstring option = LuaHelper::ToWide(2);
		const std::wstring value = LuaHelper::ToWide(3);
		skin->SetOption(sectionName, option, value, false);
	}

	return 0;
}

}  // namespace LuaSection
