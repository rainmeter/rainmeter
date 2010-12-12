/*
** Lua binding: meter_image
** Generated automatically by tolua++-1.0.92 on 11/28/10 09:56:41.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int  tolua_meter_image_open (lua_State* tolua_S);

#include "../../MeterImage.h"
#include "../LuaPush.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"CMeterImage");
 tolua_usertype(tolua_S,"WCHAR");
 tolua_usertype(tolua_S,"CMeter");
}

/* method: SetImage of class  CMeterImage */
#ifndef TOLUA_DISABLE_tolua_meter_image_CMeterImage_SetImage00
static int tolua_meter_image_CMeterImage_SetImage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterImage",0,&tolua_err) ||
     !is_wchar(tolua_S,2,"const WCHAR",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterImage* self = (CMeterImage*)  tolua_tousertype(tolua_S,1,0);
  const WCHAR* imageName = ((const WCHAR*)  to_wchar(tolua_S,2,0));
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'SetImage'", NULL);
#endif
  {
   self->SetImage(imageName);
  }
 }
 return 0;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'SetImage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* method: GetImage of class  CMeterImage */
#ifndef TOLUA_DISABLE_tolua_meter_image_CMeterImage_GetImage00
static int tolua_meter_image_CMeterImage_GetImage00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
     !tolua_isusertype(tolua_S,1,"CMeterImage",0,&tolua_err) ||
     !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
  goto tolua_lerror;
 else
#endif
 {
  CMeterImage* self = (CMeterImage*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
  if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetImage'", NULL);
#endif
  {
   const WCHAR* tolua_ret = (const WCHAR*)  self->GetImage();
    push_wchar(tolua_S,(void*)tolua_ret,"const WCHAR");
  }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetImage'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_meter_image_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
  tolua_cclass(tolua_S,"CMeterImage","CMeterImage","CMeter",NULL);
  tolua_beginmodule(tolua_S,"CMeterImage");
   tolua_function(tolua_S,"SetImage",tolua_meter_image_CMeterImage_SetImage00);
   tolua_function(tolua_S,"GetImage",tolua_meter_image_CMeterImage_GetImage00);
  tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_meter_image (lua_State* tolua_S) {
 return tolua_meter_image_open(tolua_S);
};
#endif

