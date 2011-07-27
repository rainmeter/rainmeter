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

static int MeterWindow_GetXScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetXScreen();
	lua_pushnumber(L, (lua_Number)val);

	return 1;
}

static int MeterWindow_GetYScreen(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	int val = (int)self->GetYScreen();
	lua_pushnumber(L, (lua_Number)val);

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

static int MeterWindow_GetVariable(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const char* arg = (const char*)tolua_tostring(L, 2, 0);
	std::wstring strTmp = ConvertToWide(arg);

	if (self->GetParser().GetVariable(strTmp, strTmp))
	{
		std::string val = ConvertToAscii(strTmp.c_str());
		tolua_pushstring(L, val.c_str());
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
	const char* arg = (const char*)tolua_tostring(L, 2, 0);
	std::wstring strTmp = ConvertToWide(arg);

	self->GetParser().ReplaceVariables(strTmp);
	std::string val = ConvertToAscii(strTmp.c_str());
	tolua_pushstring(L, val.c_str());

	return 1;
}

static int MeterWindow_Bang(lua_State* L)
{
	CMeterWindow* self = (CMeterWindow*)tolua_tousertype(L, 1, 0);
	const char* arg = (const char*)tolua_tostring(L, 2, 0);
	std::wstring strTmp = ConvertToWide(arg);

	CConfigParser& parser = self->GetParser();
	parser.ReplaceVariables(strTmp);
	parser.ReplaceMeasures(strTmp);
	self->GetMainObject()->ExecuteCommand(strTmp.c_str(), self);

	return 0;
}

void LuaManager::RegisterMeterWindow(lua_State* L)
{
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
	tolua_function(L, "FadeWindow", MeterWindow_FadeWindow);
	tolua_function(L, "GetSkinName", MeterWindow_GetSkinName);
	tolua_function(L, "GetSkinIniFile", MeterWindow_GetSkinIniFile);
	tolua_function(L, "GetW", MeterWindow_GetW);
	tolua_function(L, "GetH", MeterWindow_GetH);
	tolua_function(L, "GetX", MeterWindow_GetX);
	tolua_function(L, "GetY", MeterWindow_GetY);
	tolua_function(L, "GetXScreen", MeterWindow_GetXScreen);
	tolua_function(L, "GetYScreen", MeterWindow_GetYScreen);
	tolua_function(L, "MakePathAbsolute", MeterWindow_MakePathAbsolute);
	tolua_function(L, "GetMeter", MeterWindow_GetMeter);
	tolua_function(L, "GetMeasure", MeterWindow_GetMeasure);
	tolua_function(L, "GetVariable", MeterWindow_GetVariable);
	tolua_function(L, "ReplaceVariables", MeterWindow_ReplaceVariables);
	tolua_function(L, "Bang", MeterWindow_Bang);
	tolua_endmodule(L);
}
