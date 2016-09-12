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

std::vector<LuaHelper::UnicodeScript*> LuaHelper::c_ScriptStack;

LuaHelper::UnicodeScript::UnicodeScript(lua_State* state, bool unicode, int ref, std::wstring path) :
	m_State(state),
	m_Unicode(unicode),
	m_Ref(ref),
	m_File(path)
{
	LuaHelper::c_ScriptStack.push_back(this);
}

LuaHelper::UnicodeScript::~UnicodeScript()
{
	LuaHelper::c_ScriptStack.pop_back();
}

void LuaHelper::ReportErrors(const std::wstring& file)
{
	auto script = GetCurrentScript();
	lua_State* L = script->GetState();
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Skip "[string ...]".
	const char* pos = strchr(error, ':');
	if (pos)
	{
		error = pos;
	}

	std::wstring str(file, file.find_last_of(L'\\') + 1);
	str += script->IsUnicode() ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaHelper::PushWide(const WCHAR* str)
{
	auto script = GetCurrentScript();
	lua_State* L = script->GetState();
	const std::string narrowStr = script->IsUnicode() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

void LuaHelper::PushWide(const std::wstring& str)
{
	auto script = GetCurrentScript();
	lua_State* L = script->GetState();
	const std::string narrowStr = script->IsUnicode() ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

std::wstring LuaHelper::ToWide(int narg)
{
	auto script = GetCurrentScript();
	lua_State* L = script->GetState();
	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	return script->IsUnicode() ?
		StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
}
