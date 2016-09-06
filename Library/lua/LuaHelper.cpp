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

std::vector<std::pair<bool, std::wstring>> LuaHelper::m_UnicodeFile;

bool LuaHelper::LoadFile(lua_State* L, const std::wstring& file, bool& unicode)
{
	size_t fileSize = 0;
	auto fileData = FileUtil::ReadFullFile(file, &fileSize);
	if (!fileData)
	{
		return false;
	}

	bool scriptLoaded = false;

	// Treat the script as Unicode if it has the UTF-16 LE BOM.
	unicode = fileSize > 2 && fileData[0] == 0xFF && fileData[1] == 0xFE;
	m_UnicodeFile.emplace_back(unicode, file);
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

bool LuaHelper::IsFunction(lua_State* L, const char* funcName, const std::wstring& file, bool unicode)
{
	m_UnicodeFile.emplace_back(unicode, file);
	bool bExists = false;
	lua_getglobal(L, funcName);
	bExists = lua_isfunction(L, -1);
	lua_pop(L, 1);
	m_UnicodeFile.pop_back();
	return bExists;
}

// Runs file and return number of values that the file returned
bool LuaHelper::RunFile(lua_State* L, const std::wstring& file, bool& unicode)
{
	if (LoadFile(L, file, unicode))
	{
		// Execute the Lua script
		int result = lua_pcall(L, 0, 0, 0);

		if (result >= 0)
		{
			m_UnicodeFile.pop_back();
			return result;
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

	m_UnicodeFile.pop_back();
	return -1;
}

bool LuaHelper::RunString(lua_State* L, const std::wstring& str, const std::wstring& file, bool unicode)
{
	m_UnicodeFile.emplace_back(unicode, file);
	const std::string narrowStr = unicode ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);

	// Load the string as a Lua chunk
	if (luaL_loadstring(L, narrowStr.c_str()))
	{
		LuaHelper::ReportErrors(L, file);
		m_UnicodeFile.pop_back();
		return false;
	}

	if (lua_pcall(L, 0, 0, 0))
	{
		LuaHelper::ReportErrors(L, file);
		m_UnicodeFile.pop_back();
		return false;
	}

	m_UnicodeFile.pop_back();
	return true;
}

bool LuaHelper::RunFunctionWithReturn(lua_State* L, const char* funcName, const std::wstring& file, bool unicode)
{
	m_UnicodeFile.emplace_back(unicode, file);

	// Push the function onto the stack
	lua_getglobal(L, funcName);

	if (lua_pcall(L, 0, 1, 0))
	{
		LuaHelper::ReportErrors(L, file);
		return false;
	}

	m_UnicodeFile.pop_back();
	return true;
}

bool LuaHelper::RunFunction(lua_State* L, const char* funcName, const std::wstring& file, bool unicode)
{
	m_UnicodeFile.emplace_back(unicode, file);
	int n = lua_gettop(L);
	lua_getglobal(L, funcName);

	if (lua_pcall(L, 0, 0, 0))
	{
		LuaHelper::ReportErrors(L, file);
		return lua_gettop(L) - n;
	}

	m_UnicodeFile.pop_back();
	return -1;
}

void LuaHelper::ReportErrors(lua_State* L, const std::wstring& file)
{
	const char* error = lua_tostring(L, -1);
	lua_pop(L, 1);

	std::wstring str(file, file.find_last_of(L'\\') + 1);

	std::pair<bool, std::wstring>& curFile = m_UnicodeFile.back();

	// Skip "[string ...]".
	const char* pos = strchr(error, ':');
	if (pos)
	{
		error = pos;
	}

	std::wstring wc = curFile.first ? StringUtil::WidenUTF8(error) : StringUtil::Widen(error);

	if (_wcsicmp(file.c_str(), wc.c_str()) == 0)
	{
		std::pair<bool, std::wstring>& prevFile = m_UnicodeFile.front();
		if (m_UnicodeFile.size() > 1)
		{
			prevFile = m_UnicodeFile[m_UnicodeFile.size() - 2];
		}
		else
		{
			prevFile = m_UnicodeFile.back();
		}

		std::wstring originalFile(prevFile.second, prevFile.second.find_last_of(L"\\") + 1);
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		originalFile += L":" + std::to_wstring(ar.currentline) + L": " + file + L": File or path to file is invalid.";
		LogErrorF(L"Script: %s", originalFile.c_str());
		return;
	}

	str += wc;
	LogErrorF(L"Script: %s", str.c_str());
}

void LuaHelper::PushWide(lua_State* L, const WCHAR* str)
{
	std::pair<bool, std::wstring>& curFile = m_UnicodeFile.back();
	const std::string narrowStr = curFile.first ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

void LuaHelper::PushWide(lua_State* L, const std::wstring& str)
{
	std::pair<bool, std::wstring>& curFile = m_UnicodeFile.back();
	const std::string narrowStr = curFile.first ?
		StringUtil::NarrowUTF8(str) : StringUtil::Narrow(str);
	lua_pushlstring(L, narrowStr.c_str(), narrowStr.length());
}

std::wstring LuaHelper::ToWide(lua_State* L, int narg)
{
	std::pair<bool, std::wstring>& curFile = m_UnicodeFile.back();
	std::wstring result = L"";

	size_t strLen = 0;
	const char* str = lua_tolstring(L, narg, &strLen);
	result = curFile.first ?
		StringUtil::WidenUTF8(str, (int)strLen) : StringUtil::Widen(str, (int)strLen);

	return result;
}
