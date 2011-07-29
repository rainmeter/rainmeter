#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Meter.h"

static int Meter_GetName(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	const WCHAR* val = (const WCHAR*)self->GetName();
	push_wchar(L, val);
	return 1;
}

static int Meter_GetOption(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	CMeterWindow* meterWindow = self->GetMeterWindow();
	CConfigParser& parser = meterWindow->GetParser();

	const char* arg = (const char*)tolua_tostring(L, 2, 0);
	std::wstring strTmp = ConvertToWide(arg);
	strTmp = parser.GetValue(self->GetName(), strTmp, L"");

	parser.SetBuiltInVariable(L"CURRENTSECTION", self->GetName());  // Set temporarily
	parser.ReplaceVariables(strTmp);
	parser.SetBuiltInVariable(L"CURRENTSECTION", L"");  // Reset
	parser.ReplaceMeasures(strTmp);

	push_wchar(L, strTmp.c_str());
	return 1;
}

static int Meter_GetW(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetW();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Meter_GetH(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetH();
	lua_pushnumber(L, (lua_Number)val);

	return 1;

}

static int Meter_GetX(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool abs = ((bool)tolua_toboolean(L, 2, false));
	int val = (int)self->GetX(abs);
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Meter_GetY(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool abs = ((bool)tolua_toboolean(L, 2, false));
	int val = (int)self->GetY(abs);
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Meter_SetW(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int w = (int)tolua_tonumber(L, 2, 0);
	self->SetW(w);

	return 0;
}

static int Meter_SetH(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int h = (int)tolua_tonumber(L, 2, 0);
	self->SetH(h);

	return 0;
}

static int Meter_SetX(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int x = (int)tolua_tonumber(L, 2, 0);
	self->SetX(x);

	return 0;
}

static int Meter_SetY(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int y = (int)tolua_tonumber(L, 2, 0);
	self->SetY(y);

	return 0;
}

static int Meter_Hide(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	self->Hide();

	return 0;
}

static int Meter_Show(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	self->Show();

	return 0;
}

void LuaManager::RegisterMeter(lua_State* L)
{
	tolua_usertype(L, "CMeter");
	tolua_cclass(L, "CMeter", "CMeter", "", NULL);

	tolua_beginmodule(L, "CMeter");
	tolua_function(L, "GetName", Meter_GetName);
	tolua_function(L, "GetOption", Meter_GetOption);
	tolua_function(L, "GetW", Meter_GetW);
	tolua_function(L, "GetH", Meter_GetH);
	tolua_function(L, "GetX", Meter_GetX);
	tolua_function(L, "GetY", Meter_GetY);
	tolua_function(L, "SetW", Meter_SetW);
	tolua_function(L, "SetH", Meter_SetH);
	tolua_function(L, "SetX", Meter_SetX);
	tolua_function(L, "SetY", Meter_SetY);
	tolua_function(L, "Hide", Meter_Hide);
	tolua_function(L, "Show", Meter_Show);
	tolua_endmodule(L);
}
