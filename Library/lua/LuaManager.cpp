/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../../Common/StringUtil.h"
#include "LuaManager.h"
#include "../Logger.h"
#include "LuaScript.h"

int LuaManager::c_RefCount = 0;
lua_State* LuaManager::c_State = 0;

std::vector<bool> LuaManager::c_UnicodeStateStack;
/*
void LuaManager::Initialize()
{
	if (c_State == nullptr)
	{
		// Initialize Lua
		c_State = luaL_newstate();

		luaL_openlibs(c_State);

		// Register custom types and functions
		//RegisterGlobal(c_State);
		//RegisterMeasure(c_State);
		//RegisterMeter(c_State);
		//RegisterSkin(c_State);
	}

	++c_RefCount;
}

void LuaManager::Finalize()
{
	if (c_RefCount > 0)
	{
		--c_RefCount;
	}

	if (c_RefCount == 0 && c_State != nullptr)
	{
		lua_close(c_State);
		c_State = nullptr;
	}
}
*/
void LuaManager::ReportErrors(lua_State* L, const std::wstring& file)
{
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Skip "[string ...]".
	const char* pos = strchr(error, ':');
	if (pos)
	{
		error = pos;
	}

	std::wstring str(file, file.find_last_of(L'\\') + 1);
	//TODO: Check if unicode
	str += true ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaManager::PushWide(lua_State* L, const WCHAR* str)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		//TODO: Check if unicode
		const std::string narrowStr = activeScript->IsUnicode() ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
		lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
	}
	else
	{
		LogDebug(L"No active script, could not determine if file is unicode. Could not push wide string to stack!");
	}
}

void LuaManager::PushWide(lua_State* L, const std::wstring& str)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		//TODO: Check if unicode
		const std::string narrowStr = activeScript->IsUnicode() ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
		lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
	}
	else
	{
		LogDebug(L"No active script, could not determine if file is unicode. Could not push wide string to stack!");
	}
}

std::wstring LuaManager::ToWide(lua_State* L,  int narg)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		size_t strLen = 0;
		const char* str = lua_tolstring(L, narg, &strLen);
		//TODO: Check if unicode
		bool unicode = activeScript->IsUnicode();
		return unicode ?
			StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
	}
	else
	{
		LogDebug(L"No active script, could not get wide string from stack!");
	}
}
