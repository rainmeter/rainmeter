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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterLine.h"
#include "Measure.h"
#include "Error.h"

using namespace Gdiplus;

/*
** The constructor
**
*/
CMeterLine::CMeterLine(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_Autoscale(false),
	m_HorizontalLines(false),
	m_Flip(false),
	m_LineWidth(1.0),
	m_HorizontalColor(Color::Black),
	m_CurrentPos(),
	m_GraphStartLeft(false),
	m_GraphHorizontalOrientation(false)
{
}

/*
** The destructor
**
*/
CMeterLine::~CMeterLine()
{
}

/*
** create the buffer for the lines
**
*/
void CMeterLine::Initialize()
{
	CMeter::Initialize();

	size_t colorsSize = m_Colors.size();
	size_t allValuesSize = m_AllValues.size();
	size_t num = (allValuesSize > 0) ? m_AllValues[0].size() : 0;
	int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;

	if (colorsSize != allValuesSize)
	{
		if (colorsSize > allValuesSize)
		{
			for (size_t i = allValuesSize; i < colorsSize; ++i)
			{
				m_AllValues.push_back(std::vector<double>());

				if (maxSize > 0)
				{
					m_AllValues.back().assign(maxSize, 0.0);
				}
			}
		}
		else
		{
			m_AllValues.resize(colorsSize);
		}
	}

	if (maxSize < 0 || num != (size_t)maxSize)
	{
		if (m_CurrentPos >= maxSize) m_CurrentPos = 0;

		num = (maxSize < 0) ? 0 : maxSize;
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
** Read the options specified in the ini file.
**
*/
void CMeterLine::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	WCHAR tmpName[64];

	// Store the current number of lines so we know if the buffer needs to be updated
	int oldLineCount = (int)m_Colors.size();
	int oldW = m_W;

	CMeter::ReadOptions(parser, section);

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
	}

	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);
	m_Autoscale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_HorizontalLines = 0!=parser.ReadInt(section, L"HorizontalLines", 0);
	ARGB color = parser.ReadColor(section, L"HorizontalColor", Color::Black);		// This is left here for backwards compatibility
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalLineColor", color);	// This is what it should be

	if (m_Initialized &&
		(oldLineCount != lineCount || oldW != m_W))
	{
		Initialize();
	}

	const WCHAR* graph = parser.ReadString(section, L"GraphStart", L"RIGHT").c_str();
	if (_wcsicmp(graph, L"RIGHT") == 0)
	{
		m_GraphStartLeft = false;
	}
	else if (_wcsicmp(graph, L"LEFT") ==  0)
	{
		m_GraphStartLeft = true;
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"GraphStart=%s is not valid in [%s]", graph, m_Name.c_str());
	}

	graph = parser.ReadString(section, L"GraphOrientation", L"VERTICAL").c_str();
	if (_wcsicmp(graph, L"VERTICAL") == 0)
	{
		// Restart graph
		if (m_GraphHorizontalOrientation)
		{
			m_GraphHorizontalOrientation = false;
			m_AllValues.clear();
			Initialize();
			m_CurrentPos = 0;
		}
		else
		{
			m_GraphHorizontalOrientation = false;
		}
	}
	else if (_wcsicmp(graph, L"HORIZONTAL") ==  0)
	{
		// Restart graph
		if (!m_GraphHorizontalOrientation)
		{
			m_GraphHorizontalOrientation = true;
			m_AllValues.clear();
			Initialize();
			m_CurrentPos = 0;
		}
		else
		{
			m_GraphHorizontalOrientation = true;
		}
	}
	else
	{
		LogWithArgs(LOG_ERROR, L"GraphOrientation=%s is not valid in [%s]", graph, m_Name.c_str());
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool CMeterLine::Update()
{
	if (CMeter::Update() && !m_Measures.empty())
	{
		int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;

		if (maxSize > 0)
		{
			int allValuesSize = (int)m_AllValues.size();
			int counter = 0;
			for (auto i = m_Measures.cbegin(); counter < allValuesSize && i != m_Measures.cend(); ++i, ++counter)
			{
				m_AllValues[counter][m_CurrentPos] = (*i)->GetValue();
			}

			++m_CurrentPos;
			if (m_CurrentPos >= maxSize) m_CurrentPos = 0;
		}
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool CMeterLine::Draw(Graphics& graphics)
{
	int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
	if (!CMeter::Draw(graphics) || maxSize <= 0) return false;

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
		if (!m_Measures.empty())
		{
			maxValue = m_Measures[0]->GetMaxValue();

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

	if (m_GraphHorizontalOrientation)
	{
		const REAL W = m_W - 1.0f;
		counter = 0;
		std::vector< std::vector<double> >::const_iterator i = m_AllValues.begin();
		for (; i != m_AllValues.end(); ++i)
		{
			// Draw a line
			REAL X, oldX;

			const double scale = m_ScaleValues[counter] * W / maxValue;

			int pos = m_CurrentPos;

			oldX = (REAL)((*i)[pos] * scale);
			oldX = min(oldX, W);
			oldX = max(oldX, 0.0f);
			oldX = x + (m_GraphStartLeft ? oldX : W - oldX);

			// Cache all lines
			GraphicsPath path;
		
			if (!m_Flip)
			{
				for (int j = y + 1, R = y + m_H; j < R; ++j)
				{
					++pos;
					if (pos >= m_H) pos = 0;

					X = (REAL)((*i)[pos] * scale);
					X = min(X, W);
					X = max(X, 0.0f);
					X = x + (m_GraphStartLeft ? X : W - X);

					path.AddLine(oldX, (REAL)(j - 1), X, (REAL)j);

					oldX = X;
				}
			}
			else
			{
				for (int j = y + m_H, R = y + 1; j > R; --j)
				{
					++pos;
					if (pos >= m_H) pos = 0;

					X = (REAL)((*i)[pos] * scale);
					X = min(X, W);
					X = max(X, 0.0f);
					X = x + (m_GraphStartLeft ? X : W - X);

					path.AddLine(oldX, (REAL)(j - 1), X, (REAL)(j - 2));

					oldX = X;
				}
			}

			// Draw cached lines
			Pen pen(m_Colors[counter], (REAL)m_LineWidth);
			pen.SetLineJoin(LineJoinBevel);
			graphics.DrawPath(&pen, &path);

			++counter;
		}
	}
	else
	{
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
			oldY = y + (m_Flip ? oldY : H - oldY);

			// Cache all lines
			GraphicsPath path;
		
			if (!m_GraphStartLeft)
			{
				for (int j = x + 1, R = x + m_W; j < R; ++j)
				{
					++pos;
					if (pos >= m_W) pos = 0;

					Y = (REAL)((*i)[pos] * scale);
					Y = min(Y, H);
					Y = max(Y, 0.0f);
					Y = y + (m_Flip ? Y : H - Y);

					path.AddLine((REAL)(j - 1), oldY, (REAL)j, Y);

					oldY = Y;
				}
			}
			else
			{
				for (int j = x + m_W, R = x + 1; j > R; --j)
				{
					++pos;
					if (pos >= m_W) pos = 0;

					Y = (REAL)((*i)[pos] * scale);
					Y = min(Y, H);
					Y = max(Y, 0.0f);
					Y = y + (m_Flip ? Y : H - Y);

					path.AddLine((REAL)(j - 1), oldY, (REAL)(j - 2), Y);

					oldY = Y;
				}
			}

			// Draw cached lines
			Pen pen(m_Colors[counter], (REAL)m_LineWidth);
			pen.SetLineJoin(LineJoinBevel);
			graphics.DrawPath(&pen, &path);

			++counter;
		}
	}

	return true;
}

/*
** Overwritten method to handle the other measure bindings.
**
*/
void CMeterLine::BindMeasures(CConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, false))
	{
		BindSecondaryMeasures(parser, section);
	}
}
