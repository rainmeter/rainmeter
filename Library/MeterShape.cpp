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
#include "../Common/Gfx/Shape.h"
#include "../Common/Gfx/Shapes/Rectangle.h"
#include "../Common/Gfx/Shapes/RoundedRectangle.h"

MeterShape::MeterShape(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Shapes()
{
}

MeterShape::~MeterShape()
{
	Dispose();
}

void MeterShape::Dispose()
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

	// Clear any shapes
	Dispose();

	const std::wstring delimiter(1, L'|');
	std::wstring shape = parser.ReadString(section, L"Shape", L"");

	size_t i = 1;
	while (!shape.empty())
	{
		std::vector<std::wstring> args = ConfigParser::Tokenize(shape, delimiter);

		// Create shape
		if (!CreateShape(args)) break;

		// Remove shape from args
		args.erase(args.begin());

		// Parse any modifiers
		ParseModifiers(args, parser, section);

		D2D1_RECT_F bounds = m_Shapes.back()->GetBounds();
		if (!m_WDefined && m_W < bounds.right)
		{
			m_W = (int)bounds.right;
		}
		if (!m_HDefined && m_H < bounds.bottom)
		{
			m_H = (int)bounds.bottom;
		}

		// Check for Shape2 ... etc.
		const std::wstring num = std::to_wstring(++i);
		std::wstring key = L"Shape" + num;
		shape = parser.ReadString(section, key.c_str(), L"");
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

bool MeterShape::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	int x = Meter::GetX();
	int y = Meter::GetY();

	for (const auto& shape : m_Shapes)
	{
		if (shape->IsShapeDefined())
		{
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

bool MeterShape::CreateShape(std::vector<std::wstring>& args)
{
	const size_t argSize = args.size();
	const WCHAR* shapeName = args[0].c_str();
	if (_wcsnicmp(shapeName, L"RECTANGLE", 9) == 0)
	{
		shapeName += 9;
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize == 4)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseInt(tokens[0].c_str(), 0);
			FLOAT y = (FLOAT)ConfigParser::ParseInt(tokens[1].c_str(), 0);
			FLOAT w = (FLOAT)ConfigParser::ParseInt(tokens[2].c_str(), 0);
			FLOAT h = (FLOAT)ConfigParser::ParseInt(tokens[3].c_str(), 0);
			m_Shapes.emplace_back(new Gfx::Rectangle(x, y, w, h));
			return true;
		}
		else if (tokSize > 4)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseInt(tokens[0].c_str(), 0);
			FLOAT y = (FLOAT)ConfigParser::ParseInt(tokens[1].c_str(), 0);
			FLOAT w = (FLOAT)ConfigParser::ParseInt(tokens[2].c_str(), 0);
			FLOAT h = (FLOAT)ConfigParser::ParseInt(tokens[3].c_str(), 0);
			FLOAT xRadius = (FLOAT)ConfigParser::ParseInt(tokens[4].c_str(), 0);
			FLOAT yRadius = xRadius;

			if (tokSize > 5) yRadius = (FLOAT)ConfigParser::ParseInt(tokens[5].c_str(), 0);
			m_Shapes.emplace_back(new Gfx::RoundedRectangle(x, y, w, h, xRadius, yRadius));
			return true;
		}
		else
		{
			// LogError
			return false;
		}
	}
	//else if (_wcsnicmp(shapeName, L"", ) == 0)
	//{
	//}

	return false;
}

void MeterShape::ParseModifiers(std::vector<std::wstring>& args, ConfigParser& parser, const WCHAR* section, bool recursive)
{
	auto& shape = m_Shapes.back();

	for (const auto& option : args)
	{
		const WCHAR* modifier = option.c_str();

		if (_wcsnicmp(modifier, L"FILLCOLOR", 9) == 0)
		{
			modifier += 9;
			auto color = ConfigParser::ParseColor(modifier);
			shape->SetFillColor(color);
		}
		else if (_wcsnicmp(modifier, L"STROKECOLOR", 11) == 0)
		{
			modifier += 11;
			auto color = ConfigParser::ParseColor(modifier);
			shape->SetStrokeColor(color);
		}
		else if (_wcsnicmp(modifier, L"STROKEWIDTH", 11) == 0)
		{
			modifier += 11;
			int width = ConfigParser::ParseInt(modifier, 0);
			shape->SetStrokeWidth(width);
		}
		else if (_wcsnicmp(modifier, L"OFFSET", 6) == 0)
		{
			modifier += 6;
			auto offset = ConfigParser::Tokenize2(modifier, L',', PairedPunctuation::Parentheses);
			if (offset.size() >= 2)
			{
				int x = ConfigParser::ParseInt(offset[0].c_str(), 0);
				int y = ConfigParser::ParseInt(offset[1].c_str(), 0);
				shape->SetOffset(x, y);
			}

			// LogError
		}
		else if (_wcsnicmp(modifier, L"ROTATE", 6) == 0)
		{
			modifier += 6;
			FLOAT rotate = (FLOAT)ConfigParser::ParseDouble(modifier, 0);
			shape->SetRotation(rotate);
		}
		//else if (_wcsnicmp(modifier, L"", ) == 0)
		//{
		//}
		else if (_wcsnicmp(modifier, L"EXTEND", 6) == 0)
		{
			modifier += 6;
			if (!recursive)
			{
				std::vector<std::wstring> extendParameters = ConfigParser::Tokenize(modifier, L",");
				for (auto& extend : extendParameters)
				{
					std::wstring key = parser.ReadString(section, extend.c_str(), L"");
					if (!key.empty())
					{
						std::vector<std::wstring> newArgs = ConfigParser::Tokenize(key, L"|");
						ParseModifiers(newArgs, parser, section, true);
					}
				}
			}
			else
			{
				// Error
			}
		}
	}
}
