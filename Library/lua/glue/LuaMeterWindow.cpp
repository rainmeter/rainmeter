/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../LuaHelper.h"
#include "../LuaScript.h"
#include "../../Rainmeter.h"
#include "../../Skin.h"
#include "../../MeterString.h"

#define DECLARE_SELF(L) \
	void* selfData = lua_touserdata(L, 1); \
	if (!selfData) return 0; \
	Skin* self = *(Skin**)selfData;

static int Bang(lua_State* L)
{
	DECLARE_SELF(L)
	ConfigParser& parser = self->GetParser();

	std::wstring bang = LuaHelper::ToWide(2);

	int top = lua_gettop(L);
	if (top == 2)	// 1 argument
	{
		parser.ReplaceVariables(bang);
		GetRainmeter().ExecuteCommand(bang.c_str(), self);
	}
	else
	{
		const WCHAR* bangSz = bang.c_str();
		if (*bangSz == L'!')
		{
			++bangSz;	// Skip "!"
			std::vector<std::wstring> args;
			for (int i = 3; i <= top; ++i)
			{
				std::wstring tmpSz = LuaHelper::ToWide(i);
				parser.ReplaceVariables(tmpSz);
				args.push_back(tmpSz);
			}

			GetRainmeter().ExecuteBang(bangSz, args, self);
		}
	}

	return 0;
}

static int GetMeter(lua_State* L)
{
	DECLARE_SELF(L)
	const std::wstring meterName = LuaHelper::ToWide(2);

	Meter* meter = self->GetMeter(meterName);
	if (meter)
	{
		*(Meter**)lua_newuserdata(L, sizeof(Meter*)) = meter;
		lua_getglobal(L, "Meter");
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int GetMeasure(lua_State* L)
{
	DECLARE_SELF(L)
	const std::wstring measureName = LuaHelper::ToWide(2);

	Measure* measure = self->GetMeasure(measureName);
	if (measure)
	{
		*(Measure**)lua_newuserdata(L, sizeof(Measure*)) = measure;
		lua_getglobal(L, "Measure");
		lua_setmetatable(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int GetVariable(lua_State* L)
{
	DECLARE_SELF(L)

	const std::wstring name = LuaHelper::ToWide(2);
	const std::wstring* value = self->GetParser().GetVariable(name);
	if (value)
	{
		LuaHelper::PushWide(*value);
	}
	else if (lua_gettop(L) >= 3)
	{
		lua_pushvalue(L, 3);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int ReplaceVariables(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring strTmp = LuaHelper::ToWide(2);

	self->GetParser().ReplaceVariables(strTmp);
	self->GetParser().ReplaceMeasures(strTmp);
	LuaHelper::PushWide(strTmp);

	return 1;
}

static int ParseFormula(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring strTmp = LuaHelper::ToWide(2);

	double result;
	if (!self->GetParser().ParseFormula(strTmp, &result))
	{
		result = lua_tonumber(L, 2);
	}

	lua_pushnumber(L, result);

	return 1;
}

static int MoveWindow(lua_State* L)
{
	DECLARE_SELF(L)
	int x = (int)lua_tonumber(L, 2);
	int y = (int)lua_tonumber(L, 3);
	self->MoveWindow(x, y);

	return 0;
}

static int FadeWindow(lua_State* L)
{
	DECLARE_SELF(L)
	int from = (int)lua_tonumber(L, 2);
	int to = (int)lua_tonumber(L, 3);
	self->FadeWindow(from, to);

	return 0;
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
	lua_pushnumber(L, self->GetX());

	return 1;
}

static int GetY(lua_State* L)
{
	DECLARE_SELF(L)
	lua_pushnumber(L, self->GetY());

	return 1;
}

static int MakePathAbsolute(lua_State* L)
{
	DECLARE_SELF(L)
	std::wstring path = LuaHelper::ToWide(2);
	self->MakePathAbsolute(path);
	LuaHelper::PushWide(path);

	return 1;
}

void LuaScript::RegisterSkin(lua_State* L)
{
	const luaL_Reg functions[] =
	{
		{ "Bang", Bang },
		{ "GetMeter", GetMeter },
		{ "GetMeasure", GetMeasure },
		{ "GetVariable", GetVariable },
		{ "ReplaceVariables", ReplaceVariables },
		{ "ParseFormula", ParseFormula },
		{ "MoveWindow", MoveWindow },
		{ "FadeWindow", FadeWindow },
		{ "GetW", GetW },
		{ "GetH", GetH },
		{ "GetX", GetX },
		{ "GetY", GetY },
		{ "MakePathAbsolute", MakePathAbsolute },
		{ nullptr, nullptr }
	};

	luaL_register(L, "MeterWindow", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
