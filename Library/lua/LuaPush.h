
#ifndef LUA_PUSH_H
#define LUA_PUSH_H

#include <stdio.h>
#include "string"

#include "lua.hpp"
#include "tolua++.h"

std::wstring to_wstring (lua_State* L, int arg, void* type);
void push_wstring (lua_State* L, void* value, const char* type);
int is_wstring (lua_State* L, int lo, const char* type, int def, tolua_Error* err);

void push_wchar   (lua_State* L, void* value, const char* type);
const wchar_t* to_wchar   (lua_State* L, int arg, void* type);
int is_wchar (lua_State* L, int lo, const char* type, int def, tolua_Error* err);

#endif

