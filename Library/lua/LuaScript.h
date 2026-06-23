/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include "LuaHelper.h"

struct LuaResult
{
	static LuaResult Success() { return {}; }

	static LuaResult Fail(std::wstring error)
	{
		LuaResult result;
		result.error = std::make_unique<std::wstring>(std::move(error));
		return std::move(result);
	}

	const bool DidSucceed() { return !error; }
	const bool DidFail() { return !!error; }
	const wchar_t* GetError() { return error ? error->c_str() : L""; }

private:
	std::unique_ptr<std::wstring> error;
};

class LuaScript
{
public:
	LuaScript();
	~LuaScript();

	bool Initialize(const std::wstring& scriptFile);
	void Uninitialize();
	bool IsInitialized() { return m_State != nullptr; }

	const std::wstring& GetFile() { return m_File; }
	int GetRef() { return m_Ref; }
	bool IsUnicode() const { return m_Unicode; }

	LuaStateScope GetState() { return LuaStateScope(m_State, m_Unicode, m_Ref); }

	bool IsFunction(const char* funcName);
	LuaResult RunFunction(const char* funcName);
	LuaResult RunFunctionWithReturn(const char* funcName, int& valueType, double& numValue, std::wstring& strValue);
	LuaResult RunString(const std::wstring& str);
	bool RunCustomFunction(const std::wstring& funcName, const std::vector<std::wstring>& args, std::wstring& strValue);
	bool GetLuaVariable(const std::wstring& varName, std::wstring& strValue);

protected:
	static void RegisterGlobal(lua_State* L);
	static void RegisterMeasure(lua_State* L);
	static void RegisterMeter(lua_State* L);
	static void RegisterSkin(lua_State* L);

	std::wstring m_File;
	bool m_Unicode;
	int m_Ref;
	lua_State* m_State;
};

#endif
