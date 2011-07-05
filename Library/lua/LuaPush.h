#ifndef __LUAPUSH_H__
#define __LUAPUSH_H__

#include <stdio.h>
#include <string>

#include "lua.hpp"
#include "tolua++.h"

std::wstring to_wstring(lua_State* L, int arg, void* type);
void push_wstring(lua_State* L, const std::wstring& value);
int is_wstring(lua_State* L, int lo, const char* type, int def, tolua_Error* err);

void push_wchar(lua_State* L, const WCHAR* value);
const WCHAR* to_wchar(lua_State* L, int arg, void* type);
int is_wchar(lua_State* L, int lo, const char* type, int def, tolua_Error* err);

#endif
