/*
  Copyright (C) 2010 Matt King, Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include "LuaManager.h"

class LuaScript
{
public:
	LuaScript();
	~LuaScript();

	bool Initialize(const std::wstring& scriptFile);
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
};

#endif

