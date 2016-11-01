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
	Meter::Initialize();
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

void MeterShape::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	// Clear any shapes
	Dispose();

	std::unordered_map<size_t, std::wstring> combinedShapes;

	const std::wstring delimiter(1, L'|');
	std::wstring shape = parser.ReadString(section, L"Shape", L"");

	size_t i = 1;
	while (!shape.empty())
	{
		std::vector<std::wstring> args = ConfigParser::Tokenize(shape, delimiter);

		bool isCombined = false;
		if (!CreateShape(args, isCombined, i - 1)) break;

		// If the shape is combined with another, save the shape definition and
		// process later. Otherwise, parse any modifiers for the shape.
		if (isCombined)
		{
			combinedShapes.insert(std::make_pair(i - 1, shape));
		}
		else
		{
			args.erase(args.begin());
			ParseModifiers(args, parser, section);
		}

		m_Shapes.back()->ValidateTransforms();
		m_Shapes.back()->CreateStrokeStyle();

		// Check for Shape2 ... etc.
		const std::wstring num = std::to_wstring(++i);
		std::wstring key = L"Shape" + num;
		shape = parser.ReadString(section, key.c_str(), L"");
	}

	// Process combined shapes
	for (const auto& shape : combinedShapes)
	{
		std::vector<std::wstring> args = ConfigParser::Tokenize(shape.second, delimiter);
		if (!CreateCombinedShape(shape.first, args)) break;
	}

	// Adjust width/height if necessary
	if (!m_WDefined || !m_HDefined)
	{
		int newW = 0;
		int newH = 0;

		for (const auto& shape : m_Shapes)
		{
			D2D1_RECT_F bounds = shape->GetBounds();
			int shapeW = (int)bounds.right;
			int shapeH = (int)bounds.bottom;
			if (newW < shapeW) newW = shapeW;
			if (newH < shapeH) newH = shapeH;
		}

		m_W = newW;
		m_H = newH;
	}
}

bool MeterShape::Update()
{
	if (Meter::Update())
	{
		return true;
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
		if (!shape->IsCombined())
		{
			canvas.DrawGeometry(*shape, x, y);
		}
	}

	return true;
}

bool MeterShape::HitTest(int x, int y)
{
	D2D1_POINT_2F point = { (FLOAT)(x - Meter::GetX()), (FLOAT)(y - Meter::GetY()) };
	for (auto& shape : m_Shapes)
	{
		if (!shape->IsCombined() && shape->ContainsPoint(point))
		{
			return true;
		}
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

bool MeterShape::CreateShape(std::vector<std::wstring>& args, bool& isCombined, size_t keyId)
{
	auto createShape = [&](Gfx::Shape* shape) -> bool
	{
		std::wstring id = keyId == 0 ? L"" : std::to_wstring(keyId);
		bool exists = shape->DoesShapeExist();
		if (exists)
		{
			m_Shapes.push_back(shape);
		}
		else
		{
			LogErrorF(this, L"Could not create shape: Shape%s", id.c_str());
			delete shape;
		}

		return exists;
	};

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

			if (!createShape(new Gfx::Rectangle(x, y, w, h)))
			{
				return false;
			}

			return true;
		}
		else if (tokSize > 4)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseInt(tokens[0].c_str(), 0);
			FLOAT y = (FLOAT)ConfigParser::ParseInt(tokens[1].c_str(), 0);
			FLOAT w = (FLOAT)ConfigParser::ParseInt(tokens[2].c_str(), 0);
			FLOAT h = (FLOAT)ConfigParser::ParseInt(tokens[3].c_str(), 0);
			FLOAT xRadius = (FLOAT)ConfigParser::ParseInt(tokens[4].c_str(), 0);
			FLOAT yRadius = (tokSize > 5) ?
				yRadius = (FLOAT)ConfigParser::ParseInt(tokens[5].c_str(), 0) :
				xRadius;

			if (!createShape(new Gfx::RoundedRectangle(x, y, w, h, xRadius, yRadius)))
			{
				return false;
			}

			return true;
		}
		else
		{
			LogErrorF(this, L"Rectangle has too few parameters");
			return false;
		}
	}
	// Add new shapes here
	//else if (_wcsnicmp(shapeName, L"", ) == 0)
	//{
	//}
	else if (_wcsnicmp(shapeName, L"COMBINE", 7) == 0)
	{
		// Combined shapes are processed after all shapes are created
		isCombined = true;
		return true;
	}

	LogErrorF(this, L"Invalid shape: %s", shapeName);
	return false;
}

bool MeterShape::CreateCombinedShape(size_t shapeId, std::vector<std::wstring>& args)
{
	auto showError = [&shapeId, this](const WCHAR* description, const WCHAR* error) -> void
	{
		std::wstring key = L"Shape";
		key += std::to_wstring(shapeId + 1);
		LogErrorF(this, L"%s %s \"%s\"", key.c_str(), description, error);
	};

	auto getShapeId = [=](const WCHAR* shape) -> size_t
	{
		int id = _wtoi(shape) - 1;
		return id < 0 ? (size_t)0 : (size_t)id;
	};

	size_t parentId = 0;

	const WCHAR* parentName = args[0].c_str();
	parentName += 8;  // Strip off 'Combine '
	if (_wcsnicmp(parentName, L"SHAPE", 5) == 0)
	{
		parentName += 5;  // Strip off 'Shape'
		parentId = getShapeId(parentName);

		if (parentId == shapeId)
		{
			// Cannot use myself as a parent shape
			showError(L"cannot combine with:", parentName - 5);
			return false;
		}

		if (parentId < m_Shapes.size())
		{
			Gfx::Shape* clonedShape = m_Shapes[parentId]->Clone();
			if (clonedShape)
			{
				m_Shapes.insert(m_Shapes.begin() + shapeId, clonedShape);
				m_Shapes[parentId]->SetCombined();

				// Combine with empty shape
				m_Shapes[shapeId]->CombineWith(nullptr, D2D1_COMBINE_MODE_UNION);
			}
			else
			{
				// Shape could not be cloned
				return false;
			}
		}
		else
		{
			showError(L"definition contains invalid shape reference: ", parentName - 5);
			return false;
		}
	}
	else
	{
		showError(L"defintion contains invalid shape identifier: ", parentName);
		return false;
	}

	args.erase(args.begin());  // Remove Combine definition

	for (const auto& option : args)
	{
		D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_FORCE_DWORD;
		const WCHAR* combined = option.c_str();
		if (_wcsnicmp(combined, L"UNION", 5) == 0)
		{
			combined += 6;
			mode = D2D1_COMBINE_MODE_UNION;
		}
		else if (_wcsnicmp(combined, L"XOR", 3) == 0)
		{
			combined += 4;
			mode = D2D1_COMBINE_MODE_XOR;
		}
		else if (_wcsnicmp(combined, L"INTERSECT", 9) == 0)
		{
			combined += 10;
			mode = D2D1_COMBINE_MODE_INTERSECT;
		}
		else if (_wcsnicmp(combined, L"EXCLUDE", 7) == 0)
		{
			combined += 8;
			mode = D2D1_COMBINE_MODE_EXCLUDE;
		}
		else
		{
			// Combined shapes can have their own transforms
			if (ParseTransformModifers(m_Shapes[shapeId], combined))
			{
				continue;
			}
			else
			{
				showError(L"definition contains invalid combine: ", combined);
				return false;
			}
		}

		combined += 5;  // Remove 'Shape'
		size_t id = getShapeId(combined);
		if (id == shapeId)
		{
			// Cannot combine with myself
			showError(L"cannot combine with:", combined - 5);
			return false;
		}

		if (id < m_Shapes.size())
		{
			m_Shapes[id]->SetCombined();

			if (!m_Shapes[shapeId]->CombineWith(m_Shapes[id], mode))
			{
				showError(L"could not combine with: ", combined - 5);
				return false;
			}
		}
		else
		{
			showError(L"defintion contains invalid shape identifier: ", combined - 5);
			return false;
		}
	}

	return true;
}

void MeterShape::ParseModifiers(std::vector<std::wstring>& args, ConfigParser& parser, const WCHAR* section, bool recursive)
{
	auto parseCap = [this](const WCHAR* cap) -> D2D1_CAP_STYLE
	{
		while (iswspace(*cap)) ++cap;  // remove any leading whitespace
		if (_wcsnicmp(cap, L"FLAT", 4) == 0) return D2D1_CAP_STYLE_FLAT;
		else if (_wcsnicmp(cap, L"SQUARE", 6) == 0) return D2D1_CAP_STYLE_SQUARE;
		else if (_wcsnicmp(cap, L"ROUND", 5) == 0) return D2D1_CAP_STYLE_ROUND;
		else if (_wcsnicmp(cap, L"TRIANGLE", 8) == 0) return D2D1_CAP_STYLE_TRIANGLE;
		else
		{
			if (*cap) LogWarningF(this, L"Invalid cap style: %s", cap);
			return D2D1_CAP_STYLE_FLAT;
		}
	};

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
			if (width < 0)
			{
				LogWarningF(this, L"StrokeWidth must not be negative");
				width = 0;
			}

			shape->SetStrokeWidth(width);
		}
		else if (_wcsnicmp(modifier, L"STROKESTARTCAP", 14) == 0)
		{
			modifier += 14;
			shape->SetStrokeStartCap(parseCap(modifier));
		}
		else if (_wcsnicmp(modifier, L"STROKEENDCAP", 12) == 0)
		{
			modifier += 12;
			shape->SetStrokeEndCap(parseCap(modifier));
		}
		else if (_wcsnicmp(modifier, L"STROKEDASHCAP", 13) == 0)
		{
			modifier += 13;
			shape->SetStrokeDashCap(parseCap(modifier));
		}
		else if (_wcsnicmp(modifier, L"STROKELINEJOIN", 14) == 0)
		{
			modifier += 14;
			auto style = ConfigParser::Tokenize2(modifier, L',', PairedPunctuation::Parentheses);
			size_t size = style.size();

			if (size > 0)
			{
				const WCHAR* option = style[0].c_str();
				D2D1_LINE_JOIN join = D2D1_LINE_JOIN_MITER;
				FLOAT limit = 10.0f;

				if (_wcsicmp(option, L"MITER") == 0) join = D2D1_LINE_JOIN_MITER;
				else if (_wcsicmp(option, L"BEVEL") == 0) join = D2D1_LINE_JOIN_BEVEL;
				else if (_wcsicmp(option, L"ROUND") == 0) join = D2D1_LINE_JOIN_ROUND;
				else if (_wcsicmp(option, L"MITERORBEVEL") == 0) join = D2D1_LINE_JOIN_MITER_OR_BEVEL;
				else
				{
					LogWarningF(this, L"Invalid line join style: %s", option);
				}

				if (size > 1)
				{
					limit = (FLOAT)ConfigParser::ParseDouble(style[1].c_str(), 10.0);
					if (limit < 0.0f)
					{
						LogWarningF(this, L"Miter limit must be positive");
						limit = 10.0f;
					}
				}

				shape->SetStrokeLineJoin(join, limit);
			}
			else
			{
				LogErrorF(this, L"StrokeLineJoin has too few parameters");
			}
		}
		else if (_wcsnicmp(modifier, L"STROKEDASHES", 12) == 0)
		{
			modifier += 12;
			std::vector<FLOAT> dashes;
			auto definedDashes = ConfigParser::Tokenize2(modifier, L',', PairedPunctuation::Parentheses);
			for (const auto& dash : definedDashes)
			{
				FLOAT value = (FLOAT)ConfigParser::ParseDouble(dash.c_str(), 0.0);
				dashes.emplace_back(value);
			}

			shape->SetStrokeDashes(dashes);
		}
		else if (_wcsnicmp(modifier, L"STROKEDASHOFFSET", 16) == 0)
		{
			modifier += 16;
			FLOAT dashOffset = (FLOAT)ConfigParser::ParseInt(modifier, 0);
			if (dashOffset < 0.0f)
			{
				LogWarningF(this, L"Invalid stroke dash offset: %s", modifier);
				dashOffset = 0.0f;
			}

			shape->SetStrokeDashOffset(dashOffset);
		}
		// Add new modifiers here
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
				LogErrorF(this, L"Extend cannot be used recursively");
			}
		}
		else
		{
			// Parse any transform modifiers
			if (!ParseTransformModifers(shape, modifier))
			{
				LogErrorF(this, L"Invalid shape modifier: %s", modifier);
			}
		}
	}
}

bool MeterShape::ParseTransformModifers(Gfx::Shape* shape, const WCHAR* transform)
{
	if (_wcsnicmp(transform, L"OFFSET", 6) == 0)
	{
		transform += 6;
		auto offset = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		if (offset.size() >= 2)
		{
			int x = ConfigParser::ParseInt(offset[0].c_str(), 0);
			int y = ConfigParser::ParseInt(offset[1].c_str(), 0);
			shape->SetOffset(x, y);
		}
		else
		{
			LogErrorF(this, L"Offset has too few parameters");
		}

		return true;
	}
	else if (_wcsnicmp(transform, L"ROTATE", 6) == 0)
	{
		transform += 6;
		auto rotate = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		size_t size = rotate.size();
		if (size > 0)
		{
			bool anchorDefined = false;
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			FLOAT rotation = (FLOAT)ConfigParser::ParseInt(rotate[0].c_str(), 0);
			if (size > 2)
			{
				anchorX = (FLOAT)ConfigParser::ParseInt(rotate[1].c_str(), 0);
				anchorY = (FLOAT)ConfigParser::ParseInt(rotate[2].c_str(), 0);
				anchorDefined = true;
			}

			shape->SetRotation(rotation, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Rotate has too few parameters");
		}

		return true;
	}
	else if (_wcsnicmp(transform, L"SCALE", 5) == 0)
	{
		transform += 5;
		auto scale = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		size_t size = scale.size();
		if (size > 1)
		{
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			bool anchorDefined = false;

			FLOAT scaleX = (FLOAT)ConfigParser::ParseDouble(scale[0].c_str(), 1.0);
			FLOAT scaleY = (FLOAT)ConfigParser::ParseDouble(scale[1].c_str(), 1.0);

			if (size > 3)
			{
				anchorX = (FLOAT)ConfigParser::ParseInt(scale[2].c_str(), 0);
				anchorY = (FLOAT)ConfigParser::ParseInt(scale[3].c_str(), 0);
				anchorDefined = true;
			}

			shape->SetScale(scaleX, scaleY, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Scale has too few parameters");
		}

		return true;
	}
	else if (_wcsnicmp(transform, L"SKEW", 4) == 0)
	{
		transform += 4;
		auto skew = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		size_t size = skew.size();
		if (size > 1)
		{
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			bool anchorDefined = false;

			FLOAT skewX = (FLOAT)ConfigParser::ParseDouble(skew[0].c_str(), 1.0);
			FLOAT skewY = (FLOAT)ConfigParser::ParseDouble(skew[1].c_str(), 1.0);

			if (size > 3)
			{
				anchorX = (FLOAT)ConfigParser::ParseInt(skew[2].c_str(), 0);
				anchorY = (FLOAT)ConfigParser::ParseInt(skew[3].c_str(), 0);
				anchorDefined = true;
			}

			shape->SetSkew(skewX, skewY, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Skew has too few parameters");
		}

		return true;
	}
	else if (_wcsnicmp(transform, L"TRANSFORMORDER", 14) == 0)
	{
		transform += 14;
		auto order = ConfigParser::Tokenize(transform, L",");
		if (order.size() > 0)
		{
			Gfx::TransformType type = Gfx::TransformType::Invalid;
			for (const auto& definedType : order)
			{
				const WCHAR* t = definedType.c_str();
				if (_wcsnicmp(t, L"ROTATE", 6) == 0) type = Gfx::TransformType::Rotate;
				else if (_wcsnicmp(t, L"SCALE", 5) == 0) type = Gfx::TransformType::Scale;
				else if (_wcsnicmp(t, L"SKEW", 4) == 0) type = Gfx::TransformType::Skew;
				else if (_wcsnicmp(t, L"OFFSET", 6) == 0) type = Gfx::TransformType::Offset;

				if (type == Gfx::TransformType::Invalid) LogWarningF(this, L"Invalid transform type: %s", t);
				else if (!shape->AddToTransformOrder(type)) LogWarningF(this, L"Transform type already used: %s", t);
			}
		}
		else
		{
			LogErrorF(this, L"TransformOrder has too few parameters");
		}

		return true;
	}

	return false;
}
