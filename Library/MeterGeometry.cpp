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

bool MeterGeometry::ParseShape(GeometryShape& shape, const WCHAR* optionName, const WCHAR* optionValue)
{
	if (_wcsicmp(optionName, L"Rectangle") == 0) shape.m_Shape = ParseRect(shape, ConfigParser::ParseRECT(optionValue));
	else {
		return false;
	}
	UpdateSize(shape);
	return true;
}

void MeterGeometry::UpdateSize(GeometryShape & shape)
{
	if (shape.m_Shape) {
		shape.m_Shape->GetBounds(D2D1::Matrix3x2F::Identity(), &shape.m_Bounds);
		const auto shapeWidth = std::ceil(shape.m_Bounds.right + shape.m_OutlineWidth / 4 * 3 + GetWidthPadding());
		if (!m_WDefined && m_W < shapeWidth) {
			m_W = shapeWidth;
			m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);
		}
		const auto shapeHeight = std::ceil(shape.m_Bounds.bottom + shape.m_OutlineWidth / 4 * 3 + GetHeightPadding());
		if (!m_HDefined && m_H < shapeHeight) {
			m_H = shapeHeight;
			m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);
		}
	}
}

bool MeterGeometry::ReplaceShapeOption(GeometryShape& shape, const WCHAR* optionName, const WCHAR* optionValue)
{
	if (_wcsicmp(optionName, L"FillColor") == 0)			shape.m_FillColor = ToColorF(ConfigParser::ParseColor(optionValue));
	else if (_wcsicmp(optionName, L"OutlineWidth") == 0)	shape.m_OutlineWidth = (float)ConfigParser::ParseDouble(optionValue, 1.0f);
	else if (_wcsicmp(optionName, L"OutlineColor") == 0)	shape.m_OutlineColor = ToColorF(ConfigParser::ParseColor(optionValue));
	else if (_wcsicmp(optionName, L"Offset") == 0)			shape.m_Offset = ParseSize(optionValue, 0);
	else if (_wcsicmp(optionName, L"Scale") == 0)			shape.m_Scale = ParseSize(optionValue, 1);
	else if (_wcsicmp(optionName, L"Skew") == 0)			shape.m_Skew = ParsePoint(optionValue, 0);
	else if (_wcsicmp(optionName, L"Rotation") == 0)		shape.m_Rotation = ParseRotation(optionValue, 0, shape);
	else return false;
	return true;
}

bool MeterGeometry::IsPostOption(const WCHAR* option)
{
	return _wcsicmp(option, L"Rotation") == 0;
}

bool MeterGeometry::IsShape(const WCHAR* option)
{
	return _wcsicmp(option, L"Rectangle") == 0;
}
void MeterGeometry::ReadOptions(ConfigParser& parser, const WCHAR* section)
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
	std::wstring shapeName = L"Shape";
	std::wstring shapeOption = parser.ReadString(section, L"Shape", L"");
	std::map<std::wstring, std::pair<std::wstring, std::wstring>> postOptions;
	while (!shapeOption.empty()) {
		std::vector<std::wstring> shapeTokens = CustomTokenize(shapeOption.c_str(), L"|");
		m_Shapes.insert(std::pair<LPCWSTR, GeometryShape>(shapeName.c_str(), GeometryShape()));
		bool usingMeasures = false;

		std::wstring shapeType = L"", shapeParameters = L"";

		for (int optionId = 0; optionId < shapeTokens.size(); optionId++)
		{
			auto stringPos = shapeTokens[optionId].find_first_of(L':');
			auto optionName = shapeTokens[optionId].substr(0, stringPos);
			auto optionValue = shapeTokens[optionId].substr(stringPos + 1);

			if (!m_Measures.empty())
				usingMeasures = ContainsMeasures(optionValue);

			//Store options to replace in update
			if (!usingMeasures) {
				if (!IsShape(optionName.c_str())) {
					if (!IsPostOption(optionName.c_str())) {
						if (!ReplaceShapeOption(m_Shapes[shapeName.c_str()], optionName.c_str(), optionValue.c_str()))
							break;
					}
					else
						postOptions[shapeName.c_str()] = std::pair<std::wstring, std::wstring>(optionName, optionValue);
				}
				else
				{			
					shapeType = optionName;
					shapeParameters = optionValue;
				}
			}
			else
				m_MeasureOptions[shapeName.c_str()].push_back(std::pair<std::wstring, std::wstring>(optionName, optionValue));
			++optionId;
		}
		if (shapeType.empty() || shapeParameters.empty())
			break; //Invalid shape
		if (!ParseShape(m_Shapes[shapeName.c_str()], shapeType.c_str(), shapeParameters.c_str()))
			break;
		shapeName = L"Shape" + std::to_wstring(++currentShapeId);
		shapeOption = parser.ReadString(section, shapeName.c_str(), L"");
	}

	for (const auto& option : postOptions)
	{
		if (!ReplaceShapeOption(m_Shapes[option.first], option.second.first.c_str(), option.second.second.c_str()))
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
			for (const auto& shape : m_MeasureOptions)
			{
				for (const auto& option : shape.second) {
					std::wstring optionValue = option.second;
					bool replaced = ReplaceMeasures(optionValue, AUTOSCALE::AUTOSCALE_OFF, 1.0, 6, false);
					if (replaced)
						if (!IsShape(option.first.c_str())) {
							if (!ReplaceShapeOption(m_Shapes[shape.first], option.first.c_str(), optionValue.c_str()))
								continue;
						}
						else
							if (!ParseShape(m_Shapes[shape.first], option.first.c_str(), optionValue.c_str()))
								continue;
				}
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
	
	for (const auto& it : m_Shapes) {
		const auto& shape = it.second;
		if (shape.m_Shape) {
			const auto transform = D2D1::Matrix3x2F(
				D2D1::Matrix3x2F::Rotation(shape.m_Rotation, shape.m_RotationCenter) *

				D2D1::Matrix3x2F::Skew(shape.m_Skew.x, shape.m_Skew.y, D2D1::Point2F(shape.m_Bounds.left, shape.m_Bounds.top)) *
				D2D1::Matrix3x2F::Translation(shape.m_Offset) * D2D1::Matrix3x2F::Translation(D2D1::SizeF(GetMeterRectPadding().X, GetMeterRectPadding().Y)) *
				D2D1::Matrix3x2F::Scale(shape.m_Scale, D2D1::Point2F(shape.m_Bounds.left, shape.m_Bounds.top))
				);
			canvas.DrawGeometry(shape, transform, !m_AntiAlias);
		}
	}
	return true;
}
void MeterGeometry::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterGeometry::ParseRect(GeometryShape& shape, RECT& rect)
{
	D2D1_RECT_F geo_rect = D2D1::RectF(rect.left, rect.top, rect.right + rect.left, rect.bottom + rect.top);
	geo_rect.left += shape.m_OutlineWidth / 4;
	geo_rect.top += shape.m_OutlineWidth / 4;
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
		shape.m_RotationCenter.x = (shape.m_Bounds.right - shape.m_Bounds.left) / 2 + shape.m_Bounds.left;
		shape.m_RotationCenter.y = (shape.m_Bounds.bottom - shape.m_Bounds.top) / 2 + shape.m_Bounds.top;
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
		std::wstring token = str.substr(lastPos, pos - lastPos);

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
D2D1_POINT_2F MeterGeometry::ParsePoint(const WCHAR* string, double defaultVal)
{
	double x = defaultVal;
	double y = defaultVal;
	double dummy = 0.0;
	ConfigParser::ParseInt4(string, x, y, dummy, dummy);
	return D2D1::Point2F(x, y);
}
D2D1_SIZE_F MeterGeometry::ParseSize(const WCHAR* string, double defaultVal)
{
	D2D1_POINT_2F point = ParsePoint(string, defaultVal);
	return D2D1::SizeF(point.x, point.y);
}
//Slightly modified version of ReplaceMeasure, used to check if a shape uses measures
bool MeterGeometry::ContainsMeasures(const std::wstring& str)
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
