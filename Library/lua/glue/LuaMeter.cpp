#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Meter.h"

static int Meter_Update(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->Update();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_HasActiveTransition(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->HasActiveTransition();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_HasDynamicVariables(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->HasDynamicVariables();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_GetW(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetW();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Meter_GetH(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetH();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;

}

static int Meter_GetX(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool abs = ((bool)tolua_toboolean(L, 2, false));
	int val = (int)self->GetX(abs);
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int Meter_GetY(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool abs = ((bool)tolua_toboolean(L, 2, false));
	int val = (int)self->GetY(abs);
	tolua_pushnumber(L, (lua_Number)val);

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

static int Meter_GetToolTipText(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetToolTipText();
	push_wchar(L, val.c_str());

	return 1;
}

static int Meter_HasToolTip(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->HasToolTip();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_SetToolTipHidden(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool b = (bool)tolua_toboolean(L, 2, 0);
	self->SetToolTipHidden(b);

	return 0;
}

static int Meter_UpdateToolTip(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	self->UpdateToolTip();

	return 0;
}

static int Meter_HasMouseAction(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->HasMouseAction();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_HasMouseActionCursor(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->HasMouseActionCursor();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_SetMouseActionCursor(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool b = (bool)tolua_toboolean(L, 2, 0);
	self->SetMouseActionCursor(b);

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

static int Meter_IsHidden(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->IsHidden();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_HitTest(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	int x = (int)tolua_tonumber(L, 2, 0);
	int y = (int)tolua_tonumber(L, 3, 0);

	bool val = self->HitTest(x, y);
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_IsMouseOver(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	bool val = self->IsMouseOver();
	tolua_pushboolean(L, val);

	return 1;
}

static int Meter_GetName(lua_State* L)
{
	CMeter* self = (CMeter*)tolua_tousertype(L, 1, 0);
	const WCHAR* val = (const WCHAR*)self->GetName();
	push_wchar(L, val);
	return 1;
}

void LuaManager::RegisterMeter(lua_State* L)
{
	tolua_usertype(L, "CMeter");
	tolua_cclass(L, "CMeter", "CMeter", "CGroup", NULL);

	tolua_beginmodule(L, "CMeter");
	tolua_function(L, "Update", Meter_Update);
	tolua_function(L, "HasActiveTransition", Meter_HasActiveTransition);
	tolua_function(L, "HasDynamicVariables", Meter_HasDynamicVariables);
	tolua_function(L, "GetW", Meter_GetW);
	tolua_function(L, "GetH", Meter_GetH);
	tolua_function(L, "GetX", Meter_GetX);
	tolua_function(L, "GetY", Meter_GetY);
	tolua_function(L, "SetW", Meter_SetW);
	tolua_function(L, "SetH", Meter_SetH);
	tolua_function(L, "SetX", Meter_SetX);
	tolua_function(L, "SetY", Meter_SetY);
	tolua_function(L, "GetToolTipText", Meter_GetToolTipText);
	tolua_function(L, "HasToolTip", Meter_HasToolTip);
	tolua_function(L, "SetToolTipHidden", Meter_SetToolTipHidden);
	tolua_function(L, "UpdateToolTip", Meter_UpdateToolTip);
	tolua_function(L, "HasMouseAction", Meter_HasMouseAction);
	tolua_function(L, "HasMouseActionCursor", Meter_HasMouseActionCursor);
	tolua_function(L, "SetMouseActionCursor", Meter_SetMouseActionCursor);
	tolua_function(L, "Hide", Meter_Hide);
	tolua_function(L, "Show", Meter_Show);
	tolua_function(L, "IsHidden", Meter_IsHidden);
	tolua_function(L, "HitTest", Meter_HitTest);
	tolua_function(L, "IsMouseOver", Meter_IsMouseOver);
	tolua_function(L, "GetName", Meter_GetName);
	tolua_endmodule(L);
}
