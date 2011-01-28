/*
** Lua binding: rainmeter
** Generated automatically by tolua++-1.0.92 on 11/23/10 01:56:47.
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
 tolua_usertype(tolua_S,"CMeterWindow");
 tolua_usertype(tolua_S,"BOOL");
 tolua_usertype(tolua_S,"CTrayWindow");
 tolua_usertype(tolua_S,"CRainmeter");
 tolua_usertype(tolua_S,"POINT");
 tolua_usertype(tolua_S,"WCHAR");
}

/* method: GetTrayWindow of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_GetTrayWindow00
static int tolua_rainmeter_CRainmeter_GetTrayWindow00(lua_State* tolua_S)
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
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTrayWindow'", NULL);
#endif
  {
   CTrayWindow* tolua_ret = (CTrayWindow*)  self->GetTrayWindow();
    tolua_pushusertype(tolua_S,(void*)tolua_ret,"CTrayWindow");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTrayWindow'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

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
  const std::wstring config = to_wstring(tolua_S,2,0);
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
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
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

/* method: ReloadSettings of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ReloadSettings00
static int tolua_rainmeter_CRainmeter_ReloadSettings00(lua_State* tolua_S)
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
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ReloadSettings'", NULL);
#endif
  {
   self->ReloadSettings();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ReloadSettings'.",&tolua_err);
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

/* method: ReadStats of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ReadStats00
static int tolua_rainmeter_CRainmeter_ReadStats00(lua_State* tolua_S)
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
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ReadStats'", NULL);
#endif
  {
   self->ReadStats();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ReadStats'.",&tolua_err);
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteL();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteR();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteM();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteDR();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteDL();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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
   std::wstring tolua_ret = (std::wstring)  self->GetTrayExecuteDM();
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
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

/* method: ExecuteBang of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ExecuteBang00
static int tolua_rainmeter_CRainmeter_ExecuteBang00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     (tolua_isvaluenil(tolua_S,3,&tolua_err) || !is_wstring(tolua_S,3,"const std::wstring",0,&tolua_err)) ||
     !tolua_isusertype(tolua_S,4,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring* bang =  &to_wstring(tolua_S,2,0);
  const std::wstring* arg =   &to_wstring(tolua_S,3,0);
  CMeterWindow* meterWindow = ((CMeterWindow*)  tolua_tousertype(tolua_S,4,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ExecuteBang'", NULL);
#endif
  {
   BOOL tolua_ret = (BOOL)  self->ExecuteBang(*bang,*arg,meterWindow);
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((BOOL)(tolua_ret));
     tolua_pushusertype(tolua_S,tolua_obj,"BOOL");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(BOOL));
     tolua_pushusertype(tolua_S,tolua_obj,"BOOL");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ExecuteBang'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ParseCommand of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ParseCommand00
static int tolua_rainmeter_CRainmeter_ParseCommand00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,3,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* command = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  CMeterWindow* meterWindow = ((CMeterWindow*)  tolua_tousertype(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ParseCommand'", NULL);
#endif
  {
   std::wstring tolua_ret = (std::wstring)  self->ParseCommand(command,meterWindow);
   {
#ifdef __cplusplus
    void* tolua_obj = Mtolua_new((std::wstring)(tolua_ret));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#else
    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(std::wstring));
     push_wstring(tolua_S,tolua_obj,"std::wstring");
    tolua_register_gc(tolua_S,lua_gettop(tolua_S));
#endif
   }
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ParseCommand'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ExecuteCommand of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ExecuteCommand00
static int tolua_rainmeter_CRainmeter_ExecuteCommand00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CRainmeter",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isusertype(tolua_S,3,"CMeterWindow",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CRainmeter* self = (CRainmeter*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* command = ((const WCHAR*)  to_wchar(tolua_S,2,0));
  CMeterWindow* meterWindow = ((CMeterWindow*)  tolua_tousertype(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ExecuteCommand'", NULL);
#endif
  {
   self->ExecuteCommand(command,meterWindow);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ExecuteCommand'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: RefreshAll of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_RefreshAll00
static int tolua_rainmeter_CRainmeter_RefreshAll00(lua_State* tolua_S)
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
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'RefreshAll'", NULL);
#endif
  {
   self->RefreshAll();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'RefreshAll'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ClearDeleteLaterList of class  CRainmeter */
#ifndef TOLUA_DISABLE_tolua_rainmeter_CRainmeter_ClearDeleteLaterList00
static int tolua_rainmeter_CRainmeter_ClearDeleteLaterList00(lua_State* tolua_S)
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
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ClearDeleteLaterList'", NULL);
#endif
  {
   self->ClearDeleteLaterList();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ClearDeleteLaterList'.",&tolua_err);
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
   tolua_function(tolua_S,"GetTrayWindow",tolua_rainmeter_CRainmeter_GetTrayWindow00);
   tolua_function(tolua_S,"GetMeterWindow",tolua_rainmeter_CRainmeter_GetMeterWindow00);
   tolua_function(tolua_S,"GetPath",tolua_rainmeter_CRainmeter_GetPath00);
   tolua_function(tolua_S,"GetIniFile",tolua_rainmeter_CRainmeter_GetIniFile00);
   tolua_function(tolua_S,"GetLogFile",tolua_rainmeter_CRainmeter_GetLogFile00);
   tolua_function(tolua_S,"GetSkinPath",tolua_rainmeter_CRainmeter_GetSkinPath00);
   tolua_function(tolua_S,"GetPluginPath",tolua_rainmeter_CRainmeter_GetPluginPath00);
   tolua_function(tolua_S,"GetSettingsPath",tolua_rainmeter_CRainmeter_GetSettingsPath00);
   tolua_function(tolua_S,"GetConfigEditor",tolua_rainmeter_CRainmeter_GetConfigEditor00);
   tolua_function(tolua_S,"GetLogViewer",tolua_rainmeter_CRainmeter_GetLogViewer00);
   tolua_function(tolua_S,"GetStatsDate",tolua_rainmeter_CRainmeter_GetStatsDate00);
   tolua_function(tolua_S,"GetDebug",tolua_rainmeter_CRainmeter_GetDebug00);
   tolua_function(tolua_S,"ReloadSettings",tolua_rainmeter_CRainmeter_ReloadSettings00);
   tolua_function(tolua_S,"SaveSettings",tolua_rainmeter_CRainmeter_SaveSettings00);
   tolua_function(tolua_S,"ReadStats",tolua_rainmeter_CRainmeter_ReadStats00);
   tolua_function(tolua_S,"WriteStats",tolua_rainmeter_CRainmeter_WriteStats00);
   tolua_function(tolua_S,"ResetStats",tolua_rainmeter_CRainmeter_ResetStats00);
   tolua_function(tolua_S,"GetLogging",tolua_rainmeter_CRainmeter_GetLogging00);
   tolua_function(tolua_S,"StartLogging",tolua_rainmeter_CRainmeter_StartLogging00);
   tolua_function(tolua_S,"StopLogging",tolua_rainmeter_CRainmeter_StopLogging00);
   tolua_function(tolua_S,"DeleteLogFile",tolua_rainmeter_CRainmeter_DeleteLogFile00);
   tolua_function(tolua_S,"SetDebug",tolua_rainmeter_CRainmeter_SetDebug00);
   tolua_function(tolua_S,"IsMenuActive",tolua_rainmeter_CRainmeter_IsMenuActive00);
   tolua_function(tolua_S,"ShowContextMenu",tolua_rainmeter_CRainmeter_ShowContextMenu00);
   tolua_function(tolua_S,"GetTrayExecuteL",tolua_rainmeter_CRainmeter_GetTrayExecuteL00);
   tolua_function(tolua_S,"GetTrayExecuteR",tolua_rainmeter_CRainmeter_GetTrayExecuteR00);
   tolua_function(tolua_S,"GetTrayExecuteM",tolua_rainmeter_CRainmeter_GetTrayExecuteM00);
   tolua_function(tolua_S,"GetTrayExecuteDR",tolua_rainmeter_CRainmeter_GetTrayExecuteDR00);
   tolua_function(tolua_S,"GetTrayExecuteDL",tolua_rainmeter_CRainmeter_GetTrayExecuteDL00);
   tolua_function(tolua_S,"GetTrayExecuteDM",tolua_rainmeter_CRainmeter_GetTrayExecuteDM00);
   tolua_function(tolua_S,"ExecuteBang",tolua_rainmeter_CRainmeter_ExecuteBang00);
   tolua_function(tolua_S,"ParseCommand",tolua_rainmeter_CRainmeter_ParseCommand00);
   tolua_function(tolua_S,"ExecuteCommand",tolua_rainmeter_CRainmeter_ExecuteCommand00);
   tolua_function(tolua_S,"RefreshAll",tolua_rainmeter_CRainmeter_RefreshAll00);
   tolua_function(tolua_S,"ClearDeleteLaterList",tolua_rainmeter_CRainmeter_ClearDeleteLaterList00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_rainmeter (lua_State* tolua_S) {
 return tolua_rainmeter_open(tolua_S);
};
#endif

