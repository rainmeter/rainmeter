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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeasureCalc.h"
#include "Rainmeter.h"
#include "MathParser.h"

bool CMeasureCalc::c_RandSeeded = false;

/*
** CMeasureCalc
**
** The constructor
**
*/
CMeasureCalc::CMeasureCalc(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Random(),
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

	if (m_UpdateRandom) UpdateRandom();

	char* errMsg = MathParser::Parse(ConvertToAscii(m_Formula.c_str()).c_str(), this, &m_Value);
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

	std::wstring oldFormula = m_Formula;
	m_Formula = parser.ReadString(section, L"Formula", L"");

	m_LowBound = parser.ReadInt(section, L"LowBound", 0);
	m_HighBound = parser.ReadInt(section, L"HighBound", 100);
	m_UpdateRandom = 0!=parser.ReadInt(section, L"UpdateRandom", 0);

	if (!m_Initialized ||
		wcscmp(m_Formula.c_str(), oldFormula.c_str()) != 0 ||
		oldLowBound != m_LowBound ||
		oldHighBound != m_HighBound ||
		oldUpdateRandom != m_UpdateRandom)
	{
		if (!m_UpdateRandom) UpdateRandom();

		char* errMsg = MathParser::Check(ConvertToAscii(m_Formula.c_str()).c_str());
		if (errMsg != NULL)
		{
			std::wstring error = L"Calc: ";
			error += ConvertToWide(errMsg);
			error += L" in [";
			error += m_Name;
			error += L']';
			throw CError(error);
		}
	}
}

bool CMeasureCalc::GetMeasureValue(const char* str, int len, double* value)
{
	const std::list<CMeasure*>& measures = m_MeterWindow->GetMeasures();

	std::list<CMeasure*>::const_iterator iter = measures.begin();
	for ( ; iter != measures.end(); ++iter)
	{
		if (_strnicmp(str, (*iter)->GetAsciiName(), len) == 0)
		{
			*value = (*iter)->GetValue();
			return 1;
		}
	}

	if (_strnicmp(str, "counter", len) == 0)
	{
		*value = m_MeterWindow->GetUpdateCounter();
		return 1;
	}
	else if (_strnicmp(str, "random", len) == 0)
	{
		*value = (double)m_Random;
		return 1;
	}

	return 0;
}

void CMeasureCalc::UpdateRandom()
{
	int range = (m_HighBound - m_LowBound) + 1;
	srand((unsigned)rand());
	m_Random = m_LowBound + (int)(range * rand() / (RAND_MAX + 1.0));
}
