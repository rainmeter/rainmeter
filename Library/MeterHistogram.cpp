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
#include "MeterHistogram.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** CMeterHistogram
**
** The constructor
**
*/
CMeterHistogram::CMeterHistogram(CMeterWindow* meterWindow) : CMeter(meterWindow),
	m_PrimaryColor(Color::Green),
	m_SecondaryColor(Color::Red),
	m_BothColor(Color::Yellow)
{
	m_PrimaryImage.SetConfigAttributes(L"PrimaryImage", L"Primary");
	m_SecondaryImage.SetConfigAttributes(L"SecondaryImage", L"Secondary");
	m_BothImage.SetConfigAttributes(L"BothImage", L"Both");

	m_PrimaryNeedsReload = false;
	m_SecondaryNeedsReload = false;
	m_BothNeedsReload = false;
	m_SecondaryMeasure = NULL;
	m_MeterPos = 0;
	m_PrimaryValues = NULL;
	m_SecondaryValues = NULL;
	m_Autoscale = false;
	m_Flip = false;
	m_MaxPrimaryValue = 1.0;
	m_MinPrimaryValue = 0.0;
	m_MaxSecondaryValue = 1.0;
	m_MinSecondaryValue = 0.0;
	m_WidthChanged = true;
}

/*
** ~CMeterHistogram
**
** The destructor
**
*/
CMeterHistogram::~CMeterHistogram()
{
	DisposeBuffer();
}

/*
** DisposeBuffer
**
** Disposes the buffers.
**
*/
void CMeterHistogram::DisposeBuffer()
{
	// Reset current position
	m_MeterPos = 0;

	// Delete buffers
	if (m_PrimaryValues)
	{
		delete [] m_PrimaryValues;
		m_PrimaryValues = NULL;
	}
	if (m_SecondaryValues)
	{
		delete [] m_SecondaryValues;
		m_SecondaryValues = NULL;
	}
}

/*
** Initialize
**
** Load the images and calculate the dimensions of the meter from them.
** Or create the brushes if solid color histogram is used.
**
*/
void CMeterHistogram::Initialize()
{
	CMeter::Initialize();

	// A sanity check
	if (m_SecondaryMeasure && !m_PrimaryImageName.empty() && (m_BothImageName.empty() || m_SecondaryImageName.empty()))
	{
        LSLog(LOG_WARNING, APPNAME, L"You need to define SecondaryImage and BothImage also!");

		m_PrimaryImage.DisposeImage();
		m_SecondaryImage.DisposeImage();
		m_BothImage.DisposeImage();
	}
	else
	{
		// Load the bitmaps if defined
		if (!m_PrimaryImageName.empty())
		{
			m_PrimaryImage.LoadImage(m_PrimaryImageName, m_PrimaryNeedsReload);

			if (m_PrimaryImage.IsLoaded())
			{
				int oldW = m_W;

				Bitmap* bitmap = m_PrimaryImage.GetImage();

				m_W = bitmap->GetWidth();
				m_H = bitmap->GetHeight();

				if (oldW != m_W)
				{
					m_WidthChanged = true;
				}
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

		if (!m_BothImageName.empty())
		{
			m_BothImage.LoadImage(m_BothImageName, m_BothNeedsReload);
		}
		else if (m_BothImage.IsLoaded())
		{
			m_BothImage.DisposeImage();
		}
	}

	if ((!m_PrimaryImageName.empty() && !m_PrimaryImage.IsLoaded()) ||
		(!m_SecondaryImageName.empty() && !m_SecondaryImage.IsLoaded()) ||
		(!m_BothImageName.empty() && !m_BothImage.IsLoaded()))
	{
		DisposeBuffer();

		m_WidthChanged = false;
	}
	else if (m_WidthChanged)
	{
		DisposeBuffer();

		// Create buffers for values
		if (m_W > 0)
		{
			m_PrimaryValues = new double[m_W];
			memset(m_PrimaryValues, 0, sizeof(double) * m_W);
			if (m_SecondaryMeasure)
			{
				m_SecondaryValues = new double[m_W];
				memset(m_SecondaryValues, 0, sizeof(double) * m_W);
			}
		}

		m_WidthChanged = false;
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterHistogram::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldPrimaryImageName = m_PrimaryImageName;
	std::wstring oldSecondaryImageName = m_SecondaryImageName;
	std::wstring oldBothImageName = m_BothImageName;
	int oldW = m_W;
	int oldH = m_H;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_PrimaryColor = parser.ReadColor(section, L"PrimaryColor", Color::Green);
	m_SecondaryColor = parser.ReadColor(section, L"SecondaryColor", Color::Red);
	m_BothColor = parser.ReadColor(section, L"BothColor", Color::Yellow);

	if (!m_Initialized && !m_MeasureName.empty())
	{
		m_SecondaryMeasureName = parser.ReadString(section, L"MeasureName2", L"");
		if (m_SecondaryMeasureName.empty())
		{
			m_SecondaryMeasureName = parser.ReadString(section, L"SecondaryMeasureName", L"");
		}
	}

	m_PrimaryImageName = parser.ReadString(section, L"PrimaryImage", L"");
	if (!m_PrimaryImageName.empty())
	{
		m_PrimaryImageName = m_MeterWindow->MakePathAbsolute(m_PrimaryImageName);

		// Read tinting configs
		m_PrimaryImage.ReadConfig(parser, section);
	}
	else
	{
		m_PrimaryImage.ClearConfigFlags();
	}

	m_SecondaryImageName = parser.ReadString(section, L"SecondaryImage", L"");
	if (!m_SecondaryImageName.empty())
	{
		m_SecondaryImageName = m_MeterWindow->MakePathAbsolute(m_SecondaryImageName);

		// Read tinting configs
		m_SecondaryImage.ReadConfig(parser, section);
	}
	else
	{
		m_SecondaryImage.ClearConfigFlags();
	}

	m_BothImageName = parser.ReadString(section, L"BothImage", L"");
	if (!m_BothImageName.empty())
	{
		m_BothImageName = m_MeterWindow->MakePathAbsolute(m_BothImageName);

		// Read tinting configs
		m_BothImage.ReadConfig(parser, section);
	}
	else
	{
		m_BothImage.ClearConfigFlags();
	}

	m_Autoscale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);

	if (m_Initialized)
	{
		if (m_PrimaryImageName.empty())
		{
			if (oldW != m_W)
			{
				m_WidthChanged = true;
				Initialize();  // Reload the image
			}
		}
		else
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;

			m_PrimaryNeedsReload = (oldPrimaryImageName != m_PrimaryImageName);
			m_SecondaryNeedsReload = (oldSecondaryImageName != m_SecondaryImageName);
			m_BothNeedsReload = (oldBothImageName != m_BothImageName);

			if (m_PrimaryNeedsReload ||
				m_SecondaryNeedsReload ||
				m_BothNeedsReload ||
				m_PrimaryImage.IsConfigsChanged() ||
				m_SecondaryImage.IsConfigsChanged() ||
				m_BothImage.IsConfigsChanged())
			{
				Initialize();  // Reload the image
			}
		}
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterHistogram::Update()
{
	if (CMeter::Update() && m_Measure && m_PrimaryValues)
	{
		// Gather values
		m_PrimaryValues[m_MeterPos] = m_Measure->GetValue();

		if (m_SecondaryMeasure && m_SecondaryValues)
		{
			m_SecondaryValues[m_MeterPos] = m_SecondaryMeasure->GetValue();
		}

		++m_MeterPos;
		m_MeterPos %= m_W;

		m_MaxPrimaryValue = m_Measure->GetMaxValue();
		m_MinPrimaryValue = m_Measure->GetMinValue();
		m_MaxSecondaryValue = 0.0;
		m_MinSecondaryValue = 0.0;
		if (m_SecondaryMeasure)
		{
			m_MaxSecondaryValue = m_SecondaryMeasure->GetMaxValue();
			m_MinSecondaryValue = m_SecondaryMeasure->GetMinValue();
		}

		if (m_Autoscale)
		{
			// Go through all values and find the max

			double newValue = 0.0;
			for (int i = 0; i < m_W; ++i)
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

			if (m_SecondaryMeasure && m_SecondaryValues)
			{
				for (int i = 0; i < m_W; ++i)
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
bool CMeterHistogram::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics) ||
		(m_Measure && !m_PrimaryValues) ||
		(m_SecondaryMeasure && !m_SecondaryValues)) return false;

	GraphicsPath primaryPath;
	GraphicsPath secondaryPath;
	GraphicsPath bothPath;

	Bitmap* primaryBitmap = m_PrimaryImage.GetImage();
	Bitmap* secondaryBitmap = m_SecondaryImage.GetImage();
	Bitmap* bothBitmap = m_BothImage.GetImage();

	int x = GetX();
	int y = GetY();

	for (int i = 0; i < m_W; ++i)
	{
		double value = (m_MaxPrimaryValue == 0.0) ?
			  0.0
			: m_PrimaryValues[(i + m_MeterPos) % m_W] / m_MaxPrimaryValue;
		value -= m_MinPrimaryValue;
		int primaryBarHeight = (int)(m_H * value);
		primaryBarHeight = min(m_H, primaryBarHeight);
		primaryBarHeight = max(0, primaryBarHeight);

		if (m_SecondaryMeasure)
		{
			value = (m_MaxSecondaryValue == 0.0) ?
				  0.0
				: m_SecondaryValues[(i + m_MeterPos) % m_W] / m_MaxSecondaryValue;
			value -= m_MinSecondaryValue;
			int secondaryBarHeight = (int)(m_H * value);
			secondaryBarHeight = min(m_H, secondaryBarHeight);
			secondaryBarHeight = max(0, secondaryBarHeight);

			// Check which measured value is higher
			int bothBarHeight = min(primaryBarHeight, secondaryBarHeight);

			// Cache image/color rectangle for the both lines
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y, 1, bothBarHeight)
					: Rect(x + i, y + m_H - bothBarHeight, 1, bothBarHeight);

				bothPath.AddRectangle(r);  // cache
			}

			// Cache the image/color rectangle for the rest
			if (secondaryBarHeight > primaryBarHeight)
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y + bothBarHeight, 1, secondaryBarHeight - bothBarHeight)
					: Rect(x + i, y + m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight);

				secondaryPath.AddRectangle(r);  // cache
			}
			else
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y + bothBarHeight, 1, primaryBarHeight - bothBarHeight)
					: Rect(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight);

				primaryPath.AddRectangle(r);  // cache
			}
		}
		else
		{
			Rect& r = (m_Flip) ?
				  Rect(x + i, y, 1, primaryBarHeight)
				: Rect(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight);

			primaryPath.AddRectangle(r);  // cache
		}
	}

	// Draw cached rectangles
	if (primaryBitmap)
	{
		Rect r(x, y, primaryBitmap->GetWidth(), primaryBitmap->GetHeight());

		graphics.SetClip(&primaryPath);
		graphics.DrawImage(primaryBitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
		graphics.ResetClip();
	}
	else
	{
		SolidBrush brush(m_PrimaryColor);
		graphics.FillPath(&brush, &primaryPath);
	}
	if (m_SecondaryMeasure)
	{
		if (secondaryBitmap)
		{
			Rect r(x, y, secondaryBitmap->GetWidth(), secondaryBitmap->GetHeight());

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
			Rect r(x, y, bothBitmap->GetWidth(), bothBitmap->GetHeight());

			graphics.SetClip(&bothPath);
			graphics.DrawImage(bothBitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
			graphics.ResetClip();
		}
		else
		{
			SolidBrush brush(m_BothColor);
			graphics.FillPath(&brush, &bothPath);
		}
	}

	return true;
}

/*
** BindMeasure
**
** Overwritten method to handle the secondary measure binding.
**
*/
void CMeterHistogram::BindMeasure(const std::list<CMeasure*>& measures)
{
	CMeter::BindMeasure(measures);

	if(!m_SecondaryMeasureName.empty())
	{
		// Go through the list and check it there is a secondary measure for us
		std::list<CMeasure*>::const_iterator i = measures.begin();
		for( ; i != measures.end(); ++i)
		{
			if(_wcsicmp((*i)->GetName(), m_SecondaryMeasureName.c_str()) == 0)
			{
				m_SecondaryMeasure = (*i);
				CMeter::SetAllMeasures(m_SecondaryMeasure);
				return;
			}
		}

		std::wstring error = L"The meter [" + m_Name;
		error += L"] cannot be bound with [";
		error += m_SecondaryMeasureName;
		error += L"]!";
        throw CError(error, __LINE__, __FILE__);
	}
}
