/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../../Common/StringUtil.h"
#include "LuaHelper.h"
#include "../Logger.h"
#include "LuaScript.h"

void LuaHelper::ReportErrors(lua_State* L, const std::wstring& file)
{
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	// Skip "[string ...]".
	const char* pos = strchr(error, ':');
	if (pos)
	{
		error = pos;
	}

	std::wstring str(file, file.find_last_of(L'\\') + 1);
	str += true ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaHelper::PushWide(lua_State* L, const WCHAR* str)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		const std::string narrowStr = activeScript->IsUnicode() ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
		lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
	}
	else
	{
		LogDebug(L"No active script, could not determine if file is unicode. Could not push wide string to stack!");
	}
}

void LuaHelper::PushWide(lua_State* L, const std::wstring& str)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		const std::string narrowStr = activeScript->IsUnicode() ?
			StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
		lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
	}
	else
	{
		LogDebug(L"No active script, could not determine if file is unicode. Could not push wide string to stack!");
	}
}

std::wstring LuaHelper::ToWide(lua_State* L,  int narg)
{
	LuaScript* activeScript = LuaScript::GetActiveScript();
	if (activeScript) {
		size_t strLen = 0;
		const char* str = lua_tolstring(L, narg, &strLen);
		bool unicode = activeScript->IsUnicode();
		return unicode ?
			StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);
	}
	else
	{
		LogDebug(L"No active script, could not get wide string from stack!");
	}
}
