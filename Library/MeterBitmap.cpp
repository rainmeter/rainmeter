/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterBitmap.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

MeterBitmap::MeterBitmap(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"BitmapImage", nullptr, true, skin),
	m_NeedsReload(false),
	m_ZeroFrame(false),
	m_FrameCount(1),
	m_TransitionFrameCount(0),
	m_Align(ALIGN_LEFT),
	m_Extend(false),
	m_Separation(0),
	m_Digits(0),
	m_Value(0.0),
	m_TransitionStartTicks(0),
	m_TransitionStartValue(0.0)
{
}

MeterBitmap::~MeterBitmap()
{
}

/*
** Load the image and get the dimensions of the meter from it.
**
*/
void MeterBitmap::Initialize()
{
	Meter::Initialize();

	// Load the bitmaps if defined
	if (!m_ImageName.empty())
	{
		m_Image.LoadImage(m_ImageName, m_NeedsReload);

		if (m_Image.IsLoaded())
		{
			Bitmap* bitmap = m_Image.GetImage();

			m_W = bitmap->GetWidth();
			m_H = bitmap->GetHeight();

			int extraSpace = (m_Digits - 1) * m_Separation;
			extraSpace = max(0, extraSpace);

			int digits = max(1, m_Digits);

			if (m_H > m_W)
			{
				m_H = (m_H / m_FrameCount);

				if (m_Extend)  // Increase meter height to account for BitmapDigigts and BitmapSeparation options
				{
					m_H *= digits;
					m_H += extraSpace;
				}
			}
			else
			{
				m_W = (m_W / m_FrameCount);

				if (m_Extend)  // Increase meter width to account for BitmapDigigts and BitmapSeparation options
				{
					m_W *= digits;
					m_W += extraSpace;
				}
			}

			m_W += GetWidthPadding();
			m_H += GetHeightPadding();
		}
	}
	else if (m_Image.IsLoaded())
	{
		m_Image.DisposeImage();
	}
}

/*
** Checks if the given point is inside the meter.
**
*/
bool MeterBitmap::HitTest(int x, int y)
{
	if (m_Extend)
	{
		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			int tmpValue = (int)m_Value;
			tmpValue = max(0, tmpValue);		// Only positive integers are supported

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
			}
			while (tmpValue > 0);
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
		return Meter::HitTest(x, y);
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeterBitmap::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	Meter::ReadOptions(parser, section);

	m_ImageName = parser.ReadString(section, L"BitmapImage", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
	}
	else
	{
		m_Image.ClearOptionFlags();
	}

	m_FrameCount = parser.ReadInt(section, L"BitmapFrames", 1);
	m_ZeroFrame = parser.ReadBool(section, L"BitmapZeroFrame", false);

	m_Separation = parser.ReadInt(section, L"BitmapSeparation", 0);
	m_Extend = parser.ReadBool(section, L"BitmapExtend", false);
	m_Digits = parser.ReadInt(section, L"BitmapDigits", 0);

	m_TransitionFrameCount = parser.ReadInt(section, L"BitmapTransitionFrames", 0);

	const WCHAR* align = parser.ReadString(section, L"BitmapAlign", L"LEFT").c_str();
	if (_wcsicmp(align, L"LEFT") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if (_wcsicmp(align, L"RIGHT") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if (_wcsicmp(align, L"CENTER") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else
	{
		LogErrorF(this, L"BitmapAlign=%s is not valid", align);
	}

	if (m_Initialized)
	{
		m_NeedsReload = (wcscmp(oldImageName.c_str(), m_ImageName.c_str()) != 0);

		if (m_NeedsReload ||
			m_Image.IsOptionsChanged())
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
** Updates the value(s) from the measures.
**
*/
bool MeterBitmap::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
		Measure* measure = m_Measures[0];
		double value = (m_Extend) ? measure->GetValue() : measure->GetRelativeValue();

		if (m_TransitionFrameCount > 0)
		{
			int realFrames = m_FrameCount / (m_TransitionFrameCount + 1);
			if ((int)(value * realFrames) != (int)(m_Value * realFrames))
			{
				m_TransitionStartValue = m_Value;
				m_TransitionStartTicks = System::GetTickCount64();
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
** Returns true if the meter has active transition animation.
**
*/
bool MeterBitmap::HasActiveTransition()
{
	if (m_TransitionStartTicks > 0)
	{
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterBitmap::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	int newY, newX;

	if (m_FrameCount == 0 || !m_Image.IsLoaded()) return false;	// Unable to continue

	Bitmap* bitmap = m_Image.GetImage();

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	if (m_Extend)
	{
		// The 'BitmapExtend' option expands the meters width and/or height by the amount
		// of digits and separation. We need to subtract that amount here so that the meter
		// will draw in the correct place.
		int extraSpace = (m_Digits - 1) * m_Separation;
		extraSpace = max(0, extraSpace);

		int digits = max(1, m_Digits);

		if (bitmap->GetHeight() > bitmap->GetWidth())
		{
			meterRect.Height -= extraSpace;
			meterRect.Height /= digits;
		}
		else
		{
			meterRect.Width -= extraSpace;
			meterRect.Width /= digits;
		}

		__int64 value = (__int64)m_Value;
		value = max(0, value);		// Only positive integers are supported

		__int64 transitionValue = (__int64)m_TransitionStartValue;
		transitionValue = max(0, transitionValue);		// Only positive integers are supported

		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			__int64 tmpValue = value;

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
			}
			while (tmpValue > 0);
		}

		// Blit the images
		int offset;
		if (m_Align == ALIGN_RIGHT)
		{
			offset = 0;
		}
		else if (m_Align == ALIGN_CENTER)
		{
			offset = numOfNums * (meterRect.Width + m_Separation) / 2;
		}
		else
		{
			offset = numOfNums * (meterRect.Width + m_Separation);
		}

		do
		{
			offset = offset - (meterRect.Width + m_Separation);

			int realFrames = (m_FrameCount / (m_TransitionFrameCount + 1));
			int frame = (value % realFrames) * (m_TransitionFrameCount + 1);

			// If transition is ongoing the pick the correct frame
			if (m_TransitionStartTicks > 0)
			{
				int diffTicks = (int)(System::GetTickCount64() - m_TransitionStartTicks);

				int range = ((value % realFrames) - (transitionValue % realFrames)) * (m_TransitionFrameCount + 1);
				if (range < 0)
				{
					range += m_FrameCount;
				}
				int frameAdjustment = range * diffTicks / ((m_TransitionFrameCount + 1) * m_Skin->GetTransitionUpdate());
				if (frameAdjustment > range)
				{
					m_TransitionStartTicks = 0;		// The transition is over. Draw with the real value.
				}
				else
				{
					frame = (transitionValue % realFrames) * (m_TransitionFrameCount + 1);
					frame += frameAdjustment;
					frame %= m_FrameCount;
				}
			}

//			LogDebugF(L"[%u] Value: %f Frame: %i (Transition = %s)", GetTickCount(), m_Value, frame, m_TransitionStartTicks > 0 ? L"true" : L"false");

			if (bitmap->GetHeight() > bitmap->GetWidth())
			{
				newX = 0;
				newY = meterRect.Height * frame;
			}
			else
			{
				newX = meterRect.Width * frame;
				newY = 0;
			}

			canvas.DrawBitmap(bitmap, Rect(meterRect.X + offset, meterRect.Y, meterRect.Width, meterRect.Height), Rect(newX, newY, meterRect.Width, meterRect.Height));
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
		}
		while (numOfNums > 0);
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
			int diffTicks = (int)(System::GetTickCount64() - m_TransitionStartTicks);

			if (diffTicks > ((m_TransitionFrameCount + 1) * m_Skin->GetTransitionUpdate()))
			{
				m_TransitionStartTicks = 0;		// The transition is over. Draw with the real value.
			}
			else
			{
				double range = (m_Value - m_TransitionStartValue);
				double adjustment = range * diffTicks / ((m_TransitionFrameCount + 1) * m_Skin->GetTransitionUpdate());
				double frameAdjustment = adjustment * m_FrameCount;

				frame = (int)(m_TransitionStartValue * realFrames) * (m_TransitionFrameCount + 1);
				frame += (int)frameAdjustment;
				frame %= m_FrameCount;
				frame = max(0, frame);
			}
		}

//		LogDebugF(L"[%u] Value: %f Frame: %i (Transition = %s)", GetTickCount(), m_Value, frame, m_TransitionStartTicks > 0 ? L"true" : L"false");

		if (bitmap->GetHeight() > bitmap->GetWidth())
		{
			newX = 0;
			newY = frame * meterRect.Height;
		}
		else
		{
			newX = frame * meterRect.Width;
			newY = 0;
		}

		canvas.DrawBitmap(bitmap, Rect(meterRect.X, meterRect.Y, meterRect.Width, meterRect.Height), Rect(newX, newY, meterRect.Width, meterRect.Height));
	}

	return true;
}
