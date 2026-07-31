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

int GetOption(lua_State* L, Section* section, bool allowMeterStyle)
{
	Skin* skin = section->GetSkin();
	ConfigParser& parser = skin->GetParser();
	const WCHAR* sectionName = section->GetName();
	const std::wstring key = LuaHelper::ToWide(2);
	const std::wstring defValue = LuaHelper::ToWide(3);

	bool replaceMeasures = true;
	if (lua_gettop(L) > 3)
	{
		replaceMeasures = LuaHelper::ToBool(4);
	}

	parser.ReadInheritOption(sectionName, allowMeterStyle);
	const auto& value = parser.ReadString(sectionName, key.c_str(), defValue.c_str(), replaceMeasures);
	parser.ClearInheritChain();

	LuaHelper::PushWide(value);

	return 1;
}

int SetOption(lua_State* L, Section* section)
{
	return SetOption(L, section->GetSkin(), section->GetName(), 2);
}

int SetOption(lua_State* L, Skin* skin, const wchar_t* sectionName, int optionIndex, bool group)
{
	if (lua_istable(L, optionIndex))
	{
		const int top = lua_gettop(L);
		for (int i = optionIndex; i <= top; ++i)
		{
			if (!lua_istable(L, i))
			{
				continue;
			}

			lua_rawgeti(L, i, 1);
			lua_rawgeti(L, i, 2);
			if (lua_type(L, -2) == LUA_TSTRING && lua_isstring(L, -1))
			{
				const std::wstring value = LuaHelper::ToWide(-1);
				const std::wstring option = LuaHelper::ToWide(-2);
				skin->SetOption(sectionName, option, value, group);
			}

			lua_pop(L, 2);
		}
	}
	else if (lua_isstring(L, optionIndex) && lua_isstring(L, optionIndex + 1))
	{
		const std::wstring option = LuaHelper::ToWide(optionIndex);
		const std::wstring value = LuaHelper::ToWide(optionIndex + 1);
		skin->SetOption(sectionName, option, value, group);
	}

	return 0;
}

}  // namespace LuaSection
