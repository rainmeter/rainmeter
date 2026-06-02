/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../../Common/StringUtil.h"
#include "LuaHelper.h"
#include "LuaScript.h"
#include "../Logger.h"

std::vector<LuaStateScope*> LuaStateScope::c_ScopeStack;

LuaStateScope::LuaStateScope(lua_State* state, bool unicode, int ref, std::wstring path) :
	m_State(state),
	m_Unicode(unicode),
	m_Ref(ref),
	m_File(path)
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

	std::wstring str = currentScope->IsUnicode() ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaHelper::PushWide(const WCHAR* str)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	const std::string narrowStr = currentScope->IsUnicode() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

void LuaHelper::PushWide(const std::wstring& str)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	const std::string narrowStr = currentScope->IsUnicode() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

std::wstring LuaHelper::ToWide(int narg)
{
	auto currentScope = LuaStateScope::GetCurrent();
	lua_State* L = currentScope->GetState();
	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	return currentScope->IsUnicode() ?
		StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
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

	LogDebug(L"--------------- Lua Stack Dump Start ------------------");
	for (int i = lua_gettop(L); i > 0; --i)
	{
		int t = lua_type(L, i);
		switch (t)
		{
		case LUA_TSTRING:
			LogDebugF(L"%d:'%s'", i, currentScope->IsUnicode() ?
				StringUtil::WidenUTF8(lua_tostring(L, i)).c_str() :
				StringUtil::Widen(lua_tostring(L, i)).c_str());
			break;

		case LUA_TBOOLEAN:
			LogDebugF(L"%d: %s", i, lua_toboolean(L, i) ? L"true" : L"false");
			break;

		case LUA_TNUMBER:
			LogDebugF(L"%d: %g", i, lua_tonumber(L, i));
			break;

		default:
			LogDebugF(L"%d: %s", i, currentScope->IsUnicode() ?
				StringUtil::WidenUTF8(lua_typename(L, t)).c_str() :
				StringUtil::Widen(lua_typename(L, t)).c_str());
			break;
		}
	}
	LogDebug(L"--------------- Lua Stack Dump Finished ---------------");
}
