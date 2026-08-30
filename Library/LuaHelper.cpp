// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "../Common/StringUtil.h"
#include "LuaHelper.h"
#include "LuaScript.h"
#include "Logger.h"

std::vector<LuaStateScope*> LuaStateScope::c_ScopeStack;

LuaStateScope::LuaStateScope(lua_State* state, bool unicode, int ref) :
	m_State(state),
	m_Unicode(unicode),
	m_Ref(ref)
{
	c_ScopeStack.push_back(this);
}

LuaStateScope::~LuaStateScope()
{
	c_ScopeStack.pop_back();
}

void LuaHelper::LogAndPopError()
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	WideBuffer str;
	StringUtil::Widen(error, -1, currentScope->IsUnicode() ? CP_UTF8 : CP_ACP, str);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaHelper::PushWide(std::wstring_view str)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();

	NarrowBuffer narrowStr;
	StringUtil::Narrow(str.data(), (int)str.length(),
		currentScope->IsUnicode() ? CP_UTF8 : CP_ACP, narrowStr);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

LuaHelper::WideBuffer LuaHelper::ToWide(int narg)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	const int cp = currentScope->IsUnicode() ? CP_UTF8 : CP_ACP;

	return WideBuffer([&](WideBuffer& buffer)
	{
		StringUtil::Widen(str, (int)strLen, cp, buffer);
	});
}

std::wstring LuaHelper::ToWideString(int narg)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	return StringUtil::Widen(str, (int)strLen, currentScope->IsUnicode() ? CP_UTF8 : CP_ACP);
}

bool LuaHelper::ToBool(int narg)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	return lua_toboolean(L, narg);
}

void LuaHelper::StackDump()
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();

	const int cp = currentScope->IsUnicode() ? CP_UTF8 : CP_ACP;
	WideBuffer buffer;

	LogDebug(L"--------------- Lua Stack Dump Start ------------------");
	for (int i = lua_gettop(L); i > 0; --i)
	{
		int t = lua_type(L, i);
		switch (t)
		{
		case LUA_TSTRING:
			StringUtil::Widen(lua_tostring(L, i), -1, cp, buffer);
			LogDebugF(L"%d:'%s'", i, buffer.c_str());
			break;

		case LUA_TBOOLEAN:
			LogDebugF(L"%d: %s", i, lua_toboolean(L, i) ? L"true" : L"false");
			break;

		case LUA_TNUMBER:
			LogDebugF(L"%d: %g", i, lua_tonumber(L, i));
			break;

		default:
			StringUtil::Widen(lua_typename(L, t), -1, cp, buffer);
			LogDebugF(L"%d: %s", i, buffer.c_str());
			break;
		}
	}
	LogDebug(L"--------------- Lua Stack Dump Finished ---------------");
}
