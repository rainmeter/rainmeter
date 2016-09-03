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
	if (LuaScript::GetActiveScript()) {
		LogDebug(LuaScript::GetActiveScript()->IsUnicode() ?
			StringUtil::WidenUTF8(message).c_str() : StringUtil::Widen(message).c_str());
	}
	else
	{
		// Could not find active script. Just assume that it is UTF-8
		LogDebug(StringUtil::Widen(message).c_str());
	}
	return 0;
}

static int Dofile(lua_State* L)
{
	const char *fname = luaL_optstring(L, 1, NULL);
	int n = lua_gettop(L);
	std::wstring path;

	if (LuaScript::GetActiveScript()) {
		path = LuaScript::GetActiveScript()->IsUnicode() ?
			StringUtil::WidenUTF8(fname) : StringUtil::Widen(fname);
	}
	else
	{
		path = StringUtil::Widen(fname);
		LogWarning(L"Could not find active script to determine if it's unicode. Will proceed with assuming that it's not!");
	}

	LuaHelper::RunFile(L, path, n);

	return 0;
}

static const std::wstring findfile(lua_State *L, const WCHAR *name,
	const char *pname) {
	const char *path;
	std::wstring wname = name;
	size_t start_pos = wname.find(L".");
	if (start_pos != std::string::npos)
		wname.replace(start_pos, 1, L"\\");
	if(LuaScript::GetActiveScript()){
		path = lua_tostring(L, -1);
		std::wstring path = LuaScript::GetActiveScript()->GetResourceFolder();
		path.append(wname);
		path.append(L".lua");
		return path;
	}
	return std::wstring();  /* not found */
}

static int packageLoader(lua_State* L)
{
	const char *name = luaL_checkstring(L, 1);
	std::wstring wName;
	if (LuaScript::GetActiveScript()) {
		wName = LuaScript::GetActiveScript()->IsUnicode() ?
			StringUtil::WidenUTF8(name) : StringUtil::Widen(name);
	}
	else
	{
		wName = StringUtil::Widen(name);
		LogWarning(L"Could not find active script to determine if it's unicode. Will proceed with assuming that it's not!");
	}

	int n = lua_gettop(L);
	

	std::wstring path = findfile(L, wName.c_str(), "path");
	if (path.empty()) {
		return 0;  /* library not found in this path */
	}
	LuaHelper::LoadFile(L, path);
	return 1;  /* library loaded successfully */
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

	luaL_loadstring(L, "package.loaders[2] = ... ");
	lua_pushcfunction(L, packageLoader);
	lua_call(L, 1, 0);

	// Register tolua.cast() for backwards compatibility.
	const luaL_Reg toluaFuncs[] =
	{
		{ "cast", tolua_cast },
		{ nullptr, nullptr }
	};

	luaL_register(L, "tolua", toluaFuncs);
}
