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
#include "LuaManager.h"
#include "../Rainmeter.h"

int LuaManager::c_RefCount = 0;
lua_State* LuaManager::c_State = 0;

void LuaManager::Initialize()
{
	if (c_State == NULL)
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

	if (c_RefCount == 0 && c_State != NULL)
	{
		lua_close(c_State);
		c_State = NULL;
	}
}

void LuaManager::ReportErrors(lua_State* L)
{
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	LogWithArgs(LOG_ERROR, L"Script: %S", error);
}

void LuaManager::PushWide(lua_State* L, const WCHAR* str)
{
	lua_pushstring(L, ConvertToUTF8(str).c_str());
}

std::wstring LuaManager::ToWide(lua_State* L, int narg)
{
	return ConvertUTF8ToWide(lua_tostring(L, narg));
}
