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
	while (!shapeOption.empty()) {
		Gfx::Shape* shape = nullptr;
		bool newShape = true;
		if (m_Shapes.find(shapeId) != m_Shapes.end()) {
			shape = m_Shapes.at(shapeId).get();
			newShape = false;
		}
		ParseModifiers(parser, section, &shape, shapeOption.c_str());
		if(shape && newShape)
			m_Shapes.insert(std::pair<const std::wstring&, Gfx::Shape*>(shapeId, shape));
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

void MeterGeometry::ParseModifiers(ConfigParser& parser, const WCHAR* section, Gfx::Shape** mainShape, const WCHAR* modifierString, bool isExtended)
{
	std::vector<std::wstring> shapeTokens = ConfigParser::Tokenize(modifierString, L"|");
	std::vector<std::wstring> modifiers;
	for (const auto& token : shapeTokens) {
		for (const auto& shape : shapeRegistry)
		{
			if (_wcsnicmp(token.c_str(), shape.first, wcslen(shape.first)) == 0 && !isExtended) // It shouldn't be possible to define the shape in a extended option!
			{
				if (*mainShape)
				{
					//Shape already defined
					break;
				}
				(*mainShape) = shape.second(token.substr(token.find_first_not_of(' ') + wcslen(shape.first)));
				for (const auto& modifierToken : modifiers)
				{
					bool foundModifier = false;
					for (const auto& modifier : modifierRegistry)
					{
						if (_wcsnicmp(modifierToken.c_str(), modifier.first, wcslen(modifier.first)) == 0)
						{
							foundModifier = true;
							if (*mainShape) {
								modifier.second(parser, modifierToken.substr(token.find_first_not_of(' ', wcslen(modifier.first))), *mainShape, section);
							}
							else
							{
								//Shape suddenly dissappeared /shrug
							}
							break;
						}
						if (!foundModifier)
							; // modifier or shape modifier doesn't exist!
					}
				}
				break;
			}
		}

		for (const auto& modifier : modifierRegistry)
		{
			if (_wcsnicmp(token.c_str(), modifier.first, wcslen(modifier.first)) == 0)
			{
				//Unneeded comment, remove later! Not happy with this, but i can't think of any other way to stop infinite recursions except using a static int to count, which would be even worse!
				if (_wcsnicmp(token.c_str(), L"Extend", wcslen(L"Extend")) == 0 && isExtended) 
					continue;
				if (*mainShape) {
					int strLen = wcslen(modifier.first);
					modifier.second(parser, token.substr(token.find_first_not_of(' ', strLen)), *mainShape, section);
				}
				else
				{
					modifiers.push_back(token);
				}
				break;
			}
		}
	}
	if (!mainShape)
		;//Shape was never found :X
}

Gfx::Shape * MeterGeometry::ParseRectangle(const std::wstring& parameters)
{
	std::vector<Gdiplus::REAL> params = ConfigParser::ParseFloats(parameters.c_str(), L',');
	return new Gfx::RectangleShape(params);
}