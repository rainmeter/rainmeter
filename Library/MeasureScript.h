#ifndef MEASURESCRIPT_H
#define MEASURESCRIPT_H

#include "Measure.h"
#include "lua/LuaScript.h"
#include "MeterWindow.h"

class CMeasureScript : public CMeasure
{

public:

	CMeasureScript(CMeterWindow* meterWindow);
	virtual ~CMeasureScript();

	void ReadConfig(CConfigParser& parser, const WCHAR* section);

	void Initialize();

	bool Update();

	double GetValue();

	void SetValue(double d);

	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);

	void MeterMouseEvent(CMeter* p_pMeter, MOUSE p_eMouse);
	void RunFunctionWithMeter(const char* p_strFunction, CMeter* p_pMeter);

	
protected:

	LuaScript*	m_pLuaScript;

	bool		m_bUpdateDefined;
	bool		m_bGetValueDefined;
	bool		m_bGetStringValueDefined;	
	bool		m_bInitializeDefined;

	std::wstring m_strValue;
	std::wstring m_ScriptTableName;

	/*
	Sqrat::Table* m_ScriptTable;
	Sqrat::Function* m_UpdateFunc;
	Sqrat::Function* m_GetValueFunc;
	Sqrat::Function* m_GetStringValueFunc;
	Sqrat::Function* m_InitFunc;
	Sqrat::Function* m_UpdateTransitionFunc;
	*/

};

#endif