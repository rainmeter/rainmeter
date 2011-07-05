#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Measure.h"

static int Measure_GetName(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	const WCHAR* val = (const WCHAR*)self->GetName();
	push_wchar(L, val);

	return 1;
}

static int Measure_GetANSIName(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	const char* val = (const char*)self->GetANSIName();
	tolua_pushstring(L, (const char*)val);

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

static int Measure_IsDisabled(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	bool val = self->IsDisabled();
	tolua_pushboolean(L, val);

	return 1;
}

static int Measure_HasDynamicVariables(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	bool val = self->HasDynamicVariables();
	tolua_pushboolean(L, val);

	return 1;
}

static int Measure_GetValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetValue();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetRelativeValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetRelativeValue();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetValueRange(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetValueRange();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetMinValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetMinValue();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Measure_GetMaxValue(lua_State* L)
{
	CMeasure* self = (CMeasure*)tolua_tousertype(L, 1, 0);
	double val = (double)self->GetMaxValue();
	tolua_pushnumber(L, (lua_Number)val);

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
	push_wchar(L, val);

	return 1;
}

void LuaManager::RegisterMeasure(lua_State* L)
{
	tolua_constant(L, "AUTOSCALE_1024", AUTOSCALE_1024);
	tolua_constant(L, "AUTOSCALE_1000", AUTOSCALE_1000);
	tolua_constant(L, "AUTOSCALE_1024K", AUTOSCALE_1024K);
	tolua_constant(L, "AUTOSCALE_1000K", AUTOSCALE_1000K);
	tolua_constant(L, "AUTOSCALE_OFF", AUTOSCALE_OFF);
	tolua_constant(L, "AUTOSCALE_ON", AUTOSCALE_ON);

	tolua_usertype(L, "CMeasure");
	tolua_cclass(L, "CMeasure", "CMeasure", "CGroup", NULL);

	tolua_beginmodule(L, "CMeasure");
	tolua_function(L, "GetName", Measure_GetName);
	tolua_function(L, "GetANSIName", Measure_GetANSIName);
	tolua_function(L, "Disable", Measure_Disable);
	tolua_function(L, "Enable", Measure_Enable);
	tolua_function(L, "IsDisabled", Measure_IsDisabled);
	tolua_function(L, "HasDynamicVariables", Measure_HasDynamicVariables);
	tolua_function(L, "GetValue", Measure_GetValue);
	tolua_function(L, "GetRelativeValue", Measure_GetRelativeValue);
	tolua_function(L, "GetValueRange", Measure_GetValueRange);
	tolua_function(L, "GetMinValue", Measure_GetMinValue);
	tolua_function(L, "GetMaxValue", Measure_GetMaxValue);
	tolua_function(L, "GetStringValue", Measure_GetStringValue);
	tolua_endmodule(L);
}
