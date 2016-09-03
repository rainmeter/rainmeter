/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../../Common/StringUtil.h"
#include "../../Common/FileUtil.h"
#include "LuaHelper.h"
#include "../Logger.h"
#include "LuaScript.h"

bool LuaHelper::LoadFile(lua_State * L, const std::wstring & file)
{
	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(file, &fileSize);
	if (!fileData)
	{
		return false;
	}

	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	bool unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	if (unicode)
	{
		const std::string utf8Data =
			StringUtil::NarrowUTF8((WCHAR*)(fileData.get() + 2), (fileSize - 2) / sizeof(WCHAR));
		scriptLoaded = luaL_loadbuffer(L, utf8Data.c_str(), utf8Data.length(), "") == 0;
	}
	else
	{
		scriptLoaded = luaL_loadbuffer(L, (char*)fileData.get(), fileSize, "") == 0;
	}
	return scriptLoaded;
}

int LuaHelper::RunFile(lua_State* L, const std::wstring& file, int n)
{
	if (LoadFile(L, file))
	{
		// Execute the Lua script
		int result = lua_pcall(L, 0, 0, 0);

		if (result == 0)
		{
			return lua_gettop(L) - n;
		}
		else
		{
			LuaHelper::ReportErrors(L, file);
		}
	}
	else
	{
		LuaHelper::ReportErrors(L, file);
	}
	return 0;
}

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
