/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MathParser.h"
#include "MeasureCalc.h"
#include "Rainmeter.h"
#include <random>

const int DEFAULT_LOWER_BOUND = 0;
const int DEFAULT_UPPER_BOUND = 100;
const int DEFAULT_UNIQUELIMIT = 65535;

std::mt19937& GetRandomEngine()
{
	static std::unique_ptr<std::mt19937> s_Engine(new std::mt19937((uint32_t)time(nullptr)));
	return *s_Engine;
}

MeasureCalc::MeasureCalc(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_ParseError(false),
	m_LowBound(DEFAULT_LOWER_BOUND),
	m_HighBound(DEFAULT_UPPER_BOUND),
	m_UpdateRandom(false),
	m_UniqueRandom(false)
{
}

MeasureCalc::~MeasureCalc()
{
}

/*
** Updates the calculation
**
*/
void MeasureCalc::UpdateValue()
{
	const WCHAR* errMsg = MathParser::Parse(m_Formula.c_str(), &m_Value, GetMeasureValue, this);
	if (errMsg != nullptr)
	{
		if (!m_ParseError)
		{
			LogErrorF(this, L"Calc: %s", errMsg);
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
	bool oldUniqueRandom = m_UniqueRandom;

	std::wstring oldFormula = m_Formula;
	m_Formula = parser.ReadString(section, L"Formula", L"");

	m_LowBound = parser.ReadInt(section, L"LowBound", DEFAULT_LOWER_BOUND);
	m_HighBound = parser.ReadInt(section, L"HighBound", DEFAULT_UPPER_BOUND);
	m_UpdateRandom = parser.ReadBool(section, L"UpdateRandom", false);
	const size_t range = (m_HighBound - m_LowBound) + 1;

	m_UniqueRandom = (range <= DEFAULT_UNIQUELIMIT) && parser.ReadBool(section, L"UniqueRandom", false);
	if (!m_UniqueRandom)
	{
		m_UniqueNumbers.clear();
	}

	if (!m_Initialized ||
		wcscmp(m_Formula.c_str(), oldFormula.c_str()) != 0 ||
		oldLowBound != m_LowBound ||
		oldHighBound != m_HighBound ||
		oldUpdateRandom != m_UpdateRandom ||
		oldUniqueRandom != m_UniqueRandom)
	{
		// Reset bounds if |m_LowBound| is greater than |m_HighBound|.
		if (m_LowBound > m_HighBound &&
			(oldLowBound != m_LowBound || oldHighBound != m_HighBound))
		{
			LogErrorF(this, L"\"LowBound\" (%i) must be less then or equal to \"HighBound\" (%i)", m_LowBound, m_HighBound);
		}

		// Reset the list if the bounds are changed
		if (m_UniqueRandom && (
			oldLowBound != m_LowBound ||
			oldHighBound != m_HighBound))
		{
			UpdateUniqueNumberList();
		}

		if (!m_UpdateRandom)
		{
			FormulaReplace();
		}

		const WCHAR* errMsg = MathParser::Check(m_Formula.c_str());
		if (errMsg != nullptr)
		{
			LogErrorF(this, L"Calc: %s", errMsg);
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

bool MeasureCalc::GetMeasureValue(const WCHAR* str, int len, double* value, void* context)
{
	auto calc = (MeasureCalc*)context;
	const std::vector<Measure*>& measures = calc->m_Skin->GetMeasures();

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
		*value = calc->m_Skin->GetUpdateCounter();
		return true;
	}
	else if (_wcsnicmp(str, L"random", len) == 0)
	{
		*value = calc->GetRandom();
		return true;
	}

	return false;
}

int MeasureCalc::GetRandom()
{
	if (m_LowBound == m_HighBound || m_LowBound > m_HighBound)
	{
		return m_LowBound;
	}
	else if (m_UniqueRandom)
	{
		if (m_UniqueNumbers.empty())
		{
			UpdateUniqueNumberList();
		}

		const int value = m_UniqueNumbers.back();
		m_UniqueNumbers.pop_back();
		return value;
	}
	else
	{
		std::uniform_int_distribution<int> distribution(m_LowBound, m_HighBound);
		return distribution(GetRandomEngine());
	}
}

void MeasureCalc::UpdateUniqueNumberList()
{
	const size_t range = (m_HighBound - m_LowBound) + 1;
	m_UniqueNumbers.resize(range);

	for (int i = 0; i < (int)range; ++i)
	{
		m_UniqueNumbers[i] = m_LowBound + i;
	}

	std::shuffle(m_UniqueNumbers.begin(), m_UniqueNumbers.end(), GetRandomEngine());
}
