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
#include "MeterBitmap.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** CMeterBitmap
**
** The constructor
**
*/
CMeterBitmap::CMeterBitmap(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Bitmap = NULL;
	m_FrameCount = 1;
	m_ZeroFrame = false;
	m_Align = ALIGN_LEFT;
	m_Extend = false;
	m_Separation = 0;
	m_Digits = 0;
	m_Value = 0;
	m_TransitionFrameCount = 0;
	m_TransitionStartTicks = 0;
}

/*
** ~CMeterBitmap
**
** The destructor
**
*/
CMeterBitmap::~CMeterBitmap()
{
	if(m_Bitmap != NULL) delete m_Bitmap;
}

/*
** Initialize
**
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterBitmap::Initialize()
{
	CMeter::Initialize();

	// Load the bitmaps if defined
	if(!m_ImageName.empty())
	{
		if (m_Bitmap != NULL) delete m_Bitmap;
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
			DebugLog(L"Bitmap image not found: %s", m_ImageName.c_str());

			delete m_Bitmap;
			m_Bitmap = NULL;
		}
		else
		{
			m_W = m_Bitmap->GetWidth();
			m_H = m_Bitmap->GetHeight();

			if(m_H > m_W)
			{
				m_H = m_H / m_FrameCount;
			}
			else
			{
				m_W = m_W / m_FrameCount;
			}
		}
	}
	else
	{
		if (m_Bitmap)
		{
			delete m_Bitmap;
			m_Bitmap = NULL;
		}
	}
}

/*
** HitTest
**
** Checks if the given point is inside the meter.
**
*/
bool CMeterBitmap::HitTest(int x, int y)
{
	if (m_Extend)
	{
		int value = (int)m_Value;
		value = max(0, value);		// Only positive integers are supported

		int tmpValue = value;
		
		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			int realFrames = (m_FrameCount / (m_TransitionFrameCount + 1));
			do
			{
				++numOfNums;
				if (realFrames == 1)
				{
					tmpValue /= 2;
				}
				else
				{
					tmpValue /= realFrames;
				}
			} while (tmpValue > 0);
		}

		Rect rect(GetX(), GetY(), m_W * numOfNums + (numOfNums - 1) * m_Separation, m_H);

		if (m_Align == ALIGN_CENTER)
		{
			rect.Offset(-rect.Width / 2, 0);
		}
		else if (m_Align == ALIGN_RIGHT)
		{
			rect.Offset(-rect.Width, 0);
		}

		if (rect.Contains(x, y))
		{
			return true;
		}
		return false;
	}
	else
	{
		return CMeter::HitTest(x, y);
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterBitmap::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_ImageName = parser.ReadString(section, L"BitmapImage", L"");
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_ImageName);

	m_FrameCount = parser.ReadInt(section, L"BitmapFrames", 1);
	m_ZeroFrame = 0!=parser.ReadInt(section, L"BitmapZeroFrame", 0);

	m_Separation = parser.ReadInt(section, L"BitmapSeparation", 0);
	m_Extend = 0!=parser.ReadInt(section, L"BitmapExtend", 0);
	m_Digits = parser.ReadInt(section, L"BitmapDigits", 0);

	m_TransitionFrameCount = parser.ReadInt(section, L"BitmapTransitionFrames", 0);

	std::wstring align;
	align = parser.ReadString(section, L"BitmapAlign", L"LEFT");
	
	if(_wcsicmp(align.c_str(), L"LEFT") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if(_wcsicmp(align.c_str(), L"RIGHT") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if(_wcsicmp(align.c_str(), L"CENTER") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else
	{
		throw CError(std::wstring(L"BitmapAlign=") + align + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	if (m_Initialized)
	{
		if (oldImageName != m_ImageName)
		{
			Initialize();  // Reload the image
		}
		else
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;
		}
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterBitmap::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		double value = 0.0;
		if (m_Extend)
		{
			value = m_Measure->GetValue();
		}
		else
		{
			value = m_Measure->GetRelativeValue();
		}

		if (m_TransitionFrameCount > 0)
		{
			int realFrames = m_FrameCount / (m_TransitionFrameCount + 1);
			if ((int)(value * realFrames) != (int)(m_Value * realFrames))
			{
				m_TransitionStartValue = m_Value;
				m_TransitionStartTicks = GetTickCount();
			}
			else
			{
				m_TransitionStartTicks = 0;
			}
		}

		m_Value = value;

		return true;
	}
	return false;
}

/*
** HasActiveTransition
**
** Returns true if the meter has active transition animation.
**
*/
bool CMeterBitmap::HasActiveTransition()
{
	if (m_TransitionStartTicks > 0)
	{
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
bool CMeterBitmap::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	int newY, newX;

	if(m_FrameCount == 0 || m_Bitmap == NULL) return false;	// Unable to continue

	int x = GetX();
	int y = GetY();

	DWORD diffTicks = GetTickCount() - m_TransitionStartTicks;
	if (m_Extend)
	{
		int value = (int)m_Value;
		value = max(0, value);		// Only positive integers are supported

		int transitionValue = (int)m_TransitionStartValue;
		transitionValue = max(0, transitionValue);		// Only positive integers are supported

		int tmpValue = value;
		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			do
			{
				++numOfNums;
				if (m_FrameCount == 1)
				{
					tmpValue /= 2;
				}
				else
				{
					tmpValue /= m_FrameCount;
				}
			} while (tmpValue > 0);
		}

		// Blit the images
		int offset;
		if (m_Align == ALIGN_RIGHT)
		{
			offset = 0;
		}
		else if (m_Align == ALIGN_CENTER)
		{
			offset = numOfNums * (m_W + m_Separation) / 2;
		}
		else
		{
			offset = numOfNums * (m_W + m_Separation);
		}

		do
		{
			offset = offset - (m_W + m_Separation);

			Rect r(x + offset, y, m_W, m_H);

			int realFrames = (m_FrameCount / (m_TransitionFrameCount + 1));
			int frame = (value % realFrames) * (m_TransitionFrameCount + 1);

			// If transition is ongoing the pick the correct frame
			if (m_TransitionStartTicks > 0)
			{
				int range = ((value % realFrames) - ((int)transitionValue % realFrames)) * (m_TransitionFrameCount + 1);
				if (range < 0)
				{
					range += m_FrameCount;
				}
				int frameAdjustment = 0;
				frameAdjustment = range * diffTicks / ((m_TransitionFrameCount + 1) * m_MeterWindow->GetTransitionUpdate());
				if (frameAdjustment > range)
				{
					m_TransitionStartTicks = 0;		// The transition is over. Draw with the real value.
				}
				else
				{
					frame = ((int)transitionValue % realFrames) * (m_TransitionFrameCount + 1);
					frame += frameAdjustment;
					frame %= m_FrameCount;
				}
			}

//			DebugLog(L"[%i] Value: %f Frame: %i (Transition = %s)", GetTickCount(), m_Value, frame, m_TransitionStartTicks > 0 ? L"true" : L"false");

			if(m_Bitmap->GetHeight() > m_Bitmap->GetWidth())
			{
				newX = 0;
				newY = m_H * frame;
			}
			else
			{
				newX = m_W * frame;
				newY = 0;
			}

			graphics.DrawImage(m_Bitmap, r, newX, newY, m_W, m_H, UnitPixel);
			if (m_FrameCount == 1)
			{
				value /= 2;
				transitionValue /= 2;
			}
			else
			{
				value /= realFrames;
				transitionValue /= realFrames;
			}
			--numOfNums;
		} while (numOfNums > 0);
	}
	else
	{
		int frame = 0;
		int realFrames = (m_FrameCount / (m_TransitionFrameCount + 1));

		if (m_ZeroFrame)
		{
			// Use the first frame only if the value is zero
			if (m_Value > 0)
			{
				frame = (int)(m_Value * (realFrames - 1)) * (m_TransitionFrameCount + 1);
			}
		}
		else
		{
			// Select the correct frame linearly
			frame = (int)(m_Value * realFrames) * (m_TransitionFrameCount + 1);
		}

		// If transition is ongoing the pick the correct frame
		if (m_TransitionStartTicks > 0)
		{
			if ((int)diffTicks > ((m_TransitionFrameCount + 1) * m_MeterWindow->GetTransitionUpdate()))
			{
				m_TransitionStartTicks = 0;		// The transition is over. Draw with the real value.
			}
			else
			{
				double range = (m_Value - m_TransitionStartValue);
				double adjustment = range * diffTicks / ((m_TransitionFrameCount + 1) * m_MeterWindow->GetTransitionUpdate());
				double frameAdjustment = adjustment * m_FrameCount;

				frame = (int)(m_TransitionStartValue * realFrames) * (m_TransitionFrameCount + 1);
				frame += (int)frameAdjustment;
				frame %= m_FrameCount;
				frame = max(0, frame);
			}
		}

//		DebugLog(L"[%i] Value: %f Frame: %i (Transition = %s)", GetTickCount(), m_Value, frame, m_TransitionStartTicks > 0 ? L"true" : L"false");

		if(m_Bitmap->GetHeight() > m_Bitmap->GetWidth())
		{
			newX = 0;
			newY = frame * m_H;
		}
		else
		{
			newX = frame * m_W;
			newY = 0;
		}

		// Blit the image
		Rect r(x, y, m_W, m_H);
		graphics.DrawImage(m_Bitmap, r, newX, newY, m_W, m_H, UnitPixel);
	}

	return true;
}

