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

#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../MeterString.h"

static int MeterString_GetX(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	bool abs = (bool)tolua_toboolean(L, 2, false);
	int val = self->GetX(abs);
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterString_Update(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	bool val = self->Update();
	lua_pushboolean(L, val);

	return 1;
}

static int MeterString_SetText(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	std::wstring str = LuaManager::ToWide(L, 2);
	self->SetText(str.c_str());

	return 0;
}

void LuaManager::RegisterMeterString(lua_State* L)
{
	tolua_usertype(L, "CMeterString");
	tolua_cclass(L, "CMeterString", "CMeterString", "CMeter", NULL);

	tolua_beginmodule(L, "CMeterString");
	tolua_function(L, "GetX", MeterString_GetX);
	tolua_function(L, "Update", MeterString_Update);
	tolua_function(L, "SetText", MeterString_SetText);
	tolua_endmodule(L);
}
