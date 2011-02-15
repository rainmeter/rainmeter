/*
** Lua binding: meter_string
** Generated automatically by tolua++-1.0.92 on 02/15/11 03:01:13.
*/

#include "../../StdAfx.h"

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_meter_string_open (lua_State* tolua_S);

#include "../../MeterString.h"
#include "../LuaPush.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CMeterString");
 tolua_usertype(tolua_S,"WCHAR");
 tolua_usertype(tolua_S,"CMeter");
}

/* method: GetX of class  CMeterString */
#ifndef TOLUA_DISABLE_tolua_meter_string_CMeterString_GetX00
static int tolua_meter_string_CMeterString_GetX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterString",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterString* self = (CMeterString*)  tolua_tousertype(tolua_S,1,0);
  bool abs = ((bool)  tolua_toboolean(tolua_S,2,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetX'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetX(abs);
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

/* method: Update of class  CMeterString */
#ifndef TOLUA_DISABLE_tolua_meter_string_CMeterString_Update00
static int tolua_meter_string_CMeterString_Update00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterString",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterString* self = (CMeterString*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Update'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->Update();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Update'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetText of class  CMeterString */
#ifndef TOLUA_DISABLE_tolua_meter_string_CMeterString_SetText00
static int tolua_meter_string_CMeterString_SetText00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterString",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterString* self = (CMeterString*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* text = ((const WCHAR*)  to_wchar(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetText'", NULL);
#endif
  {
   self->SetText(text);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetText'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE


/* method: GetRect of class  CMeterString */
#ifndef TOLUA_DISABLE_tolua_meter_string_CMeterString_GetRect00
static int tolua_meter_string_CMeterString_GetRect00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterString",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterString* self = (CMeterString*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetRect'", NULL);
#endif
  {
	  Gdiplus::RectF rect = self->GetRect();

	  lua_newtable(tolua_S);
	  lua_pushstring(tolua_S, "x");
	  lua_pushnumber(tolua_S, rect.X);
	  lua_settable(tolua_S, -3);

	  lua_pushstring(tolua_S, "y");
	  lua_pushnumber(tolua_S, rect.Y);
	  lua_settable(tolua_S, -3);

	  lua_pushstring(tolua_S, "width");
	  lua_pushnumber(tolua_S, rect.Width);
	  lua_settable(tolua_S, -3);

	  lua_pushstring(tolua_S, "height");
	  lua_pushnumber(tolua_S, rect.Height);
	  lua_settable(tolua_S, -3);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetRect'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE


/* Open function */
TOLUA_API int tolua_meter_string_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_cclass(tolua_S,"CMeterString","CMeterString","CMeter",NULL);
  tolua_beginmodule(tolua_S,"CMeterString");
   tolua_function(tolua_S,"GetX",tolua_meter_string_CMeterString_GetX00);
   tolua_function(tolua_S,"Update",tolua_meter_string_CMeterString_Update00);
   tolua_function(tolua_S,"SetText",tolua_meter_string_CMeterString_SetText00);
   tolua_function(tolua_S,"GetRect",tolua_meter_string_CMeterString_GetRect00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_meter_string (lua_State* tolua_S) {
 return tolua_meter_string_open(tolua_S);
};
#endif

