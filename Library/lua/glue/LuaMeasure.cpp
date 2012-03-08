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

static int Measure_GetName(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	const WCHAR* val = (const WCHAR*)self->GetName();
	LuaManager::PushWide(L, val);

	return 1;
}

static int Measure_GetOption(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), L"");

	LuaManager::PushWide(L, strTmp.c_str());
	return 1;
}

static int Measure_ReadString(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();
	
	std::wstring strTmp = LuaManager::ToWide(L, 2);
	strTmp = parser.ReadString(self->GetName(), strTmp.c_str(), LuaManager::ToWide(L, 3).c_str());

	LuaManager::PushWide(L, strTmp.c_str());
	return 1;
}

static int Measure_ReadNumber(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	double value = parser.ReadFloat(self->GetName(), strTmp.c_str(), tolua_tonumber(L, 3, 0));

	lua_pushnumber(L, value);
	return 1;
}

static int Measure_ReadFormula(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	std::wstring strTmp = LuaManager::ToWide(L, 2);
	double value = parser.ReadFormula(self->GetName(), strTmp.c_str(), tolua_tonumber(L, 3, 0));
	
	lua_pushnumber(L, value);
	return 1;
}

static int Measure_Disable(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	self->Disable();

	return 0;
}

static int Measure_Enable(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	self->Enable();

	return 0;
}

static int Measure_GetValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetValue();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetRelativeValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetRelativeValue();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetValueRange(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetValueRange();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetMinValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetMinValue();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetMaxValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetMaxValue();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetStringValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	AUTOSCALE autoScale = ((AUTOSCALE)(int)tolua_tonumber(L, 2, AUTOSCALE_OFF));
	double scale = (double)tolua_tonumber(L, 3, 1.0);
	int decimals = (int)tolua_tonumber(L, 4, 0);
	bool percentual = (bool)tolua_toboolean(L, 5, false);

	const WCHAR* val = self->GetStringValue(autoScale, scale, decimals, percentual);
	LuaManager::PushWide(L, val);

	return 1;
}

void LuaManager::RegisterMeasure(lua_State* L)
{
	tolua_usertype(L, "CMeasure");
	tolua_cclass(L, "CMeasure", "CMeasure", "", NULL);

	tolua_beginmodule(L, "CMeasure");
	tolua_function(L, "GetName", Measure_GetName);
	tolua_function(L, "GetOption", Measure_GetOption);
	tolua_function(L, "ReadString", Measure_ReadString);
	tolua_function(L, "ReadNumber", Measure_ReadNumber);
	tolua_function(L, "ReadFormula", Measure_ReadFormula);
	tolua_function(L, "Disable", Measure_Disable);
	tolua_function(L, "Enable", Measure_Enable);
	tolua_function(L, "GetValue", Measure_GetValue);
	tolua_function(L, "GetRelativeValue", Measure_GetRelativeValue);
	tolua_function(L, "GetValueRange", Measure_GetValueRange);
	tolua_function(L, "GetMinValue", Measure_GetMinValue);
	tolua_function(L, "GetMaxValue", Measure_GetMaxValue);
	tolua_function(L, "GetStringValue", Measure_GetStringValue);
	tolua_endmodule(L);
}
