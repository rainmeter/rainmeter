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
#include "../../Rainmeter.h"
#include "../../MeterWindow.h"
#include "../../MeterString.h"

extern CRainmeter* Rainmeter;

inline CMeterWindow* GetSelf(lua_State* L)
{
	return *(CMeterWindow**)lua_touserdata(L, 1);
}

static int Bang(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	CConfigParser& parser = self->GetParser();

	std::wstring bang = LuaManager::ToWide(L, 2);

	int top = lua_gettop(L);
	if (top == 2)	// 1 argument
	{
		parser.ReplaceVariables(bang);
		Rainmeter->ExecuteCommand(bang.c_str(), self);
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
				std::wstring tmpSz = LuaManager::ToWide(L, i);
				parser.ReplaceVariables(tmpSz);
				args.push_back(tmpSz);
			}

			Rainmeter->ExecuteBang(bangSz, args, self);
		}
	}

	return 0;
}

static int GetMeter(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	const std::wstring meterName = LuaManager::ToWide(L, 2);

	CMeter* meter = self->GetMeter(meterName);
	if (meter)
	{
		*(CMeter**)lua_newuserdata(L, sizeof(CMeter*)) = meter;
		lua_getglobal(L, "CMeter");
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
	CMeterWindow* self = GetSelf(L);
	const std::wstring measureName = LuaManager::ToWide(L, 2);

	CMeasure* measure = self->GetMeasure(measureName);
	if (measure)
	{
		*(CMeasure**)lua_newuserdata(L, sizeof(CMeasure*)) = measure;
		lua_getglobal(L, "CMeasure");
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
	CMeterWindow* self = GetSelf(L);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	const std::wstring* value = self->GetParser().GetVariable(strTmp);
	if (value)
	{
		LuaManager::PushWide(L, (*value).c_str());
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int ReplaceVariables(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	self->GetParser().ReplaceVariables(strTmp);
	self->GetParser().ReplaceMeasures(strTmp);
	LuaManager::PushWide(L, strTmp.c_str());

	return 1;
}

static int ParseFormula(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

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
	CMeterWindow* self = GetSelf(L);
	int x = (int)lua_tonumber(L, 2);
	int y = (int)lua_tonumber(L, 3);
	self->MoveWindow(x, y);

	return 0;
}

static int FadeWindow(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	int from = (int)lua_tonumber(L, 2);
	int to = (int)lua_tonumber(L, 3);
	self->FadeWindow(from, to);

	return 0;
}

static int GetW(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	lua_pushnumber(L, self->GetW());

	return 1;
}

static int GetH(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	lua_pushnumber(L, self->GetH());

	return 1;
}

static int GetX(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	lua_pushnumber(L, self->GetX());

	return 1;
}

static int GetY(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	lua_pushnumber(L, self->GetY());

	return 1;
}

static int MakePathAbsolute(lua_State* L)
{
	CMeterWindow* self = GetSelf(L);
	std::wstring path = LuaManager::ToWide(L, 2);
	self->MakePathAbsolute(path);
	LuaManager::PushWide(L, path.c_str());

	return 1;
}

void LuaManager::RegisterMeterWindow(lua_State* L)
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
		{ NULL, NULL }
	};

	luaL_register(L, "CMeterWindow", functions);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
}
