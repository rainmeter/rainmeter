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

using namespace Gdiplus;

TintedImageHelper_DefineOptionArray(MeterHistogram::c_PrimaryOptionArray, L"Primary");
TintedImageHelper_DefineOptionArray(MeterHistogram::c_SecondaryOptionArray, L"Secondary");
TintedImageHelper_DefineOptionArray(MeterHistogram::c_BothOptionArray, L"Both");

MeterHistogram::MeterHistogram(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_PrimaryColor(Color::Green),
	m_SecondaryColor(Color::Red),
	m_OverlapColor(Color::Yellow),
	m_MeterPos(),
	m_Autoscale(false),
	m_Flip(false),
	m_PrimaryImage(L"PrimaryImage", c_PrimaryOptionArray, false, skin),
	m_SecondaryImage(L"SecondaryImage", c_SecondaryOptionArray, false, skin),
	m_OverlapImage(L"BothImage", c_BothOptionArray, false, skin),
	m_PrimaryNeedsReload(false),
	m_SecondaryNeedsReload(false),
	m_OverlapNeedsReload(false),
	m_PrimaryValues(),
	m_SecondaryValues(),
	m_MaxPrimaryValue(1.0),
	m_MinPrimaryValue(),
	m_MaxSecondaryValue(1.0),
	m_MinSecondaryValue(),
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
			m_PrimaryImage.LoadImage(m_PrimaryImageName, m_PrimaryNeedsReload);

			if (m_PrimaryImage.IsLoaded())
			{
				int oldSize = m_GraphHorizontalOrientation ? m_H : m_W;

				Bitmap* bitmap = m_PrimaryImage.GetImage();

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
			m_SecondaryImage.LoadImage(m_SecondaryImageName, m_SecondaryNeedsReload);
		}
		else if (m_SecondaryImage.IsLoaded())
		{
			m_SecondaryImage.DisposeImage();
		}

		if (!m_OverlapImageName.empty())
		{
			m_OverlapImage.LoadImage(m_OverlapImageName, m_OverlapNeedsReload);
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
	std::wstring oldPrimaryImageName = m_PrimaryImageName;
	std::wstring oldSecondaryImageName = m_SecondaryImageName;
	std::wstring oldBothImageName = m_OverlapImageName;
	int oldW = m_W;
	int oldH = m_H;
	bool oldGraphHorizontalOrientation = m_GraphHorizontalOrientation;

	Meter::ReadOptions(parser, section);

	m_PrimaryColor = parser.ReadColor(section, L"PrimaryColor", Color::Green);
	m_SecondaryColor = parser.ReadColor(section, L"SecondaryColor", Color::Red);
	m_OverlapColor = parser.ReadColor(section, L"BothColor", Color::Yellow);

	m_PrimaryImageName = parser.ReadString(section, L"PrimaryImage", L"");
	if (!m_PrimaryImageName.empty())
	{
		// Read tinting options
		m_PrimaryImage.ReadOptions(parser, section);
	}
	else
	{
		m_PrimaryImage.ClearOptionFlags();
	}

	m_SecondaryImageName = parser.ReadString(section, L"SecondaryImage", L"");
	if (!m_SecondaryImageName.empty())
	{
		// Read tinting options
		m_SecondaryImage.ReadOptions(parser, section);
	}
	else
	{
		m_SecondaryImage.ClearOptionFlags();
	}

	m_OverlapImageName = parser.ReadString(section, L"BothImage", L"");
	if (!m_OverlapImageName.empty())
	{
		// Read tinting options
		m_OverlapImage.ReadOptions(parser, section);
	}
	else
	{
		m_OverlapImage.ClearOptionFlags();
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

			m_PrimaryNeedsReload = (wcscmp(oldPrimaryImageName.c_str(), m_PrimaryImageName.c_str()) != 0);
			m_SecondaryNeedsReload = (wcscmp(oldSecondaryImageName.c_str(), m_SecondaryImageName.c_str()) != 0);
			m_OverlapNeedsReload = (wcscmp(oldBothImageName.c_str(), m_OverlapImageName.c_str()) != 0);
			m_SizeChanged = (oldGraphHorizontalOrientation != m_GraphHorizontalOrientation);

			if (m_PrimaryNeedsReload ||
				m_SecondaryNeedsReload ||
				m_OverlapNeedsReload ||
				m_PrimaryImage.IsOptionsChanged() ||
				m_SecondaryImage.IsOptionsChanged() ||
				m_OverlapImage.IsOptionsChanged())
			{
				Initialize();  // Reload the image
			}
			else if (m_SizeChanged)
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

	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

	Measure* secondaryMeasure = (m_Measures.size() >= 2) ? m_Measures[1] : nullptr;

	GraphicsPath primaryPath;
	GraphicsPath secondaryPath;
	GraphicsPath bothPath;

	Bitmap* primaryBitmap = m_PrimaryImage.GetImage();
	Bitmap* secondaryBitmap = m_SecondaryImage.GetImage();
	Bitmap* bothBitmap = m_OverlapImage.GetImage();

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	// Default values (GraphStart=Right, GraphOrientation=Vertical)
	int i;
	int startValue = 0;
	int* endValueLHS = &i;
	int* endValueRHS = &meterRect.Width;
	int step = 1;
	int endValue = -1; //(should be 0, but need to simulate <=)

	// GraphStart=Left, GraphOrientation=Vertical
	if (!m_GraphHorizontalOrientation)
	{
		if (m_GraphStartLeft)
		{
			startValue = meterRect.Width - 1;
			endValueLHS = &endValue;
			endValueRHS = &i;
			step = -1;
		}
	}
	else
	{
		if (!m_Flip)
		{
			endValueRHS = &meterRect.Height;
		}
		else
		{
			startValue = meterRect.Height - 1;
			endValueLHS = &endValue;
			endValueRHS = &i;
			step = -1;
		}
	}

	// Horizontal or Vertical graph
	if (m_GraphHorizontalOrientation)
	{
		for (i = startValue; *endValueLHS < *endValueRHS; i += step)
		{
			double value = (m_MaxPrimaryValue == 0.0) ?
				  0.0
				: m_PrimaryValues[(i + (m_MeterPos % meterRect.Height)) % meterRect.Height] / m_MaxPrimaryValue;
			value -= m_MinPrimaryValue;
			int primaryBarHeight = (int)(meterRect.Width * value);
			primaryBarHeight = min(meterRect.Width, primaryBarHeight);
			primaryBarHeight = max(0, primaryBarHeight);

			if (secondaryMeasure)
			{
				value = (m_MaxSecondaryValue == 0.0) ?
					  0.0
					: m_SecondaryValues[(i + m_MeterPos) % meterRect.Height] / m_MaxSecondaryValue;
				value -= m_MinSecondaryValue;
				int secondaryBarHeight = (int)(meterRect.Width * value);
				secondaryBarHeight = min(meterRect.Width, secondaryBarHeight);
				secondaryBarHeight = max(0, secondaryBarHeight);

				// Check which measured value is higher
				int bothBarHeight = min(primaryBarHeight, secondaryBarHeight);

				// Cache image/color rectangle for the both lines
				{
					Rect& r = m_GraphStartLeft ?
						  Rect(meterRect.X, meterRect.Y + startValue + (step * i), bothBarHeight, 1)
						: Rect(meterRect.X + meterRect.Width - bothBarHeight, meterRect.Y + startValue + (step * i), bothBarHeight, 1);

					bothPath.AddRectangle(r);  // cache
				}

				// Cache the image/color rectangle for the rest
				if (secondaryBarHeight > primaryBarHeight)
				{
					Rect& r = m_GraphStartLeft ?
						  Rect(meterRect.X + bothBarHeight, meterRect.Y + startValue + (step * i), secondaryBarHeight - bothBarHeight, 1)
						: Rect(meterRect.X + meterRect.Width - secondaryBarHeight, meterRect.Y + startValue + (step * i), secondaryBarHeight - bothBarHeight, 1);

					secondaryPath.AddRectangle(r);  // cache
				}
				else
				{
					Rect& r = m_GraphStartLeft ?
						  Rect(meterRect.X + bothBarHeight, meterRect.Y + startValue + (step * i), primaryBarHeight - bothBarHeight, 1)
						: Rect(meterRect.X + meterRect.Width - primaryBarHeight, meterRect.Y + startValue + (step * i), primaryBarHeight - bothBarHeight, 1);

					primaryPath.AddRectangle(r);  // cache
				}
			}
			else
			{
				Rect& r = m_GraphStartLeft ?
					  Rect(meterRect.X, meterRect.Y + startValue + (step * i), primaryBarHeight, 1)
					: Rect(meterRect.X + meterRect.Width - primaryBarHeight, meterRect.Y + startValue + (step * i), primaryBarHeight, 1);

				primaryPath.AddRectangle(r);  // cache
			}
		}
	}
	else	// GraphOrientation=Vertical
	{
		for (i = startValue; *endValueLHS < *endValueRHS; i += step)
		{
			double value = (m_MaxPrimaryValue == 0.0) ?
				  0.0
				: m_PrimaryValues[(i + m_MeterPos) % meterRect.Width] / m_MaxPrimaryValue;
			value -= m_MinPrimaryValue;
			int primaryBarHeight = (int)(meterRect.Height * value);
			primaryBarHeight = min(meterRect.Height, primaryBarHeight);
			primaryBarHeight = max(0, primaryBarHeight);

			if (secondaryMeasure)
			{
				value = (m_MaxSecondaryValue == 0.0) ?
					  0.0
					: m_SecondaryValues[(i + m_MeterPos) % meterRect.Width] / m_MaxSecondaryValue;
				value -= m_MinSecondaryValue;
				int secondaryBarHeight = (int)(meterRect.Height * value);
				secondaryBarHeight = min(meterRect.Height, secondaryBarHeight);
				secondaryBarHeight = max(0, secondaryBarHeight);

				// Check which measured value is higher
				int bothBarHeight = min(primaryBarHeight, secondaryBarHeight);

				// Cache image/color rectangle for the both lines
				{
					Rect& r = m_Flip ?
						  Rect(meterRect.X + startValue + (step * i), meterRect.Y, 1, bothBarHeight)
						: Rect(meterRect.X + startValue + (step * i), meterRect.Y + meterRect.Height - bothBarHeight, 1, bothBarHeight);

					bothPath.AddRectangle(r);  // cache
				}

				// Cache the image/color rectangle for the rest
				if (secondaryBarHeight > primaryBarHeight)
				{
					Rect& r = m_Flip ?
						  Rect(meterRect.X + startValue + (step * i), meterRect.Y + bothBarHeight, 1, secondaryBarHeight - bothBarHeight)
						: Rect(meterRect.X + startValue + (step * i), meterRect.Y + meterRect.Height - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight);

					secondaryPath.AddRectangle(r);  // cache
				}
				else
				{
					Rect& r = m_Flip ?
						  Rect(meterRect.X + startValue + (step * i), meterRect.Y + bothBarHeight, 1, primaryBarHeight - bothBarHeight)
						: Rect(meterRect.X + startValue + (step * i), meterRect.Y + meterRect.Height - primaryBarHeight, 1, primaryBarHeight - bothBarHeight);

					primaryPath.AddRectangle(r);  // cache
				}
			}
			else
			{
				Rect& r = m_Flip ?
					  Rect(meterRect.X + startValue + (step * i), meterRect.Y, 1, primaryBarHeight)
					: Rect(meterRect.X + startValue + (step * i), meterRect.Y + meterRect.Height - primaryBarHeight, 1, primaryBarHeight);

				primaryPath.AddRectangle(r);  // cache
			}
		}
	}

	// Draw cached rectangles
	if (primaryBitmap)
	{
		Rect r(meterRect.X, meterRect.Y, primaryBitmap->GetWidth(), primaryBitmap->GetHeight());

		graphics.SetClip(&primaryPath);
		graphics.DrawImage(primaryBitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
		graphics.ResetClip();
	}
	else
	{
		SolidBrush brush(m_PrimaryColor);
		graphics.FillPath(&brush, &primaryPath);
	}
	if (secondaryMeasure)
	{
		if (secondaryBitmap)
		{
			Rect r(meterRect.X, meterRect.Y, secondaryBitmap->GetWidth(), secondaryBitmap->GetHeight());

			graphics.SetClip(&secondaryPath);
			graphics.DrawImage(secondaryBitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
			graphics.ResetClip();
		}
		else
		{
			SolidBrush brush(m_SecondaryColor);
			graphics.FillPath(&brush, &secondaryPath);
		}
		if (bothBitmap)
		{
			Rect r(meterRect.X, meterRect.Y, bothBitmap->GetWidth(), bothBitmap->GetHeight());

			graphics.SetClip(&bothPath);
			graphics.DrawImage(bothBitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
			graphics.ResetClip();
		}
		else
		{
			SolidBrush brush(m_OverlapColor);
			graphics.FillPath(&brush, &bothPath);
		}
	}

	canvas.EndGdiplusContext();

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
