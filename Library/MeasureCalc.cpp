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

hqStrMap* CMeasureCalc::c_VarMap = NULL;
bool CMeasureCalc::c_RandSeeded = false;

/*
** CMeasureCalc
**
** The constructor
**
*/
CMeasureCalc::CMeasureCalc(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Parser(MathParser_Create(NULL)),
	m_LowBound(),
	m_HighBound(100),
	m_UpdateRandom(false)
{
	if (!c_RandSeeded)
	{
		c_RandSeeded = true;
		srand((unsigned)time(0));
	}

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
		std::wstring error = L"Calc: ";
		error += ConvertToWide(errMsg);
		error += L" in [";
		error += m_Name;
		error += L']';
		Log(LOG_ERROR, error.c_str());
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

	const std::list<CMeasure*>& measures = meterWindow.GetMeasures();

	std::list<CMeasure*>::const_iterator iter = measures.begin();
	for ( ; iter != measures.end(); ++iter)
	{
		const char* name = (*iter)->GetAsciiName();
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
		wcscmp(m_FormulaHolder.c_str(), m_Formula.c_str()) != 0 ||
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
	size_t start = 0, pos;

	do
	{
		pos = m_Formula.find_first_of(L"Rr", start);
		if (pos != std::wstring::npos)
		{
			if (_wcsnicmp(L"Random", m_Formula.c_str() + pos, 6) == 0 &&
				(pos == 0 || IsDelimiter(*(m_Formula.c_str() + pos - 1))) &&
				(pos == (m_Formula.length() - 6) || IsDelimiter(*(m_Formula.c_str() + pos + 6))))
			{
				int range = (m_HighBound - m_LowBound) + 1;
				srand((unsigned) rand());
				int randNumber = m_LowBound + (int)(range * rand() / (RAND_MAX + 1.0));

				WCHAR buffer[32];
				_itow_s(randNumber, buffer, 10);
				size_t len = wcslen(buffer);

				m_Formula.replace(pos, 6, buffer, len);
				start = pos + len;
			}
			else
			{
				start = pos + 1;
			}
		}
	}
	while (pos != std::wstring::npos);
}

/*
** IsDelimiter
**
** Checks whether the given character is a operator or a delimiter.
**
*/
bool CMeasureCalc::IsDelimiter(WCHAR ch)
{
	const WCHAR* symbols = L" \t\n()+-/*^~<>%$,?:=&|;";

	for (const WCHAR* sch = symbols; *sch != L'\0'; ++sch)
	{
		if (ch == *sch)
		{
			return true;
		}
	}

	return false;
}
