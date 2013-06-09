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
	LuaManager::PushWide(L, self->GetName());

	return 1;
}

static int GetOption(lua_State* L)
{
	DECLARE_SELF(L)
	MeterWindow* meterWindow = self->GetMeterWindow();
	ConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), LuaManager::ToWide(L, 3).c_str());

	LuaManager::PushWide(L, strTmp);
	return 1;
}

static int GetNumberOption(lua_State* L)
{
	DECLARE_SELF(L)
	MeterWindow* meterWindow = self->GetMeterWindow();
	ConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
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

	AUTOSCALE autoScale = AUTOSCALE_OFF;
	double scale = 1.0;
	int decimals = 0;
	bool percentual = false;

	const int top = lua_gettop(L);
	if (top == 2 && lua_istable(L, 2))
	{
		lua_pushvalue(L, 2);
		lua_pushnil(L);

		// Stack: nil, table, ...
		while (lua_next(L, -2))
		{
			// Stack: value, key, table, ...
			// Push copy of key to avoid changing orignal key.
			lua_pushvalue(L, -2);

			// Stack: key, value, key, table, ...
			const char* key = lua_tostring(L, -1);
			if (strcmp(key, "AutoScale") == 0)
			{
				// TODO: Implement something more reliable than casting.
				autoScale = (AUTOSCALE)(int)lua_tonumber(L, -2);
			}
			else if (strcmp(key, "Scale") == 0)
			{
				scale = lua_tonumber(L, -2);
			}
			else if (strcmp(key, "NumOfDecimals") == 0)
			{
				decimals = (int)lua_tonumber(L, -2);
			}
			else if (strcmp(key, "Percentual") == 0)
			{
				percentual = lua_toboolean(L, -2);
			}

			lua_pop(L, 2);
			// Stack: key, table, ...
		}
		// Stack: table, ...

		lua_pop(L, 1);
	}
	else
	{
		// For backwards compatibility.
		if (top > 1) autoScale = (AUTOSCALE)(int)lua_tonumber(L, 2);
		if (top > 2) scale = lua_tonumber(L, 3);
		if (top > 3) decimals = (int)lua_tonumber(L, 4);
		if (top > 4) percentual = lua_toboolean(L, 5);
	}

	const WCHAR* val = self->GetStringOrFormattedValue(autoScale, scale, decimals, percentual);
	LuaManager::PushWide(L, val);

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
