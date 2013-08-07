/*
  Copyright (C) 2010 Matt King, Birunthan Mohanathas

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

#include "../StdAfx.h"
#include "../../Common/StringUtil.h"
#include "LuaManager.h"
#include "../Logger.h"

int LuaManager::c_RefCount = 0;
lua_State* LuaManager::c_State = 0;

std::vector<bool> LuaManager::c_UnicodeStateStack;

void LuaManager::Initialize()
{
	if (c_State == nullptr)
	{
		// Initialize Lua
		c_State = luaL_newstate();

		luaL_openlibs(c_State);

		// Register custom types and functions
		RegisterGlobal(c_State);
		RegisterMeasure(c_State);
		RegisterMeter(c_State);
		RegisterMeterWindow(c_State);
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

void LuaManager::ReportErrors(const std::wstring& file)
{
	lua_State* L = c_State;
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Skip "[string ...]".
	const char* pos = strchr(error, ':');
	if (pos)
	{
		error = pos;
	}

	std::wstring str(file, file.find_last_of(L'\\') + 1);
	str += IsUnicodeState() ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaManager::PushWide(const WCHAR* str)
{
	lua_State* L = c_State;
	const std::string narrowStr = IsUnicodeState() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

void LuaManager::PushWide(const std::wstring& str)
{
	lua_State* L = c_State;
	const std::string narrowStr = IsUnicodeState() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

std::wstring LuaManager::ToWide(int narg)
{
	lua_State* L = c_State;
	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	return IsUnicodeState() ?
		StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
}
