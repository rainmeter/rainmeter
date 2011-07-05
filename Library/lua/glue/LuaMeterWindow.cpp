#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Rainmeter.h"
#include "../../MeterWindow.h"
#include "../../MeterString.h"

static int MeterWindow_MoveMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int x = (int)tolua_tonumber(L, 2, 0);
	int y = (int)tolua_tonumber(L, 3, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 4, 0));
	self->MoveMeter(x, y, name);

	return 0;
}

static int MeterWindow_HideMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->HideMeter(name, group);

	return 0;
}

static int MeterWindow_ShowMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->ShowMeter(name, group);

	return 0;
}

static int MeterWindow_ToggleMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->ToggleMeter(name, group);

	return 0;
}

static int MeterWindow_UpdateMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->UpdateMeter(name, group);

	return 0;
}

static int MeterWindow_DisableMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->DisableMeasure(name, group);

	return 0;
}

static int MeterWindow_EnableMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->EnableMeasure(name, group);

	return 0;
}

static int MeterWindow_ToggleMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));
	self->ToggleMeasure(name, group);

	return 0;
}

static int MeterWindow_UpdateMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const WCHAR* name = ((const WCHAR*)to_wchar(L, 2, 0));
	bool group = ((bool)tolua_toboolean(L, 3, false));

	self->UpdateMeasure(name, group);

	return 0;
}

static int MeterWindow_Redraw(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	self->Redraw();

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

static int MeterWindow_ChangeZPos(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	ZPOSITION zPos = ((ZPOSITION) (int)tolua_tonumber(L, 2, 0));
	bool all = ((bool)tolua_toboolean(L, 3, false));
	self->ChangeZPos(zPos, all);

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

static int MeterWindow_GetSkinName(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetSkinName();
	push_wstring(L, val);

	return 1;
}

static int MeterWindow_GetSkinIniFile(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring& val = self->GetSkinIniFile();
	push_wchar(L, val.c_str());

	return 1;
}

static int MeterWindow_GetWindowZPosition(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	ZPOSITION val = (ZPOSITION)self->GetWindowZPosition();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetXPercentage(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetXPercentage();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetYPercentage(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetYPercentage();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetXFromRight(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetXFromRight();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetYFromBottom(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetYFromBottom();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetW(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetW();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetH(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetH();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetX(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetX();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetY(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetY();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetXScreenDefined(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetXScreenDefined();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetYScreenDefined(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetYScreenDefined();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetXScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetXScreen();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetYScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetYScreen();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetNativeTransparency(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetNativeTransparency();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetClickThrough(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetClickThrough();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetKeepOnScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetKeepOnScreen();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetAutoSelectScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetAutoSelectScreen();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetWindowDraggable(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetWindowDraggable();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetSavePosition(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetSavePosition();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetSnapEdges(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	bool val = self->GetSnapEdges();
	tolua_pushboolean(L, val);

	return 1;
}

static int MeterWindow_GetWindowHide(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	HIDEMODE val = (HIDEMODE)self->GetWindowHide();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetAlphaValue(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetAlphaValue();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetUpdateCounter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetUpdateCounter();
	tolua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetTransitionUpdate(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetTransitionUpdate();
	tolua_pushnumber(L, (lua_Number)val);
	return 1;
}

static int MeterWindow_MakePathAbsolute(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring path = to_wstring(L, 2, 0);

	std::wstring val = self->MakePathAbsolute(path);
	push_wstring(L, val);

	return 1;
}

static int MeterWindow_GetMeter(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring meterName = to_wstring(L, 2, 0);

	CMeter* meter = self->GetMeter(meterName);
	if (!meter)
	{
		std::wstring error = L"Script: No such meter as ";
		error += meterName;
		error += L".";
		Log(LOG_ERROR, error.c_str());
		return 0;
	}

	if (CMeterString* stringMeter = dynamic_cast<CMeterString*>(meter))
	{
		tolua_pushusertype(L, stringMeter, "CMeterString");
	}
	else
	{
		tolua_pushusertype(L, meter, "CMeter");
	}

	return 1;
}

static int MeterWindow_GetMeasure(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const std::wstring measureName = to_wstring(L, 2, 0);

	CMeasure* val = self->GetMeasure(measureName);
	tolua_pushusertype(L, (void*)val, "CMeasure");

	return 1;
}

static int MeterWindow_ReplaceVariables(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const char* arg = (const char*)tolua_tostring(L, 2, 0);
	const char* val = (const char*)self->ReplaceVariables(arg);
	tolua_pushstring(L, (const char*)val);

	return 1;
}

static int MeterWindow_Bang(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const char* p_str = (const char*)tolua_tostring(L, 2, 0);

	p_str = self->ReplaceVariables(p_str);
	std::wstring bang = ConvertToWide(p_str);
	self->GetMainObject()->ExecuteCommand(bang.c_str(), self);

	return 0;
}

void LuaManager::RegisterMeterWindow(lua_State* L)
{
	tolua_constant(L, "ZPOSITION_ONDESKTOP", ZPOSITION_ONDESKTOP);
	tolua_constant(L, "ZPOSITION_ONBOTTOM", ZPOSITION_ONBOTTOM);
	tolua_constant(L, "ZPOSITION_NORMAL", ZPOSITION_NORMAL);
	tolua_constant(L, "ZPOSITION_ONTOP", ZPOSITION_ONTOP);
	tolua_constant(L, "ZPOSITION_ONTOPMOST", ZPOSITION_ONTOPMOST);
	tolua_constant(L, "HIDEMODE_NONE", HIDEMODE_NONE);
	tolua_constant(L, "HIDEMODE_HIDE", HIDEMODE_HIDE);
	tolua_constant(L, "HIDEMODE_FADEIN", HIDEMODE_FADEIN);
	tolua_constant(L, "HIDEMODE_FADEOUT", HIDEMODE_FADEOUT);

	tolua_usertype(L, "CMeterWindow");
	tolua_cclass(L, "CMeterWindow", "CMeterWindow", "CGroup", NULL);

	tolua_beginmodule(L, "CMeterWindow");
	tolua_function(L, "MoveMeter", MeterWindow_MoveMeter);
	tolua_function(L, "HideMeter", MeterWindow_HideMeter);
	tolua_function(L, "ShowMeter", MeterWindow_ShowMeter);
	tolua_function(L, "ToggleMeter", MeterWindow_ToggleMeter);
	tolua_function(L, "UpdateMeter", MeterWindow_UpdateMeter);
	tolua_function(L, "DisableMeasure", MeterWindow_DisableMeasure);
	tolua_function(L, "EnableMeasure", MeterWindow_EnableMeasure);
	tolua_function(L, "ToggleMeasure", MeterWindow_ToggleMeasure);
	tolua_function(L, "UpdateMeasure", MeterWindow_UpdateMeasure);
	tolua_function(L, "Redraw", MeterWindow_Redraw);
	tolua_function(L, "MoveWindow", MeterWindow_MoveWindow);
	tolua_function(L, "ChangeZPos", MeterWindow_ChangeZPos);
	tolua_function(L, "FadeWindow", MeterWindow_FadeWindow);
	tolua_function(L, "GetSkinName", MeterWindow_GetSkinName);
	tolua_function(L, "GetSkinIniFile", MeterWindow_GetSkinIniFile);
	tolua_function(L, "GetWindowZPosition", MeterWindow_GetWindowZPosition);
	tolua_function(L, "GetXPercentage", MeterWindow_GetXPercentage);
	tolua_function(L, "GetYPercentage", MeterWindow_GetYPercentage);
	tolua_function(L, "GetXFromRight", MeterWindow_GetXFromRight);
	tolua_function(L, "GetYFromBottom", MeterWindow_GetYFromBottom);
	tolua_function(L, "GetW", MeterWindow_GetW);
	tolua_function(L, "GetH", MeterWindow_GetH);
	tolua_function(L, "GetX", MeterWindow_GetX);
	tolua_function(L, "GetY", MeterWindow_GetY);
	tolua_function(L, "GetXScreenDefined", MeterWindow_GetXScreenDefined);
	tolua_function(L, "GetYScreenDefined", MeterWindow_GetYScreenDefined);
	tolua_function(L, "GetXScreen", MeterWindow_GetXScreen);
	tolua_function(L, "GetYScreen", MeterWindow_GetYScreen);
	tolua_function(L, "GetNativeTransparency", MeterWindow_GetNativeTransparency);
	tolua_function(L, "GetClickThrough", MeterWindow_GetClickThrough);
	tolua_function(L, "GetKeepOnScreen", MeterWindow_GetKeepOnScreen);
	tolua_function(L, "GetAutoSelectScreen", MeterWindow_GetAutoSelectScreen);
	tolua_function(L, "GetWindowDraggable", MeterWindow_GetWindowDraggable);
	tolua_function(L, "GetSavePosition", MeterWindow_GetSavePosition);
	tolua_function(L, "GetSnapEdges", MeterWindow_GetSnapEdges);
	tolua_function(L, "GetWindowHide", MeterWindow_GetWindowHide);
	tolua_function(L, "GetAlphaValue", MeterWindow_GetAlphaValue);
	tolua_function(L, "GetUpdateCounter", MeterWindow_GetUpdateCounter);
	tolua_function(L, "GetTransitionUpdate", MeterWindow_GetTransitionUpdate);
	tolua_function(L, "MakePathAbsolute", MeterWindow_MakePathAbsolute);
	tolua_function(L, "GetMeter", MeterWindow_GetMeter);
	tolua_function(L, "GetMeasure", MeterWindow_GetMeasure);
	tolua_function(L, "ReplaceVariables", MeterWindow_ReplaceVariables);
	tolua_function(L, "Bang", MeterWindow_Bang);
	tolua_endmodule(L);
}
