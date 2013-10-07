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
#include "Logger.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

/*
** The constructor
**
*/
MeterLine::MeterLine(MeterWindow* meterWindow, const WCHAR* name) : Meter(meterWindow, name),
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
MeterLine::~MeterLine()
{
}

/*
** create the buffer for the lines
**
*/
void MeterLine::Initialize()
{
	Meter::Initialize();

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
void MeterLine::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	WCHAR tmpName[64];

	// Store the current number of lines so we know if the buffer needs to be updated
	int oldLineCount = (int)m_Colors.size();
	int oldSize = m_GraphHorizontalOrientation ? m_H : m_W;
	bool oldGraphHorizontalOrientation = m_GraphHorizontalOrientation;

	Meter::ReadOptions(parser, section);

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

	m_Flip = parser.ReadBool(section, L"Flip", false);
	m_Autoscale = parser.ReadBool(section, L"AutoScale", false);
	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_HorizontalLines = parser.ReadBool(section, L"HorizontalLines", false);
	ARGB color = parser.ReadColor(section, L"HorizontalColor", Color::Black);		// This is left here for backwards compatibility
	m_HorizontalColor = parser.ReadColor(section, L"HorizontalLineColor", color);	// This is what it should be

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
		LogErrorF(this, L"GraphStart=%s is not valid", graph);
	}

	graph = parser.ReadString(section, L"GraphOrientation", L"VERTICAL").c_str();
	if (_wcsicmp(graph, L"VERTICAL") == 0)
	{
		m_GraphHorizontalOrientation = false;
	}
	else if (_wcsicmp(graph, L"HORIZONTAL") ==  0)
	{
		m_GraphHorizontalOrientation = true;
	}
	else
	{
		LogErrorF(this, L"GraphOrientation=%s is not valid", graph);
	}

	if (m_Initialized)
	{
		int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
		if (oldLineCount != lineCount || oldSize != maxSize || oldGraphHorizontalOrientation != m_GraphHorizontalOrientation)
		{
			m_AllValues.clear();
			m_CurrentPos = 0;
			Initialize();
		}
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterLine::Update()
{
	if (Meter::Update() && !m_Measures.empty())
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
			m_CurrentPos %= maxSize;
		}
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterLine::Draw(Gfx::Canvas& canvas)
{
	int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
	if (!Meter::Draw(canvas) || maxSize <= 0) return false;
	
	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

	double maxValue = 0.0;
	int counter = 0;

	// Find the maximum value
	if (m_Autoscale)
	{
		double newValue = 0;
		counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			double scale = m_ScaleValues[counter];
			for (auto j = (*i).cbegin(); j != (*i).cend(); ++j)
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
		for (auto i = m_Measures.cbegin(); i != m_Measures.cend(); ++i)
		{
			double val = (*i)->GetMaxValue();
			maxValue = max(maxValue, val);
		}

		if (maxValue == 0.0)
		{
			maxValue = 1.0;
		}
	}

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	// Draw the horizontal lines
	if (m_HorizontalLines)
	{
		// Calc the max number of lines we should draw
		int maxLines = meterRect.Height / 4;	// one line per 4 pixels is max
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
			Y = (REAL)((j + 1) * meterRect.Height / (numOfLines + 1));
			Y = meterRect.Y + meterRect.Height - Y - 1;
			graphics.DrawLine(&pen, (REAL)meterRect.X, Y, (REAL)(meterRect.X + meterRect.Width - 1), Y);	// GDI+
		}
	}

	// Draw all the lines

	if (m_GraphHorizontalOrientation)
	{
		const REAL W = meterRect.Width - 1.0f;
		counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			// Draw a line
			REAL X, oldX;

			const double scale = m_ScaleValues[counter] * W / maxValue;

			int pos = m_CurrentPos;

			auto calcX = [&](REAL& _x)
			{
				_x = (REAL)((*i)[pos] * scale);
				_x = min(_x, W);
				_x = max(_x, 0.0f);
				_x = meterRect.X + (m_GraphStartLeft ? _x : W - _x);
			};

			calcX(oldX);

			// Cache all lines
			GraphicsPath path;
		
			if (!m_Flip)
			{
				for (int j = meterRect.Y + 1, R = meterRect.Y + meterRect.Height; j < R; ++j)
				{
					++pos;
					pos %= meterRect.Height;

					calcX(X);

					path.AddLine(oldX, (REAL)(j - 1), X, (REAL)j);

					oldX = X;
				}
			}
			else
			{
				for (int j = meterRect.Y + meterRect.Height, R = meterRect.Y + 1; j > R; --j)
				{
					++pos;
					pos %= meterRect.Height;

					calcX(X);

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
		const REAL H = meterRect.Height - 1.0f;
		counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			// Draw a line
			REAL Y, oldY;

			const double scale = m_ScaleValues[counter] * H / maxValue;

			int pos = m_CurrentPos;

			auto calcY = [&](REAL& _y)
			{
				_y = (REAL)((*i)[pos] * scale);
				_y = min(_y, H);
				_y = max(_y, 0.0f);
				_y = meterRect.Y + (m_Flip ? _y : H - _y);
			};

			calcY(oldY);

			// Cache all lines
			GraphicsPath path;
		
			if (!m_GraphStartLeft)
			{
				for (int j = meterRect.X + 1, R = meterRect.X + meterRect.Width; j < R; ++j)
				{
					++pos;
					pos %= meterRect.Width;

					calcY(Y);

					path.AddLine((REAL)(j - 1), oldY, (REAL)j, Y);

					oldY = Y;
				}
			}
			else
			{
				for (int j = meterRect.X + meterRect.Width, R = meterRect.X + 1; j > R; --j)
				{
					++pos;
					pos %= meterRect.Width;

					calcY(Y);

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

	canvas.EndGdiplusContext();

	return true;
}

/*
** Overwritten method to handle the other measure bindings.
**
*/
void MeterLine::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, false))
	{
		BindSecondaryMeasures(parser, section);
	}
}
