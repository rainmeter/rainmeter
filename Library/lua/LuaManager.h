#ifndef LUA_MANAGER_H
#define LUA_MANAGER_H

#include "lua.hpp"
#include "tolua++.h"

class LuaManager
{
public:

	static void Init();

	static void CleanUp();

	static lua_State* GetState() { return m_pState; }
	
	static void ReportErrors(lua_State * L);

	static void LuaLog(const char* format, ... );

protected:

	static bool m_bInitialized;

	static lua_State* m_pState;
};

#endif
