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

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

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
CMeterHistogram::CMeterHistogram(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_SecondaryMeasure = NULL;
	m_PrimaryColor = 0;
	m_SecondaryColor = 0;
	m_BothColor = 0;
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
}

/*
** ~CMeterHistogram
**
** The destructor
**
*/
CMeterHistogram::~CMeterHistogram()
{
	if (m_PrimaryBitmap) delete m_PrimaryBitmap;
	if (m_SecondaryBitmap) delete m_SecondaryBitmap;
	if (m_BothBitmap) delete m_BothBitmap;
	if (m_PrimaryValues) delete [] m_PrimaryValues;
	if (m_SecondaryValues) delete [] m_SecondaryValues;
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

	// Load the bitmaps if defined
	if(!m_PrimaryImageName.empty())
	{
		m_PrimaryBitmap = new Bitmap(m_PrimaryImageName.c_str());
		Status status = m_PrimaryBitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"PrimaryImage not found: ") + m_PrimaryImageName, __LINE__, __FILE__);
		}

		m_W = m_PrimaryBitmap->GetWidth();
		m_H = m_PrimaryBitmap->GetHeight();
	}

	if(!m_SecondaryImageName.empty())
	{
		m_SecondaryBitmap = new Bitmap(m_SecondaryImageName.c_str());
		Status status = m_SecondaryBitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"SecondaryImage not found: ") + m_SecondaryImageName, __LINE__, __FILE__);
		}
	}

	if(!m_BothImageName.empty())
	{
		m_BothBitmap = new Bitmap(m_BothImageName.c_str());
		Status status = m_BothBitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"BothImage not found: ") + m_BothImageName, __LINE__, __FILE__);
		}
	}

	// A sanity check
	if (m_SecondaryMeasure && !m_PrimaryImageName.empty() && (m_BothImageName.empty() || m_SecondaryImageName.empty()))
	{
        throw CError(std::wstring(L"You need to define SecondaryImage and BothImage also!"), __LINE__, __FILE__);
	}

	// Create buffers for values
	m_PrimaryValues = new double[m_W];
	memset(m_PrimaryValues, 0, sizeof(double) * m_W);
	if (m_SecondaryMeasure)
	{
		m_SecondaryValues = new double[m_W];
		memset(m_SecondaryValues, 0, sizeof(double) * m_W);
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
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_PrimaryColor = parser.ReadColor(section, L"PrimaryColor", Color::Green);
	m_SecondaryColor = parser.ReadColor(section, L"SecondaryColor", Color::Red);
	m_BothColor = parser.ReadColor(section, L"BothColor", Color::Yellow);

	m_SecondaryMeasureName = parser.ReadString(section, L"SecondaryMeasureName", L"");

	m_PrimaryImageName = parser.ReadString(section, L"PrimaryImage", L"");
	m_PrimaryImageName = m_MeterWindow->MakePathAbsolute(m_PrimaryImageName);

	m_SecondaryImageName = parser.ReadString(section, L"SecondaryImage", L"");
	m_SecondaryImageName = m_MeterWindow->MakePathAbsolute(m_SecondaryImageName);

	m_BothImageName = parser.ReadString(section, L"BothImage", L"");
	m_BothImageName = m_MeterWindow->MakePathAbsolute(m_BothImageName);

	m_Autoscale = 0!=parser.ReadInt(section, L"AutoScale", 0);
	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterHistogram::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		// Gather values
		m_PrimaryValues[m_MeterPos] = m_Measure->GetValue();

		if (m_SecondaryMeasure != NULL)
		{
			m_SecondaryValues[m_MeterPos] = m_SecondaryMeasure->GetValue();
		}

		m_MeterPos++;
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

			int i;
			double newValue = 0;
			for (i = 0; i != m_W; i++)
			{
				newValue = max(newValue, m_PrimaryValues[i]);
			}

			// Scale the value up to nearest power of 2
			m_MaxPrimaryValue = 2;
			while(m_MaxPrimaryValue < newValue && m_MaxPrimaryValue != 0)
			{
				m_MaxPrimaryValue *= 2;
			}

			if (m_SecondaryMeasure)
			{
				for (i = 0; i != m_W; i++)
				{
					newValue = max(newValue, m_SecondaryValues[i]);
				}

				// Scale the value up to nearest power of 2
				m_MaxSecondaryValue = 2;
				while(m_MaxSecondaryValue < newValue && m_MaxSecondaryValue != 0)
				{
					m_MaxSecondaryValue *= 2;
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
bool CMeterHistogram::Draw()
{
	if(!CMeter::Draw()) return false;

	Graphics graphics(m_MeterWindow->GetDoubleBuffer());
	Pen primaryPen(m_PrimaryColor);
	Pen secondaryPen(m_SecondaryColor);
	Pen bothPen(m_BothColor);
	
	int x = GetX();
	int y = GetY();

	for (int i = 0; i < m_W; i++)
	{
		double value;
		
		if (m_MaxPrimaryValue == 0.0)
		{
			value = 0;
		}
		else
		{
			value = m_PrimaryValues[(i + m_MeterPos) % m_W] / m_MaxPrimaryValue;
		}
		value -= m_MinPrimaryValue;
		int primaryBarHeight = (int)(m_H * value);
		primaryBarHeight = min(m_H, primaryBarHeight);
		primaryBarHeight = max(0, primaryBarHeight);

		if (m_SecondaryMeasure != NULL)
		{
			if (m_MaxSecondaryValue == 0.0)
			{
				value = 0;
			}
			else
			{
				value = m_SecondaryValues[(i + m_MeterPos) % m_W] / m_MaxSecondaryValue;
			}
			value -= m_MinSecondaryValue;
			int secondaryBarHeight = (int)(m_H * value);
			secondaryBarHeight = min(m_H, secondaryBarHeight);
			secondaryBarHeight = max(0, secondaryBarHeight);

			// Check which measured value is higher
			int bothBarHeight;
			if (secondaryBarHeight > primaryBarHeight)
			{
				bothBarHeight = primaryBarHeight;
			}
			else
			{
				bothBarHeight = secondaryBarHeight;
			}

			// Draw image/color for the both lines
			if (m_PrimaryBitmap)
			{
				if (m_Flip)
				{
					Rect r(x + i, y + bothBarHeight, 1, -bothBarHeight);
					graphics.DrawImage(m_BothBitmap, r, i, m_H - bothBarHeight, 1, bothBarHeight, UnitPixel);
				}
				else
				{
					Rect r(x + i, y + m_H - bothBarHeight, 1, bothBarHeight);
					graphics.DrawImage(m_BothBitmap, r, i, m_H - bothBarHeight, 1, bothBarHeight, UnitPixel);
				}
			}
			else
			{
				if (m_Flip)
				{
					graphics.DrawLine(&bothPen, x + i, y, x + i, y + bothBarHeight);
				}
				else
				{
					graphics.DrawLine(&bothPen, x + i, y + m_H, x + i, y + m_H - bothBarHeight);
				}
			}

			// Draw the image/color for the rest
			if (secondaryBarHeight > primaryBarHeight)
			{
				if (m_SecondaryBitmap)
				{
					if (m_Flip)
					{
						Rect r(x + i, y + secondaryBarHeight, 1, -(secondaryBarHeight - bothBarHeight));
						graphics.DrawImage(m_SecondaryBitmap, r, i, m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight, UnitPixel);
					}
					else
					{
						Rect r(x + i, y + m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight);
						graphics.DrawImage(m_SecondaryBitmap, r, i, m_H - secondaryBarHeight, 1, secondaryBarHeight - bothBarHeight, UnitPixel);
					}
				}
				else
				{
					if (m_Flip)
					{
						graphics.DrawLine(&secondaryPen, x + i, y + bothBarHeight, x + i, y + secondaryBarHeight);
					}
					else
					{
						graphics.DrawLine(&secondaryPen, x + i, y + m_H - bothBarHeight, x + i, y + m_H - secondaryBarHeight);
					}
				}
			}
			else
			{
				if (m_PrimaryBitmap)
				{
					if (m_Flip)
					{
						Rect r(x + i, y + primaryBarHeight, 1, -(primaryBarHeight - bothBarHeight));
						graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight, UnitPixel);
					}
					else
					{
						Rect r(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight);
						graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight - bothBarHeight, UnitPixel);
					}
				}
				else
				{
					if (m_Flip)
					{
						graphics.DrawLine(&primaryPen, x + i, y, x + i, y + primaryBarHeight);
					}
					else
					{
						graphics.DrawLine(&primaryPen, x + i, y + m_H - bothBarHeight, x + i, y + m_H - primaryBarHeight);
					}
				}
			}
		}
		else
		{
			if (m_PrimaryBitmap)
			{
				if (m_Flip)
				{
					Rect r(x + i, y + primaryBarHeight, 1, -primaryBarHeight);
					graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight, UnitPixel);
				}
				else
				{
					Rect r(x + i, y + m_H - primaryBarHeight, 1, primaryBarHeight);
					graphics.DrawImage(m_PrimaryBitmap, r, i, m_H - primaryBarHeight, 1, primaryBarHeight, UnitPixel);
				}
			}
			else
			{
				if (m_Flip)
				{
					graphics.DrawLine(&primaryPen, x + i, y, x + i, y + primaryBarHeight);
				}
				else
				{
					graphics.DrawLine(&primaryPen, x + i, y + m_H, x + i, y + m_H - primaryBarHeight);
				}
			}
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
void CMeterHistogram::BindMeasure(std::list<CMeasure*>& measures)
{
	CMeter::BindMeasure(measures);

	if(!m_SecondaryMeasureName.empty())
	{
		// Go through the list and check it there is a secondary measure for us
		std::list<CMeasure*>::iterator i = measures.begin();
		for( ; i != measures.end(); i++)
		{
			if(_wcsicmp((*i)->GetName(), m_SecondaryMeasureName.c_str()) == 0)
			{
				m_SecondaryMeasure = (*i);
				return;
			}
		}

        throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + m_SecondaryMeasureName + L"]!", __LINE__, __FILE__);
	}
}
