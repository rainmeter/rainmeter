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
#include "../Common/PathUtil.h"
#include "../Common/Gfx/Canvas.h"

GeneralImageHelper_DefineOptionArray(MeterImage::c_MaskOptionArray, L"Mask");

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
	m_Image.LoadImage(imageName);

	if (m_Image.IsLoaded())
	{
		bool useMaskSize = false;
		m_MaskImage.LoadImage(m_MaskImageName);
		if (m_MaskImage.IsLoaded()) useMaskSize = true;

		// Calculate size of the meter
		Gfx::D2DBitmap* bitmap = useMaskSize ? m_MaskImage.GetImage() : m_Image.GetImage();

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
					m_ImageNameResult = m_Measures[0]->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, 0, false);
				}
				else
				{
					m_ImageNameResult = m_ImageName;
					if (!ReplaceMeasures(m_ImageNameResult, AUTOSCALE_OFF))
					{
						// ImageName doesn't contain any measures, so use the result of MeasureName.
						m_ImageNameResult = m_Measures[0]->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, 0, false);
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
		Gfx::D2DBitmap* drawBitmap = m_Image.GetImage();

		int imageW = drawBitmap->GetWidth();
		int imageH = drawBitmap->GetHeight();
		
		if (imageW == 0 || imageH == 0 || m_W == 0 || m_H == 0) return true;

		D2D1_RECT_F meterRect = Gfx::Util::ToRectF(GetMeterRectPadding());

		FLOAT drawW = meterRect.right - meterRect.left;
		FLOAT drawH = meterRect.bottom - meterRect.top;

		if (m_MaskImage.IsLoaded())
		{
			Gfx::D2DBitmap* maskBitmap = m_MaskImage.GetImage();

			imageW = maskBitmap->GetWidth();
			imageH = maskBitmap->GetHeight();
			int imageMW = drawBitmap->GetWidth();
			int imageMH = drawBitmap->GetHeight();

			D2D1_RECT_F crop = {0};
			crop.right = imageMW;
			crop.bottom = imageMH;

			const FLOAT imageratio = imageMW / imageMH;
			const FLOAT meterRatio = drawW / drawH;

			if (imageratio != meterRatio)
			{
				if (imageratio > meterRatio)
				{
					crop.left = (imageMW - imageMH * meterRatio) / 2;
					crop.right = (imageMW + imageMH * meterRatio) / 2;
				}
				else
				{
					crop.top = (imageMH - imageMW / meterRatio) / 2;
					crop.bottom = (imageMH + imageMW / meterRatio) / 2;
				}
			}

			canvas.DrawMaskedBitmap(drawBitmap, maskBitmap, meterRect, D2D1::RectF(0, 0, imageW, imageH), crop);
		}

		else if (drawW == imageW && drawH == imageH &&
			m_ScaleMargins.left == 0 && m_ScaleMargins.top == 0 && m_ScaleMargins.right == 0 && m_ScaleMargins.bottom == 0)
		{
			canvas.DrawBitmap(drawBitmap, meterRect, D2D1::RectF(0, 0, drawW, drawH));
		}
		else if (m_DrawMode == DRAWMODE_TILE)
		{
			canvas.DrawTiledBitmap(drawBitmap, meterRect, D2D1::RectF(0,0,drawW, drawH));
		}
		else if (m_DrawMode == DRAWMODE_KEEPRATIO || m_DrawMode == DRAWMODE_KEEPRATIOANDCROP)
		{
			D2D1_RECT_F crop = {0};
			crop.right = imageW;
			crop.bottom = imageH;

			if (m_WDefined && m_HDefined)
			{
				FLOAT imageRatio = imageW / (FLOAT)imageH;
				FLOAT meterRatio = drawW / drawH;

				if (imageRatio != meterRatio)
				{
					if (m_DrawMode == DRAWMODE_KEEPRATIO)
					{
						if (imageRatio > meterRatio)
						{
							drawH = drawW * imageH / imageW;
							meterRect.top += (meterRect.bottom - meterRect.top - drawH) / 2;
						}
						else
						{
							drawW = drawH * imageW / imageH;
							meterRect.left += (meterRect.right - meterRect.left - drawW) / 2;
						}
					}
					else
					{
						if (imageRatio > meterRatio)
						{
							crop.left = (imageW - imageH * meterRatio) / 2;
							crop.right = (imageW + imageH * meterRatio) / 2;
						}
						else
						{
							crop.top = (imageH - imageW / meterRatio) / 2;
							crop.right = (imageH + imageW / meterRatio) / 2;
						}
					}
				}
			}

			canvas.DrawBitmap(drawBitmap, meterRect, crop);
		}
		else
		{
			const RECT& m = m_ScaleMargins;

			if (m.top > 0)
			{
				if (m.left > 0)
				{
					// Top-Left
					D2D1_RECT_F r = { meterRect.left, meterRect.top, m.left + meterRect.left, m.top + meterRect.top };
					canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0, 0, m.left, m.top));
				}

				// Top
				D2D1_RECT_F r = { meterRect.left + m.left, meterRect.top, meterRect.left + drawW - m.right, meterRect.top + m.top };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(m.left, 0, imageW - m.right, m.top));

				if (m.right > 0)
				{
					// Top-Right
					D2D1_RECT_F r = { meterRect.left + drawW - m.right, meterRect.top, meterRect.left + drawW, meterRect.top + m.top };
					canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(imageW - m.right, 0, imageW, m.top));
				}
			}

			if (m.left > 0)
			{
				// Left
				D2D1_RECT_F r = { meterRect.left, meterRect.top + m.top, meterRect.left + m.left, meterRect.top + drawH - m.bottom };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0, m.top, m.left, imageH - m.bottom));
			}

			// Center
			D2D1_RECT_F r = { meterRect.left + m.left, meterRect.top + m.top, meterRect.left + drawW - m.right, meterRect.top + drawH - m.bottom };
			canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(m.left, m.top, imageW - m.right, imageH - m.bottom));

			if (m.right > 0)
			{
				// Right
				D2D1_RECT_F r = { meterRect.left + drawW - m.right, meterRect.top + m.top, meterRect.left + drawW, meterRect.top + drawH - m.bottom };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(imageW - m.right, m.top, imageW, imageH - m.bottom));
			}

			if (m.bottom > 0)
			{
				if (m.left > 0)
				{
					// Bottom-Left
					D2D1_RECT_F r = { meterRect.left, meterRect.top + drawH - m.bottom, meterRect.left + m.left, meterRect.top + drawH };
					canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0, imageH - m.bottom, m.left, imageH));
				}

				// Bottom
				D2D1_RECT_F r = { meterRect.left + m.left, meterRect.top + drawH - m.bottom, meterRect.left + drawW - m.right, meterRect.top + drawH };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(m.left, imageH - m.bottom, imageW - m.right, imageH));

				if (m.right > 0)
				{
					// Bottom-Right
					D2D1_RECT_F r = { meterRect.left + drawW - m.right, meterRect.top + drawH - m.bottom, meterRect.left + drawW, meterRect.top + drawH };
					canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(imageW - m.right, imageH - m.bottom, imageW, imageH));
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
