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
CMeterLine::CMeterLine(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_Autoscale(false),
	m_HorizontalLines(false),
	m_Flip(false),
	m_LineWidth(1.0),
	m_HorizontalColor(Color::Black),
	m_CurrentPos()
{
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

	size_t colorsSize = m_Colors.size();
	size_t allValuesSize = m_AllValues.size();
	size_t num = (allValuesSize > 0) ? m_AllValues[0].size() : 0;

	if (colorsSize != allValuesSize)
	{
		if (colorsSize > allValuesSize)
		{
			for (size_t i = allValuesSize; i < colorsSize; ++i)
			{
				m_AllValues.push_back(std::vector<double>());

				if (m_W > 0)
				{
					m_AllValues.back().assign(m_W, 0.0);
				}
			}
		}
		else
		{
			m_AllValues.resize(colorsSize);
		}
	}

	if (m_W < 0 || num != (size_t)m_W)
	{
		if (m_CurrentPos >= m_W) m_CurrentPos = 0;

		num = (m_W < 0) ? 0 : m_W;
		for (size_t i = 0; i < allValuesSize; ++i)
		{
			if (num != m_AllValues[i].size())
			{
				m_AllValues[i].resize(num, 0.0);
			}
		}
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterLine::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	WCHAR tmpName[64];

	// Store the current number of lines so we know if the buffer needs to be updated
	int oldLineCount = (int)m_Colors.size();
	int oldW = m_W;

	// Read common configs
	CMeter::ReadConfig(parser, section);

	int lineCount = parser.ReadInt(section, L"LineCount", 1);

	m_Colors.clear();
	m_ScaleValues.clear();

	for (int i = 0; i < lineCount; ++i)
	{
		if (i == 0)
		{
			wcsncpy_s(tmpName, L"LineColor", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"LineColor%i", i + 1);
		}

		m_Colors.push_back(parser.ReadColor(section, tmpName, Color::White));

		if (i == 0)
		{
			wcsncpy_s(tmpName, L"Scale", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"Scale%i", i + 1);
		}

		m_ScaleValues.push_back(parser.ReadFloat(section, tmpName, 1.0));

		if (!m_Initialized && !m_MeasureName.empty())
		{
			if (i != 0)
			{
				_snwprintf_s(tmpName, _TRUNCATE, L"MeasureName%i", i + 1);
				m_MeasureNames.push_back(parser.ReadString(section, tmpName, L""));
			}
		}
	}

	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);
	m_Autoscale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_HorizontalLines = 0!=parser.ReadInt(section, L"HorizontalLines", 0);
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalColor", Color::Black);			// This is left here for backwards compatibility
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalLineColor", m_HorizontalColor);	// This is what it should be

	if (m_Initialized &&
		(oldLineCount != lineCount ||
		oldW != m_W))
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
		if (m_W > 0)
		{
			// Collect the values
			if (!m_Measure->IsDisabled())
			{
				double value = m_Measure->GetValue();

				m_AllValues[0][m_CurrentPos] = value;
			}

			int counter = 1;
			std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
			for ( ; i != m_Measures.end(); ++i)
			{
				double value = (*i)->GetValue();

				m_AllValues[counter][m_CurrentPos] = value;
				++counter;
			}

			++m_CurrentPos;
			if (m_CurrentPos >= m_W) m_CurrentPos = 0;
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
	if (!CMeter::Draw(graphics) || m_W <= 0) return false;

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
			double scale = m_ScaleValues[counter];
			std::vector<double>::const_iterator j = (*i).begin();
			for (; j != (*i).end(); ++j)
			{
				double val = (*j) * scale;
				newValue = max(newValue, val);
			}
			++counter;
		}

		// Scale the value up to nearest power of 2
		if (newValue > DBL_MAX / 2.0)
		{
			maxValue = DBL_MAX;
		}
		else
		{
			maxValue = 2.0;
			while (maxValue < newValue)
			{
				maxValue *= 2.0;
			}
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
				double val = (*i)->GetMaxValue();
				maxValue = max(maxValue, val);
			}
		}

		if (maxValue == 0.0)
		{
			maxValue = 1.0;
		}
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
		while (power < maxLines)
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
	const REAL H = m_H - 1.0f;
	counter = 0;
	std::vector< std::vector<double> >::const_iterator i = m_AllValues.begin();
	for (; i != m_AllValues.end(); ++i)
	{
		// Draw a line
		REAL Y, oldY;

		const double scale = m_ScaleValues[counter] * H / maxValue;

		int pos = m_CurrentPos;

		oldY = (REAL)((*i)[pos] * scale);
		oldY = min(oldY, H);
		oldY = max(oldY, 0.0f);
		oldY = y + ((m_Flip) ? oldY : H - oldY);

		// Cache all lines
		GraphicsPath path;
		for (int j = x + 1, R = x + m_W; j < R; ++j)
		{
			++pos;
			if (pos >= m_W) pos = 0;

			Y = (REAL)((*i)[pos] * scale);
			Y = min(Y, H);
			Y = max(Y, 0.0f);
			Y = y + ((m_Flip) ? Y : H - Y);

			path.AddLine((REAL)(j - 1), oldY, (REAL)j, Y);

			oldY = Y;
		}

		// Draw cached lines
		Pen pen(m_Colors[counter], (REAL)m_LineWidth);
		pen.SetLineJoin(LineJoinBevel);
		graphics.DrawPath(&pen, &path);

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
void CMeterLine::BindMeasure(const std::list<CMeasure*>& measures)
{
	CMeter::BindMeasure(measures);

	std::vector<std::wstring>::const_iterator j = m_MeasureNames.begin();
	for (; j != m_MeasureNames.end(); ++j)
	{
		// Go through the list and check it there is a secondary measure for us
		std::list<CMeasure*>::const_iterator i = measures.begin();
		for ( ; i != measures.end(); ++i)
		{
			if (_wcsicmp((*i)->GetName(), (*j).c_str()) == 0)
			{
				m_Measures.push_back(*i);
				break;
			}
		}

		if (i == measures.end())
		{
			std::wstring error = L"The meter [" + m_Name;
			error += L"] cannot be bound with [";
			error += (*j);
			error += L"]!";
			throw CError(error, __LINE__, __FILE__);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}
