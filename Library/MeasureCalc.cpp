/*
  Copyright (C) 2004 Kimmo Pekkola

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4996)

#include "MeasureCalc.h"
#include "Rainmeter.h"

hqStrMap* CMeasureCalc::c_VarMap = NULL;
int CMeasureCalc::m_Loop = 0;

/*
** CMeasureCalc
**
** The constructor
**
*/
CMeasureCalc::CMeasureCalc(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_Parser = MathParser_Create(NULL);
}

/*
** ~CMeasureCalc
**
** The destructor
**
*/
CMeasureCalc::~CMeasureCalc()
{
	MathParser_Destroy(m_Parser);

	if (c_VarMap)
	{
		StrMap_Destroy(c_VarMap);
		c_VarMap = NULL;
	}
}

/*
** Update
**
** Updates the calculation
**
*/
bool CMeasureCalc::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	m_Parser->Parameters = c_VarMap;

	char* errMsg = MathParser_Parse(m_Parser, ConvertToAscii(m_Formula.c_str()).c_str(), &m_Value);
	if (errMsg != NULL)
	{
		DebugLog(ConvertToWide(errMsg).c_str());
	}

	return PostUpdate();
}

void CMeasureCalc::UpdateVariableMap(CMeterWindow& meterWindow)
{
	// Delete the old map
	if (c_VarMap)
	{
		StrMap_Destroy(c_VarMap);
		c_VarMap = NULL;
	}

	// Create the variable map
	c_VarMap = Strmap_Create(sizeof(double), 0); 

	std::list<CMeasure*>& measures = meterWindow.GetMeasures();

	std::list<CMeasure*>::iterator iter = measures.begin();
	for( ; iter != measures.end(); iter++)
	{
		const char* name = (*iter)->GetANSIName();
		double val = (*iter)->GetValue();

		StrMap_AddString(c_VarMap, name, &val);
	}

	// Add the counter
	double counter = meterWindow.GetUpdateCounter();
	StrMap_AddString(c_VarMap, "Counter", &counter);
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureCalc::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	m_Formula = parser.ReadString(section, L"Formula", L"");
}
