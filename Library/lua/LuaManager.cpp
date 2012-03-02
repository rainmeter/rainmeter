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
		c_State = lua_open();

		// Load Lua base libraries
		luaL_openlibs(c_State);

		// Initialize tolua
		tolua_open(c_State);

		// Register custom types and functions
		tolua_module(c_State, NULL, 0);
		tolua_beginmodule(c_State, NULL);
		RegisterGlobal(c_State);
		RegisterMeasure(c_State);
		RegisterMeasure(c_State);
		RegisterMeter(c_State);
		RegisterMeterWindow(c_State);
		RegisterMeterString(c_State);
		tolua_endmodule(c_State);
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
	std::string error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Get rid of everything up to the filename
	std::string::size_type pos = 4; // Skip the drive
	pos = error.find_first_of(':', pos);
	pos = error.find_last_of('\\', pos);
	if (pos != std::string::npos)
	{
		error.erase(0, ++pos);
	}

	std::wstring str = L"Script: ";
	str += ConvertToWide(error.c_str());
	Log(LOG_ERROR, str.c_str());
}

void LuaManager::PushWide(lua_State* L, const WCHAR* str)
{
	lua_pushstring(L, ConvertToAscii(str).c_str());
}

std::wstring LuaManager::ToWide(lua_State* L, int narg)
{
	return ConvertToWide(lua_tostring(L, narg));
}
