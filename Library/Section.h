// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include "Group.h"

class ConfigParser;
class Skin;

class __declspec(novtable) Section : public Group
{
public:
	virtual ~Section();

	Section(const Section& other) = delete;

	// Identifies the concrete type of the section (e.g. TypeID<MeterString>()).
	virtual UINT GetTypeID() = 0;

	// Identifies the base type of the section, i.e. either TypeID<Meter>() or TypeID<Measure>().
	virtual UINT GetBaseTypeID() = 0;

	const WCHAR* GetName() const { return m_Name.c_str(); }
	const std::wstring& GetOriginalName() const { return m_Name; }

	bool HasDynamicVariables() const { return m_DynamicVariables; }
	void SetDynamicVariables(bool b) { m_DynamicVariables = b; }

	void ResetUpdateCounter() { m_UpdateCounter = m_UpdateDivider; }
	void AdvanceUpdateCounter(UINT count);
	int GetUpdateCounter() const { return m_UpdateCounter; }
	int GetUpdateDivider() const { return m_UpdateDivider; }

	const std::wstring& GetOnUpdateAction() { return m_OnUpdateAction; }
	void DoUpdateAction();

	Skin* GetSkin() { return m_Skin; }

protected:
	Section(Skin* skin, const WCHAR* name);

	virtual void ReadOptions(ConfigParser& parser, std::wstring_view section);

	bool UpdateCounter();

	// Plugins may access this string through RmGetMeasureName(). This should never be modified to
	// ensure thread-safety.
	const std::wstring m_Name;

	bool m_DynamicVariables;		// If true, the section contains dynamic variables
	int m_UpdateDivider;			// Divider for the update
	int m_UpdateCounter;			// Current update counter

	std::wstring m_OnUpdateAction;

	Skin* m_Skin;
};
