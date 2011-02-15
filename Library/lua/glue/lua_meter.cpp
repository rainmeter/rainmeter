/*
** Lua binding: meter
** Generated automatically by tolua++-1.0.92 on 02/15/11 18:55:46.
*/

#include "../../StdAfx.h"

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_meter_open (lua_State* tolua_S);

#include "../../Meter.h"
#include "../LuaPush.h"

/* function to release collected object via destructor */
#ifdef __cplusplus

static int tolua_collect_std__wstring (lua_State* tolua_S)
{
 std::wstring* self = (std::wstring*) tolua_tousertype(tolua_S,1,0);
	Mtolua_delete(self);
	return 0;
}
#endif


/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"std::wstring");
 tolua_usertype(tolua_S,"CMeter");
 tolua_usertype(tolua_S,"Gdiplus::Matrix");
 tolua_usertype(tolua_S,"CGroup");
 tolua_usertype(tolua_S,"WCHAR");
}

/* method: Update of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_Update00
static int tolua_meter_CMeter_Update00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* method: HasActiveTransition of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HasActiveTransition00
static int tolua_meter_CMeter_HasActiveTransition00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasActiveTransition'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasActiveTransition();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasActiveTransition'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasDynamicVariables of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HasDynamicVariables00
static int tolua_meter_CMeter_HasDynamicVariables00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* method: GetW of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetW00
static int tolua_meter_CMeter_GetW00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* method: GetH of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetH00
static int tolua_meter_CMeter_GetH00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* method: GetX of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetX00
static int tolua_meter_CMeter_GetX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* method: GetY of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetY00
static int tolua_meter_CMeter_GetY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,1,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  bool abs = ((bool)  tolua_toboolean(tolua_S,2,false));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetY'", NULL);
#endif
  {
   int tolua_ret = (int)  self->GetY(abs);
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

/* method: SetW of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetW00
static int tolua_meter_CMeter_SetW00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  int w = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetW'", NULL);
#endif
  {
   self->SetW(w);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetW'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetH of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetH00
static int tolua_meter_CMeter_SetH00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  int h = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetH'", NULL);
#endif
  {
   self->SetH(h);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetH'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetX of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetX00
static int tolua_meter_CMeter_SetX00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  int x = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetX'", NULL);
#endif
  {
   self->SetX(x);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetX'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetY of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetY00
static int tolua_meter_CMeter_SetY00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  int y = ((int)  tolua_tonumber(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetY'", NULL);
#endif
  {
   self->SetY(y);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetY'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetToolTipText of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetToolTipText00
static int tolua_meter_CMeter_GetToolTipText00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetToolTipText'", NULL);
#endif
  {
   const std::wstring& tolua_ret = (const std::wstring&)  self->GetToolTipText();
    push_wstring(tolua_S,(void*)&tolua_ret,"const std::wstring");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetToolTipText'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasToolTip of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HasToolTip00
static int tolua_meter_CMeter_HasToolTip00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasToolTip'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasToolTip();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasToolTip'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetToolTipHidden of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetToolTipHidden00
static int tolua_meter_CMeter_SetToolTipHidden00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  bool b = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetToolTipHidden'", NULL);
#endif
  {
   self->SetToolTipHidden(b);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetToolTipHidden'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: UpdateToolTip of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_UpdateToolTip00
static int tolua_meter_CMeter_UpdateToolTip00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'UpdateToolTip'", NULL);
#endif
  {
   self->UpdateToolTip();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'UpdateToolTip'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasMouseAction of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HasMouseAction00
static int tolua_meter_CMeter_HasMouseAction00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasMouseAction'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasMouseAction();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasMouseAction'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HasMouseActionCursor of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HasMouseActionCursor00
static int tolua_meter_CMeter_HasMouseActionCursor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HasMouseActionCursor'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HasMouseActionCursor();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HasMouseActionCursor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: SetMouseActionCursor of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_SetMouseActionCursor00
static int tolua_meter_CMeter_SetMouseActionCursor00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  bool b = ((bool)  tolua_toboolean(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetMouseActionCursor'", NULL);
#endif
  {
   self->SetMouseActionCursor(b);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetMouseActionCursor'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Hide of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_Hide00
static int tolua_meter_CMeter_Hide00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Hide'", NULL);
#endif
  {
   self->Hide();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Hide'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: Show of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_Show00
static int tolua_meter_CMeter_Show00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'Show'", NULL);
#endif
  {
   self->Show();
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Show'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsHidden of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_IsHidden00
static int tolua_meter_CMeter_IsHidden00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsHidden'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsHidden();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsHidden'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetTransformationMatrix of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetTransformationMatrix00
static int tolua_meter_CMeter_GetTransformationMatrix00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetTransformationMatrix'", NULL);
#endif
  {
   const Gdiplus::Matrix& tolua_ret = (const Gdiplus::Matrix&)  self->GetTransformationMatrix();
    tolua_pushusertype(tolua_S,(void*)&tolua_ret,"const Gdiplus::Matrix");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetTransformationMatrix'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: HitTest of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_HitTest00
static int tolua_meter_CMeter_HitTest00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
     !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
  int x = ((int)  tolua_tonumber(tolua_S,2,0));
  int y = ((int)  tolua_tonumber(tolua_S,3,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'HitTest'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->HitTest(x,y);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'HitTest'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: IsMouseOver of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_IsMouseOver00
static int tolua_meter_CMeter_IsMouseOver00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'IsMouseOver'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->IsMouseOver();
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'IsMouseOver'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetName of class  CMeter */
#ifndef TOLUA_DISABLE_tolua_meter_CMeter_GetName00
static int tolua_meter_CMeter_GetName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeter",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeter* self = (CMeter*)  tolua_tousertype(tolua_S,1,0);
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

/* Open function */
TOLUA_API int tolua_meter_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_cclass(tolua_S,"CMeter","CMeter","CGroup",NULL);
  tolua_beginmodule(tolua_S,"CMeter");
   tolua_function(tolua_S,"Update",tolua_meter_CMeter_Update00);
   tolua_function(tolua_S,"HasActiveTransition",tolua_meter_CMeter_HasActiveTransition00);
   tolua_function(tolua_S,"HasDynamicVariables",tolua_meter_CMeter_HasDynamicVariables00);
   tolua_function(tolua_S,"GetW",tolua_meter_CMeter_GetW00);
   tolua_function(tolua_S,"GetH",tolua_meter_CMeter_GetH00);
   tolua_function(tolua_S,"GetX",tolua_meter_CMeter_GetX00);
   tolua_function(tolua_S,"GetY",tolua_meter_CMeter_GetY00);
   tolua_function(tolua_S,"SetW",tolua_meter_CMeter_SetW00);
   tolua_function(tolua_S,"SetH",tolua_meter_CMeter_SetH00);
   tolua_function(tolua_S,"SetX",tolua_meter_CMeter_SetX00);
   tolua_function(tolua_S,"SetY",tolua_meter_CMeter_SetY00);
   tolua_function(tolua_S,"GetToolTipText",tolua_meter_CMeter_GetToolTipText00);
   tolua_function(tolua_S,"HasToolTip",tolua_meter_CMeter_HasToolTip00);
   tolua_function(tolua_S,"SetToolTipHidden",tolua_meter_CMeter_SetToolTipHidden00);
   tolua_function(tolua_S,"UpdateToolTip",tolua_meter_CMeter_UpdateToolTip00);
   tolua_function(tolua_S,"HasMouseAction",tolua_meter_CMeter_HasMouseAction00);
   tolua_function(tolua_S,"HasMouseActionCursor",tolua_meter_CMeter_HasMouseActionCursor00);
   tolua_function(tolua_S,"SetMouseActionCursor",tolua_meter_CMeter_SetMouseActionCursor00);
   tolua_function(tolua_S,"Hide",tolua_meter_CMeter_Hide00);
   tolua_function(tolua_S,"Show",tolua_meter_CMeter_Show00);
   tolua_function(tolua_S,"IsHidden",tolua_meter_CMeter_IsHidden00);
   tolua_function(tolua_S,"GetTransformationMatrix",tolua_meter_CMeter_GetTransformationMatrix00);
   tolua_function(tolua_S,"HitTest",tolua_meter_CMeter_HitTest00);
   tolua_function(tolua_S,"IsMouseOver",tolua_meter_CMeter_IsMouseOver00);
   tolua_function(tolua_S,"GetName",tolua_meter_CMeter_GetName00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_meter (lua_State* tolua_S) {
 return tolua_meter_open(tolua_S);
};
#endif

