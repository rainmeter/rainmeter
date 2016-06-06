/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterGeometry.h"
#include "Logger.h"

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

MeterGeometry::MeterGeometry(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Shapes(),
	m_MeasureOptions(),
	m_NeedsRedraw(false)
{
}

MeterGeometry::~MeterGeometry()
{
}

void MeterGeometry::Initialize()
{
	Meter::Initialize();
}

bool MeterGeometry::ParseShape(GeometryShape & shape, const LPCWSTR & optionName, const LPCWSTR & optionValue)
{
	if (_wcsicmp(optionName, L"Rectangle") == 0) shape.m_Shape = ParseRect(shape, ConfigParser::ParseRECT(optionValue));
	else return false;
	//Easy to do with a rectangle, but this will calculate the bounds of all shape types
	if (shape.m_Shape) {
		shape.m_Shape->GetBounds(D2D1::Matrix3x2F::Identity(), &shape.m_Bounds);
		if (!m_WDefined && m_W < shape.m_Bounds.right + shape.m_OutlineWidth / 2 + GetWidthPadding()) {
			m_W = shape.m_Bounds.right + shape.m_OutlineWidth / 2 + GetWidthPadding();
			if (!IsHidden())
				m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);
		}
		float H = Meter::GetH();
		if (!m_HDefined && m_H < shape.m_Bounds.bottom + shape.m_OutlineWidth / 2 + GetHeightPadding()) {
			m_H = shape.m_Bounds.bottom + shape.m_OutlineWidth / 2 + GetHeightPadding();
			if(!IsHidden())
				m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);
		}
	}
	return true;
}

bool MeterGeometry::ReplaceShapeOption(GeometryShape & shape, const LPCWSTR & optionName, const LPCWSTR & optionValue)
{
	if (_wcsicmp(optionName, L"FillColor") == 0)			shape.m_FillColor = ToColorF(ConfigParser::ParseColor(optionValue));
	else if (_wcsicmp(optionName, L"OutlineWidth") == 0)	shape.m_OutlineWidth = (float)ConfigParser::ParseDouble(optionValue, 1.0f);
	else if (_wcsicmp(optionName, L"OutlineColor") == 0)	shape.m_OutlineColor = ToColorF(ConfigParser::ParseColor(optionValue));
	else if (_wcsicmp(optionName, L"Offset") == 0)			shape.m_Offset = ParseSize(optionName, 0);
	else if (_wcsicmp(optionName, L"Scale") == 0)			shape.m_Scale = ParseSize(optionName, 1);
	else if (_wcsicmp(optionName, L"Skew") == 0)			shape.m_Skew = ParsePoint(optionName, 0);
	else if (_wcsicmp(optionName, L"Rotation") == 0)		shape.m_Rotation = ParseRotation(optionName, 0, shape);
	else return false;
	return true;
}

bool MeterGeometry::IsPostOption(const LPCWSTR & option)
{
	return _wcsicmp(option, L"Rotation") == 0;
}

bool MeterGeometry::IsShape(const LPCWSTR & option)
{
	return _wcsicmp(option, L"Rectangle") == 0;
}
void MeterGeometry::ReadOptions(ConfigParser & parser, const WCHAR * section)
{
	Meter::ReadOptions(parser, section);
	//Setting Width and Height to 0 to stop padding from consuming everything
	if (!m_WDefined)
		m_W = 0;
	if (!m_HDefined)
		m_H = 0;

	m_Shapes.clear();
	m_MeasureOptions.clear();
	size_t currentShapeId = 1;
	std::wstring shapeOption = parser.ReadString(section, L"Shape", L"");
	std::map<LPCWSTR, std::pair<LPCWSTR, LPCWSTR>> postOptions;
	if (!shapeOption.empty()) 
		do
		{
			std::vector<std::wstring> shapeTokens = CustomTokenize(shapeOption.c_str(), L"|:");
			m_Shapes.insert(std::pair<LPCWSTR, GeometryShape>(shapeOption.c_str(), GeometryShape()));
			bool usingMeasures = false;
			for (int optionId = 0; optionId < shapeTokens.size(); optionId++)
			{
				if (shapeTokens.size() < optionId + 1) break; //not a pair

				std::wstring optionName = shapeTokens[optionId].c_str();
				std::wstring optionValue = shapeTokens[optionId + 1].c_str();

				if (!m_Measures.empty())
					usingMeasures = ContainsMeasures(optionValue);

				//Store options to replace in update
				if (!usingMeasures) {
					if (!IsShape(optionName.c_str())) {
						if (!IsPostOption(optionName.c_str())) {
							if (!ReplaceShapeOption(m_Shapes[shapeOption.c_str()], optionName.c_str(), optionValue.c_str()))
								break;
						}
						else
							postOptions[shapeOption.c_str()] = std::pair<LPCWSTR, LPCWSTR>(optionName.c_str(), optionValue.c_str());
					}
					else
						if (!ParseShape(m_Shapes[shapeOption.c_str()], optionName.c_str(), optionValue.c_str()))
							break;
				}
				else
					m_MeasureOptions[shapeOption.c_str()] = std::pair<std::wstring, std::wstring>(optionName, optionValue);
				++optionId;
			}
			shapeOption = parser.ReadString(section, (L"Shape" + std::to_wstring(++currentShapeId)).c_str(), L"");
		} while (!shapeOption.empty());

	for (const auto& option : postOptions)
	{
		if (!ReplaceShapeOption(m_Shapes[option.first], option.second.first, option.second.second))
			continue;
	}
	if (m_Initialized && m_Measures.empty() && !m_DynamicVariables)
	{
		Initialize();
		m_NeedsRedraw = true;
	}
}

bool MeterGeometry::Update()
{
	if (Meter::Update())
	{
		if (!m_Measures.empty() || m_DynamicVariables)
		{
			for (const auto& option : m_MeasureOptions)
			{
				std::wstring optionValue = option.second.second;
				bool replaced = ReplaceMeasures(optionValue);
				if (replaced)
					if (!IsShape(option.second.first.c_str())) {
						if (!ReplaceShapeOption(m_Shapes[option.first], option.second.first.c_str(), optionValue.c_str()))
							continue;
					}
					else
						if (!ParseShape(m_Shapes[option.first], option.second.first.c_str(), optionValue.c_str()))
							continue;
			}
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

bool MeterGeometry::Draw(Gfx::Canvas & canvas)
{
	if (!Meter::Draw(canvas)) return false;
	
	for (const auto& shape : m_Shapes) {
		if (shape.second.m_Shape) {
			D2D1_POINT_2F centerPoint;
			centerPoint = D2D1::Point2F(shape.second.m_RotationCenter.x, shape.second.m_RotationCenter.y);
			D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F(
				D2D1::Matrix3x2F::Rotation(shape.second.m_Rotation, centerPoint) *

				D2D1::Matrix3x2F::Skew(shape.second.m_Skew.x, shape.second.m_Skew.y, D2D1::Point2F(shape.second.m_Bounds.left, shape.second.m_Bounds.top)) *
				D2D1::Matrix3x2F::Translation(shape.second.m_Offset) * D2D1::Matrix3x2F::Translation(D2D1::SizeF(GetMeterRectPadding().X, GetMeterRectPadding().Y)) *
				D2D1::Matrix3x2F::Scale(shape.second.m_Scale, D2D1::Point2F(shape.second.m_Bounds.left, shape.second.m_Bounds.top))
				);
			canvas.DrawGeometry(shape.second, transform);
		}
	}
	return true;
}
/*
** Not necessary for the Geometry meter to be bound to any measures
**
*/
void MeterGeometry::BindMeasures(ConfigParser & parser, const WCHAR * section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterGeometry::ParseRect(GeometryShape& shape, RECT& rect)
{
	D2D1_RECT_F geo_rect = D2D1::RectF(rect.left, rect.top, rect.right + rect.left, rect.bottom + rect.top);
	geo_rect.left += shape.m_OutlineWidth / 2;
	geo_rect.top += shape.m_OutlineWidth / 2;
	return Gfx::Canvas::CreateRectangle(geo_rect);
}

double MeterGeometry::ParseRotation(const WCHAR* string, double defaultValue, GeometryShape& shape)
{
	std::vector<std::wstring> tokens = CustomTokenize(string, L",");
	double rotation = 0;
	if (tokens.size() > 0)  rotation = ConfigParser::ParseDouble(tokens[0].c_str(), defaultValue);
	if (tokens.size() > 2) {
		shape.m_RotationCenter = ParsePoint((tokens[1] + L"," + tokens[2]).c_str(), FLT_MIN);
		if (shape.m_RotationCenter.x == FLT_MIN) shape.m_RotationCenter.x = (shape.m_Bounds.right - shape.m_Bounds.left) / 2 - shape.m_Bounds.left;
		if (shape.m_RotationCenter.y == FLT_MIN) shape.m_RotationCenter.y = (shape.m_Bounds.bottom - shape.m_Bounds.top) / 2 - shape.m_Bounds.top;
	}
	else {
		shape.m_RotationCenter.x = (shape.m_Bounds.right - shape.m_Bounds.left) / 2 - shape.m_Bounds.left;
		shape.m_RotationCenter.y = (shape.m_Bounds.bottom - shape.m_Bounds.top) / 2 - shape.m_Bounds.top;
	}
	return rotation;
}

std::vector<std::wstring> MeterGeometry::CustomTokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	size_t lastPos, pos = 0;
	do
	{
		lastPos = str.find_first_not_of(delimiters, pos);
		if (lastPos == std::wstring::npos) break;

		pos = str.find_first_of(delimiters, lastPos + 1);
		std::wstring token = str.substr(lastPos, pos - lastPos);  // len = (pos != std::wstring::npos) ? pos - lastPos : pos

		size_t pos2 = token.find_first_not_of(L" \t\r\n");
		//If a token fix was needed, then are the pos one token off, just needed to keep it right
		bool offsett = false;
		if (pos2 != std::wstring::npos)
		{
			//Check wether it's even needed to try combine mathematic expressions
			if (token.find(L'(') != std::wstring::npos || token.find(L'[') != std::wstring::npos || token.find(L'#') != std::wstring::npos)
			{
				//Add the next token while the order of ( and ), and [ and ] are still uneven 
				while (std::count(token.begin(), token.end(), L'(') != std::count(token.begin(), token.end(), L')') &&
					std::count(token.begin(), token.end(), L'[') != std::count(token.begin(), token.end(), L']') &&
					std::count(token.begin(), token.end(), L'#') != std::count(token.begin(), token.end(), L'#') &&
					pos != std::wstring::npos && lastPos != std::wstring::npos)
				{
					lastPos = str.find_first_not_of(delimiters, pos);
					if (lastPos == std::wstring::npos) break;
					pos = str.find_first_of(delimiters, lastPos + 1);
					token += str.at(lastPos - 1) + str.substr(lastPos, pos - lastPos);
					if (pos == std::wstring::npos) break;
					++pos;
					offsett = true;
				}
			}
			//End of addition
			size_t lastPos2 = token.find_last_not_of(L" \t\r\n");
			if (pos2 != 0 || lastPos2 != (token.size() - 1))
			{
				// Trim white-space
				token.assign(token, pos2, lastPos2 - pos2 + 1);
			}
			tokens.push_back(token);
		}

		if (pos == std::wstring::npos) break;
		if (!offsett)
			++pos;
	} while (true);

	return tokens;
}
//Modified version of ParseRect
D2D1_POINT_2F MeterGeometry::ParsePoint(const LPCWSTR& string, double defaultVal)
{
	double x = defaultVal, y = defaultVal;

	if (wcschr(string, L','))
	{
		std::wstring str = string;
		std::vector<double> tokens;
		size_t start = 0;
		size_t end = 0;
		int parens = 0;
		auto getToken = [&]() -> void
		{
			start = str.find_first_not_of(L" \t", start); // skip any leading whitespace
			if (start <= end)
			{
				tokens.push_back(ConfigParser::ParseDouble(str.substr(start, end - start).c_str(), 0));
			}
		};
		for (auto iter : str)
		{
			switch (iter)
			{
			case '(': ++parens; break;
			case ')': --parens; break;
			case ',':
			{
				if (parens == 0)
				{
					getToken();
					start = end + 1; // skip comma
					break;
				}
				//else multi arg function ?
			}
			}
			++end;
		}

		// read last token
		getToken();

		size_t size = tokens.size();
		if (size > 0) x = tokens[0];
		if (size > 1) y = tokens[1];

	}
	return D2D1::Point2F(x, y);
}
D2D1_SIZE_F MeterGeometry::ParseSize(const LPCWSTR& string, double defaultVal)
{
	D2D1_POINT_2F point = ParsePoint(string, defaultVal);
	return D2D1::SizeF(point.x, point.y);
}
//Slightly modified version of ReplaceMeasure, used to check if a shape uses measures
bool MeterGeometry::ContainsMeasures(const std::wstring & str)
{
	if (str.find(L'%') != std::wstring::npos)
	{
		WCHAR buffer[64];
		for (size_t i = m_Measures.size(); i > 0; --i)
		{
			size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%%%i", (int)i);
			size_t start = 0, pos;
			do
			{
				pos = str.find(buffer, start, len);
				if (pos != std::wstring::npos)
				{
					start = pos + len;
					return true;
				}
			} while (pos != std::wstring::npos);
		}
	}
	return false;
}
