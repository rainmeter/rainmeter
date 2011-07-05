#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Rainmeter.h"

static int Rainmeter_GetConfigEditor(lua_State* L)
{
	CRainmeter* self = (CRainmeter*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetConfigEditor();
	push_wstring(L, val);

	return 1;
}

static int Rainmeter_GetLogViewer(lua_State* L)
{
	CRainmeter* self = (CRainmeter*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetLogViewer();
	push_wstring(L, val);

	return 1;
}

static int Rainmeter_GetStatsDate(lua_State* L)
{
	CRainmeter* self = (CRainmeter*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetStatsDate();
	push_wstring(L, val);

	return 1;
}

static int Rainmeter_GetDebug(lua_State* L)
{
	bool val = CRainmeter::GetDebug();
	tolua_pushboolean(L, val);

	return 1;
}

static int Rainmeter_IsMenuActive(lua_State* L)
{
	CRainmeter* self = (CRainmeter*)tolua_tousertype(L, 1, 0);
	bool val = self->IsMenuActive();
	tolua_pushboolean(L, val);

	return 1;
}

void LuaManager::RegisterRainmeter(lua_State* L)
{
	tolua_usertype(L, "CRainmeter");
	tolua_cclass(L, "CRainmeter", "CRainmeter", "", NULL);

	tolua_beginmodule(L, "CRainmeter");
	tolua_function(L, "GetConfigEditor", Rainmeter_GetConfigEditor);
	tolua_function(L, "GetLogViewer", Rainmeter_GetLogViewer);
	tolua_function(L, "GetStatsDate", Rainmeter_GetStatsDate);
	tolua_function(L, "GetDebug", Rainmeter_GetDebug);
	tolua_function(L, "IsMenuActive", Rainmeter_IsMenuActive);
	tolua_endmodule(L);
}
