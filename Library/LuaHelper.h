// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#ifndef __LUAHELPER_H__
#define __LUAHELPER_H__

#include <string>
#include <string_view>
#include <vector>

#include "../Common/StringBuffer.h"

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

	// Buffers for strings crossing the Lua boundary. 512 characters covers essentially every string
	// a skin passes across it; anything longer spills to the heap.
	using WideBuffer = StringBuffer<WCHAR, 512>;
	using NarrowBuffer = StringBuffer<char, 512 * 2>;

	static void PushWide(std::wstring_view str);

	// Overload so that a null pointer stays valid input, as it was before.
	static void PushWide(const WCHAR* str) { PushWide(str ? std::wstring_view(str) : std::wstring_view()); }

	// The returned buffer converts implicitly to std::wstring_view and has c_str(), which covers
	// nearly every caller. Use ToWideString() when an owning, growable string is needed.
	static WideBuffer ToWide(int narg);
	static std::wstring ToWideString(int narg);

	static bool ToBool(int narg);

	static void StackDump();
};

#endif
