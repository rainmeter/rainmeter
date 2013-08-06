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

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Measure* self = *(Measure**)selfData;

static int GetName(lua_State* L)
{
	DECLARE_SELF(L)
	LuaManager::PushWide(self->GetName());

	return 1;
}

static int GetOption(lua_State* L)
{
	DECLARE_SELF(L)
	MeterWindow* meterWindow = self->GetMeterWindow();
	ConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), LuaManager::ToWide(3).c_str());

	LuaManager::PushWide(strTmp);
	return 1;
}

static int GetNumberOption(lua_State* L)
{
	DECLARE_SELF(L)
	MeterWindow* meterWindow = self->GetMeterWindow();
	ConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(2);
	double value = parser.ReadFloat(self->GetName(), strTmp.c_str(), lua_tonumber(L, 3));

	lua_pushnumber(L, value);
	return 1;
}

static int Disable(lua_State* L)
{
	DECLARE_SELF(L)
	self->Disable();

	return 0;
}

static int Enable(lua_State* L)
{
	DECLARE_SELF(L)
	self->Enable();

	return 0;
}

static int GetValue(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetValue());

	return 1;
}

static int GetRelativeValue(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetRelativeValue());

	return 1;
}

static int GetValueRange(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetValueRange());

	return 1;
}

static int GetMinValue(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetMinValue());

	return 1;
}

static int GetMaxValue(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetMaxValue());

	return 1;
}

static int GetStringValue(lua_State* L)
{
	DECLARE_SELF(L)

	int top = lua_gettop(L);
	AUTOSCALE autoScale = (top > 1) ? (AUTOSCALE)(int)lua_tonumber(L, 2) : AUTOSCALE_OFF;
	double scale = (top > 2) ? lua_tonumber(L, 3) : 1.0;
	int decimals = (int)lua_tonumber(L, 4);
	bool percentual = lua_toboolean(L, 5);

	const WCHAR* val = self->GetStringOrFormattedValue(autoScale, scale, decimals, percentual);
	LuaManager::PushWide(val);

	return 1;
}

void LuaManager::RegisterMeasure(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "GetName", GetName },
		{ "GetOption", GetOption },
		{ "GetNumberOption", GetNumberOption },
		{ "Disable", Disable },
		{ "Enable", Enable },
		{ "GetValue", GetValue },
		{ "GetRelativeValue", GetRelativeValue },
		{ "GetValueRange", GetValueRange },
		{ "GetMinValue", GetMinValue },
		{ "GetMaxValue", GetMaxValue },
		{ "GetStringValue", GetStringValue },
		{ nullptr, nullptr }
	};

	luaL_register(L, "Measure", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
