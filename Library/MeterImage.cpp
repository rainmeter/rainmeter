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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterImage.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"
#include "System.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

/*
** The constructor
**
*/
CMeterImage::CMeterImage(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_NeedsRedraw(false),
	m_PreserveAspectRatio(false),
	m_Tile(false),
	m_ScaleMargins()
{
}

/*
** The destructor
**
*/
CMeterImage::~CMeterImage()
{
}

/*
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterImage::Initialize()
{
	CMeter::Initialize();

	if (m_Measures.empty() && !m_DynamicVariables && !m_ImageName.empty())
	{
		m_ImageNameResult = m_Path;
		m_ImageNameResult += m_ImageName;
		m_MeterWindow->MakePathAbsolute(m_ImageNameResult);
		LoadImage(m_ImageNameResult, true);
	}
}

/*
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

		if (m_WDefined)
		{
			if (!m_HDefined)
			{
				m_H = (imageW == 0) ? 0 : (m_Tile) ? imageH : m_W * imageH / imageW;
			}
		}
		else
		{
			if (m_HDefined)
			{
				m_W = (imageH == 0) ? 0 : (m_Tile) ? imageW : m_H * imageW / imageH;
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
** Read the options specified in the ini file.
**
*/
void CMeterImage::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	CMeter::ReadOptions(parser, section);

	m_Path = parser.ReadString(section, L"Path", L"");
	if (!m_Path.empty())
	{
		if (!CSystem::IsPathSeparator(m_Path[m_Path.length() - 1]))
		{
			m_Path += L'\\';
		}
	}

	m_ImageName = parser.ReadString(section, L"ImageName", L"");

	m_PreserveAspectRatio = 0!=parser.ReadInt(section, L"PreserveAspectRatio", 0);
	m_Tile = 0!=parser.ReadInt(section, L"Tile", 0);

	static const RECT defMargins = {0};
	m_ScaleMargins = parser.ReadRECT(section, L"ScaleMargins", defMargins);

	// Read tinting options
	m_Image.ReadOptions(parser, section);

	if (m_Initialized && m_Measures.empty() && !m_DynamicVariables)
	{
		Initialize();
		m_NeedsRedraw = true;
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool CMeterImage::Update()
{
	if (CMeter::Update())
	{
		if (!m_Measures.empty() || m_DynamicVariables)
		{
			// Store the current values so we know if the image needs to be updated
			std::wstring oldResult = m_ImageNameResult;

			if (!m_Measures.empty())  // read from the measures
			{
				if (m_ImageName.empty())
				{
					m_ImageNameResult = m_Measures[0]->GetStringValue(AUTOSCALE_OFF, 1, 0, false);
				}
				else
				{
					m_ImageNameResult = m_ImageName;
					if (!ReplaceMeasures(m_ImageNameResult))
					{
						// ImageName doesn't contain any measures, so use the result of MeasureName.
						m_ImageNameResult = m_Measures[0]->GetStringValue(AUTOSCALE_OFF, 1, 0, false);
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
				m_MeterWindow->MakePathAbsolute(m_ImageNameResult);
			}

			LoadImage(m_ImageNameResult, (wcscmp(oldResult.c_str(), m_ImageNameResult.c_str()) != 0));
			return true;
		}
		else if (m_NeedsRedraw)
		{
			m_NeedsRedraw = false;
			return true;
		}
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool CMeterImage::Draw(Graphics& graphics)
{
	if (!CMeter::Draw(graphics)) return false;

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

		if (drawW == imageW && drawH == imageH &&
			m_ScaleMargins.left == 0 && m_ScaleMargins.top == 0 && m_ScaleMargins.right == 0 && m_ScaleMargins.bottom == 0)
		{
			Rect r(x, y, drawW, drawH);
			graphics.DrawImage(drawBitmap, r, 0, 0, imageW, imageH, UnitPixel);
		}
		else if (m_Tile)
		{
			ImageAttributes imgAttr;
			imgAttr.SetWrapMode(WrapModeTile);

			Rect r(x, y, drawW, drawH);
			graphics.DrawImage(drawBitmap, r, 0, 0, drawW, drawH, UnitPixel, &imgAttr);
		}
		else if (m_PreserveAspectRatio)
		{
			if (m_WDefined && m_HDefined)
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

			Rect r(x, y, drawW, drawH);
			graphics.DrawImage(drawBitmap, r, 0, 0, imageW, imageH, UnitPixel);
		}
		else
		{
			const RECT m = m_ScaleMargins;

			if (m.top > 0)
			{
				if (m.left > 0)
				{
					// Top-Left
					Rect r(x, y, m.left, m.top);
					graphics.DrawImage(drawBitmap, r, 0, 0, m.left, m.top, UnitPixel);
				}

				// Top
				Rect r(x + m.left, y, drawW - m.left - m.right, m.top);
				graphics.DrawImage(drawBitmap, r, m.left, 0, imageW - m.left - m.right, m.top, UnitPixel);

				if (m.right > 0)
				{
					// Top-Right
					Rect r(x + drawW - m.right, y, m.right, m.top);
					graphics.DrawImage(drawBitmap, r, imageW - m.right, 0, m.right, m.top, UnitPixel);
				}
			}

			if (m.left > 0)
			{
				// Left
				Rect r(x, y + m.top, m.left, drawH - m.top - m.bottom);
				graphics.DrawImage(drawBitmap, r, 0, m.top, m.left, imageH - m.top - m.bottom, UnitPixel);
			}

			// Center
			Rect r(x + m.left, y + m.top, drawW - m.left - m.right, drawH - m.top - m.bottom);
			graphics.DrawImage(drawBitmap, r, m.left, m.top, imageW - m.left - m.right, imageH - m.top - m.bottom, UnitPixel);

			if (m.right > 0)
			{
				// Right
				Rect r(x + drawW - m.right, y + m.top, m.right, drawH - m.top - m.bottom);
				graphics.DrawImage(drawBitmap, r, imageW - m.right, m.top, m.right, imageH - m.top - m.bottom, UnitPixel);
			}

			if (m.bottom > 0)
			{
				if (m.left > 0)
				{
					// Bottom-Left
					Rect r(x, y + drawH - m.bottom, m.left, m.bottom);
					graphics.DrawImage(drawBitmap, r, 0, imageH - m.bottom, m.left, m.bottom, UnitPixel);
				}

				// Bottom
				Rect r(x + m.left, y + drawH - m.bottom, drawW - m.left - m.right, m.bottom);
				graphics.DrawImage(drawBitmap, r, m.left, imageH - m.bottom, imageW - m.left - m.right, m.bottom, UnitPixel);

				if (m.right > 0)
				{
					// Bottom-Right
					Rect r(x + drawW - m.right, y + drawH - m.bottom, m.right, m.bottom);
					graphics.DrawImage(drawBitmap, r, imageW - m.right, imageH - m.bottom, m.right, m.bottom, UnitPixel);
				}
			}
		}
	}

	return true;
}

/*
** Overridden method. The Image meters need not to be bound on anything
**
*/
void CMeterImage::BindMeasures(CConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}
