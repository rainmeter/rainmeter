/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterHistogram.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"
#include "../Common/Gfx/Shapes/Rectangle.h"

GeneralImageHelper_DefineOptionArray(MeterHistogram::c_PrimaryOptionArray, L"Primary");
GeneralImageHelper_DefineOptionArray(MeterHistogram::c_SecondaryOptionArray, L"Secondary");
GeneralImageHelper_DefineOptionArray(MeterHistogram::c_BothOptionArray, L"Both");

MeterHistogram::MeterHistogram(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_PrimaryColor(D2D1::ColorF(D2D1::ColorF::Green)),
	m_SecondaryColor(D2D1::ColorF(D2D1::ColorF::Red)),
	m_OverlapColor(D2D1::ColorF(D2D1::ColorF::Yellow)),
	m_MeterPos(0),
	m_Autoscale(false),
	m_Flip(false),
	m_PrimaryImage(L"PrimaryImage", c_PrimaryOptionArray, false, skin),
	m_SecondaryImage(L"SecondaryImage", c_SecondaryOptionArray, false, skin),
	m_OverlapImage(L"BothImage", c_BothOptionArray, false, skin),
	m_PrimaryValues(nullptr),
	m_SecondaryValues(nullptr),
	m_MaxPrimaryValue(1.0),
	m_MinPrimaryValue(0.0),
	m_MaxSecondaryValue(1.0),
	m_MinSecondaryValue(0.0),
	m_SizeChanged(true),
	m_GraphStartLeft(false),
	m_GraphHorizontalOrientation(false)
{
}

MeterHistogram::~MeterHistogram()
{
	DisposeBuffer();
}

/*
** Disposes the buffers.
**
*/
void MeterHistogram::DisposeBuffer()
{
	// Reset current position
	m_MeterPos = 0;

	// Delete buffers
	delete [] m_PrimaryValues;
	m_PrimaryValues = nullptr;

	delete [] m_SecondaryValues;
	m_SecondaryValues = nullptr;
}

/*
** Creates the buffers.
**
*/
void MeterHistogram::CreateBuffer()
{
	DisposeBuffer();

	// Create buffers for values
	int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
	if (maxSize > 0)
	{
		m_PrimaryValues = new double[maxSize]();
		if (m_Measures.size() >= 2)
		{
			m_SecondaryValues = new double[maxSize]();
		}
	}
}

/*
** Load the images and calculate the dimensions of the meter from them.
** Or create the brushes if solid color histogram is used.
**
*/
void MeterHistogram::Initialize()
{
	Meter::Initialize();

	Measure* secondaryMeasure = (m_Measures.size() >= 2) ? m_Measures[1] : nullptr;

	// A sanity check
	if (secondaryMeasure && !m_PrimaryImageName.empty() && (m_OverlapImageName.empty() || m_SecondaryImageName.empty()))
	{
		LogWarningF(this, L"Histogram: SecondaryImage and BothImage not defined");

		m_PrimaryImage.DisposeImage();
		m_SecondaryImage.DisposeImage();
		m_OverlapImage.DisposeImage();
	}
	else
	{
		// Load the bitmaps if defined
		if (!m_PrimaryImageName.empty())
		{
			m_PrimaryImage.LoadImage(m_PrimaryImageName);

			if (m_PrimaryImage.IsLoaded())
			{
				int oldSize = m_GraphHorizontalOrientation ? m_H : m_W;

				Gfx::D2DBitmap* bitmap = m_PrimaryImage.GetImage();

				m_W = bitmap->GetWidth();
				m_H = bitmap->GetHeight();

				int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
				if (oldSize != maxSize)
				{
					m_SizeChanged = true;
				}

				m_W += GetWidthPadding();
				m_H += GetHeightPadding();
			}
		}
		else if (m_PrimaryImage.IsLoaded())
		{
			m_PrimaryImage.DisposeImage();
		}

		if (!m_SecondaryImageName.empty())
		{
			m_SecondaryImage.LoadImage(m_SecondaryImageName);
		}
		else if (m_SecondaryImage.IsLoaded())
		{
			m_SecondaryImage.DisposeImage();
		}

		if (!m_OverlapImageName.empty())
		{
			m_OverlapImage.LoadImage(m_OverlapImageName);
		}
		else if (m_OverlapImage.IsLoaded())
		{
			m_OverlapImage.DisposeImage();
		}
	}

	if ((!m_PrimaryImageName.empty() && !m_PrimaryImage.IsLoaded()) ||
		(!m_SecondaryImageName.empty() && !m_SecondaryImage.IsLoaded()) ||
		(!m_OverlapImageName.empty() && !m_OverlapImage.IsLoaded()))
	{
		DisposeBuffer();
		m_SizeChanged = false;
	}
	else if (m_SizeChanged)
	{
		CreateBuffer();
		m_SizeChanged = false;
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeterHistogram::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	int oldW = m_W;
	int oldH = m_H;
	bool oldGraphHorizontalOrientation = m_GraphHorizontalOrientation;

	Meter::ReadOptions(parser, section);

	m_PrimaryColor = parser.ReadColor(section, L"PrimaryColor", D2D1::ColorF(D2D1::ColorF::Green));
	m_SecondaryColor = parser.ReadColor(section, L"SecondaryColor", D2D1::ColorF(D2D1::ColorF::Red));
	m_OverlapColor = parser.ReadColor(section, L"BothColor", D2D1::ColorF(D2D1::ColorF::Yellow));

	m_PrimaryImageName = parser.ReadString(section, L"PrimaryImage", L"");
	if (!m_PrimaryImageName.empty())
	{
		// Read tinting options
		m_PrimaryImage.ReadOptions(parser, section);
	}

	m_SecondaryImageName = parser.ReadString(section, L"SecondaryImage", L"");
	if (!m_SecondaryImageName.empty())
	{
		// Read tinting options
		m_SecondaryImage.ReadOptions(parser, section);
	}

	m_OverlapImageName = parser.ReadString(section, L"BothImage", L"");
	if (!m_OverlapImageName.empty())
	{
		// Read tinting options
		m_OverlapImage.ReadOptions(parser, section);
	}

	m_Autoscale = parser.ReadBool(section, L"AutoScale", false);
	m_Flip = parser.ReadBool(section, L"Flip", false);

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
		if (m_PrimaryImageName.empty())
		{
			int oldSize = oldGraphHorizontalOrientation ? oldH : oldW;
			int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;
			if (oldSize != maxSize || oldGraphHorizontalOrientation != m_GraphHorizontalOrientation)
			{
				m_SizeChanged = true;
				Initialize();  // Reload the image
			}
		}
		else
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;

			m_SizeChanged = (oldGraphHorizontalOrientation != m_GraphHorizontalOrientation);

			Initialize();  // Reload the image
			
			if (m_SizeChanged)
			{
				CreateBuffer();
			}
		}
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterHistogram::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
		int maxSize = m_GraphHorizontalOrientation ? m_H : m_W;

		if (m_PrimaryValues && maxSize > 0)  // m_PrimaryValues must not be nullptr
		{
			Measure* measure = m_Measures[0];
			Measure* secondaryMeasure = (m_Measures.size() >= 2) ? m_Measures[1] : nullptr;

			// Gather values
			m_PrimaryValues[m_MeterPos] = measure->GetValue();

			if (secondaryMeasure && m_SecondaryValues)
			{
				m_SecondaryValues[m_MeterPos] = secondaryMeasure->GetValue();
			}

			++m_MeterPos;
			m_MeterPos %= maxSize;

			m_MaxPrimaryValue = measure->GetMaxValue();
			m_MinPrimaryValue = measure->GetMinValue();
			m_MaxSecondaryValue = 0.0;
			m_MinSecondaryValue = 0.0;
			if (secondaryMeasure)
			{
				m_MaxSecondaryValue = secondaryMeasure->GetMaxValue();
				m_MinSecondaryValue = secondaryMeasure->GetMinValue();
			}

			if (m_Autoscale)
			{
				// Go through all values and find the max

				double newValue = 0.0;
				for (int i = 0; i < maxSize; ++i)
				{
					newValue = max(newValue, m_PrimaryValues[i]);
				}

				// Scale the value up to nearest power of 2
				if (newValue > DBL_MAX / 2.0)
				{
					m_MaxPrimaryValue = DBL_MAX;
				}
				else
				{
					m_MaxPrimaryValue = 2.0;
					while (m_MaxPrimaryValue < newValue)
					{
						m_MaxPrimaryValue *= 2.0;
					}
				}

				if (secondaryMeasure && m_SecondaryValues)
				{
					for (int i = 0; i < maxSize; ++i)
					{
						newValue = max(newValue, m_SecondaryValues[i]);
					}

					// Scale the value up to nearest power of 2
					if (newValue > DBL_MAX / 2.0)
					{
						m_MaxSecondaryValue = DBL_MAX;
					}
					else
					{
						m_MaxSecondaryValue = 2.0;
						while (m_MaxSecondaryValue < newValue)
						{
							m_MaxSecondaryValue *= 2.0;
						}
					}
				}
			}
		}
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterHistogram::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas) ||
		(m_Measures.size() >= 1 && !m_PrimaryValues) ||
		(m_Measures.size() >= 2 && !m_SecondaryValues)) return false;

	Measure* secondaryMeasure = (m_Measures.size() >= 2) ? m_Measures[1] : nullptr;

	Gfx::D2DBitmap* primaryBitmap = m_PrimaryImage.GetImage();
	Gfx::D2DBitmap* secondaryBitmap = m_SecondaryImage.GetImage();
	Gfx::D2DBitmap* bothBitmap = m_OverlapImage.GetImage();

	D2D1_RECT_F meterRect = GetMeterRectPadding();
	int displayW = (int)(meterRect.right - meterRect.left);
	int displayH = (int)(meterRect.bottom - meterRect.top);

	// Default values (GraphStart=Right, GraphOrientation=Vertical)
	int i = 0;
	int startValue = 0;
	int* endValueLHS = &i;
	int* endValueRHS = &displayW;
	int step = 1;
	int endValue = -1; //(should be 0, but need to simulate <=)

	// GraphStart=Left, GraphOrientation=Vertical
	if (!m_GraphHorizontalOrientation)
	{
		if (m_GraphStartLeft)
		{
			startValue = displayW - 1;
			endValueLHS = &endValue;
			endValueRHS = &i;
			step = -1;
		}
	}
	else
	{
		if (!m_Flip)
		{
			endValueRHS = &displayH;
		}
		else
		{
			startValue = displayH - 1;
			endValueLHS = &endValue;
			endValueRHS = &i;
			step = -1;
		}
	}

	auto draw = [&](Gfx::D2DBitmap* bitmap, const D2D1_COLOR_F& color, const D2D1_RECT_F& dst) -> void
	{
		if (!bitmap)
		{
			canvas.FillRectangle(dst, color);
			return;
		}

		const D2D1_RECT_F src = [&]() -> D2D1_RECT_F
		{
			return D2D1::RectF(
				dst.left - meterRect.left,
				dst.top - meterRect.top,
				dst.right - meterRect.left,
				dst.bottom - meterRect.top);
		} ();

		canvas.DrawBitmap(bitmap, dst, src);
	};

	// Horizontal or Vertical graph
	if (m_GraphHorizontalOrientation)
	{
		for (i = startValue; *endValueLHS < *endValueRHS; i += step)
		{
			const FLOAT startStep = (FLOAT)(startValue + (step * i));

			double range = m_MaxPrimaryValue - m_MinPrimaryValue;
			double value = (range < 0.0) ? 0.0 : (range == 0.0) ? 1.0 :
				(m_PrimaryValues[(i + m_MeterPos) % displayH] - m_MinPrimaryValue) / range;

			int primaryBarHeight = (int)(displayW * value);
			primaryBarHeight = min(displayW, primaryBarHeight);
			primaryBarHeight = max(0, primaryBarHeight);

			const FLOAT primaryBarHeightF = (FLOAT)primaryBarHeight;

			if (secondaryMeasure)
			{
				range = m_MaxSecondaryValue - m_MinSecondaryValue;
				value = (range < 0.0) ? 0.0 : (range == 0.0) ? 1.0 :
					(m_SecondaryValues[(i + m_MeterPos) % displayH] - m_MinSecondaryValue) / range;

				int secondaryBarHeight = (int)(displayW * value);
				secondaryBarHeight = min(displayW, secondaryBarHeight);
				secondaryBarHeight = max(0, secondaryBarHeight);

				// Check which measured value is higher
				const FLOAT bothBarHeight = (FLOAT)min(primaryBarHeight, secondaryBarHeight);
				const FLOAT secondaryBarHeightF = (FLOAT)secondaryBarHeight;

				// Draw image/color rectangle for both lines
				{
					const D2D1_RECT_F& dst = m_GraphStartLeft ?
						Gfx::Util::ToRectF(meterRect.left, meterRect.top + startStep, bothBarHeight, 1.0f) :
						Gfx::Util::ToRectF(meterRect.right - bothBarHeight, meterRect.top + startStep, bothBarHeight, 1.0f);
					draw(bothBitmap, m_OverlapColor, dst);
				}

				// Draw image/color rectangle for the rest
				if (secondaryBarHeight > primaryBarHeight)
				{
					const D2D1_RECT_F& dst = m_GraphStartLeft ?
						Gfx::Util::ToRectF(meterRect.left + bothBarHeight, meterRect.top + startStep, secondaryBarHeightF - bothBarHeight, 1.0f) :
						Gfx::Util::ToRectF(meterRect.right - secondaryBarHeightF, meterRect.top + startStep, secondaryBarHeightF - bothBarHeight, 1.0f);
					draw(secondaryBitmap, m_SecondaryColor, dst);
				}
				else
				{
					const D2D1_RECT_F& dst = m_GraphStartLeft ?
						Gfx::Util::ToRectF(meterRect.left + bothBarHeight, meterRect.top + startStep, primaryBarHeightF - bothBarHeight, 1.0f) :
						Gfx::Util::ToRectF(meterRect.right - primaryBarHeightF, meterRect.top + startStep, primaryBarHeightF - bothBarHeight, 1.0f);
					draw(primaryBitmap, m_PrimaryColor, dst);
				}
			}
			else
			{
				const D2D1_RECT_F& dst = m_GraphStartLeft ?
					Gfx::Util::ToRectF(meterRect.left, meterRect.top + startStep, primaryBarHeightF, 1.0f) :
					Gfx::Util::ToRectF(meterRect.right - primaryBarHeightF, meterRect.top + startStep, primaryBarHeightF, 1.0f);
				draw(primaryBitmap, m_PrimaryColor, dst);
			}
		}
	}
	else	// GraphOrientation=Vertical
	{
		for (i = startValue; *endValueLHS < *endValueRHS; i += step)
		{
			const FLOAT startStep = (FLOAT)(startValue + (step * i));

			double range = m_MaxPrimaryValue - m_MinPrimaryValue;
			double value = (range < 0.0) ? 0.0 : (range == 0.0) ? 1.0 :
				(m_PrimaryValues[((i + m_MeterPos) % displayW)] - m_MinPrimaryValue) / range;

			int primaryBarHeight = (int)(displayH * value);
			primaryBarHeight = min(displayH, primaryBarHeight);
			primaryBarHeight = max(0, primaryBarHeight);

			const FLOAT primaryBarHeightF = (FLOAT)primaryBarHeight;

			if (secondaryMeasure)
			{
				range = m_MaxSecondaryValue - m_MinSecondaryValue;
				value = (range < 0.0) ? 0.0 : (range == 0.0) ? 1.0 :
					(m_SecondaryValues[(i + m_MeterPos) % displayW] - m_MinSecondaryValue) / range;

				int secondaryBarHeight = (int)(displayH * value);
				secondaryBarHeight = min(displayH, secondaryBarHeight);
				secondaryBarHeight = max(0, secondaryBarHeight);

				// Check which measured value is higher
				const FLOAT bothBarHeight = (FLOAT)min(primaryBarHeight, secondaryBarHeight);
				const FLOAT secondaryBarHeightF = (FLOAT)secondaryBarHeight;

				// Draw image/color rectangle for both lines
				{
					const D2D1_RECT_F& dst = m_Flip ?
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.top, 1.0f, bothBarHeight) :
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.bottom - bothBarHeight, 1.0f, bothBarHeight);
					draw(bothBitmap, m_OverlapColor, dst);
				}

				// Draw image/color rectangle for the rest
				if (secondaryBarHeight > primaryBarHeight)
				{
					const D2D1_RECT_F& dst = m_Flip ?
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.top + bothBarHeight, 1.0f, secondaryBarHeightF - bothBarHeight) :
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.bottom - secondaryBarHeightF, 1.0f, secondaryBarHeightF - bothBarHeight);
					draw(secondaryBitmap, m_SecondaryColor, dst);
				}
				else
				{
					const D2D1_RECT_F& dst = m_Flip ?
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.top + bothBarHeight, 1.0f, primaryBarHeightF - bothBarHeight) :
						Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.bottom - primaryBarHeightF, 1.0f, primaryBarHeightF - bothBarHeight);
					draw(primaryBitmap, m_PrimaryColor, dst);
				}
			}
			else
			{
				const D2D1_RECT_F& dst = m_Flip ?
					Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.top, 1.0f, primaryBarHeightF) :
					Gfx::Util::ToRectF(meterRect.left + startStep, meterRect.bottom - primaryBarHeightF, 1.0f, primaryBarHeightF);
				draw(primaryBitmap, m_PrimaryColor, dst);
			}
		}
	}

	return true;
}

/*
** Overwritten method to handle the secondary measure binding.
**
*/
void MeterHistogram::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, false))
	{
		const std::wstring* secondaryMeasure = &parser.ReadString(section, L"MeasureName2", L"");
		if (secondaryMeasure->empty())
		{
			// For backwards compatibility.
			secondaryMeasure = &parser.ReadString(section, L"SecondaryMeasureName", L"");
		}

		Measure* measure = parser.GetMeasure(*secondaryMeasure);
		if (measure)
		{
			m_Measures.push_back(measure);
		}
	}
}
