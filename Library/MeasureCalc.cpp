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

#include "StdAfx.h"
#include "MeasureCalc.h"
#include "Rainmeter.h"

using namespace std;

hqStrMap* CMeasureCalc::c_VarMap = NULL;
bool CMeasureCalc::c_RandSeeded = false;

/*
** CMeasureCalc
**
** The constructor
**
*/
CMeasureCalc::CMeasureCalc(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	if(!c_RandSeeded)
	{
		c_RandSeeded = true;
		srand((unsigned)time(0)); 
	}

	m_Parser = MathParser_Create(NULL);

	m_LowBound = 0;
	m_HighBound = 100;
	m_UpdateRandom = false;

	rand();
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
	if (m_UpdateRandom)
	{
		FormulaReplace();
	}

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

	std::list<CMeasure*>::const_iterator iter = measures.begin();
	for( ; iter != measures.end(); ++iter)
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

	// Store the current values so we know if the value needs to be updated
	int oldLowBound = m_LowBound;
	int oldHighBound = m_HighBound;
	bool oldUpdateRandom = m_UpdateRandom;

	m_Formula = parser.ReadString(section, L"Formula", L"");

	m_LowBound = parser.ReadInt(section, L"LowBound", 0);
	m_HighBound = parser.ReadInt(section, L"HighBound", 100);
	m_UpdateRandom = 0!=parser.ReadInt(section, L"UpdateRandom", 0);

	if (!m_Initialized ||
		m_FormulaHolder != m_Formula ||
		oldLowBound != m_LowBound ||
		oldHighBound != m_HighBound ||
		oldUpdateRandom != m_UpdateRandom)
	{
		// Hold onto the formula, we are going to change it
		m_FormulaHolder = m_Formula;

		if (!m_UpdateRandom)
		{
			FormulaReplace();
		}
	}
}

/*
** FormulaReplace
**
** This replaces the word Random in m_Formula with a random number
**
*/
void CMeasureCalc::FormulaReplace()
{
	//To implement random numbers the word "Random" in the string
	//formula is being replaced by the random number value
	m_Formula = m_FormulaHolder;
	std::wstring::size_type loc = 0;

	while ((loc = m_Formula.find_first_of(L"Rr", loc)) != std::wstring::npos)
	{
		if (_wcsnicmp(L"Random", m_Formula.c_str() + loc, 6) == 0 &&
			(loc == 0 || IsDelimiter(*(m_Formula.c_str() + loc - 1))) &&
			(loc == (m_Formula.length() - 6) || IsDelimiter(*(m_Formula.c_str() + loc + 6))))
		{
			int range = (m_HighBound - m_LowBound) + 1; 
			srand((unsigned) rand()); 
			int randNumber = m_LowBound + (int)(range * rand()/(RAND_MAX + 1.0)); 

			WCHAR buffer[32];
			wsprintf(buffer, L"%i", randNumber);

			m_Formula.replace(loc, 6, buffer);
			loc += wcslen(buffer);
		}
		else
		{
			++loc;
		}
	}
}

/*
** IsDelimiter
**
** Checks whether the given character is a operator or a delimiter.
**
*/
bool CMeasureCalc::IsDelimiter(WCHAR ch)
{
	const WCHAR symbols[] = L" \t\n()+-/*^~<>%$,?:=&|;";

	for (size_t i = 0, len = wcslen(symbols); i < len; ++i)
	{
		if (ch == symbols[i])
		{
			return true;
		}
	}

	return false;
}
