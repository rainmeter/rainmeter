/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterImage.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/PathUtil.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

TintedImageHelper_DefineOptionArray(MeterImage::c_MaskOptionArray, L"Mask");

MeterImage::MeterImage(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"ImageName", nullptr, false, skin),
	m_MaskImage(L"MaskImageName", c_MaskOptionArray, false, skin),
	m_NeedsRedraw(false),
	m_DrawMode(DRAWMODE_NONE),
	m_ScaleMargins()
{
}

MeterImage::~MeterImage()
{
}

/*
** Load the image and get the dimensions of the meter from it.
**
*/
void MeterImage::Initialize()
{
	Meter::Initialize();

	if (m_Measures.empty() && !m_DynamicVariables && !m_ImageName.empty())
	{
		m_ImageNameResult = m_ImageName;
		LoadImage(m_ImageName, true);
	}
}

/*
** Loads the image from disk
**
*/
void MeterImage::LoadImage(const std::wstring& imageName, bool bLoadAlways)
{
	m_Image.LoadImage(imageName, bLoadAlways);

	if (m_Image.IsLoaded())
	{
		bool useMaskSize = false;
		m_MaskImage.LoadImage(m_MaskImageName, true);
		if (m_MaskImage.IsLoaded()) useMaskSize = true;

		// Calculate size of the meter
		Bitmap* bitmap = useMaskSize ? m_MaskImage.GetImage() : m_Image.GetImage();

		int imageW = bitmap->GetWidth();
		int imageH = bitmap->GetHeight();

		if (m_WDefined)
		{
			if (!m_HDefined)
			{
				m_H = (imageW == 0) ? 0 : (m_DrawMode == DRAWMODE_TILE) ? imageH : m_W * imageH / imageW;
				m_H += GetHeightPadding();
			}
		}
		else
		{
			if (m_HDefined)
			{
				m_W = (imageH == 0) ? 0 : (m_DrawMode == DRAWMODE_TILE) ? imageW : m_H * imageW / imageH;
				m_W += GetWidthPadding();
			}
			else
			{
				m_W = imageW + GetWidthPadding();
				m_H = imageH + GetHeightPadding();
			}
		}
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeterImage::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	m_ImageName = parser.ReadString(section, L"ImageName", L"");
	m_MaskImageName = parser.ReadString(section, L"MaskImageName", L"");

	int mode = parser.ReadInt(section, L"Tile", 0);
	if (mode != 0)
	{
		m_DrawMode = DRAWMODE_TILE;
	}
	else
	{
		mode = parser.ReadInt(section, L"PreserveAspectRatio", 0);
		switch (mode)
		{
		case 0:
			m_DrawMode = DRAWMODE_NONE;
			break;
		case 1:
		default:
			m_DrawMode = DRAWMODE_KEEPRATIO;
			break;
		case 2:
			m_DrawMode = DRAWMODE_KEEPRATIOANDCROP;
			break;
		}
	}

	static const RECT defMargins = {0};
	m_ScaleMargins = parser.ReadRECT(section, L"ScaleMargins", defMargins);

	// Deprecated!
	std::wstring path = parser.ReadString(section, L"Path", L"");
	PathUtil::AppendBackslashIfMissing(path);

	// Read tinting options
	m_Image.ReadOptions(parser, section, path.c_str());

	m_MaskImage.ReadOptions(parser, section, L"");

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
bool MeterImage::Update()
{
	if (Meter::Update())
	{
		if (!m_Measures.empty() || m_DynamicVariables)
		{
			// Store the current values so we know if the image needs to be updated
			std::wstring oldResult = m_ImageNameResult;

			if (!m_Measures.empty())  // read from the measures
			{
				if (m_ImageName.empty())
				{
					m_ImageNameResult = m_Measures[0]->GetStringOrFormattedValue(AUTOSCALE_OFF, 1, 0, false);
				}
				else
				{
					m_ImageNameResult = m_ImageName;
					if (!ReplaceMeasures(m_ImageNameResult, AUTOSCALE_OFF))
					{
						// ImageName doesn't contain any measures, so use the result of MeasureName.
						m_ImageNameResult = m_Measures[0]->GetStringOrFormattedValue(AUTOSCALE_OFF, 1, 0, false);
					}
				}
			}
			else  // read from the skin
			{
				m_ImageNameResult = m_ImageName;
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
bool MeterImage::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	if (m_Image.IsLoaded())
	{
		// Copy the image over the doublebuffer
		Bitmap* drawBitmap = m_Image.GetImage();

		int imageW = drawBitmap->GetWidth();
		int imageH = drawBitmap->GetHeight();
		
		if (imageW == 0 || imageH == 0 || m_W == 0 || m_H == 0) return true;

		Gdiplus::Rect meterRect = GetMeterRectPadding();

		int drawW = meterRect.Width;
		int drawH = meterRect.Height;

		if (m_MaskImage.IsLoaded())
		{
			Bitmap* maskBitmap = m_MaskImage.GetImage();

			imageW = maskBitmap->GetWidth();
			imageH = maskBitmap->GetHeight();
			int imageMW = drawBitmap->GetWidth();
			int imageMH = drawBitmap->GetHeight();

			int cropX = 0;
			int cropY = 0;
			int cropW = imageMW;
			int cropH = imageMH;

			REAL imageratio = imageMW / (REAL)imageMH;
			REAL meterRatio = meterRect.Width / (REAL)meterRect.Height;

			if (imageratio != meterRatio)
			{
				if (imageratio > meterRatio)
				{
					cropW = (int)(imageMH * meterRatio);
					cropX = (imageMW - cropW) / 2;
				}
				else
				{
					cropH = (int)(imageMW / meterRatio);
					cropY = (imageMH - cropH) / 2;
				}
			}

			canvas.DrawMaskedBitmap(drawBitmap, maskBitmap, meterRect, Rect(0, 0, imageW, imageH), Gdiplus::Rect(cropX, cropY, cropW, cropH));
		}

		else if (drawW == imageW && drawH == imageH &&
			m_ScaleMargins.left == 0 && m_ScaleMargins.top == 0 && m_ScaleMargins.right == 0 && m_ScaleMargins.bottom == 0)
		{
			canvas.DrawBitmap(drawBitmap, Rect(meterRect.X, meterRect.Y, drawW, drawH), Rect(0, 0, imageW, imageH));
		}
		else if (m_DrawMode == DRAWMODE_TILE)
		{
			Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

			ImageAttributes imgAttr;
			imgAttr.SetWrapMode(WrapModeTile);

			Rect r(meterRect.X, meterRect.Y, drawW, drawH);
			graphics.DrawImage(drawBitmap, r, 0, 0, drawW, drawH, UnitPixel, &imgAttr);

			canvas.EndGdiplusContext();
		}
		else if (m_DrawMode == DRAWMODE_KEEPRATIO || m_DrawMode == DRAWMODE_KEEPRATIOANDCROP)
		{
			int cropX = 0;
			int cropY = 0;
			int cropW = imageW;
			int cropH = imageH;

			if (m_WDefined && m_HDefined)
			{
				REAL imageRatio = imageW / (REAL)imageH;
				REAL meterRatio = meterRect.Width / (REAL)meterRect.Height;

				if (imageRatio != meterRatio)
				{
					if (m_DrawMode == DRAWMODE_KEEPRATIO)
					{
						if (imageRatio > meterRatio)
						{
							drawH = meterRect.Width * imageH / imageW;
							meterRect.Y += (meterRect.Height - drawH) / 2;
						}
						else
						{
							drawW = meterRect.Height * imageW / imageH;
							meterRect.X += (meterRect.Width - drawW) / 2;
						}
					}
					else
					{
						if (imageRatio > meterRatio)
						{
							cropW = (int)(imageH * meterRatio);
							cropX = (imageW - cropW) / 2;
						}
						else
						{
							cropH = (int)(imageW / meterRatio);
							cropY = (imageH - cropH) / 2;
						}
					}
				}
			}

			Rect r(meterRect.X, meterRect.Y, drawW, drawH);
			canvas.DrawBitmap(drawBitmap, r, Rect(cropX, cropY, cropW, cropH));
		}
		else
		{
			const RECT& m = m_ScaleMargins;

			if (m.top > 0)
			{
				if (m.left > 0)
				{
					// Top-Left
					Rect r(meterRect.X, meterRect.Y, m.left, m.top);
					canvas.DrawBitmap(drawBitmap, r, Rect(0, 0, m.left, m.top));
				}

				// Top
				Rect r(meterRect.X + m.left, meterRect.Y, drawW - m.left - m.right, m.top);
				canvas.DrawBitmap(drawBitmap, r, Rect(m.left, 0, imageW - m.left - m.right, m.top));

				if (m.right > 0)
				{
					// Top-Right
					Rect r(meterRect.X + drawW - m.right, meterRect.Y, m.right, m.top);
					canvas.DrawBitmap(drawBitmap, r, Rect(imageW - m.right, 0, m.right, m.top));
				}
			}

			if (m.left > 0)
			{
				// Left
				Rect r(meterRect.X, meterRect.Y + m.top, m.left, drawH - m.top - m.bottom);
				canvas.DrawBitmap(drawBitmap, r, Rect(0, m.top, m.left, imageH - m.top - m.bottom));
			}

			// Center
			Rect r(meterRect.X + m.left, meterRect.Y + m.top, drawW - m.left - m.right, drawH - m.top - m.bottom);
			canvas.DrawBitmap(drawBitmap, r, Rect(m.left, m.top, imageW - m.left - m.right, imageH - m.top - m.bottom));

			if (m.right > 0)
			{
				// Right
				Rect r(meterRect.X + drawW - m.right, meterRect.Y + m.top, m.right, drawH - m.top - m.bottom);
				canvas.DrawBitmap(drawBitmap, r, Rect(imageW - m.right, m.top, m.right, imageH - m.top - m.bottom));
			}

			if (m.bottom > 0)
			{
				if (m.left > 0)
				{
					// Bottom-Left
					Rect r(meterRect.X, meterRect.Y + drawH - m.bottom, m.left, m.bottom);
					canvas.DrawBitmap(drawBitmap, r, Rect(0, imageH - m.bottom, m.left, m.bottom));
				}

				// Bottom
				Rect r(meterRect.X + m.left, meterRect.Y + drawH - m.bottom, drawW - m.left - m.right, m.bottom);
				canvas.DrawBitmap(drawBitmap, r, Rect(m.left, imageH - m.bottom, imageW - m.left - m.right, m.bottom));

				if (m.right > 0)
				{
					// Bottom-Right
					Rect r(meterRect.X + drawW - m.right, meterRect.Y + drawH - m.bottom, m.right, m.bottom);
					canvas.DrawBitmap(drawBitmap, r, Rect(imageW - m.right, imageH - m.bottom, m.right, m.bottom));
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
void MeterImage::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}
