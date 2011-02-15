/*
** Lua binding: group
** Generated automatically by tolua++-1.0.92 on 02/15/11 03:01:14.
*/

#include "../../StdAfx.h"

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_group_open (lua_State* tolua_S);

#include "../../Group.h"
#include "../LuaPush.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CGroup");
 tolua_usertype(tolua_S,"std::wstring");
}

/* method: BelongsToGroup of class  CGroup */
#ifndef TOLUA_DISABLE_tolua_group_CGroup_BelongsToGroup00
static int tolua_group_CGroup_BelongsToGroup00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CGroup",0,&tolua_err) ||
     (tolua_isvaluenil(tolua_S,2,&tolua_err) || !is_wstring(tolua_S,2,"const std::wstring",0,&tolua_err)) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CGroup* self = (CGroup*)  tolua_tousertype(tolua_S,1,0);
  const std::wstring group = ((const std::wstring)  to_wstring(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'BelongsToGroup'", NULL);
#endif
  {
   bool tolua_ret = (bool)  self->BelongsToGroup(group);
   tolua_pushboolean(tolua_S,(bool)tolua_ret);
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'BelongsToGroup'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_group_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_cclass(tolua_S,"CGroup","CGroup","",NULL);
  tolua_beginmodule(tolua_S,"CGroup");
   tolua_function(tolua_S,"BelongsToGroup",tolua_group_CGroup_BelongsToGroup00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_group (lua_State* tolua_S) {
 return tolua_group_open(tolua_S);
};
#endif

