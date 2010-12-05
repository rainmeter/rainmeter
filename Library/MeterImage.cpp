/*
  Copyright (C) 2002 Kimmo Pekkola

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
#include "MeterImage.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

/*
** CMeterImage
**
** The constructor
**
*/
CMeterImage::CMeterImage(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_NeedsReload = false;
	m_WidthDefined = false;
	m_HeightDefined = false;
	m_PreserveAspectRatio = false;
	m_Tile = false;
}

/*
** ~CMeterImage
**
** The destructor
**
*/
CMeterImage::~CMeterImage()
{
}

/*
** Initialize
**
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterImage::Initialize()
{
	CMeter::Initialize();

	if (!m_Measure && !m_DynamicVariables && !m_ImageName.empty())
	{
		m_ImageNameResult = m_Path;
		m_ImageNameResult += m_ImageName;
		m_ImageNameResult = m_MeterWindow->MakePathAbsolute(m_ImageNameResult);
		LoadImage(m_ImageNameResult, true);
	}
}

/*
** LoadImage
**
** Loads the image from disk
**
*/
void CMeterImage::LoadImage(const std::wstring& imageName, bool bLoadAlways)
{
	m_Image.LoadImage(imageName, bLoadAlways);

	if (m_Image.IsLoaded())
	{
		// Calculate size of the meter
		Bitmap* bitmap = m_Image.GetImage();

		int imageW = bitmap->GetWidth();
		int imageH = bitmap->GetHeight();

		if (m_WidthDefined)
		{
			if (!m_HeightDefined)
			{
				m_H = (imageW == 0) ? 0 : (m_Tile) ? imageH : (int)(m_W * imageH / (double)imageW);
			}
		}
		else
		{
			if (m_HeightDefined)
			{
				m_W = (imageH == 0) ? 0 : (m_Tile) ? imageW : (int)(m_H * imageW / (double)imageH);
			}
			else
			{
				m_W = imageW;
				m_H = imageH;
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
void CMeterImage::ReadConfig(const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	// Check for extra measures
	if (!m_Initialized && !m_MeasureName.empty())
	{
		ReadMeasureNames(parser, section, m_MeasureNames);
	}

	m_Path = parser.ReadString(section, L"Path", L"");
	if (!m_Path.empty())
	{
		WCHAR ch = m_Path[m_Path.length() - 1];
		if (ch != L'\\' && ch != L'/')
		{
			m_Path += L"\\";
		}
	}

	m_ImageName = parser.ReadString(section, L"ImageName", L"");

	m_PreserveAspectRatio = 0!=parser.ReadInt(section, L"PreserveAspectRatio", 0);
	m_Tile = 0!=parser.ReadInt(section, L"Tile", 0);

	if (parser.IsValueDefined(section, L"W"))
	{
		m_WidthDefined = true;
	}
	if (parser.IsValueDefined(section, L"H"))
	{
		m_HeightDefined = true;
	}

	// Read tinting configs
	m_Image.ReadConfig(parser, section);
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterImage::Update()
{
	if (CMeter::Update())
	{
		if (m_Measure || m_DynamicVariables)
		{
			// Store the current values so we know if the image needs to be updated
			std::wstring oldResult = m_ImageNameResult;

			if (m_Measure)  // read from the measures
			{
				std::wstring val = m_Measure->GetStringValue(false, 1, 0, false);

				if (m_ImageName.empty())
				{
					m_ImageNameResult = val;
				}
				else
				{
					std::vector<std::wstring> stringValues;

					stringValues.push_back(val);

					// Get the values for the other measures
					for (size_t i = 0; i < m_Measures.size(); ++i)
					{
						stringValues.push_back(m_Measures[i]->GetStringValue(false, 1, 0, false));
					}

					m_ImageNameResult = m_ImageName;
					if (!ReplaceMeasures(stringValues, m_ImageNameResult))
					{
						// ImageName doesn't contain any measures, so use the result of MeasureName.
						m_ImageNameResult = val;
					}
				}
			}
			else  // read from the skin
			{
				m_ImageNameResult = m_ImageName;
			}

			if (!m_ImageNameResult.empty())
			{
				m_ImageNameResult.insert(0, m_Path);
				m_ImageNameResult = m_MeterWindow->MakePathAbsolute(m_ImageNameResult);
			}

			LoadImage(m_ImageNameResult, oldResult != m_ImageNameResult);
			return true;
		}
	}
	return false;
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterImage::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	if (m_Image.IsLoaded())
	{
		// Copy the image over the doublebuffer
		Bitmap* drawBitmap = m_Image.GetImage();

		int imageW = drawBitmap->GetWidth();
		int imageH = drawBitmap->GetHeight();

		if (imageW == 0 || imageH == 0 || m_W == 0 || m_H == 0) return true;

		int x = GetX();
		int y = GetY();

		int drawW = m_W;
		int drawH = m_H;

		ImageAttributes imgAttr;
		bool useImgAttr = false;

		if (m_Tile)
		{
			imageW = m_W;
			imageH = m_H;

			imgAttr.SetWrapMode(WrapModeTile);
			useImgAttr = true;
		}
		else if (m_PreserveAspectRatio)
		{
			if (m_WidthDefined && m_HeightDefined)
			{
				REAL imageRatio = imageW / (REAL)imageH;
				REAL meterRatio = m_W / (REAL)m_H;

				if (imageRatio >= meterRatio)
				{
					drawW = m_W;
					drawH = m_W * imageH / imageW;
				}
				else
				{
					drawW = m_H * imageW / imageH;
					drawH = m_H;
				}

				// Centering
				x += (m_W - drawW) / 2;
				y += (m_H - drawH) / 2;
			}
		}

		Rect r(x, y, drawW, drawH);
		graphics.DrawImage(drawBitmap, r, 0, 0, imageW, imageH, UnitPixel, (useImgAttr) ? &imgAttr : NULL);
	}

	return true;
}

/*
** BindMeasure
**
** Overridden method. The Image meters need not to be bound on anything
**
*/
void CMeterImage::BindMeasure(const std::list<CMeasure*>& measures)
{
	if (m_MeasureName.empty()) return;	// Allow NULL measure binding

	CMeter::BindMeasure(measures);

	std::vector<std::wstring>::const_iterator j = m_MeasureNames.begin();
	for (; j != m_MeasureNames.end(); ++j)
	{
		// Go through the list and check it there is a secondary measures for us
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
			std::wstring error = L"The meter [" + m_Name;
			error += L"] cannot be bound with [";
			error += (*j);
			error += L"]!";
			throw CError(error, __LINE__, __FILE__);
		}
	}
	CMeter::SetAllMeasures(m_Measures);
}
