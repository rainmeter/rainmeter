#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Litestep.h"

static int Global_Log(lua_State* L)
{
	const char* str = tolua_tostring(L, 1, 0);
	LuaManager::LuaLog(LOG_NOTICE, str);

	return 0;
}

static const luaL_reg TO_funcs[] =
{
	{ "LuaLog", Global_Log }, { NULL, NULL }
};

void LuaManager::RegisterGlobal(lua_State* L)
{
	lua_register(L, "print", Global_Log);
	luaL_register(L, "TO", TO_funcs);	// For backwards compatibility
}


