// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterString.h"
#include "Rainmeter.h"
#include "Measure.h"
#include "../Common/Gfx/Canvas.h"

#define PI	(3.14159265f)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0f / PI))

MeterString::MeterString(Skin* skin, const WCHAR* name) : MeterStringBase(skin, name),
	m_Angle(),
	m_AutoScale(AUTOSCALE_OFF),
	m_Scale(1.0),
	m_NoDecimals(true),
	m_Percentual(true),
	m_NumOfDecimals(-1),
	m_TrailingSpaces(false)
{
}

MeterString::~MeterString()
{
}

void MeterString::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	MeterStringBase::ReadOptions(parser, section);

	m_Prefix = parser.ReadString(section, L"Prefix", L"");
	m_Postfix = parser.ReadString(section, L"Postfix", L"");
	m_Text = parser.ReadString(section, L"Text", L"");

	m_Angle = (FLOAT)parser.ReadFloat(section, L"Angle", 0.0);

	m_Percentual = parser.ReadBool(section, L"Percentual", false);
	m_NumOfDecimals = parser.ReadInt(section, L"NumOfDecimals", -1);

	const std::wstring& autoscale = parser.ReadString(section, L"AutoScale", L"0");
	int autoscaleValue = _wtoi(autoscale.c_str());
	if (autoscaleValue == 0)
	{
		m_AutoScale = AUTOSCALE_OFF;
	}
	else
	{
		if (autoscale.find_last_of(L"kK") == std::wstring::npos)
		{
			m_AutoScale = (autoscaleValue == 2) ? AUTOSCALE_1000 : AUTOSCALE_1024;
		}
		else
		{
			m_AutoScale = (autoscaleValue == 2) ? AUTOSCALE_1000K : AUTOSCALE_1024K;
		}
	}

	const std::wstring& scale = parser.ReadString(section, L"Scale", L"1");
	m_NoDecimals = (scale.find(L'.') == std::wstring::npos);
	m_Scale = parser.ParseDouble(scale.c_str(), 1);

	m_TrailingSpaces = parser.ReadBool(section, L"TrailingSpaces", false);
}

bool MeterString::Update()
{
	if (Meter::Update())
	{
		int decimals = (m_NumOfDecimals != -1) ? m_NumOfDecimals : (m_NoDecimals && (m_Percentual || m_AutoScale == AUTOSCALE_OFF)) ? 0 : 1;

		// Create the text
		m_String = m_Prefix;
		if (!m_Measures.empty())
		{
			if (m_Text.empty())
			{
				m_String += m_Measures[0]->GetStringOrFormattedValue(
					m_AutoScale, m_Scale, decimals, m_Percentual);
			}
			else
			{
				std::wstring tmpText = m_Text;
				ReplaceMeasures(tmpText, m_AutoScale, m_Scale, decimals, m_Percentual);
				m_String += tmpText;
			}
		}
		else
		{
			m_String += m_Text;
		}
		if (!m_Postfix.empty()) m_String += m_Postfix;

		ApplyCase(m_String);

		for (size_t i = 0; i < m_String.length(); ++i)
		{
			if (m_String[i] == L'\u00A0' ||  // No-Break Space
				m_String[i] == L'\u205F')    // Medium Mathematical Space
			{
				// Ugly hack to make D2D render trailing spaces followed by a non-breaking space
				// correctly. By default, D2D ignores all trailing whitespace. Both GDI+ and D2D,
				// however, acknowledge the presense of the zero-width space (and give it a width
				// of 0px), so we append the zero-width space after each non-breaking space.
				++i;
				m_String.insert(i, 1, L'\u200B');
			}
			else if (m_String[i] == L'\r')
			{
				// GDI+ seems to ignore carriage returns, so strip it entirely to make it behave
				// similarly with D2D as well.
				m_String.erase(i, 1);
				--i;
			}
		}

		if (m_TrailingSpaces)
		{
			m_String += L'\u200B';
		}

		UpdateTextFormat();
		UpdateAutoSize();

		return true;
	}
	return false;
}

bool MeterString::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	if (m_Angle == 0.0f) return DrawString(canvas);

	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
	canvas.GetTransform(&matrix);

	// GDI+ compatibility: the text rotates about its own anchor, which follows StringAlign.
	const D2D1_RECT_F rect = GetMeterRectPadding();
	FLOAT cx = rect.left;
	switch (m_TextFormat->GetHorizontalAlignment())
	{
	case Gfx::HorizontalAlignment::Center: cx = (rect.left + rect.right) / 2.0f; break;
	case Gfx::HorizontalAlignment::Right:  cx = rect.right; break;
	}

	canvas.SetTransform(
		D2D1::Matrix3x2F::Rotation(CONVERT_TO_DEGREES(m_Angle), D2D1::Point2F(cx, rect.top)) * matrix);

	const bool result = DrawString(canvas);

	canvas.ResetTransform();
	return result;
}

void MeterString::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}
