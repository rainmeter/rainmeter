// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#ifndef __LUAHELPER_H__
#define __LUAHELPER_H__

#include <vector>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaStateScope
{
public:
	static LuaStateScope* GetCurrent() { return c_ScopeStack.back(); }

	LuaStateScope(lua_State* state, bool unicode, int ref);
	~LuaStateScope();

	operator lua_State*() { return m_State; }
	lua_State* GetState() { return m_State; }

	int GetRef() { return m_Ref; }
	bool IsUnicode() { return m_Unicode; }

private:
	lua_State* m_State;
	int m_Ref;
	bool m_Unicode;

	static std::vector<LuaStateScope*> c_ScopeStack;
};

class LuaHelper
{
public:
	static void LogAndPopError();

	static void PushWide(const WCHAR* str);
	static void PushWide(const std::wstring& str);
	static std::wstring ToWide(int narg);
	static bool ToBool(int narg);

	static void StackDump();
};

#endif
