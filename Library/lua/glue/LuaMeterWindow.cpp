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

static int MeterWindow_Bang(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	CConfigParser& parser = self->GetParser();
	parser.ReplaceVariables(strTmp);
	self->GetMainObject()->ExecuteCommand(strTmp.c_str(), self);

	return 0;
}

static int MeterWindow_GetMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring meterName = LuaManager::ToWide(L, 2);

	CMeter* meter = self->GetMeter(meterName);
	if (!meter)
	{
		std::wstring error = L"Script: No such meter as ";
		error += meterName;
		Log(LOG_ERROR, error.c_str());
		return 0;
	}

	tolua_pushusertype(L, meter, "CMeter");

	return 1;
}

static int MeterWindow_GetMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring measureName = LuaManager::ToWide(L, 2);

	CMeasure* val = self->GetMeasure(measureName);
	tolua_pushusertype(L, (void*)val, "CMeasure");

	return 1;
}

static int MeterWindow_GetVariable(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	if (self->GetParser().GetVariable(strTmp, strTmp))
	{
		LuaManager::PushWide(L, strTmp.c_str());
		return 1;
	}
	else
	{
		return 0;
	}
}

static int MeterWindow_ReplaceVariables(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	self->GetParser().ReplaceVariables(strTmp);
	LuaManager::PushWide(L, strTmp.c_str());

	return 1;
}

static int MeterWindow_ParseFormula(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	std::wstring strTmp = LuaManager::ToWide(L, 2);

	double result;
	if (self->GetParser().ParseFormula(strTmp, &result))
	{
		lua_pushnumber(L, result);
		return 1;
	}

	return 0;
}

static int MeterWindow_MoveWindow(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int x = (int)tolua_tonumber(L, 2, 0);
	int y = (int)tolua_tonumber(L, 3, 0);

	self->MoveWindow(x, y);

	return 0;
}

static int MeterWindow_FadeWindow(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int from = (int)tolua_tonumber(L, 2, 0);
	int to = (int)tolua_tonumber(L, 3, 0);
	self->FadeWindow(from, to);

	return 0;
}

static int MeterWindow_GetW(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetW();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetH(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetH();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetX(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetX();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetY(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetY();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_MakePathAbsolute(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	std::wstring path = LuaManager::ToWide(L, 2);

	self->MakePathAbsolute(path);
	LuaManager::PushWide(L, path.c_str());

	return 1;
}

void LuaManager::RegisterMeterWindow(lua_State* L)
{
	tolua_usertype(L, "CMeterWindow");
	tolua_cclass(L, "CMeterWindow", "CMeterWindow", "", NULL);

	tolua_beginmodule(L, "CMeterWindow");
	tolua_function(L, "Bang", MeterWindow_Bang);
	tolua_function(L, "GetMeter", MeterWindow_GetMeter);
	tolua_function(L, "GetMeasure", MeterWindow_GetMeasure);
	tolua_function(L, "GetVariable", MeterWindow_GetVariable);
	tolua_function(L, "ReplaceVariables", MeterWindow_ReplaceVariables);
	tolua_function(L, "ParseFormula", MeterWindow_ParseFormula);
	tolua_function(L, "MoveWindow", MeterWindow_MoveWindow);
	tolua_function(L, "FadeWindow", MeterWindow_FadeWindow);
	tolua_function(L, "GetW", MeterWindow_GetW);
	tolua_function(L, "GetH", MeterWindow_GetH);
	tolua_function(L, "GetX", MeterWindow_GetX);
	tolua_function(L, "GetY", MeterWindow_GetY);
	tolua_function(L, "MakePathAbsolute", MeterWindow_MakePathAbsolute);
	tolua_endmodule(L);
}
