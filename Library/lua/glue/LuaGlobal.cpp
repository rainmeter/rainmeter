#include "../../StdAfx.h"
#include "../LuaManager.h"
#include "../../Litestep.h"

static int Global_Log(lua_State* L)
{
	// Modified version of luaB_print()
	std::string message;

	int n = lua_gettop(L);		// Number of arguments
	lua_getglobal(L, "tostring");

	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1);	// Function to be called
		lua_pushvalue(L, i);	// Value to print
		lua_call(L, 1, 1);

		// Get result
		const char* s = lua_tostring(L, -1);
		if (s == NULL)
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}

		if (i > 1)
		{
			// About dialog List View doesn't like tabs, just use 4 spaces instead
			message += "    ";
		}

		message += s;

		// Pop result
		lua_pop(L, 1);
	}

	LuaManager::LuaLog(LOG_DEBUG, message.c_str());
	return 0;
}

static const luaL_reg TO_funcs[] =
{
	{ "LuaLog", Global_Log },
	{ NULL, NULL }
};

void LuaManager::RegisterGlobal(lua_State* L)
{
	lua_register(L, "print", Global_Log);
	luaL_register(L, "TO", TO_funcs);	// For backwards compatibility
}
