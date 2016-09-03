/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaManager.h"
#include "../../Logger.h"
#include "../../../Common/StringUtil.h"
#include "../../../Common/FileUtil.h"

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

	LogDebug(LuaManager::IsUnicodeState() ?
		StringUtil::WidenUTF8(message).c_str() : StringUtil::Widen(message).c_str());
	return 0;
}

static int DoFile(lua_State* L)
{
	int ref = LuaManager::GetActiveScriptRef();
	if (ref >= 0) {
		// Modified version of luaB_dofile
		std::wstring fname = StringUtil::WidenUTF8(luaL_optstring(L, 1, NULL));
		int n = lua_gettop(L);

		size_t fileSize = 0;
		auto fileData = FileUtil::ReadFullFile(fname, &fileSize);
		if (!fileData)
		{
			return 0;
		}

		bool scriptLoaded = false;
		bool isUnicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
		if (isUnicode)
		{
			const std::string utf8Data =
				StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (fileSize - 2) / sizeof(WCHAR));
			scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), "") == 0;
		}
		else
		{
			scriptLoaded = luaL_loadbuffer(L, (char*)fileData.get(), fileSize, "") == 0;
		}

		if (scriptLoaded) {
			lua_rawgeti(L, LUA_GLOBALSINDEX, ref);

			// Set the environment for the function to be run in to be the table that
			// has been created for the script/
			lua_setfenv(L, -2);
			lua_call(L, 0, LUA_MULTRET);
			return lua_gettop(L) - n;
		}
		else
		{
			LuaManager::ReportErrors(fname);
			return 0;
		}
	}
	else
	{
		LogDebug(L"Could not execute dofile due to no active script being set!");
		return 0;
	}
}

static int tolua_cast(lua_State* L)
{
	// Simply push first argument onto stack.
	lua_pushvalue(L, 1);
	return 1;
}

void LuaManager::RegisterGlobal(lua_State* L)
{
	lua_register(L, "print", Print);

	lua_register(L, "dofile", DoFile);

	// Register tolua.cast() for backwards compatibility.
	const luaL_Reg toluaFuncs[] =
	{
		{ "cast", tolua_cast },
		{ nullptr, nullptr }
	};

	luaL_register(L, "tolua", toluaFuncs);
}
