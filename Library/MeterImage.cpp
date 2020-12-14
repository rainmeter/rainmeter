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
	else if (m_Image.IsLoaded())
	{
		m_Image.DisposeImage();

		if (m_MaskImage.IsLoaded())
		{
			m_MaskImage.DisposeImage();
		}
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
	else
	{
		if (!m_WDefined) m_W = 0;
		if (!m_HDefined) m_H = 0;
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

		D2D1_RECT_F meterRect = GetMeterRectPadding();

		FLOAT drawW = meterRect.right - meterRect.left;
		FLOAT drawH = meterRect.bottom - meterRect.top;

		if (m_MaskImage.IsLoaded())
		{
			Gfx::D2DBitmap* maskBitmap = m_MaskImage.GetImage();

			imageW = maskBitmap->GetWidth();
			imageH = maskBitmap->GetHeight();
			UINT imageMW = drawBitmap->GetWidth();
			UINT imageMH = drawBitmap->GetHeight();

			D2D1_RECT_F crop = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
			crop.right = (FLOAT)imageMW;
			crop.bottom = (FLOAT)imageMH;

			const FLOAT imageratio = imageMW / (FLOAT)imageMH;
			const FLOAT meterRatio = drawW / drawH;

			if (imageratio != meterRatio)
			{
				if (imageratio > meterRatio)
				{
					crop.left = (imageMW - imageMH * meterRatio) / 2.0f;
					crop.right = (imageMW + imageMH * meterRatio) / 2.0f;
				}
				else
				{
					crop.top = (imageMH - imageMW / meterRatio) / 2.0f;
					crop.bottom = (imageMH + imageMW / meterRatio) / 2.0f;
				}
			}

			canvas.DrawMaskedBitmap(
				drawBitmap, maskBitmap, meterRect, D2D1::RectF(0.0f, 0.0f, (FLOAT)imageW, (FLOAT)imageH), crop);
		}

		else if (drawW == imageW && drawH == imageH &&
			m_ScaleMargins.left == 0 && m_ScaleMargins.top == 0 && m_ScaleMargins.right == 0 && m_ScaleMargins.bottom == 0)
		{
			canvas.DrawBitmap(drawBitmap, meterRect, D2D1::RectF(0.0f, 0.0f, drawW, drawH));
		}
		else if (m_DrawMode == DRAWMODE_TILE)
		{
			canvas.DrawTiledBitmap(drawBitmap, meterRect, D2D1::RectF(0.0f, 0.0f, drawW, drawH));
		}
		else if (m_DrawMode == DRAWMODE_KEEPRATIO || m_DrawMode == DRAWMODE_KEEPRATIOANDCROP)
		{
			D2D1_RECT_F crop = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
			crop.right = (FLOAT)imageW;
			crop.bottom = (FLOAT)imageH;

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
							meterRect.top += (meterRect.bottom - meterRect.top - drawH) / 2.0f;
						}
						else
						{
							drawW = drawH * imageW / imageH;
							meterRect.left += (meterRect.right - meterRect.left - drawW) / 2.0f;
						}
					}
					else
					{
						if (imageRatio > meterRatio)
						{
							crop.left = (imageW - imageH * meterRatio) / 2.0f;
							crop.right = (imageW + imageH * meterRatio) / 2.0f;
						}
						else
						{
							crop.top = (imageH - imageW / meterRatio) / 2.0f;
							crop.bottom = (imageH + imageW / meterRatio) / 2.0f;
						}
					}
				}
			}

			D2D1_RECT_F rect = meterRect;
			rect.right = rect.left + drawW;
			rect.bottom = rect.top + drawH;

			canvas.DrawBitmap(drawBitmap, rect, crop);
		}
		else
		{
			const RECT& m = m_ScaleMargins;

			if (m.top > 0)
			{
				if (m.left > 0)
				{
					// Top-Left
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left,
						meterRect.top,
						(FLOAT)m.left + meterRect.left,
						(FLOAT)m.top + meterRect.top);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF(0.0f, 0.0f, (FLOAT)m.left, (FLOAT)m.top));
				}

				// Top
				{
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left + (FLOAT)m.left,
						meterRect.top,
						meterRect.left + drawW - (FLOAT)m.right,
						meterRect.top + (FLOAT)m.top);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF((FLOAT)m.left, 0.0f, (FLOAT)(imageW - m.right), (FLOAT)m.top));
				}

				if (m.right > 0)
				{
					// Top-Right
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left + drawW - (FLOAT)m.right,
						meterRect.top,
						meterRect.left + drawW,
						meterRect.top + (FLOAT)m.top);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF((FLOAT)(imageW - m.right), 0.0f, (FLOAT)imageW, (FLOAT)m.top));
				}
			}

			if (m.left > 0)
			{
				// Left
				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left,
					meterRect.top + (FLOAT)m.top,
					meterRect.left + (FLOAT)m.left,
					meterRect.top + drawH - (FLOAT)m.bottom);
				canvas.DrawBitmap(
					drawBitmap,
					r,
					D2D1::RectF(0.0f, (FLOAT)m.top, (FLOAT)m.left, (FLOAT)(imageH - m.bottom)));
			}

			// Center
			{
				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left + (FLOAT)m.left,
					meterRect.top + (FLOAT)m.top,
					meterRect.left + drawW - (FLOAT)m.right,
					meterRect.top + drawH - (FLOAT)m.bottom);
				canvas.DrawBitmap(
					drawBitmap,
					r,
					D2D1::RectF((FLOAT)m.left, (FLOAT)m.top, (FLOAT)(imageW - m.right), (FLOAT)imageH - m.bottom));
			}

			if (m.right > 0)
			{
				// Right
				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left + drawW - (FLOAT)m.right,
					meterRect.top + (FLOAT)m.top,
					meterRect.left + drawW,
					meterRect.top + drawH - (FLOAT)m.bottom);
				canvas.DrawBitmap(
					drawBitmap,
					r,
					D2D1::RectF((FLOAT)(imageW - m.right), (FLOAT)m.top, (FLOAT)imageW, (FLOAT)(imageH - m.bottom)));
			}

			if (m.bottom > 0)
			{
				if (m.left > 0)
				{
					// Bottom-Left
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left,
						meterRect.top + drawH - (FLOAT)m.bottom,
						meterRect.left + (FLOAT)m.left,
						meterRect.top + drawH);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF(0, (FLOAT)(imageH - m.bottom), (FLOAT)m.left, (FLOAT)imageH));
				}

				// Bottom
				{
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left + (FLOAT)m.left,
						meterRect.top + drawH - (FLOAT)m.bottom,
						meterRect.left + drawW - (FLOAT)m.right,
						meterRect.top + drawH);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF((FLOAT)m.left, (FLOAT)(imageH - m.bottom), (FLOAT)(imageW - m.right), (FLOAT)imageH));
				}

				if (m.right > 0)
				{
					// Bottom-Right
					const D2D1_RECT_F r = D2D1::RectF(
						meterRect.left + drawW - (FLOAT)m.right,
						meterRect.top + drawH - (FLOAT)m.bottom,
						meterRect.left + drawW,
						meterRect.top + drawH);
					canvas.DrawBitmap(
						drawBitmap,
						r,
						D2D1::RectF((FLOAT)(imageW - m.right), (FLOAT)(imageH - m.bottom), (FLOAT)imageW, (FLOAT)imageH));
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
