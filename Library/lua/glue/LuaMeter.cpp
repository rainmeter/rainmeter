/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaHelper.h"
#include "../LuaScript.h"
#include "../../Meter.h"
#include "../../MeterString.h"

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Meter* self = *(Meter**)selfData;

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
	const WCHAR* name = self->GetName();

	const std::wstring section = LuaHelper::ToWide(2);
	const std::wstring defValue = LuaHelper::ToWide(3);

	const std::wstring& style = parser.ReadString(name, L"MeterStyle", L"");
	if (!style.empty())
	{
		parser.SetStyleTemplate(style);
	}

	const std::wstring& value =
		parser.ReadString(name, section.c_str(), defValue.c_str());
	LuaHelper::PushWide(value);

	parser.ClearStyleTemplate();

	return 1;
}

static int GetW(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetW());

	return 1;
}

static int GetH(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetH());

	return 1;
}

static int GetX(lua_State* L)
{
	DECLARE_SELF(L)
	const bool abs = lua_toboolean(L, 2) != 0;
	lua_pushnumber(L, self->GetX(abs));

	return 1;
}

static int GetY(lua_State* L)
{
	DECLARE_SELF(L)
	const bool abs = lua_toboolean(L, 2) != 0;
	lua_pushnumber(L, self->GetY(abs));

	return 1;
}

static int SetW(lua_State* L)
{
	DECLARE_SELF(L)
	int w = (int)lua_tonumber(L, 2);
	self->SetW(w);

	return 0;
}

static int SetH(lua_State* L)
{
	DECLARE_SELF(L)
	int h = (int)lua_tonumber(L, 2);
	self->SetH(h);

	return 0;
}

static int SetX(lua_State* L)
{
	DECLARE_SELF(L)
	int x = (int)lua_tonumber(L, 2);
	self->SetX(x);

	return 0;
}

static int SetY(lua_State* L)
{
	DECLARE_SELF(L)
	int y = (int)lua_tonumber(L, 2);
	self->SetY(y);

	return 0;
}

static int Hide(lua_State* L)
{
	DECLARE_SELF(L)
	self->Hide();

	return 0;
}

static int Show(lua_State* L)
{
	DECLARE_SELF(L)
	self->Show();

	return 0;
}

static int SetText(lua_State* L)
{
	DECLARE_SELF(L)
	if (self->GetTypeID() == TypeID<MeterString>())
	{
		MeterString* string = (MeterString*)self;
		std::wstring str = LuaHelper::ToWide(2);
		string->SetText(str.c_str());
	}

	return 0;
}

void LuaScript::RegisterMeter(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "GetName", GetName },
		{ "GetOption", GetOption },
		{ "GetW", GetW },
		{ "GetH", GetH },
		{ "GetX", GetX },
		{ "GetY", GetY },
		{ "SetW", SetW },
		{ "SetH", SetH },
		{ "SetX", SetX },
		{ "SetY", SetY },
		{ "Hide", Hide },
		{ "Show", Show },
		{ "SetText", SetText },
		{ nullptr, nullptr }
	};

	luaL_register(L, "Meter", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
