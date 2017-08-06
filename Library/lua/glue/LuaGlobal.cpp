/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaHelper.h"
#include "../LuaScript.h"
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

	LogDebug(LuaHelper::GetCurrentScript()->IsUnicode() ?
		StringUtil::WidenUTF8(message).c_str() : StringUtil::Widen(message).c_str());

	return 0;
}

static int Dofile(lua_State* L)
{
	//Modified version of luaB_dofile()
	auto curScript = LuaHelper::GetCurrentScript();
	const char* fname = luaL_optstring(L, 1, NULL);
	std::wstring path = curScript->IsUnicode() ? StringUtil::WidenUTF8(fname) : StringUtil::Widen(fname);

	int n = lua_gettop(L);

	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(path, &fileSize);
	if (!fileData)
	{
		LogErrorF(L"Script: Unable to open the file: %s", path.c_str());
		return 0;
	}

	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	bool unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (unicode)
	{
		const std::string utf8Data =
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (int)((fileSize - 2) / sizeof(WCHAR)));
		scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), "") == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(L, (char*)fileData.get(), fileSize, "") == 0;
	}

	if (scriptLoaded)
	{
		auto script = LuaHelper::GetState(L, unicode, curScript->GetRef(), path);
		lua_rawgeti(L, LUA_GLOBALSINDEX, script.GetRef());
		lua_setfenv(L, -2);

		int result = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (result == 0)
		{
			return lua_gettop(L) - n;
		}
		else
		{
			LuaHelper::ReportErrors(path);
		}
	}
	else
	{
		LuaHelper::ReportErrors(path);
	}

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
	lua_register(L, "dofile", Dofile);

	// Register tolua.cast() for backwards compatibility.
	const luaL_Reg toluaFuncs[] =
	{
		{ "cast", tolua_cast },
		{ nullptr, nullptr }
	};

	luaL_register(L, "tolua", toluaFuncs);
}
