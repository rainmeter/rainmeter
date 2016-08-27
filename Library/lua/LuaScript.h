/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include "LuaManager.h"

class LuaScript
{
public:
	LuaScript();
	~LuaScript();

	bool Initialize(const std::wstring& scriptFile, const std::wstring& packagePath);
	void Uninitialize();
	bool IsInitialized() { return m_Ref != LUA_NOREF; }

	const std::wstring& GetFile() { return m_File; }
	int GetRef() { return m_Ref; }
	bool IsUnicode() const { return m_Unicode; }

	LuaManager::ScopedLuaState GetState() { return LuaManager::GetState(m_Unicode); }

	bool IsFunction(const char* funcName);
	void RunFunction(const char* funcName);
	int RunFunctionWithReturn(const char* funcName, double& numValue, std::wstring& strValue);
	void RunString(const std::wstring& str);

protected:
	std::wstring m_File;
	int m_Ref;
	bool m_Unicode;

private:
	void RegisterPackagePath(const std::wstring& path);
};

#endif

