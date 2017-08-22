/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURESCRIPT_H__
#define __MEASURESCRIPT_H__

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

	bool CommandWithReturn(const std::wstring& command, std::wstring& strValue);

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

#endif
