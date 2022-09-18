/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterLine.h"
#include "Measure.h"
#include "Logger.h"
#include "../Common/Gfx/Canvas.h"
#include "../Common/Gfx/Shapes/Path.h"
#include "../Common/Gfx/Shapes/Line.h"

MeterLine::MeterLine(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Autoscale(false),
	m_HorizontalLines(false),
	m_Flip(false),
	m_LineWidth(1.0),
	m_HorizontalColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_StrokeType(D2D1_STROKE_TRANSFORM_TYPE_NORMAL),
	m_CurrentPos(),
	m_GraphStartLeft(false),
	m_GraphHorizontalOrientation(false)
{
}

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

		m_Colors.push_back(parser.ReadColor(section, tmpName, D2D1::ColorF(D2D1::ColorF::White)));

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
	m_LineWidth = max(1.0, m_LineWidth);
	m_HorizontalLines = parser.ReadBool(section, L"HorizontalLines", false);

	D2D1_COLOR_F color = parser.ReadColor(section, L"HorizontalColor", D2D1::ColorF(D2D1::ColorF::Black));		// This is left here for backwards compatibility
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

	const WCHAR* type = parser.ReadString(section, L"TransformStroke", L"NORMAL").c_str();
	if (_wcsicmp(type, L"FIXED") == 0)
	{
		m_StrokeType = D2D1_STROKE_TRANSFORM_TYPE_FIXED;
	}
	else
	{
		m_StrokeType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
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

	// Set the max/min values to the floating point limits
	double maxValue = -(DBL_MAX);  // Intentional
	double minValue = DBL_MAX;

	// Find the maximum / minimum value
	if (m_Autoscale)
	{
		double newValue = 0.0;
		int counter = 0;
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

		for (auto i = m_Measures.cbegin(); i != m_Measures.cend(); ++i)
		{
			double val = (*i)->GetMinValue();
			minValue = min(minValue, val);
		}
	}
	else
	{
		for (auto i = m_Measures.cbegin(); i != m_Measures.cend(); ++i)
		{
			double val = (*i)->GetMaxValue();
			maxValue = max(maxValue, val);

			val = (*i)->GetMinValue();
			minValue = min(minValue, val);
		}
	}

	if (maxValue == minValue)
	{
		minValue = 0.0;
		maxValue = 1.0;
	}

	D2D1_RECT_F meterRect = GetMeterRectPadding();
	int drawW = (int)(meterRect.right - meterRect.left);
	int drawH = (int)(meterRect.bottom - meterRect.top);

	// Draw the horizontal lines
	if (m_HorizontalLines)
	{
		// Calc the max number of lines we should draw
		int maxLines = drawH / 4;	// one line per 4 pixels is max
		int numOfLines;

		// Check the highest power of 2 that fits in maxLines
		int power = 2;
		while (power < maxLines)
		{
			power *= 2;
		}

		numOfLines = ((int)maxValue % power) + 1;

		for (int j = 0; j < numOfLines; ++j)
		{
			FLOAT Y = (FLOAT)((j + 1) * drawH / (numOfLines + 1));
			Y = meterRect.bottom - Y - 0.5f;

			Gfx::Line line(meterRect.left, Y, meterRect.right - 1.0f, Y);
			line.SetStrokeFill(m_HorizontalColor);
			line.CreateStrokeStyle(m_StrokeType);

			canvas.DrawGeometry(line, 0, 0);
		}
	}

	// GDI+ compatibility.
	FLOAT offset = 0.55f;

	// Draw all the lines
	auto draw = [&](Gfx::Path& path, int& counter) -> void
	{
		path.Close(D2D1_FIGURE_END_OPEN);
		path.SetFill(Gfx::Util::c_Transparent_Color_F);
		path.SetStrokeFill(m_Colors[counter]);
		path.SetStrokeWidth((FLOAT)m_LineWidth);
		path.SetStrokeLineJoin(D2D1_LINE_JOIN_BEVEL, 10.0f);
		canvas.DrawGeometry(path, 0, 0);
	};

	const double range = maxValue - minValue;
	if (m_GraphHorizontalOrientation)
	{
		const FLOAT W = (FLOAT)(drawW - 1);
		int counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			const double scale = (m_ScaleValues[counter] * W) / range;
			int pos = m_CurrentPos;

			auto calcX = [&](FLOAT& _x)
			{
				_x = ((FLOAT)(((*i)[pos] - minValue) * scale) + offset);
				_x = min(_x, W + offset);
				_x = max(_x, offset);
				_x = meterRect.left + (m_GraphStartLeft ? _x : W - _x + 1.0f);
			};

			FLOAT X = 0.0f;
			FLOAT oldX = 0.0f;
			calcX(oldX);

			Gfx::Path path(
				oldX,
				!m_Flip ? meterRect.top : meterRect.bottom,
				D2D1_FILL_MODE_WINDING);
			path.CreateStrokeStyle(m_StrokeType);

			if (!m_Flip)
			{
				for (FLOAT j = (meterRect.top + 1.0f); j < meterRect.bottom; ++j)
				{
					++pos;
					pos %= drawH;

					calcX(X);

					path.AddLine(oldX, j - 1.0f);
					path.AddLine(X, j);

					oldX = X;
				}
			}
			else
			{
				for (FLOAT j = meterRect.bottom; j > (meterRect.top + 1.0f); --j)
				{
					++pos;
					pos %= drawH;

					calcX(X);

					path.AddLine(oldX, j - 1.0f);
					path.AddLine(X, j - 2.0f);

					oldX = X;
				}
			}

			draw(path, counter);
			++counter;
		}
	}
	else	// GraphOrientation=Vertical
	{
		const FLOAT H = (FLOAT)(drawH - 1);
		int counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			const double scale = (m_ScaleValues[counter] * H) / range;
			int pos = m_CurrentPos;

			auto calcY = [&](FLOAT& _y)
			{
				_y = ((FLOAT)(((*i)[pos] - minValue) * scale) + offset);
				_y = min(_y, H + offset);
				_y = max(_y, offset);
				_y = meterRect.top + (m_Flip ? _y : H - _y + 1.0f);
			};

			FLOAT Y = 0.0f;
			FLOAT oldY = 0.0f;
			calcY(oldY);

			Gfx::Path path(
				!m_GraphStartLeft ? meterRect.left : meterRect.right,
				oldY,
				D2D1_FILL_MODE_WINDING);
			path.CreateStrokeStyle(m_StrokeType);

			if (!m_GraphStartLeft)
			{
				for (FLOAT j = (meterRect.left + 1.0f); j < meterRect.right; ++j)
				{
					++pos;
					pos %= drawW;

					calcY(Y);

					path.AddLine(j - 1.0f, oldY);
					path.AddLine(j, Y);

					oldY = Y;
				}
			}
			else
			{
				for (FLOAT j = meterRect.right; j > (meterRect.left + 1.0f); --j)
				{
					++pos;
					pos %= drawW;

					calcY(Y);

					path.AddLine(j - 1.0f, oldY);
					path.AddLine(j - 2.0f, Y);

					oldY = Y;
				}
			}

			draw(path, counter);
			++counter;
		}
	}

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
