#ifndef LUA_MANAGER_H
#define LUA_MANAGER_H

#include "lua.hpp"
#include "tolua++.h"

class LuaManager
{
public:

	static void Init();

	static void CleanUp();

	static lua_State* GetState() { return c_pState; }
	
	static void ReportErrors(lua_State * L);

	static void LuaLog(int nLevel, const char* format, ... );

protected:

	static int c_RefCount;

	static lua_State* c_pState;
};

#endif
