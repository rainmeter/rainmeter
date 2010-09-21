/*
  Copyright (C) 2001 Kimmo Pekkola

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
#include "MeterLine.h"
#include "Measure.h"
#include "Error.h"

using namespace Gdiplus;

/*
** CMeterLine
**
** The constructor
**
*/
CMeterLine::CMeterLine(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Autoscale = false;
	m_HorizontalLines = false;
	m_HorizontalColor = 0;
	m_CurrentPos = 0;
	m_Flip = false;
	m_LineWidth = 1.0;
}

/*
** ~CMeterLine
**
** The destructor
**
*/
CMeterLine::~CMeterLine()
{
}

/*
** Initialize
**
** create the buffer for the lines
**
*/
void CMeterLine::Initialize()
{
	CMeter::Initialize();

	if (m_Colors.size() != m_AllValues.size())
	{
		if (m_Colors.size() > m_AllValues.size())
		{
			size_t num = (!m_AllValues.empty()) ? m_AllValues[0].size() : 0;

			for (size_t i = m_AllValues.size(), end = m_Colors.size(); i < end; ++i)
			{
				m_AllValues.push_back(std::vector<double>());

				if (num > 0)
				{
					m_AllValues.back().assign(num, 0);
				}
			}
		}
		else
		{
			m_AllValues.resize(m_Colors.size());
		}
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterLine::ReadConfig(const WCHAR* section)
{
	WCHAR tmpName[64];

	// Store the current number of lines so we know if the buffer needs to be updated
	int oldLineCount = (int)m_Colors.size();

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	int lineCount = parser.ReadInt(section, L"LineCount", 1);

	m_Colors.clear();
	m_ScaleValues.clear();
	m_MeasureNames.clear();

	for (int i = 0; i < lineCount; ++i)
	{
		if (i == 0)
		{
			wcscpy(tmpName, L"LineColor");
		}
		else
		{
			swprintf(tmpName, L"LineColor%i", i + 1);
		}

		m_Colors.push_back(parser.ReadColor(section, tmpName, Color::White));

		if (i == 0)
		{
			wcscpy(tmpName, L"Scale");
		}
		else
		{
			swprintf(tmpName, L"Scale%i", i + 1);
		}

		m_ScaleValues.push_back(parser.ReadFloat(section, tmpName, 1.0));

		if (i != 0)
		{
			swprintf(tmpName, L"MeasureName%i", i + 1);
			m_MeasureNames.push_back(parser.ReadString(section, tmpName, L""));
		}
	}

	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);
	m_Autoscale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_HorizontalLines = 0!=parser.ReadInt(section, L"HorizontalLines", 0);
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalColor", Color::Black);			// This is left here for backwards compatibility
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalLineColor", m_HorizontalColor);	// This is what it should be

	if (m_Initialized &&
		oldLineCount != lineCount)
	{
		Initialize();
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterLine::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		// Collect the values
		if (!m_Measure->IsDisabled())
		{
			double value = m_Measure->GetValue();

			if ((int)m_AllValues[0].size() < m_W)
			{
				m_AllValues[0].push_back(value);
			}
			else
			{
				m_AllValues[0][m_CurrentPos] = value;
			}
		}

		int counter = 1;
		std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
		for ( ; i != m_Measures.end(); ++i)
		{
			double value = (*i)->GetValue();

			if ((int)m_AllValues[counter].size() < m_W)
			{
				m_AllValues[counter].push_back(value);
			}
			else
			{
				m_AllValues[counter][m_CurrentPos] = value;
			}
			++counter;
		}

		++m_CurrentPos;
		if (m_CurrentPos >= m_W)
		{
			m_CurrentPos = 0;
		}
		return true;
	}
	return false;
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterLine::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	double maxValue = 0.0;
	int counter = 0;

	// Find the maximum value
	if (m_Autoscale)
	{
		double newValue = 0;
		std::vector< std::vector<double> >::const_iterator i = m_AllValues.begin();
		counter = 0;
		for (; i != m_AllValues.end(); ++i)
		{
			std::vector<double>::const_iterator j = (*i).begin();
			for (; j != (*i).end(); ++j)
			{
				newValue = max(newValue, (*j) * m_ScaleValues[counter]);
			}
			++counter;
		}

		// Scale the value up to nearest power of 2
		maxValue = 2;
		while(maxValue < newValue && maxValue != 0)
		{
			maxValue *= 2;
		}
	}
	else
	{
		if (m_Measure)
		{
			maxValue = m_Measure->GetMaxValue();

			std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
			for (; i != m_Measures.end(); ++i)
			{
				maxValue = max(maxValue, (*i)->GetMaxValue());
			}
		}
	}

	if (maxValue == 0.0)
	{
		maxValue = 1.0;
	}
	
	int x = GetX();
	int y = GetY();

	// Draw the horizontal lines
	if (m_HorizontalLines)
	{
		// Calc the max number of lines we should draw
		int maxLines = m_H / 4;	// one line per 4 pixels is max
		int numOfLines;

		// Check the highest power of 2 that fits in maxLines
		int power = 2;
		while(power < maxLines)
		{
			power *= 2;
		}

		numOfLines = ((int)maxValue % power) + 1;

		Pen pen(m_HorizontalColor);

		REAL Y;
		for (int j = 0; j < numOfLines; ++j)
		{
			Y = (REAL)((j + 1) * m_H / (numOfLines + 1));
			Y = y + m_H - Y - 1;
			graphics.DrawLine(&pen, (REAL)x, Y, (REAL)(x + m_W - 1), Y);	// GDI+
		}
	}

	// Draw all the lines
	counter = 0;
	std::vector< std::vector<double> >::const_iterator i = m_AllValues.begin();
	for (; i != m_AllValues.end(); ++i)
	{
		// Draw a line
		int X = x;
		REAL Y = 0;
		REAL oldY = 0;
		int pos = m_CurrentPos;
		if (pos >= m_W) pos = 0;

		int size = (int)(*i).size();
		
		Pen pen(m_Colors[counter], (REAL)m_LineWidth);

		for (int j = 0; j < m_W; ++j)
		{
			if (pos < size)
			{
				Y = (REAL)((*i)[pos] * m_ScaleValues[counter] * (m_H - 1) / maxValue);
				Y = min(Y, m_H - 1);
				Y = max(Y, 0);
			}
			else
			{
				Y = 0;			
			}

			if (m_Flip)
			{
				Y = y + Y;
			}
			else
			{
				Y = y + m_H - Y - 1;
			}

			if (j != 0)
			{
				graphics.DrawLine(&pen, (REAL)X - 1, oldY, (REAL)X, Y);	// GDI+
			}
			oldY = Y;

			++X;
			++pos;
			if (pos >= m_W) pos = 0;
		}

		++counter;
	}

	return true;
}

/*
** BindMeasure
**
** Overwritten method to handle the other measure bindings.
**
*/
void CMeterLine::BindMeasure(std::list<CMeasure*>& measures)
{
	CMeter::BindMeasure(measures);

	std::vector<std::wstring>::const_iterator j = m_MeasureNames.begin();
	for (; j != m_MeasureNames.end(); ++j)
	{
		// Go through the list and check it there is a secondary measure for us
		std::list<CMeasure*>::const_iterator i = measures.begin();
		for( ; i != measures.end(); ++i)
		{
			if(_wcsicmp((*i)->GetName(), (*j).c_str()) == 0)
			{
				m_Measures.push_back(*i);
				break;
			}
		}
		
		if (i == measures.end())
		{
	        throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + (*j) + L"]!", __LINE__, __FILE__);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}
