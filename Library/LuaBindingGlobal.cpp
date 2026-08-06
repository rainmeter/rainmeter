// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "LuaHelper.h"
#include "LuaScript.h"
#include "Logger.h"
#include "../Common/StringUtil.h"
#include "../Common/FileUtil.h"

EXTERN_C int luaopen_utf8(lua_State* L);

int luaopen_rm(lua_State* L);

static int Print(lua_State* L)
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
		if (s == nullptr)
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}

		if (i > 1)
		{
			// Debug dialog List View doesn't like tabs, just use 4 spaces instead
			message += "    ";
		}

		message += s;

		// Pop result
		lua_pop(L, 1);
	}

	LogDebug(LuaStateScope::GetCurrent()->IsUnicode() ?
		StringUtil::WidenUTF8(message).c_str() : StringUtil::Widen(message).c_str());

	return 0;
}

// Modified version of luaB_dofile()
static int Dofile(lua_State* L)
{
	const auto currentRef = LuaStateScope::GetCurrent()->GetRef();
	const char* fname = luaL_optstring(L, 1, NULL);
	std::wstring path = LuaStateScope::GetCurrent()->IsUnicode() ? StringUtil::WidenUTF8(fname) : StringUtil::Widen(fname);

	int n = lua_gettop(L);

	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(path, &fileSize);
	if (!fileData)
	{
		LogErrorF(L"Script: Unable to open the file: %s", path.c_str());
		return 0;
	}

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	bool unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;

	std::wstring tmp = std::wstring(path, path.find_last_of(L'\\') + 1);
	std::string file = unicode ? StringUtil::NarrowUTF8(tmp) : StringUtil::Narrow(tmp);
	file.insert(0, "@");

	bool scriptLoaded = false;
	if (unicode)
	{
		const std::string utf8Data =
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (int)((fileSize - 2) / sizeof(WCHAR)));
		scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), file.c_str()) == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(L, (char*)fileData.get(), fileSize, file.c_str()) == 0;
	}

	if (scriptLoaded)
	{
		LuaStateScope script(L, unicode, currentRef);
		lua_rawgeti(L, LUA_GLOBALSINDEX, script.GetRef());
		lua_setfenv(L, -2);

		lua_call(L, 0, -1);
		return lua_gettop(L) - n;
	}
	else
	{
		LuaHelper::LogAndPopError();
	}

	return 0;
}

struct BuiltinModule
{
	const char* name;
	lua_CFunction open;
	bool unicodeOnly;
};

static const BuiltinModule g_BuiltinModules[] =
{
	// Both modules read and write UTF-8 encoded strings, which is what the strings of a Unicode
	// script are. In an ANSI script they are in the active code page instead.
	{ "rm", luaopen_rm, true },
	{ "utf8", luaopen_utf8, true }
};

// Loads one of the modules built into Rainmeter. The package library is not available, so this is
// not the Lua loader: nothing is searched for on disk and nothing outside the table above loads.
// The modules already loaded are kept in the table upvalue.
static int Require(lua_State* L)
{
	const char* name = luaL_checkstring(L, 1);

	// Stack: [module]
	lua_getfield(L, lua_upvalueindex(1), name);
	if (!lua_isnil(L, -1))
	{
		return 1;
	}

	// Stack: []
	lua_pop(L, 1);

	for (const auto& module : g_BuiltinModules)
	{
		if (strcmp(module.name, name) != 0) continue;

		if (module.unicodeOnly && !LuaStateScope::GetCurrent()->IsUnicode())
		{
			return luaL_error(L, "module " LUA_QS " requires a Unicode script", name);
		}

		// Stack: [openFunction]
		lua_pushcfunction(L, module.open);

		// Stack: [module]
		lua_call(L, 0, 1);

		// The open function of a Lua 5.1 module also stores it in the global table. Remove it
		// from there so that require() is the only way to reach it.
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, name);

		// Stack: [module, module]
		lua_pushvalue(L, -1);

		// Stack: [module]
		lua_setfield(L, lua_upvalueindex(1), name);

		return 1;
	}

	return luaL_error(L, "module " LUA_QS " not found", name);
}

static int tolua_cast(lua_State* L)
{
	// Simply push first argument onto stack.
	lua_pushvalue(L, 1);
	return 1;
}

void LuaScript::RegisterGlobal(lua_State* L)
{
	lua_register(L, "print", Print);
	lua_register(L, "dofile", Dofile);

	// Stack: [loadedModules]
	lua_newtable(L);

	// Stack: [require]
	lua_pushcclosure(L, Require, 1);

	// Stack: []
	lua_setglobal(L, "require");

	// Register tolua.cast() for backwards compatibility.
	const luaL_Reg toluaFuncs[] =
	{
		{ "cast", tolua_cast },
		{ nullptr, nullptr }
	};

	luaL_register(L, "tolua", toluaFuncs);
}
