/*
** Lua binding: rainmeter
** Generated automatically by tolua++-1.0.92 on 02/15/11 21:07:15.
*/

#include "../../StdAfx.h"

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_rainmeter_open (lua_State* tolua_S);

#include "../../Rainmeter.h"
#include "../LuaPush.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_std__wstring (lua_State* tolua_S)
{
 std::wstring* self = (std::wstring*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}

static int tolua_collect_BOOL (lua_State* tolua_S)
{
 BOOL* self = (BOOL*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"std::wstring");
 tolua_usertype(tolua_S,"CRainmeter");
 tolua_usertype(tolua_S,"CMeterWindow");
 tolua_usertype(tolua_S,"POINT");
}

/* method: GetMeterWindow of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetMeterWindow00
static int tolua_rainmeter_CRainmeter_GetMeterWindow00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring config = ((const std::wstring)  to_wstring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetMeterWindow'", NULL);
#endif
  {
   CMeterWindow* tolua_ret = (CMeterWindow*)  self->GetMeterWindow(config);
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"CMeterWindow");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetMeterWindow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetPath of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetPath00
static int tolua_rainmeter_CRainmeter_GetPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetPath'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetPath();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetPath'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetIniFile of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetIniFile00
static int tolua_rainmeter_CRainmeter_GetIniFile00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetIniFile'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetIniFile();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetIniFile'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetLogFile of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetLogFile00
static int tolua_rainmeter_CRainmeter_GetLogFile00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetLogFile'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetLogFile();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetLogFile'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSkinPath of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetSkinPath00
static int tolua_rainmeter_CRainmeter_GetSkinPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSkinPath'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetSkinPath();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetSkinPath'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetPluginPath of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetPluginPath00
static int tolua_rainmeter_CRainmeter_GetPluginPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetPluginPath'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetPluginPath();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetPluginPath'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetAddonPath of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetAddonPath00
static int tolua_rainmeter_CRainmeter_GetAddonPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetAddonPath'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetAddonPath();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetAddonPath'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetSettingsPath of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetSettingsPath00
static int tolua_rainmeter_CRainmeter_GetSettingsPath00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetSettingsPath'", NULL);
#endif
  {
   std::wstring tolua_ret = (std::wstring)  self->GetSettingsPath();
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
 tolua_error(tolua_S,"#ferror in function 'GetSettingsPath'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetConfigEditor of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetConfigEditor00
static int tolua_rainmeter_CRainmeter_GetConfigEditor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetConfigEditor'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetConfigEditor();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetConfigEditor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetLogViewer of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetLogViewer00
static int tolua_rainmeter_CRainmeter_GetLogViewer00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetLogViewer'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetLogViewer();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetLogViewer'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetStatsDate of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetStatsDate00
static int tolua_rainmeter_CRainmeter_GetStatsDate00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetStatsDate'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetStatsDate();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetStatsDate'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetDebug of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetDebug00
static int tolua_rainmeter_CRainmeter_GetDebug00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertable(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  {
   bool tolua_ret = (bool)  CRainmeter::GetDebug();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetDebug'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SaveSettings of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_SaveSettings00
static int tolua_rainmeter_CRainmeter_SaveSettings00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SaveSettings'", NULL);
#endif
  {
   self->SaveSettings();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SaveSettings'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: WriteStats of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_WriteStats00
static int tolua_rainmeter_CRainmeter_WriteStats00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  bool bForce = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'WriteStats'", NULL);
#endif
  {
   self->WriteStats(bForce);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'WriteStats'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ResetStats of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ResetStats00
static int tolua_rainmeter_CRainmeter_ResetStats00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ResetStats'", NULL);
#endif
  {
   self->ResetStats();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ResetStats'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetLogging of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetLogging00
static int tolua_rainmeter_CRainmeter_GetLogging00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetLogging'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetLogging();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetLogging'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: StartLogging of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_StartLogging00
static int tolua_rainmeter_CRainmeter_StartLogging00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'StartLogging'", NULL);
#endif
  {
   self->StartLogging();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'StartLogging'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: StopLogging of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_StopLogging00
static int tolua_rainmeter_CRainmeter_StopLogging00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'StopLogging'", NULL);
#endif
  {
   self->StopLogging();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'StopLogging'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: DeleteLogFile of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_DeleteLogFile00
static int tolua_rainmeter_CRainmeter_DeleteLogFile00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'DeleteLogFile'", NULL);
#endif
  {
   self->DeleteLogFile();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'DeleteLogFile'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetDisableRDP of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetDisableRDP00
static int tolua_rainmeter_CRainmeter_GetDisableRDP00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetDisableRDP'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetDisableRDP();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetDisableRDP'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetDisableDragging of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetDisableDragging00
static int tolua_rainmeter_CRainmeter_GetDisableDragging00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetDisableDragging'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->GetDisableDragging();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetDisableDragging'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetDisableDragging of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_SetDisableDragging00
static int tolua_rainmeter_CRainmeter_SetDisableDragging00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  bool dragging = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetDisableDragging'", NULL);
#endif
  {
   self->SetDisableDragging(dragging);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetDisableDragging'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetDebug of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_SetDebug00
static int tolua_rainmeter_CRainmeter_SetDebug00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  bool debug = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetDebug'", NULL);
#endif
  {
   self->SetDebug(debug);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetDebug'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMenuActive of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_IsMenuActive00
static int tolua_rainmeter_CRainmeter_IsMenuActive00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMenuActive'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMenuActive();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMenuActive'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ShowContextMenu of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ShowContextMenu00
static int tolua_rainmeter_CRainmeter_ShowContextMenu00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !tolua_isusertype(tolua_S,2,"POINT",0,&tolua_err)) ||
     !tolua_isusertype(tolua_S,3,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  POINT pos = *((POINT*)  tolua_tousertype(tolua_S,2,0));
  CMeterWindow* meterWindow = ((CMeterWindow*)  tolua_tousertype(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ShowContextMenu'", NULL);
#endif
  {
   self->ShowContextMenu(pos,meterWindow);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ShowContextMenu'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteL of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteL00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteL00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteL'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteL();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteL'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteR of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteR00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteR00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteR'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteR();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteR'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteM of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteM00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteM00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteM'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteM();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteM'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteDR of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteDR00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteDR00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteDR'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteDR();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteDR'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteDL of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteDL00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteDL00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteDL'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteDL();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteDL'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTrayExecuteDM of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayExecuteDM00
static int tolua_rainmeter_CRainmeter_GetTrayExecuteDM00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayExecuteDM'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetTrayExecuteDM();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayExecuteDM'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_rainmeter_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_constant(tolua_S,"MAX_LINE_LENGTH",MAX_LINE_LENGTH);
  tolua_cclass(tolua_S,"CRainmeter","CRainmeter","",NULL);
  tolua_beginmodule(tolua_S,"CRainmeter");
   tolua_function(tolua_S,"GetMeterWindow",tolua_rainmeter_CRainmeter_GetMeterWindow00);
   tolua_function(tolua_S,"GetPath",tolua_rainmeter_CRainmeter_GetPath00);
   tolua_function(tolua_S,"GetIniFile",tolua_rainmeter_CRainmeter_GetIniFile00);
   tolua_function(tolua_S,"GetLogFile",tolua_rainmeter_CRainmeter_GetLogFile00);
   tolua_function(tolua_S,"GetSkinPath",tolua_rainmeter_CRainmeter_GetSkinPath00);
   tolua_function(tolua_S,"GetPluginPath",tolua_rainmeter_CRainmeter_GetPluginPath00);
   tolua_function(tolua_S,"GetAddonPath",tolua_rainmeter_CRainmeter_GetAddonPath00);
   tolua_function(tolua_S,"GetSettingsPath",tolua_rainmeter_CRainmeter_GetSettingsPath00);
   tolua_function(tolua_S,"GetConfigEditor",tolua_rainmeter_CRainmeter_GetConfigEditor00);
   tolua_function(tolua_S,"GetLogViewer",tolua_rainmeter_CRainmeter_GetLogViewer00);
   tolua_function(tolua_S,"GetStatsDate",tolua_rainmeter_CRainmeter_GetStatsDate00);
   tolua_function(tolua_S,"GetDebug",tolua_rainmeter_CRainmeter_GetDebug00);
   tolua_function(tolua_S,"SaveSettings",tolua_rainmeter_CRainmeter_SaveSettings00);
   tolua_function(tolua_S,"WriteStats",tolua_rainmeter_CRainmeter_WriteStats00);
   tolua_function(tolua_S,"ResetStats",tolua_rainmeter_CRainmeter_ResetStats00);
   tolua_function(tolua_S,"GetLogging",tolua_rainmeter_CRainmeter_GetLogging00);
   tolua_function(tolua_S,"StartLogging",tolua_rainmeter_CRainmeter_StartLogging00);
   tolua_function(tolua_S,"StopLogging",tolua_rainmeter_CRainmeter_StopLogging00);
   tolua_function(tolua_S,"DeleteLogFile",tolua_rainmeter_CRainmeter_DeleteLogFile00);
   tolua_function(tolua_S,"GetDisableRDP",tolua_rainmeter_CRainmeter_GetDisableRDP00);
   tolua_function(tolua_S,"GetDisableDragging",tolua_rainmeter_CRainmeter_GetDisableDragging00);
   tolua_function(tolua_S,"SetDisableDragging",tolua_rainmeter_CRainmeter_SetDisableDragging00);
   tolua_function(tolua_S,"SetDebug",tolua_rainmeter_CRainmeter_SetDebug00);
   tolua_function(tolua_S,"IsMenuActive",tolua_rainmeter_CRainmeter_IsMenuActive00);
   tolua_function(tolua_S,"ShowContextMenu",tolua_rainmeter_CRainmeter_ShowContextMenu00);
   tolua_function(tolua_S,"GetTrayExecuteL",tolua_rainmeter_CRainmeter_GetTrayExecuteL00);
   tolua_function(tolua_S,"GetTrayExecuteR",tolua_rainmeter_CRainmeter_GetTrayExecuteR00);
   tolua_function(tolua_S,"GetTrayExecuteM",tolua_rainmeter_CRainmeter_GetTrayExecuteM00);
   tolua_function(tolua_S,"GetTrayExecuteDR",tolua_rainmeter_CRainmeter_GetTrayExecuteDR00);
   tolua_function(tolua_S,"GetTrayExecuteDL",tolua_rainmeter_CRainmeter_GetTrayExecuteDL00);
   tolua_function(tolua_S,"GetTrayExecuteDM",tolua_rainmeter_CRainmeter_GetTrayExecuteDM00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_rainmeter (lua_State* tolua_S) {
 return tolua_rainmeter_open(tolua_S);
};
#endif

