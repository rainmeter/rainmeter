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
#include "../../Meter.h"
#include "../../MeterString.h"

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Meter* self = *(Meter**)selfData;

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
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), L"");

	LuaManager::PushWide(strTmp);
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
	bool abs = (bool)lua_toboolean(L, 2);
	lua_pushnumber(L, self->GetX(abs));

	return 1;
}

static int GetY(lua_State* L)
{
	DECLARE_SELF(L)
	bool abs = (bool)lua_toboolean(L, 2);
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
		std::wstring str = LuaManager::ToWide(2);
		string->SetText(str.c_str());
	}

	return 0;
}

void LuaManager::RegisterMeter(lua_State* L)
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
