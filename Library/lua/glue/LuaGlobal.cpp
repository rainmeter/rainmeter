/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaScript.h"
#include "../../Logger.h"
#include "../../../Common/StringUtil.h"

static int Print(lua_State* L)
{
	// Modified version of luaB_print()
	std::string message;

	int n = lua_gettop(L);		// Number of arguments
	lua_getglobal(L, "tostring");

	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1);	// Function to be called
		lua_pushvalue(L, i);	// Value to print
		lua_call(L, 1, 1);

		// Get result
		const char* s = lua_tostring(L, -1);
		if (s == nullptr)
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}

		if (i > 1)
		{
			// About dialog List View doesn't like tabs, just use 4 spaces instead
			message += "    ";
		}

		message += s;

		// Pop result
		lua_pop(L, 1);
	}

	//TODO: Fix this
	LogDebug(/*LuaManager::IsUnicodeState()*/ true ?
		StringUtil::WidenUTF8(message).c_str() : StringUtil::Widen(message).c_str());
	return 0;
}

static int tolua_cast(lua_State* L)
{
	// Simply push first argument onto stack.
	lua_pushvalue(L, 1);
	return 1;
}

void LuaScript::RegisterGlobal(lua_State* L)
{
	lua_register(L, "print", Print);

	// Register tolua.cast() for backwards compatibility.
	const luaL_Reg toluaFuncs[] =
	{
		{ "cast", tolua_cast },
		{ nullptr, nullptr }
	};

	luaL_register(L, "tolua", toluaFuncs);
}
