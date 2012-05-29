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

		// Initialize standard libraries except debug, modified from linit.c
		const luaL_Reg lualibs[] =
		{
			{ "", luaopen_base },
			{ LUA_LOADLIBNAME, luaopen_package },
			{ LUA_TABLIBNAME, luaopen_table },
			{ LUA_IOLIBNAME, luaopen_io },
			{ LUA_OSLIBNAME, luaopen_os },
			{ LUA_STRLIBNAME, luaopen_string },
			{ LUA_MATHLIBNAME, luaopen_math },
			{ NULL, NULL }
		};

		for (const luaL_Reg* lib = lualibs; lib->func; ++lib)
		{
			lua_pushcfunction(c_State, lib->func);
			lua_pushstring(c_State, lib->name);
			lua_call(c_State, 1, 0);
		}

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

	const char* pos = error + 4; // Skip the drive

	// Get rid of everything up to the filename
	while (*pos != ':')
	{
		if (*pos == '\\')
		{
			error = pos + 1;
		}
		++pos;
	}

	LogWithArgs(LOG_ERROR, L"Script: %S", error);
}

void LuaManager::PushWide(lua_State* L, const WCHAR* str)
{
	lua_pushstring(L, ConvertToAscii(str).c_str());
}

std::wstring LuaManager::ToWide(lua_State* L, int narg)
{
	return ConvertToWide(lua_tostring(L, narg));
}
