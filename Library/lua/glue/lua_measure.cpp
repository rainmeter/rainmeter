/*
** Lua binding: measure
** Generated automatically by tolua++-1.0.92 on 01/19/11 04:59:42.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_measure_open (lua_State* tolua_S);

#include "../../MeterWindow.h"
#include "../../Litestep.h"
#include "../../Group.h"
#include "../../Measure.h"
#include "../LuaPush.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CGroup");
 tolua_usertype(tolua_S,"CMeasure");
 tolua_usertype(tolua_S,"WCHAR");
}

/* method: SetName of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_SetName00
static int tolua_measure_CMeasure_SetName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* name = ((const WCHAR*)  to_wchar(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetName'", NULL);
#endif
  {
   self->SetName(name);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetANSIName of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetANSIName00
static int tolua_measure_CMeasure_GetANSIName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetANSIName'", NULL);
#endif
  {
   const char* tolua_ret = (const char*)  self->GetANSIName();
   tolua_pushstring(tolua_S,(const char*)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetANSIName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Disable of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_Disable00
static int tolua_measure_CMeasure_Disable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Disable'", NULL);
#endif
  {
   self->Disable();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Disable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Enable of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_Enable00
static int tolua_measure_CMeasure_Enable00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Enable'", NULL);
#endif
  {
   self->Enable();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Enable'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsDisabled of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_IsDisabled00
static int tolua_measure_CMeasure_IsDisabled00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsDisabled'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsDisabled();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsDisabled'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetName of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetName00
static int tolua_measure_CMeasure_GetName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetName'", NULL);
#endif
  {
   const WCHAR* tolua_ret = (const WCHAR*)  self->GetName();
    push_wchar(tolua_S,(void*)tolua_ret,"const WCHAR");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasDynamicVariables of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_HasDynamicVariables00
static int tolua_measure_CMeasure_HasDynamicVariables00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasDynamicVariables'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasDynamicVariables();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasDynamicVariables'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: ExecuteBang of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_ExecuteBang00
static int tolua_measure_CMeasure_ExecuteBang00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* args = ((const WCHAR*)  to_wchar(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'ExecuteBang'", NULL);
#endif
  {
   self->ExecuteBang(args);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'ExecuteBang'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetValue of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetValue00
static int tolua_measure_CMeasure_GetValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetValue'", NULL);
#endif
  {
   double tolua_ret = (double)  self->GetValue();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetRelativeValue of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetRelativeValue00
static int tolua_measure_CMeasure_GetRelativeValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetRelativeValue'", NULL);
#endif
  {
   double tolua_ret = (double)  self->GetRelativeValue();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetRelativeValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetValueRange of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetValueRange00
static int tolua_measure_CMeasure_GetValueRange00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetValueRange'", NULL);
#endif
  {
   double tolua_ret = (double)  self->GetValueRange();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetValueRange'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetMinValue of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetMinValue00
static int tolua_measure_CMeasure_GetMinValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetMinValue'", NULL);
#endif
  {
   double tolua_ret = (double)  self->GetMinValue();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetMinValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetMaxValue of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetMaxValue00
static int tolua_measure_CMeasure_GetMaxValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetMaxValue'", NULL);
#endif
  {
   double tolua_ret = (double)  self->GetMaxValue();
   tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetMaxValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetStringValue of class  CMeasure */
#ifndef TOLUA_DISABLE_tolua_measure_CMeasure_GetStringValue00
static int tolua_measure_CMeasure_GetStringValue00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeasure",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,1,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,1,&tolua_err) ||
     !tolua_isnumber(tolua_S,4,1,&tolua_err) ||
     !tolua_isboolean(tolua_S,5,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,6,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeasure* self = (CMeasure*)  tolua_tousertype(tolua_S,1,0);
  AUTOSCALE autoScale = ((AUTOSCALE) (int)  tolua_tonumber(tolua_S,2,0));
  double scale = ((double)  tolua_tonumber(tolua_S,3,1.0));
  int decimals = ((int)  tolua_tonumber(tolua_S,4,0));
  bool percentual = ((bool)  tolua_toboolean(tolua_S,5,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetStringValue'", NULL);
#endif
  {
   const WCHAR* tolua_ret = (const WCHAR*)  self->GetStringValue(autoScale,scale,decimals,percentual);
    push_wchar(tolua_S,(void*)tolua_ret,"const WCHAR");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetStringValue'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_measure_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_cclass(tolua_S,"CMeasure","CMeasure","CGroup",NULL);
  tolua_beginmodule(tolua_S,"CMeasure");
   tolua_function(tolua_S,"SetName",tolua_measure_CMeasure_SetName00);
   tolua_function(tolua_S,"GetANSIName",tolua_measure_CMeasure_GetANSIName00);
   tolua_function(tolua_S,"Disable",tolua_measure_CMeasure_Disable00);
   tolua_function(tolua_S,"Enable",tolua_measure_CMeasure_Enable00);
   tolua_function(tolua_S,"IsDisabled",tolua_measure_CMeasure_IsDisabled00);
   tolua_function(tolua_S,"GetName",tolua_measure_CMeasure_GetName00);
   tolua_function(tolua_S,"HasDynamicVariables",tolua_measure_CMeasure_HasDynamicVariables00);
   tolua_function(tolua_S,"ExecuteBang",tolua_measure_CMeasure_ExecuteBang00);
   tolua_function(tolua_S,"GetValue",tolua_measure_CMeasure_GetValue00);
   tolua_function(tolua_S,"GetRelativeValue",tolua_measure_CMeasure_GetRelativeValue00);
   tolua_function(tolua_S,"GetValueRange",tolua_measure_CMeasure_GetValueRange00);
   tolua_function(tolua_S,"GetMinValue",tolua_measure_CMeasure_GetMinValue00);
   tolua_function(tolua_S,"GetMaxValue",tolua_measure_CMeasure_GetMaxValue00);
   tolua_function(tolua_S,"GetStringValue",tolua_measure_CMeasure_GetStringValue00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_measure (lua_State* tolua_S) {
 return tolua_measure_open(tolua_S);
};
#endif

