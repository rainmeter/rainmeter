#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../MeterString.h"

static int MeterString_GetX(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	bool abs = (bool)tolua_toboolean(L, 2, false);
	int val = self->GetX(abs);
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterString_Update(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	bool val = self->Update();
	lua_pushboolean(L, val);

	return 1;
}

static int MeterString_SetText(lua_State* L)
{
	CMeterString* self = (CMeterString*)tolua_tousertype(L, 1, 0);
	const WCHAR* text = to_wchar(L, 2, 0);
	self->SetText(text);

	return 0;
}

void LuaManager::RegisterMeterString(lua_State* L)
{
	tolua_usertype(L, "CMeterString");
	tolua_cclass(L, "CMeterString", "CMeterString", "CMeter", NULL);

	tolua_beginmodule(L, "CMeterString");
	tolua_function(L, "GetX", MeterString_GetX);
	tolua_function(L, "Update", MeterString_Update);
	tolua_function(L, "SetText", MeterString_SetText);
	tolua_endmodule(L);
}
