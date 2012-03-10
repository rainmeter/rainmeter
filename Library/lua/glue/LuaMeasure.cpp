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
#include "../../Measure.h"
#include "../../MeterWindow.h"

inline CMeasure* GetSelf(lua_State* L)
{
	return *(CMeasure**)lua_touserdata(L, 1);
}

static int GetName(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	LuaManager::PushWide(L, self->GetName());

	return 1;
}

static int GetOption(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), L"");

	LuaManager::PushWide(L, strTmp.c_str());
	return 1;
}

static int ReadString(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();
	
	std::wstring strTmp = LuaManager::ToWide(L, 2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), LuaManager::ToWide(L, 3).c_str());

	LuaManager::PushWide(L, strTmp.c_str());
	return 1;
}

static int ReadNumber(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	double value = parser.ReadFormula(self->GetName(), strTmp.c_str(), lua_tonumber(L, 3));

	lua_pushnumber(L, value);
	return 1;
}

static int Disable(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	self->Disable();

	return 0;
}

static int Enable(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	self->Enable();

	return 0;
}

static int GetValue(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	lua_pushnumber(L, self->GetValue());

	return 1;
}

static int GetRelativeValue(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	lua_pushnumber(L, self->GetRelativeValue());

	return 1;
}

static int GetValueRange(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	lua_pushnumber(L, self->GetValueRange());

	return 1;
}

static int GetMinValue(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	lua_pushnumber(L, self->GetMinValue());

	return 1;
}

static int GetMaxValue(lua_State* L)
{
	CMeasure* self = GetSelf(L);
	lua_pushnumber(L, self->GetMaxValue());

	return 1;
}

static int GetStringValue(lua_State* L)
{
	CMeasure* self = GetSelf(L);

	int top = lua_gettop(L);
	AUTOSCALE autoScale = (top > 1) ? (AUTOSCALE)(int)lua_tonumber(L, 2) : AUTOSCALE_OFF;
	double scale = (top > 2) ? lua_tonumber(L, 3) : 1.0;
	int decimals = (int)lua_tonumber(L, 4);
	bool percentual = lua_toboolean(L, 5);

	const WCHAR* val = self->GetStringValue(autoScale, scale, decimals, percentual);
	LuaManager::PushWide(L, val);

	return 1;
}

void LuaManager::RegisterMeasure(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "GetName", GetName },
		{ "GetOption", GetOption },
		{ "ReadString", ReadString },
		{ "ReadNumber", ReadNumber },
		{ "Disable", Disable },
		{ "Enable", Enable },
		{ "GetValue", GetValue },
		{ "GetRelativeValue", GetRelativeValue },
		{ "GetValueRange", GetValueRange },
		{ "GetMinValue", GetMinValue },
		{ "GetMaxValue", GetMaxValue },
		{ "GetStringValue", GetStringValue },
		{ NULL, NULL }
	};

	luaL_register(L, "CMeasure", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
