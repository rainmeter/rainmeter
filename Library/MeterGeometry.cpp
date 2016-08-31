/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterGeometry.h"
#include "Logger.h"

MeterGeometry::MeterGeometry(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Shapes()
{
}

MeterGeometry::~MeterGeometry()
{
	for (auto& pair : m_Shapes)
	{
		if (pair.second)
			delete pair.second;
	}
	m_Shapes.clear();
}

void MeterGeometry::Initialize()
{
	Meter::Initialize();
}

void MeterGeometry::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);
	std::wstring shapeOption = parser.ReadString(section, L"Shape", L"");
	size_t currentShapeId = 1;
	std::wstring shapeId = L"Shape";
	for (auto& pair : m_Shapes)
	{
		if (pair.second)
			delete pair.second;
	}
	m_Shapes.clear();
	while (!shapeOption.empty()) {
		Gfx::Shape* shape = nullptr;
		bool newShape = true;
		ParseModifiers(parser, section, shape, shapeOption.c_str());
		if (shape) {
			m_Shapes.insert({ shapeId, shape });
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

bool MeterGeometry::Update()
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

bool MeterGeometry::Draw(Gfx::Canvas & canvas)
{
	if (!Meter::Draw(canvas)) return false;	
	for (const auto& it : m_Shapes) {
		const auto& shape = it.second;
		if (shape->IsShapeDefined()) {
			canvas.DrawGeometry(*shape);
		}
	}
	return true;
}

bool MeterGeometry::HitTest(int x, int y)
{
	return false;
}

void MeterGeometry::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

const WCHAR* MeterGeometry::HandleModifier(const WCHAR* modifier, ConfigParser& parser, const WCHAR* parameters, Gfx::Shape* shape, const WCHAR* section, int recursion)
{
	if (CompareWChar(L"STROKEWIDTH", modifier))
	{
		shape->SetStrokeWidth(parser.ParseInt(parameters, 1));
		return nullptr;
	}
	else if (CompareWChar(L"STROKECOLOR", modifier))
	{
		shape->SetStrokeColor(ToGeometryColor(parser.ParseColor(parameters)));
		return nullptr;
	}
	else if (CompareWChar(L"FILLCOLOR", modifier))
	{
		shape->SetFillColor(ToGeometryColor(parser.ParseColor(parameters)));
		return nullptr;
	}
	else if (CompareWChar(L"EXTEND", modifier))
	{
		if(recursion)
			return L"Extend is used recursively, this is not good practise!";
		std::vector<std::wstring> tokens = parser.Tokenize(parameters, L",");
		for (auto token : tokens) {
			std::wstring options = parser.ReadString(section, token.c_str(), L"");
			if (!options.empty())
				ParseModifiers(parser, section, shape, options.c_str(), true);
		}

		return nullptr;
	}
	return L"Modifier not found!";
}

const WCHAR* MeterGeometry::IsModifier(const WCHAR* modifier)
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
	else if (CompareWChar(L"EXTEND", modifier)) 
	{
		return L"EXTEND";
	}

	return nullptr;
}

const WCHAR * MeterGeometry::HandleShape(Gfx::Shape*& shape, const WCHAR* shapeType, const WCHAR* parameters)
{
	if (CompareWChar(L"RECTANGLE", shapeType))
	{
		shape = ParseRectangle(parameters);
		return nullptr;
	}
	return L"Shape not found";
}

const WCHAR * MeterGeometry::IsShape(const WCHAR * shape)
{
	if (CompareWChar(L"RECTANGLE", shape)) 
	{
		return L"RECTANGLE";
	}
	
	return nullptr;
}

bool MeterGeometry::CompareWChar(const WCHAR* str1, const WCHAR* str2)
{
	return _wcsnicmp(str2, str1, wcslen(str1)) == 0;
}

void MeterGeometry::ParseModifiers(ConfigParser& parser, const WCHAR* section, Gfx::Shape*& mainShape, const WCHAR* modifierString, int recursion)
{
	std::vector<std::wstring> shapeTokens = ConfigParser::Tokenize(modifierString, L"|");
	std::vector<std::wstring> modifiers;
	for (const auto& token : shapeTokens) {
		const WCHAR* shape = IsShape(token.c_str());
		const WCHAR* modifier = IsModifier(token.c_str());
		if(shape) 
		{
			const WCHAR* error = HandleShape(mainShape, shape, token.substr(token.find_first_not_of(' ', wcslen(shape))).c_str());
			if (error)
			{
				LogWarningF(section, error);
			}

			if (mainShape)
			{
				for (const auto& modifierToken : modifiers)
				{
					const WCHAR* modifier = IsModifier(modifierToken.c_str());
					const WCHAR* error = HandleModifier(modifier, parser, modifierToken.substr(modifierToken.find_first_not_of(' ', wcslen(modifier))).c_str(), mainShape, section, recursion);
					if (error)
					{
						LogWarningF(section, error);
					}
				}
				modifiers.clear();
			}
		}
		else if (modifier)
		{
			if (mainShape)
			{
				const WCHAR* error = HandleModifier(modifier, parser, token.substr(token.find_first_not_of(' ', wcslen(modifier))).c_str(), mainShape, section, recursion);
				if (error)
				{
					LogWarningF(section, error);
				}
			}
			else
			{
				modifiers.push_back(token);
			}
		}
		else
		{
			LogWarningF(section, L"%s not recognized as a modifier or shape", token);
		}
	}
}

Gfx::Shape * MeterGeometry::ParseRectangle(const std::wstring& parameters)
{
	std::vector<Gdiplus::REAL> params = ConfigParser::ParseFloats(parameters.c_str(), L',');
	return new Gfx::RectangleShape(params);
}