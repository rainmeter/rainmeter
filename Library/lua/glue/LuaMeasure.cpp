/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaHelper.h"
#include "../LuaScript.h"
#include "../../Measure.h"
#include "../../Skin.h"

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Measure* self = *(Measure**)selfData;

static int GetName(lua_State* L)
{
	DECLARE_SELF(L)
	LuaHelper::PushWide(self->GetName());

	return 1;
}

static int GetOption(lua_State* L)
{
	DECLARE_SELF(L)
	Skin* skin = self->GetSkin();
	ConfigParser& parser = skin->GetParser();

	const std::wstring section = LuaHelper::ToWide(2);
	const std::wstring defValue = LuaHelper::ToWide(3);
	const std::wstring& value =
		parser.ReadString(self->GetName(), section.c_str(), defValue.c_str());
	LuaHelper::PushWide(value);
	return 1;
}

static int GetNumberOption(lua_State* L)
{
	DECLARE_SELF(L)
	Skin* skin = self->GetSkin();
	ConfigParser& parser = skin->GetParser();

	std::wstring strTmp = LuaHelper::ToWide(2);
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
	bool percentual = lua_toboolean(L, 5) != 0;

	const WCHAR* val = self->GetStringOrFormattedValue(autoScale, scale, decimals, percentual);
	LuaHelper::PushWide(val);

	return 1;
}

void LuaScript::RegisterMeasure(lua_State* L)
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
