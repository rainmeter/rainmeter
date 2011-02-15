/*
** Lua binding: meterwindow
** Generated automatically by tolua++-1.0.92 on 02/15/11 18:55:45.
*/

#include "../../StdAfx.h"

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_meterwindow_open (lua_State* tolua_S);

#include "../../MeterWindow.h"
#include "../../Rainmeter.h"
#include "../../Litestep.h"
#include "../LuaPush.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_std__wstring (lua_State* tolua_S)
{
 std::wstring* self = (std::wstring*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}

static int tolua_collect_HWND (lua_State* tolua_S)
{
 HWND* self = (HWND*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CGroup");
 tolua_usertype(tolua_S,"CMeterWindow");
 tolua_usertype(tolua_S,"CMeter");
 tolua_usertype(tolua_S,"CMeasure");
 tolua_usertype(tolua_S,"HWND");
 tolua_usertype(tolua_S,"std::wstring");
 tolua_usertype(tolua_S,"WCHAR");
}

/* method: MoveMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_MoveMeter00
static int tolua_meterwindow_CMeterWindow_MoveMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !is_wchar(tolua_S,4,"const WCHAR",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  int x = ((int)  tolua_tonumber(tolua_S,2,0));
  int y = ((int)  tolua_tonumber(tolua_S,3,0));
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MoveMeter'", NULL);
#endif
  {
   self->MoveMeter(x,y,name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MoveMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HideMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_HideMeter00
static int tolua_meterwindow_CMeterWindow_HideMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HideMeter'", NULL);
#endif
  {
   self->HideMeter(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HideMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ShowMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_ShowMeter00
static int tolua_meterwindow_CMeterWindow_ShowMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ShowMeter'", NULL);
#endif
  {
   self->ShowMeter(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ShowMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ToggleMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_ToggleMeter00
static int tolua_meterwindow_CMeterWindow_ToggleMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ToggleMeter'", NULL);
#endif
  {
   self->ToggleMeter(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ToggleMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: UpdateMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_UpdateMeter00
static int tolua_meterwindow_CMeterWindow_UpdateMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'UpdateMeter'", NULL);
#endif
  {
   self->UpdateMeter(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'UpdateMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: DisableMeasure of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_DisableMeasure00
static int tolua_meterwindow_CMeterWindow_DisableMeasure00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'DisableMeasure'", NULL);
#endif
  {
   self->DisableMeasure(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'DisableMeasure'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: EnableMeasure of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_EnableMeasure00
static int tolua_meterwindow_CMeterWindow_EnableMeasure00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'EnableMeasure'", NULL);
#endif
  {
   self->EnableMeasure(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'EnableMeasure'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ToggleMeasure of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_ToggleMeasure00
static int tolua_meterwindow_CMeterWindow_ToggleMeasure00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ToggleMeasure'", NULL);
#endif
  {
   self->ToggleMeasure(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ToggleMeasure'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: UpdateMeasure of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_UpdateMeasure00
static int tolua_meterwindow_CMeterWindow_UpdateMeasure00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  bool group = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'UpdateMeasure'", NULL);
#endif
  {
   self->UpdateMeasure(name,group);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'UpdateMeasure'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Redraw of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_Redraw00
static int tolua_meterwindow_CMeterWindow_Redraw00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Redraw'", NULL);
#endif
  {
   self->Redraw();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Redraw'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MoveWindow of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_MoveWindow00
static int tolua_meterwindow_CMeterWindow_MoveWindow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  int x = ((int)  tolua_tonumber(tolua_S,2,0));
  int y = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MoveWindow'", NULL);
#endif
  {
   self->MoveWindow(x,y);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MoveWindow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ChangeZPos of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_ChangeZPos00
static int tolua_meterwindow_CMeterWindow_ChangeZPos00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isboolean(tolua_S,3,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  ZPOSITION zPos = ((ZPOSITION) (int)  tolua_tonumber(tolua_S,2,0));
  bool all = ((bool)  tolua_toboolean(tolua_S,3,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ChangeZPos'", NULL);
#endif
  {
   self->ChangeZPos(zPos,all);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ChangeZPos'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: FadeWindow of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_FadeWindow00
static int tolua_meterwindow_CMeterWindow_FadeWindow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  int from = ((int)  tolua_tonumber(tolua_S,2,0));
  int to = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'FadeWindow'", NULL);
#endif
  {
   self->FadeWindow(from,to);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'FadeWindow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetWindow of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetWindow00
static int tolua_meterwindow_CMeterWindow_GetWindow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetWindow'", NULL);
#endif
  {
   HWND tolua_ret = (HWND)  self->GetWindow();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((HWND)(tolua_ret));
     tolua_pushusertype(tolua_S,tolua_obj,"HWND");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(HWND));
     tolua_pushusertype(tolua_S,tolua_obj,"HWND");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetWindow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSkinAuthor of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetSkinAuthor00
static int tolua_meterwindow_CMeterWindow_GetSkinAuthor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSkinAuthor'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetSkinAuthor();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSkinAuthor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSkinName of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetSkinName00
static int tolua_meterwindow_CMeterWindow_GetSkinName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSkinName'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetSkinName();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSkinName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSkinIniFile of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetSkinIniFile00
static int tolua_meterwindow_CMeterWindow_GetSkinIniFile00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSkinIniFile'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetSkinIniFile();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSkinIniFile'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetWindowZPosition of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetWindowZPosition00
static int tolua_meterwindow_CMeterWindow_GetWindowZPosition00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetWindowZPosition'", NULL);
#endif
  {
   ZPOSITION tolua_ret = (ZPOSITION)  self->GetWindowZPosition();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetWindowZPosition'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetXPercentage of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetXPercentage00
static int tolua_meterwindow_CMeterWindow_GetXPercentage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetXPercentage'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetXPercentage();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetXPercentage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetYPercentage of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetYPercentage00
static int tolua_meterwindow_CMeterWindow_GetYPercentage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetYPercentage'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetYPercentage();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetYPercentage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetXFromRight of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetXFromRight00
static int tolua_meterwindow_CMeterWindow_GetXFromRight00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetXFromRight'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetXFromRight();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetXFromRight'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetYFromBottom of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetYFromBottom00
static int tolua_meterwindow_CMeterWindow_GetYFromBottom00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetYFromBottom'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetYFromBottom();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetYFromBottom'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetW of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetW00
static int tolua_meterwindow_CMeterWindow_GetW00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetW'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetW();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetW'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetH of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetH00
static int tolua_meterwindow_CMeterWindow_GetH00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetH'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetH();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetH'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetX of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetX00
static int tolua_meterwindow_CMeterWindow_GetX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetX'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetX();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetY of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetY00
static int tolua_meterwindow_CMeterWindow_GetY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetY'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetY();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetXScreenDefined of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetXScreenDefined00
static int tolua_meterwindow_CMeterWindow_GetXScreenDefined00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetXScreenDefined'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetXScreenDefined();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetXScreenDefined'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetYScreenDefined of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetYScreenDefined00
static int tolua_meterwindow_CMeterWindow_GetYScreenDefined00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetYScreenDefined'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetYScreenDefined();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetYScreenDefined'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetXScreen of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetXScreen00
static int tolua_meterwindow_CMeterWindow_GetXScreen00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetXScreen'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetXScreen();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetXScreen'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetYScreen of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetYScreen00
static int tolua_meterwindow_CMeterWindow_GetYScreen00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetYScreen'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetYScreen();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetYScreen'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetNativeTransparency of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetNativeTransparency00
static int tolua_meterwindow_CMeterWindow_GetNativeTransparency00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetNativeTransparency'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetNativeTransparency();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetNativeTransparency'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetClickThrough of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetClickThrough00
static int tolua_meterwindow_CMeterWindow_GetClickThrough00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetClickThrough'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetClickThrough();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetClickThrough'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetKeepOnScreen of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetKeepOnScreen00
static int tolua_meterwindow_CMeterWindow_GetKeepOnScreen00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetKeepOnScreen'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetKeepOnScreen();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetKeepOnScreen'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetAutoSelectScreen of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetAutoSelectScreen00
static int tolua_meterwindow_CMeterWindow_GetAutoSelectScreen00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetAutoSelectScreen'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetAutoSelectScreen();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetAutoSelectScreen'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetWindowDraggable of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetWindowDraggable00
static int tolua_meterwindow_CMeterWindow_GetWindowDraggable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetWindowDraggable'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetWindowDraggable();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetWindowDraggable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSavePosition of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetSavePosition00
static int tolua_meterwindow_CMeterWindow_GetSavePosition00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSavePosition'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetSavePosition();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSavePosition'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSnapEdges of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetSnapEdges00
static int tolua_meterwindow_CMeterWindow_GetSnapEdges00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSnapEdges'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetSnapEdges();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSnapEdges'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetWindowHide of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetWindowHide00
static int tolua_meterwindow_CMeterWindow_GetWindowHide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetWindowHide'", NULL);
#endif
  {
   HIDEMODE tolua_ret = (HIDEMODE)  self->GetWindowHide();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetWindowHide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetAlphaValue of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetAlphaValue00
static int tolua_meterwindow_CMeterWindow_GetAlphaValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetAlphaValue'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetAlphaValue();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetAlphaValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetUpdateCounter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetUpdateCounter00
static int tolua_meterwindow_CMeterWindow_GetUpdateCounter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetUpdateCounter'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetUpdateCounter();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetUpdateCounter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTransitionUpdate of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetTransitionUpdate00
static int tolua_meterwindow_CMeterWindow_GetTransitionUpdate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTransitionUpdate'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetTransitionUpdate();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTransitionUpdate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: MakePathAbsolute of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_MakePathAbsolute00
static int tolua_meterwindow_CMeterWindow_MakePathAbsolute00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring path = ((const std::wstring)  to_wstring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'MakePathAbsolute'", NULL);
#endif
  {
   std::wstring tolua_ret = (std::wstring)  self->MakePathAbsolute(path);
   {
#ifdef __cplusplus
     push_wstring(tolua_S,(void*)&tolua_ret,"std::wstring");
#else
     push_wstring(tolua_S,(void*)&tolua_ret,"std::wstring");
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'MakePathAbsolute'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetMeter of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetMeter00
static int tolua_meterwindow_CMeterWindow_GetMeter00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring meterName = ((const std::wstring)  to_wstring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetMeter'", NULL);
#endif
  {
   CMeter* tolua_ret = (CMeter*)  self->GetMeter(meterName);
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"CMeter");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetMeter'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetMeasure of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_GetMeasure00
static int tolua_meterwindow_CMeterWindow_GetMeasure00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring measureName = ((const std::wstring)  to_wstring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetMeasure'", NULL);
#endif
  {
   CMeasure* tolua_ret = (CMeasure*)  self->GetMeasure(measureName);
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"CMeasure");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetMeasure'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ReplaceVariables of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_ReplaceVariables00
static int tolua_meterwindow_CMeterWindow_ReplaceVariables00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const char* p_str = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ReplaceVariables'", NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->ReplaceVariables(p_str);
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ReplaceVariables'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Bang of class  CMeterWindow */
#ifndef TOLUA_DISABLE_tolua_meterwindow_CMeterWindow_Bang00
static int tolua_meterwindow_CMeterWindow_Bang00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterWindow",0,&tolua_err) ||
     !tolua_isstring(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterWindow* self = (CMeterWindow*)  tolua_tousertype(tolua_S,1,0);
  const char* p_str = ((const char*)  tolua_tostring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Bang'", NULL);
#endif
  {
	  p_str = self->ReplaceVariables(p_str);
	  std::wstring bang = ConvertToWide(p_str);
	  self->GetMainObject()->ExecuteCommand(bang.c_str(), self);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Bang'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_meterwindow_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_constant(tolua_S,"ZPOSITION_ONDESKTOP",ZPOSITION_ONDESKTOP);
  tolua_constant(tolua_S,"ZPOSITION_ONBOTTOM",ZPOSITION_ONBOTTOM);
  tolua_constant(tolua_S,"ZPOSITION_NORMAL",ZPOSITION_NORMAL);
  tolua_constant(tolua_S,"ZPOSITION_ONTOP",ZPOSITION_ONTOP);
  tolua_constant(tolua_S,"ZPOSITION_ONTOPMOST",ZPOSITION_ONTOPMOST);
  tolua_constant(tolua_S,"HIDEMODE_NONE",HIDEMODE_NONE);
  tolua_constant(tolua_S,"HIDEMODE_HIDE",HIDEMODE_HIDE);
  tolua_constant(tolua_S,"HIDEMODE_FADEIN",HIDEMODE_FADEIN);
  tolua_constant(tolua_S,"HIDEMODE_FADEOUT",HIDEMODE_FADEOUT);
  tolua_cclass(tolua_S,"CMeterWindow","CMeterWindow","CGroup",NULL);
  tolua_beginmodule(tolua_S,"CMeterWindow");
   tolua_function(tolua_S,"MoveMeter",tolua_meterwindow_CMeterWindow_MoveMeter00);
   tolua_function(tolua_S,"HideMeter",tolua_meterwindow_CMeterWindow_HideMeter00);
   tolua_function(tolua_S,"ShowMeter",tolua_meterwindow_CMeterWindow_ShowMeter00);
   tolua_function(tolua_S,"ToggleMeter",tolua_meterwindow_CMeterWindow_ToggleMeter00);
   tolua_function(tolua_S,"UpdateMeter",tolua_meterwindow_CMeterWindow_UpdateMeter00);
   tolua_function(tolua_S,"DisableMeasure",tolua_meterwindow_CMeterWindow_DisableMeasure00);
   tolua_function(tolua_S,"EnableMeasure",tolua_meterwindow_CMeterWindow_EnableMeasure00);
   tolua_function(tolua_S,"ToggleMeasure",tolua_meterwindow_CMeterWindow_ToggleMeasure00);
   tolua_function(tolua_S,"UpdateMeasure",tolua_meterwindow_CMeterWindow_UpdateMeasure00);
   tolua_function(tolua_S,"Redraw",tolua_meterwindow_CMeterWindow_Redraw00);
   tolua_function(tolua_S,"MoveWindow",tolua_meterwindow_CMeterWindow_MoveWindow00);
   tolua_function(tolua_S,"ChangeZPos",tolua_meterwindow_CMeterWindow_ChangeZPos00);
   tolua_function(tolua_S,"FadeWindow",tolua_meterwindow_CMeterWindow_FadeWindow00);
   tolua_function(tolua_S,"GetWindow",tolua_meterwindow_CMeterWindow_GetWindow00);
   tolua_function(tolua_S,"GetSkinAuthor",tolua_meterwindow_CMeterWindow_GetSkinAuthor00);
   tolua_function(tolua_S,"GetSkinName",tolua_meterwindow_CMeterWindow_GetSkinName00);
   tolua_function(tolua_S,"GetSkinIniFile",tolua_meterwindow_CMeterWindow_GetSkinIniFile00);
   tolua_function(tolua_S,"GetWindowZPosition",tolua_meterwindow_CMeterWindow_GetWindowZPosition00);
   tolua_function(tolua_S,"GetXPercentage",tolua_meterwindow_CMeterWindow_GetXPercentage00);
   tolua_function(tolua_S,"GetYPercentage",tolua_meterwindow_CMeterWindow_GetYPercentage00);
   tolua_function(tolua_S,"GetXFromRight",tolua_meterwindow_CMeterWindow_GetXFromRight00);
   tolua_function(tolua_S,"GetYFromBottom",tolua_meterwindow_CMeterWindow_GetYFromBottom00);
   tolua_function(tolua_S,"GetW",tolua_meterwindow_CMeterWindow_GetW00);
   tolua_function(tolua_S,"GetH",tolua_meterwindow_CMeterWindow_GetH00);
   tolua_function(tolua_S,"GetX",tolua_meterwindow_CMeterWindow_GetX00);
   tolua_function(tolua_S,"GetY",tolua_meterwindow_CMeterWindow_GetY00);
   tolua_function(tolua_S,"GetXScreenDefined",tolua_meterwindow_CMeterWindow_GetXScreenDefined00);
   tolua_function(tolua_S,"GetYScreenDefined",tolua_meterwindow_CMeterWindow_GetYScreenDefined00);
   tolua_function(tolua_S,"GetXScreen",tolua_meterwindow_CMeterWindow_GetXScreen00);
   tolua_function(tolua_S,"GetYScreen",tolua_meterwindow_CMeterWindow_GetYScreen00);
   tolua_function(tolua_S,"GetNativeTransparency",tolua_meterwindow_CMeterWindow_GetNativeTransparency00);
   tolua_function(tolua_S,"GetClickThrough",tolua_meterwindow_CMeterWindow_GetClickThrough00);
   tolua_function(tolua_S,"GetKeepOnScreen",tolua_meterwindow_CMeterWindow_GetKeepOnScreen00);
   tolua_function(tolua_S,"GetAutoSelectScreen",tolua_meterwindow_CMeterWindow_GetAutoSelectScreen00);
   tolua_function(tolua_S,"GetWindowDraggable",tolua_meterwindow_CMeterWindow_GetWindowDraggable00);
   tolua_function(tolua_S,"GetSavePosition",tolua_meterwindow_CMeterWindow_GetSavePosition00);
   tolua_function(tolua_S,"GetSnapEdges",tolua_meterwindow_CMeterWindow_GetSnapEdges00);
   tolua_function(tolua_S,"GetWindowHide",tolua_meterwindow_CMeterWindow_GetWindowHide00);
   tolua_function(tolua_S,"GetAlphaValue",tolua_meterwindow_CMeterWindow_GetAlphaValue00);
   tolua_function(tolua_S,"GetUpdateCounter",tolua_meterwindow_CMeterWindow_GetUpdateCounter00);
   tolua_function(tolua_S,"GetTransitionUpdate",tolua_meterwindow_CMeterWindow_GetTransitionUpdate00);
   tolua_function(tolua_S,"MakePathAbsolute",tolua_meterwindow_CMeterWindow_MakePathAbsolute00);
   tolua_function(tolua_S,"GetMeter",tolua_meterwindow_CMeterWindow_GetMeter00);
   tolua_function(tolua_S,"GetMeasure",tolua_meterwindow_CMeterWindow_GetMeasure00);
   tolua_function(tolua_S,"ReplaceVariables",tolua_meterwindow_CMeterWindow_ReplaceVariables00);
   tolua_function(tolua_S,"Bang", tolua_meterwindow_CMeterWindow_Bang00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_meterwindow (lua_State* tolua_S) {
 return tolua_meterwindow_open(tolua_S);
};
#endif

