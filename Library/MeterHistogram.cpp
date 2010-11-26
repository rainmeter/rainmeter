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
	m_SecondaryMeasure = NULL;
	m_MeterPos = 0;
	m_PrimaryBitmap = NULL;
	m_SecondaryBitmap = NULL;
	m_BothBitmap = NULL;
	m_PrimaryValues = NULL;
	m_SecondaryValues = NULL;
	m_Autoscale = false;
	m_Flip = false;
	m_MaxPrimaryValue = 1.0;
	m_MinPrimaryValue = 0.0;
	m_MaxSecondaryValue = 1.0;
	m_MinSecondaryValue = 0.0;
	m_WidthChanged = false;
}

/*
** ~CMeterHistogram
**
** The destructor
**
*/
CMeterHistogram::~CMeterHistogram()
{
	DisposeImage();
	DisposeBuffer();
}

/*
** DisposeImage
**
** Disposes the image buffers.
**
*/
void CMeterHistogram::DisposeImage()
{
	if (m_PrimaryBitmap)
	{
		delete m_PrimaryBitmap;
		m_PrimaryBitmap = NULL;
	}
	if (m_SecondaryBitmap)
	{
		delete m_SecondaryBitmap;
		m_SecondaryBitmap = NULL;
	}
	if (m_BothBitmap)
	{
		delete m_BothBitmap;
		m_BothBitmap = NULL;
	}
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
        LSLog(LOG_DEBUG, APPNAME, L"You need to define SecondaryImage and BothImage also!");

		DisposeImage();
	}
	else
	{
		// Load the bitmaps if defined
		if(!m_PrimaryImageName.empty())
		{
			if (m_PrimaryBitmap) delete m_PrimaryBitmap;
			m_PrimaryBitmap = new Bitmap(m_PrimaryImageName.c_str());
			Status status = m_PrimaryBitmap->GetLastStatus();
			if(Ok != status)
			{
				DebugLog(L"PrimaryImage not found: %s", m_PrimaryImageName.c_str());

				delete m_PrimaryBitmap;
				m_PrimaryBitmap = NULL;
			}
			else
			{
				int oldW = m_W;

				m_W = m_PrimaryBitmap->GetWidth();
				m_H = m_PrimaryBitmap->GetHeight();

				if (oldW != m_W)
				{
					m_WidthChanged = true;
				}
			}
		}
		else
		{
			if (m_PrimaryBitmap)
			{
				delete m_PrimaryBitmap;
				m_PrimaryBitmap = NULL;
			}
		}

		if(!m_SecondaryImageName.empty())
		{
			if (m_SecondaryBitmap) delete m_SecondaryBitmap;
			m_SecondaryBitmap = new Bitmap(m_SecondaryImageName.c_str());
			Status status = m_SecondaryBitmap->GetLastStatus();
			if(Ok != status)
			{
				DebugLog(L"SecondaryImage not found: %s", m_SecondaryImageName.c_str());

				delete m_SecondaryBitmap;
				m_SecondaryBitmap = NULL;
			}
		}
		else
		{
			if (m_SecondaryBitmap)
			{
				delete m_SecondaryBitmap;
				m_SecondaryBitmap = NULL;
			}
		}

		if(!m_BothImageName.empty())
		{
			if (m_BothBitmap) delete m_BothBitmap;
			m_BothBitmap = new Bitmap(m_BothImageName.c_str());
			Status status = m_BothBitmap->GetLastStatus();
			if(Ok != status)
			{
				DebugLog(L"BothImage not found: %s", m_BothImageName.c_str());

				delete m_BothBitmap;
				m_BothBitmap = NULL;
			}
		}
		else
		{
			if (m_BothBitmap)
			{
				delete m_BothBitmap;
				m_BothBitmap = NULL;
			}
		}
	}

	if ((!m_PrimaryImageName.empty() && !m_PrimaryBitmap) ||
		(!m_SecondaryImageName.empty() && !m_SecondaryBitmap) ||
		(!m_BothImageName.empty() && !m_BothBitmap))
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

	m_SecondaryMeasureName = parser.ReadString(section, L"MeasureName2", L"");
	if (m_SecondaryMeasureName.empty())
	{
		m_SecondaryMeasureName = parser.ReadString(section, L"SecondaryMeasureName", L"");
	}

	m_PrimaryImageName = parser.ReadString(section, L"PrimaryImage", L"");
	if (!m_PrimaryImageName.empty())
	{
		m_PrimaryImageName = m_MeterWindow->MakePathAbsolute(m_PrimaryImageName);
	}

	m_SecondaryImageName = parser.ReadString(section, L"SecondaryImage", L"");
	if (!m_SecondaryImageName.empty())
	{
		m_SecondaryImageName = m_MeterWindow->MakePathAbsolute(m_SecondaryImageName);
	}

	m_BothImageName = parser.ReadString(section, L"BothImage", L"");
	if (!m_BothImageName.empty())
	{
		m_BothImageName = m_MeterWindow->MakePathAbsolute(m_BothImageName);
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

			if (oldPrimaryImageName != m_PrimaryImageName ||
				oldSecondaryImageName != m_SecondaryImageName ||
				oldBothImageName != m_BothImageName)
			{
				Initialize();  // Reload the image
			}
		}
	}
	else
	{
		m_WidthChanged = true;
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
		m_MaxSecondaryValue = 0;
		m_MinSecondaryValue = 0;
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

		if (m_SecondaryMeasure != NULL)
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

			// Draw image/color for the both lines
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y + bothBarHeight, 1, -bothBarHeight)
					: Rect(x + i, y + m_H - bothBarHeight, 1, bothBarHeight);

				if (m_BothBitmap)
				{
					graphics.DrawImage(m_BothBitmap, r, i, m_H - bothBarHeight, 1, bothBarHeight, UnitPixel);
				}
				else
				{
					bothPath.AddRectangle(r);  // cache
				}
			}

			// Draw the image/color for the rest
			if (secondaryBarHeight > primaryBarHeight)
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y + secondaryBarHeight, 1, -(secondaryBarHeight - bothBarHeight))
					: Rect(x + i, y + m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight);

				if (m_SecondaryBitmap)
				{
					graphics.DrawImage(m_SecondaryBitmap, r, i, m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight, UnitPixel);
				}
				else
				{
					secondaryPath.AddRectangle(r);  // cache
				}
			}
			else
			{
				Rect& r = (m_Flip) ?
					  Rect(x + i, y + primaryBarHeight, 1, -(primaryBarHeight - bothBarHeight))
					: Rect(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight);

				if (m_PrimaryBitmap)
				{
					graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight, UnitPixel);
				}
				else
				{
					primaryPath.AddRectangle(r);  // cache
				}
			}
		}
		else
		{
			Rect& r = (m_Flip) ?
				  Rect(x + i, y + primaryBarHeight, 1, -primaryBarHeight)
				: Rect(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight);

			if (m_PrimaryBitmap)
			{
				graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight, UnitPixel);
			}
			else
			{
				primaryPath.AddRectangle(r);  // cache
			}
		}
	}

	// Draw cached rectangles
	if (m_SecondaryMeasure != NULL)
	{
		if (!m_BothBitmap)
		{
			SolidBrush brush(m_BothColor);
			graphics.FillPath(&brush, &bothPath);
		}
		if (!m_SecondaryBitmap)
		{
			SolidBrush brush(m_SecondaryColor);
			graphics.FillPath(&brush, &secondaryPath);
		}
	}
	if (!m_PrimaryBitmap)
	{
		SolidBrush brush(m_PrimaryColor);
		graphics.FillPath(&brush, &primaryPath);
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
