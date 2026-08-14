// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "LuaBindingSection.h"
#include "LuaHelper.h"
#include "LuaScript.h"
#include "Meter.h"
#include "MeterString.h"
#include "MeterStringEdit.h"

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
	return LuaSection::GetOption(L, self, true);
}

static int SetOption(lua_State* L)
{
	DECLARE_SELF(L)
	return LuaSection::SetOption(L, self);
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
	if (self->GetTypeID() == TypeID<MeterString>() || self->GetTypeID() == TypeID<MeterStringEdit>())
	{
		MeterStringBase* text = (MeterStringBase*)self;
		std::wstring str = LuaHelper::ToWide(2);
		text->SetText(str);
	}

	return 0;
}

void LuaScript::RegisterMeter(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "GetName", GetName },
		{ "GetOption", GetOption },
		{ "SetOption", SetOption },
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
