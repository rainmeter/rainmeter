// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterRotator.h"
#include "Measure.h"
#include "Util.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

MeterRotator::MeterRotator(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"ImageName", nullptr, false, skin),
	m_OffsetX(),
	m_OffsetY(),
	m_StartAngle(),
	m_RotationAngle(PI * 2.0),
	m_ValueRemainder(),
	m_Value()
{
}

MeterRotator::~MeterRotator()
{
}

void MeterRotator::Initialize()
{
	Meter::Initialize();

	// Load the bitmaps if defined
	if (!m_ImageName.empty())
	{
		m_Image.LoadImage(m_ImageName);
	}
	else if (m_Image.IsLoaded())
	{
		m_Image.DisposeImage();
	}
}

void MeterRotator::InvalidateDeviceResources()
{
	Meter::InvalidateDeviceResources();
	m_Image.InvalidateDeviceResources();
}

void MeterRotator::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;

	Meter::ReadOptions(parser, section);

	parser.ReadString(m_ImageName, section, L"ImageName", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
	}

	m_OffsetX = parser.ReadFloat(section, L"OffsetX", 0.0);
	m_OffsetY = parser.ReadFloat(section, L"OffsetY", 0.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", PI * 2.0);

	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);

	if (m_Initialized)
	{
		Initialize();  // Reload the image
	}
}

bool MeterRotator::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
		Measure* measure = m_Measures[0];
		if (m_ValueRemainder > 0)
		{
			LONGLONG time = (LONGLONG)measure->GetValue();
			m_Value = (double)(time % m_ValueRemainder);
			m_Value /= (double)m_ValueRemainder;
		}
		else
		{
			m_Value = measure->GetRelativeValue();
		}
		return true;
	}
	return false;
}


bool MeterRotator::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	if (m_Image.IsLoaded())
	{
		Gfx::Bitmap* drawBitmap = m_Image.GetImage();
		const FLOAT width = (FLOAT)drawBitmap->GetWidth();
		const FLOAT height = (FLOAT)drawBitmap->GetHeight();

		D2D1_RECT_F meterRect = GetMeterRectPadding();

		FLOAT cx = meterRect.left + m_W / 2.0f;
		FLOAT cy = meterRect.top + m_H / 2.0f;
		D2D1_POINT_2F center = D2D1::Point2F(cx, cy);

		// Calculate the rotation
		FLOAT angle = (FLOAT)(CONVERT_TO_DEGREES(m_RotationAngle * m_Value + m_StartAngle));

		// Get current transform
		D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
		canvas.GetTransform(&matrix);

		canvas.SetTransform(
			D2D1::Matrix3x2F::Translation((FLOAT)(-m_OffsetX), (FLOAT)(-m_OffsetY)) *
			D2D1::Matrix3x2F::Rotation(angle) *
			D2D1::Matrix3x2F::Translation(cx, cy) *
			matrix);

		const D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, width, height);
		canvas.DrawBitmap(drawBitmap, rect, rect);

		canvas.ResetTransform();
	}

	return true;
}
