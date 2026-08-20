// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "ConfigParser.h"
#include "Measure.h"
#include "Export.h"

typedef UINT (*INITIALIZE)(HMODULE, LPCTSTR, LPCTSTR, UINT);
typedef VOID (*FINALIZE)(HMODULE, UINT);
typedef UINT (*UPDATE)(UINT);
typedef double (*UPDATE2)(UINT);
typedef LPCTSTR (*GETSTRING)(UINT, UINT);
typedef void (*EXECUTEBANG)(LPCWSTR, UINT);

typedef void (*NEWINITIALIZE)(void*, void*);
typedef void (*NEWRELOAD)(void*, void*, double*);
typedef void (*NEWFINALIZE)(void*);
typedef double (*NEWUPDATE)(void*);
typedef LPCWSTR (*NEWGETSTRING)(void*);
typedef void (*NEWEXECUTEBANG)(void*, LPCWSTR);

typedef LPCWSTR(*CUSTOMFUNCTION)(void*, const int, const WCHAR* argv[]);

class MeasurePlugin : public Measure
{
public:
	MeasurePlugin(Skin* skin, const WCHAR* name);
	virtual ~MeasurePlugin();

	MeasurePlugin(const MeasurePlugin& other) = delete;
	MeasurePlugin& operator=(MeasurePlugin other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasurePlugin>(); }

	virtual const WCHAR* GetStringValue();
	virtual void Command(const std::wstring& command);

	bool IsDpiAware() const { return m_HandleSkinSettingChangeFunc != nullptr; }
	ConfigParser::MonitorVariableMode GetMonitorVariableMode() const { return m_MonitorVariableMode; }

	static void HandleSkinSettingChange(Skin* skin, RmSkinSettingChange setting);
	void HandleSkinSettingChange(RmSkinSettingChange setting);

	bool CommandWithReturn(const std::wstring& command, std::wstring& strValue, void* delayedLogEntry = nullptr);

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();

private:
	bool IsNewApi() const { return m_ReloadFunc != nullptr; }

	HMODULE m_Plugin;

	ConfigParser::MonitorVariableMode m_MonitorVariableMode;

	union
	{
		struct
		{
			UINT m_ID;
			bool m_Update2;
		};

		struct
		{
			void* m_PluginData;
		};
	};

	void* m_ReloadFunc;
	void* m_UpdateFunc;
	void* m_GetStringFunc;
	void* m_ExecuteBangFunc;

	typedef void (*HandleSkinSettingChangeFunc)(void*, void*, RmSkinSettingChange);
	HandleSkinSettingChangeFunc m_HandleSkinSettingChangeFunc;
};
