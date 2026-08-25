// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureScript.h"
#include "LuaHelper.h"
#include "Util.h"
#include "Rainmeter.h"
#include "../Common/StringParser.h"

const char* g_InitializeFunctionName = "Initialize";
const char* g_UpdateFunctionName = "Update";
const char* g_GetStringFunctionName = "GetStringValue";

MeasureScript::MeasureScript(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_LuaScript(skin->GetMathParser()),
	m_HasUpdateFunction(false),
	m_HasGetStringFunction(false),
	m_ValueType(LUA_TNIL)
{
}

MeasureScript::~MeasureScript()
{
	UninitializeLuaScript();
}

void MeasureScript::UninitializeLuaScript()
{
	m_LuaScript.Uninitialize();

	m_HasUpdateFunction = false;
	m_HasGetStringFunction = false;
}

void MeasureScript::Initialize()
{
	Measure::Initialize();

	if (m_LuaScript.IsFunction(g_InitializeFunctionName))
	{
		auto run = m_LuaScript.RunFunction(g_InitializeFunctionName);
		if (run.DidFail())
		{
			LogErrorF(this, L"Script Initialize() failed: %s", run.GetError());
		}
	}
}

void MeasureScript::UpdateValue()
{
	if (m_HasUpdateFunction)
	{
		auto updateRun = m_LuaScript.RunFunctionWithReturn(g_UpdateFunctionName, m_ValueType, m_Value, m_StringValue);
		if (updateRun.DidFail())
		{
			LogErrorF(this, L"Script Update() failed: %s", updateRun.GetError());
		}

		if (m_ValueType == LUA_TNIL && m_HasGetStringFunction)
		{
			// For backwards compatbility
			auto getStringRun = m_LuaScript.RunFunctionWithReturn(g_GetStringFunctionName, m_ValueType, m_Value, m_StringValue);
			if (getStringRun.DidFail())
			{
				LogErrorF(this, L"Script GetStringValue() failed: %s", getStringRun.GetError());
			}
		}
	}
}

const WCHAR* MeasureScript::GetStringValue()
{
	return (m_ValueType == LUA_TSTRING) ? CheckSubstitute(m_StringValue.c_str()) : nullptr;
}

void MeasureScript::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	std::wstring scriptFile = parser.ReadString(section, L"ScriptFile", L"");
	if (!scriptFile.empty())
	{
		if (m_Skin)
		{
			m_Skin->MakePathAbsolute(scriptFile);
		}

		if (!m_Initialized ||
			wcscmp(scriptFile.c_str(), m_LuaScript.GetFile().c_str()) != 0)
		{
			UninitializeLuaScript();

			if (m_LuaScript.Initialize(scriptFile))
			{
				bool hasInitializeFunction = m_LuaScript.IsFunction(g_InitializeFunctionName);
				m_HasUpdateFunction = m_LuaScript.IsFunction(g_UpdateFunctionName);

				auto L = m_LuaScript.GetState();
				lua_rawgeti(L, LUA_GLOBALSINDEX, m_LuaScript.GetRef());

				*(Skin**)lua_newuserdata(L, sizeof(Skin*)) = m_Skin;
				lua_getglobal(L, "MeterWindow");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SKIN");

				*(Measure**)lua_newuserdata(L, sizeof(Measure*)) = this;
				lua_getglobal(L, "Measure");
				lua_setmetatable(L, -2);
				lua_setfield(L, -2, "SELF");

				if (!m_LuaScript.IsUnicode())
				{
					// For backwards compatibility.

					m_HasGetStringFunction = m_LuaScript.IsFunction(g_GetStringFunctionName);
					if (m_HasGetStringFunction)
					{
						LogWarningF(this, L"Script: Using deprecated GetStringValue()");
					}

					lua_getfield(L, -1, "PROPERTIES");
					if (lua_isnil(L, -1) == 0)
					{
						lua_pushnil(L);

						// Look in the table for values to read from the section
						while (lua_next(L, -2))
						{
							lua_pop(L, 1);
							const char* strKey = lua_tostring(L, -1);
							const std::wstring wstrKey = StringUtil::Widen(strKey);
							const std::wstring& wstrValue =
								parser.ReadString(section, wstrKey.c_str(), L"");
							if (!wstrValue.empty())
							{
								const std::string strStrVal = StringUtil::Narrow(wstrValue);
								lua_pushstring(L, strStrVal.c_str());
								lua_setfield(L, -3, strKey);
							}
						}
					}

					// Pop PROPERTIES table.
					lua_pop(L, 1);
				}

				// Pop our table.
				lua_pop(L, 1);

				if (m_Initialized)
				{
					// If the measure is already initialized and the script has changed, we need to
					// manually call Initialize().
					Initialize();
				}

				// Valid script.
				return;
			}
		}
		else if (m_LuaScript.IsInitialized())
		{
			// Already initialized.
			return;
		}
	}

	// Disabled measures do not log any messages.
	// In this case, temporarily enable the measure to log the error message.
	bool isDisabled = m_Disabled;
	m_Disabled = false;
	LogErrorF(this, L"Script: File not valid");
	m_Disabled = isDisabled;

	UninitializeLuaScript();
}

void MeasureScript::Command(const std::wstring& command)
{
	if (!m_LuaScript.IsInitialized()) return;

	auto run = m_LuaScript.RunString(command);
	if (run.DidFail())
	{
		LogErrorF(this, L"!CommandMeasure failed: %s", run.GetError());
	}
}

bool MeasureScript::CommandWithReturn(const std::wstring& command, std::wstring& strValue, void* delayedLogEntry)
{
	// Scripts need to be initialized so that any variables declared in
	// the Initialize() function in the lua script file are accessible.
	// Return "0" so that no errors are thrown for forumlas.
	if (!m_Initialized)
	{
		strValue = L"0";
		return true;
	}

	// A command is either a function call, "Function(Arg1, Arg2)", or the name of a variable.
	StringParser parser(command);
	const std::wstring_view funcName = parser.ConsumeUntil(L'(');
	if (!funcName.empty() || !parser.IsConsumed())
	{
		// Function call. Anything after the closing parenthesis is ignored to match the old code,
		// which silently accepted e.g. "[ScriptMeasure:Function(),4]".
		if (funcName.empty() || !parser.ConsumeSuffixFromLast(L')'))
		{
			WCHAR errMsg[MAX_LINE_LENGTH];
			_snwprintf_s(errMsg, _TRUNCATE, L"Invalid function call: %s", command.c_str());
			if (delayedLogEntry)
			{
				std::wstring source = m_Skin->GetSkinPath();
				source += L" - [";
				source += GetOriginalName();
				source += L']';

				// Since scripts can accept single brackets as input, the nested variable parser
				// can send incomplete section variable to the script, so store a delayed message
				// in case the "actual" section variable is invalid. If the "final" variable the
				// parser finds is a valid variable, this error message will not be logged.
				// See: |ConfigParser::ParseVariables|
				auto* log = (Logger::Entry*)delayedLogEntry;
				*log = { Logger::Level::Error, L"", source.c_str(), errMsg };
			}
			else
			{
				LogErrorF(this, errMsg);
			}
			return false;
		}

		std::vector<std::wstring_view> args;
		parser.ConsumeWhitespace();
		while (!parser.IsConsumed())
		{
			args.emplace_back(parser.ConsumeUntilOrRest(
				L',', StringParser::SkipWhitespace | StringParser::SkipQuoted));
			parser.ConsumeWhitespace();
		}

		if (!m_LuaScript.RunCustomFunction(std::wstring(funcName), args, strValue))
		{
			if (!strValue.empty())
			{
				LogErrorF(this, L"%s", strValue.c_str());
			}
			return false;
		}
	}
	else
	{
		if (!m_LuaScript.GetLuaVariable(command, strValue))
		{
			LogErrorF(this, L"%s", strValue.c_str());
			return false;
		}
	}

	return true;
}
