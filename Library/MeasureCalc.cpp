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

bool MeasureCalc::c_RandSeeded = false;

/*
** The constructor
**
*/
MeasureCalc::MeasureCalc(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_ParseError(false),
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
** The destructor
**
*/
MeasureCalc::~MeasureCalc()
{
}

/*
** Updates the calculation
**
*/
void MeasureCalc::UpdateValue()
{
	const WCHAR* errMsg = MathParser::Parse(m_Formula.c_str(), this, &m_Value);
	if (errMsg != NULL)
	{
		if (!m_ParseError)
		{
			LogErrorF(L"Calc: %s in [%s]", errMsg, m_Name.c_str());
			m_ParseError = true;
		}
	}
	else
	{
		m_ParseError = false;
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureCalc::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

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
		if (!m_UpdateRandom)
		{
			FormulaReplace();
		}

		const WCHAR* errMsg = MathParser::Check(m_Formula.c_str());
		if (errMsg != NULL)
		{
			LogErrorF(L"Calc: %s in [%s]", errMsg, m_Name.c_str());
			m_Formula.clear();
		}
	}
}

/*
** This replaces the word Random in the formula with a random number
**
*/
void MeasureCalc::FormulaReplace()
{
	size_t start = 0, pos;
	do
	{
		pos = m_Formula.find_first_of(L"Rr", start);
		if (pos != std::wstring::npos)
		{
			if (_wcsnicmp(L"random", m_Formula.c_str() + pos, 6) == 0 &&
				(pos == 0 || MathParser::IsDelimiter((*(m_Formula.c_str() + pos - 1))) &&
				(pos == (m_Formula.length() - 6) || MathParser::IsDelimiter((*(m_Formula.c_str() + pos + 6))))))
			{
				int randNumber = GetRandom();

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

bool MeasureCalc::GetMeasureValue(const WCHAR* str, int len, double* value)
{
	const std::vector<Measure*>& measures = m_MeterWindow->GetMeasures();

	std::vector<Measure*>::const_iterator iter = measures.begin();
	for ( ; iter != measures.end(); ++iter)
	{
		if ((*iter)->GetOriginalName().length() == len &&
			_wcsnicmp(str, (*iter)->GetName(), len) == 0)
		{
			*value = (*iter)->GetValue();
			return true;
		}
	}

	if (_wcsnicmp(str, L"counter", len) == 0)
	{
		*value = m_MeterWindow->GetUpdateCounter();
		return true;
	}
	else if (_wcsnicmp(str, L"random", len) == 0)
	{
		*value = GetRandom();
		return true;
	}

	return false;
}

int MeasureCalc::GetRandom()
{
	double range = (m_HighBound - m_LowBound) + 1;
	srand((unsigned)rand());
	return m_LowBound + (int)(range * rand() / (RAND_MAX + 1.0));
}
