/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterShape.h"
#include "Logger.h"
#include "../Common/StringUtil.h"

MeterShape::MeterShape(Skin* skin, const WCHAR* name) : Meter(skin, name),
m_Shapes()
{
}

MeterShape::~MeterShape()
{
	for (auto& shape : m_Shapes)
	{
		delete shape;
	}
	m_Shapes.clear();
}

void MeterShape::Initialize()
{
	Meter::Initialize();
}

void MeterShape::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);
	std::wstring shapeOption = parser.ReadString(section, L"Shape", L"");
	size_t currentShapeId = 1;
	std::wstring shapeId = L"Shape";
	while (!shapeOption.empty()) {
		Gfx::Shape* shape = nullptr;
		bool newShape = true;
		if (m_Shapes.size() > currentShapeId - 1 && m_Shapes[currentShapeId - 1] != nullptr)
		{
			newShape = false;
			shape = m_Shapes[currentShapeId - 1];
		}
		bool error = ParseModifiers(parser, section, shape, shapeId.c_str(), shapeOption.c_str());
		if (shape && !error) {
			if (newShape)
				m_Shapes.push_back(shape);
			D2D1_RECT_F bounds = shape->GetBounds();
			if (!m_WDefined && m_W < bounds.right)
			{
				m_W = bounds.right;
			}
			if (!m_HDefined && m_H < bounds.bottom)
			{
				m_H = bounds.bottom;
			}
		}
		else
		{
			if (newShape && shape)
				delete shape;
			if (currentShapeId < m_Shapes.size())
			{
				for (int i = currentShapeId; i < m_Shapes.size(); ++i)
				{
					delete m_Shapes[i];
				}
				m_Shapes.resize(currentShapeId);
			}
			break;
		}
		shapeId = L"Shape" + std::to_wstring(++currentShapeId);
		shapeOption = parser.ReadString(section, shapeId.c_str(), L"");
	}
	if (m_Initialized && m_Measures.empty() && !m_DynamicVariables)
	{
		Initialize();
		m_NeedsRedraw = true;
	}
}

bool MeterShape::Update()
{
	if (Meter::Update())
	{
		if (!m_Measures.empty() || m_DynamicVariables)
		{
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

bool MeterShape::Draw(Gfx::Canvas & canvas)
{
	if (!Meter::Draw(canvas)) return false;
	int x = GetX();
	int y = GetY();
	for (const auto& shape : m_Shapes) {
		if (shape->IsShapeDefined()) {
			canvas.DrawGeometry(*shape, x, y);
		}
	}
	return true;
}

bool MeterShape::HitTest(int x, int y)
{
	for (auto& shape : m_Shapes)
	{
		if (shape->ContainsPoint(x - m_X, y - m_Y))
			return true;
	}
	return false;
}

void MeterShape::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

std::wstring MeterShape::HandleModifier(const WCHAR* modifier, ConfigParser& parser, const WCHAR* parameters, Gfx::Shape* shape, const WCHAR* section, int recursion)
{
	std::wstring error;
	if (CompareWChar(L"STROKEWIDTH", modifier))
	{
		int strokeWidth = parser.ParseInt(parameters, 1);
		if (strokeWidth >= 0)
		{
			shape->SetStrokeWidth(strokeWidth);
		}
		else
		{
			error = L"StrokeWidth cannot be a negative value";
		}
	}
	else if (CompareWChar(L"STROKECOLOR", modifier))
	{
		std::vector<Gdiplus::REAL> paramTokens = ConfigParser::ParseFloats(parameters, L',');
		if (paramTokens.size() >= 3)
		{
			if (paramTokens.size() == 3)
			{
				shape->SetStrokeColor(D2D1::ColorF(paramTokens[0] / 255.0f, paramTokens[1] / 255.0f, paramTokens[2] / 255.0f));
			}
			else
			{
				shape->SetStrokeColor(D2D1::ColorF(paramTokens[0] / 255.0f, paramTokens[1] / 255.0f, paramTokens[2] / 255.0f, paramTokens[3] / 255.0f));
			}
		}
		else
		{
			error = L"StrokeColor has to few parameters";
		}
	}
	else if (CompareWChar(L"FILLCOLOR", modifier))
	{
		std::vector<Gdiplus::REAL> paramTokens = ConfigParser::ParseFloats(parameters, L',');
		if (paramTokens.size() >= 3)
		{
			if (paramTokens.size() == 3)
			{
				shape->SetFillColor(D2D1::ColorF(paramTokens[0] / 255.0f, paramTokens[1] / 255.0f, paramTokens[2] / 255.0f));
			}
			else
			{
				shape->SetFillColor(D2D1::ColorF(paramTokens[0] / 255.0f, paramTokens[1] / 255.0f, paramTokens[2] / 255.0f, paramTokens[3] / 255.0f));
			}
		}
		else
		{
			error = L"FillColor has to few parameters";
		}
	}
	else if (CompareWChar(L"OFFSET", modifier))
	{
		std::vector<Gdiplus::REAL> paramTokens = ConfigParser::ParseFloats(parameters, L',');
		if (paramTokens.size() >= 2)
		{
			shape->SetOffset(paramTokens[0], paramTokens[1]);
		}
		else if (paramTokens.size() < 2)
		{
			error = L"Offset has too few parameters";
		}
	}
	else if (CompareWChar(L"ROTATE", modifier))
	{
		std::vector<Gdiplus::REAL> paramTokens = ConfigParser::ParseFloats(parameters, L',');
		if (paramTokens.size() >= 1)
		{
			shape->SetRotation(paramTokens[0]);
		}
		else if (paramTokens.size() < 1)
		{
			error = L"Rotate has too few parameters";
		}
	}
	else if (CompareWChar(L"EXTEND", modifier))
	{
		if (!recursion) {
			std::vector<std::wstring> paramTokens = parser.Tokenize(parameters, L",");
			for (auto param : paramTokens) {
				std::wstring options = parser.ReadString(section, param.c_str(), L"");
				if (!options.empty())
				{
					if (ParseModifiers(parser, section, shape, param.c_str(), options.c_str(), true))
						return StringUtil::Format(L"Extend could not extend option %s", param.c_str());
				}
				else
				{
					error = StringUtil::Format(L"Extend could not extend option %s", param.c_str());
				}
			}
		}
		else
		{
			error = L"Extend cannot be used recursively";
		}
	}

	return error;
}

WCHAR* MeterShape::IsModifier(const WCHAR* modifier)
{
	if (CompareWChar(L"STROKEWIDTH", modifier))
	{
		return L"STROKEWIDTH";
	}
	else if (CompareWChar(L"STROKECOLOR", modifier))
	{
		return L"STROKECOLOR";
	}
	else if (CompareWChar(L"FILLCOLOR", modifier))
	{
		return L"FILLCOLOR";
	}
	else if (CompareWChar(L"OFFSET", modifier))
	{
		return L"OFFSET";
	}
	else if (CompareWChar(L"ROTATE", modifier))
	{
		return L"ROTATE";
	}
	else if (CompareWChar(L"EXTEND", modifier))
	{
		return L"EXTEND";
	}

	return nullptr;
}

const WCHAR* MeterShape::HandleShape(Gfx::Shape*& shape, const WCHAR* shapeType, const WCHAR* parameters)
{
	const WCHAR* error = nullptr;
	if (CompareWChar(L"RECTANGLE", shapeType))
	{
		std::vector<Gdiplus::REAL> params = ConfigParser::ParseFloats(parameters, L',');
		if (params.size() > 3)
		{
			if (!shape)
			{
				shape = new Gfx::RectangleShape(params);
			}
			else
			{
				shape->UpdateShape(params);
			}
		}
		else
		{
			error = L"Rectangle has to few parameters";
		}
	}
	return error;
}

WCHAR * MeterShape::IsShape(const WCHAR * shape)
{
	if (CompareWChar(L"RECTANGLE", shape))
	{
		return L"RECTANGLE";
	}

	return nullptr;
}

bool MeterShape::CompareWChar(const WCHAR* str1, const WCHAR* str2)
{
	return _wcsnicmp(str2, str1, wcslen(str1)) == 0;
}

bool MeterShape::ParseModifiers(ConfigParser& parser, const WCHAR* section, Gfx::Shape*& mainShape, const WCHAR* shapeId, const WCHAR* modifierString, int recursion)
{
	std::vector<std::wstring> shapeTokens = ConfigParser::Tokenize(modifierString, L"|");
	std::vector<std::wstring> modifiers;
	for (const auto& token : shapeTokens) {
		WCHAR* shape = IsShape(token.c_str());
		WCHAR* modifier = IsModifier(token.c_str());
		if (shape)
		{
			const WCHAR* error = HandleShape(mainShape, shape, token.substr(token.find_first_not_of(' ', wcslen(shape))).c_str());
			if (error)
			{
				LogErrorF(this, L"%s: %s", shapeId, error);
				return true;
			}

			if (mainShape)
			{
				for (const auto& modifierToken : modifiers)
				{
					WCHAR* modifier = IsModifier(modifierToken.c_str());
					const std::wstring error = HandleModifier(modifier, parser, modifierToken.substr(modifierToken.find_first_not_of(' ', wcslen(modifier))).c_str(), mainShape, section, recursion);
					if (!error.empty())
					{
						LogErrorF(this, L"%s: %s", shapeId, error.c_str());
						return true;
					}
				}
				modifiers.clear();
			}
		}
		else if (modifier)
		{
			if (mainShape)
			{
				size_t parametersLen = token.find_first_not_of(' ', wcslen(modifier));
				std::wstring error;
				if (parametersLen != std::wstring::npos)
					error = HandleModifier(modifier, parser, token.substr(parametersLen).c_str(), mainShape, section, recursion);
				else
					error = HandleModifier(modifier, parser, L"", mainShape, section, recursion);
				if (!error.empty())
				{
					LogErrorF(this, L"%s: %s", shapeId, error.c_str());
					return true;
				}
			}
			else
			{
				modifiers.push_back(token);
			}
		}
		else
		{
			LogErrorF(this, L"%s: '%s' not recognized as a modifier or shape", shapeId, token.c_str());
			return true;
		}
	}
	return false;
}