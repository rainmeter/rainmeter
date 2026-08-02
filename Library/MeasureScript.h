// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include "lua/LuaScript.h"
#include "Skin.h"

class MeasureScript : public Measure
{
public:
	MeasureScript(Skin* skin, const WCHAR* name);
	virtual ~MeasureScript();

	MeasureScript(const MeasureScript& other) = delete;
	MeasureScript& operator=(MeasureScript other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureScript>(); }

	virtual const WCHAR* GetStringValue();
	virtual void Command(const std::wstring& command);

	bool CommandWithReturn(const std::wstring& command, std::wstring& strValue, void* delayedLogEntry = nullptr);

	const std::wstring& GetScriptFile() { return m_LuaScript.GetFile(); }

	void UninitializeLuaScript();

protected:
	virtual void Initialize();
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	LuaScript m_LuaScript;

	bool m_HasUpdateFunction;
	bool m_HasGetStringFunction;

	int m_ValueType;

	std::wstring m_StringValue;
};
