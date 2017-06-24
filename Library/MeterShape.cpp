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
#include "../Common/Gfx/Util/D2DUtil.h"
#include "../Common/Gfx/Shape.h"
#include "../Common/Gfx/Shapes/Rectangle.h"
#include "../Common/Gfx/Shapes/RoundedRectangle.h"
#include "../Common/Gfx/Shapes/Ellipse.h"
#include "../Common/Gfx/Shapes/Line.h"
#include "../Common/Gfx/Shapes/Arc.h"
#include "../Common/Gfx/Shapes/Curve.h"
#include "../Common/Gfx/Shapes/QuadraticCurve.h"
#include "../Common//Gfx/Shapes/Path.h"

namespace {

// Helpers to allow default values to be used instead of needing to be defined
auto ParseNumber = [](auto var, const WCHAR* value, auto defValue, auto* func) -> decltype(var)
{
	if (_wcsnicmp(value, L"*", 1) == 0) return var;
	return (decltype(var))func(value, defValue);
};

auto ParseBool = [](auto& var, const WCHAR* value)
{
	if (_wcsnicmp(value, L"*", 1) != 0) var = (ConfigParser::ParseInt(value, 0) == 0);
};

}  // namespace

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

	std::map<size_t, std::wstring> combinedShapes;

	const WCHAR delimiter = L'|';
	std::wstring shape = parser.ReadString(section, L"Shape", L"");

	size_t i = 1;
	while (!shape.empty())
	{
		auto args = ConfigParser::Tokenize2(shape, delimiter, PairedPunctuation::Parentheses);

		bool isCombined = false;
		if (!CreateShape(args, parser, section, isCombined, i - 1)) break;

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

		// Check for Shape2 ... etc.
		const std::wstring num = std::to_wstring(++i);
		std::wstring key = L"Shape" + num;
		shape = parser.ReadString(section, key.c_str(), L"");
	}

	// Process combined shapes
	for (const auto& shape : combinedShapes)
	{
		auto args = ConfigParser::Tokenize2(shape.second, delimiter, PairedPunctuation::Parentheses);
		if (!CreateCombinedShape(shape.first, args)) break;
	}

	// Adjust width/height if necessary
	if (!m_WDefined || !m_HDefined)
	{
		int newW = 0;
		int newH = 0;

		for (const auto& shape : m_Shapes)
		{
			if (shape->IsCombined()) continue;

			D2D1_RECT_F bounds = shape->GetBounds();
			int shapeW = (int)ceil(bounds.right);  // Account for 'half-pixels'
			int shapeH = (int)ceil(bounds.bottom);
			if (newW < shapeW) newW = shapeW;
			if (newH < shapeH) newH = shapeH;
		}

		m_W = newW + GetWidthPadding();
		m_H = newH + GetHeightPadding();
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

	auto padding = GetMeterRectPadding();

	for (const auto& shape : m_Shapes)
	{
		if (!shape->IsCombined())
		{
			canvas.DrawGeometry(*shape, padding.X, padding.Y);
		}
	}

	return true;
}

bool MeterShape::HitTest(int x, int y)
{
	const Gdiplus::Matrix* matrix = GetTransformationMatrix();
	D2D1_POINT_2F point = { (FLOAT)(x - Meter::GetX()), (FLOAT)(y - Meter::GetY()) };
	for (auto& shape : m_Shapes)
	{
		if (!shape->IsCombined() && shape->ContainsPoint(point, matrix))
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

bool MeterShape::CreateShape(std::vector<std::wstring>& args, ConfigParser& parser,
	const WCHAR* section, bool& isCombined, size_t keyId)
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
	std::wstring shapeName = args[0];
	if (StringUtil::CaseInsensitiveCompareN(shapeName, L"RECTANGLE"))
	{
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize == 4)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT w = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT h = (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0);

			if (!createShape(new Gfx::Rectangle(x, y, w, h)))
			{
				return false;
			}

			return true;
		}
		else if (tokSize > 4)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT w = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT h = (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0);
			FLOAT xRadius = (FLOAT)ConfigParser::ParseDouble(tokens[4].c_str(), 0.0);
			FLOAT yRadius = (tokSize > 5) ?
				(FLOAT)ConfigParser::ParseDouble(tokens[5].c_str(), 0.0) :
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
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"ELLIPSE"))
	{
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize > 2)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT xRadius = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT yRadius = (tokSize > 3) ? (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0) : xRadius;

			if (!createShape(new Gfx::Ellipse(x, y, xRadius, yRadius)))
			{
				return false;
			}

			return true;
		}
		else
		{
			LogErrorF(this, L"Ellipse has too few parameters");
			return false;
		}
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"LINE"))
	{
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize > 3)
		{
			FLOAT x1 = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y1 = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT x2 = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT y2 = (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0);

			if (!createShape(new Gfx::Line(x1, y1, x2, y2)))
			{
				return false;
			}

			return true;
		}
		else
		{
			LogErrorF(this, L"Line has too few parameters");
			return false;
		}
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"ARC"))
	{
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize > 3)
		{
			FLOAT x1 = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y1 = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT x2 = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT y2 = (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0);
			FLOAT dx = x2 - x1;
			FLOAT dy = y2 - y1;
			FLOAT xRadius = std::sqrtf(dx * dx + dy * dy) / 2.0f;
			FLOAT angle = 0.0f;
			bool sweep = true;
			bool size = true;
			bool open = true;

			if (tokSize > 4) xRadius = ParseNumber(xRadius, tokens[4].c_str(), 0.0, ConfigParser::ParseDouble);

			FLOAT yRadius = xRadius;
			if (tokSize > 5) yRadius = ParseNumber(yRadius, tokens[5].c_str(), 0.0, ConfigParser::ParseDouble);
			if (tokSize > 6) angle = ParseNumber(angle, tokens[6].c_str(), 0.0, ConfigParser::ParseDouble);
			if (tokSize > 7) ParseBool(sweep, tokens[7].c_str());
			if (tokSize > 8) ParseBool(size, tokens[8].c_str());
			if (tokSize > 9) ParseBool(open, tokens[9].c_str());

			if (!createShape(new Gfx::Arc(x1, y1, x2, y2, xRadius, yRadius, angle,
				sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
				size ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE,
				open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED)))
			{
				return false;
			}

			// Set the 'Fill Color' to transparent for open shapes.
			// This can be overridden if an actual 'Fill Color' is defined.
			if (open) m_Shapes.back()->SetFill(Gdiplus::Color::Transparent);
			return true;
		}
		else
		{
			LogErrorF(this, L"Arc has too few parameters");
			return false;
		}
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"CURVE"))
	{
		auto tokens = ConfigParser::Tokenize2(shapeName, L',', PairedPunctuation::Parentheses);
		auto tokSize = tokens.size();

		if (tokSize > 5)
		{
			FLOAT x1 = (FLOAT)ConfigParser::ParseDouble(tokens[0].c_str(), 0.0);
			FLOAT y1 = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			FLOAT x2 = (FLOAT)ConfigParser::ParseDouble(tokens[2].c_str(), 0.0);
			FLOAT y2 = (FLOAT)ConfigParser::ParseDouble(tokens[3].c_str(), 0.0);
			FLOAT cx1 = (FLOAT)ConfigParser::ParseDouble(tokens[4].c_str(), 0.0);
			FLOAT cy1 = (FLOAT)ConfigParser::ParseDouble(tokens[5].c_str(), 0.0);
			bool open = true;

			if (tokSize == 6 || tokSize == 7)
			{
				if (tokSize == 7) open = ConfigParser::ParseInt(tokens[6].c_str(), 0) == 0;
				
				if (!createShape(new Gfx::QuadraticCurve(x1, y1, x2, y2, cx1, cy1,
					open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED)))
				{
					return false;
				}
			}
			else if (tokSize > 7)
			{
				FLOAT cx2 = (FLOAT)ConfigParser::ParseDouble(tokens[6].c_str(), 0.0);
				FLOAT cy2 = (FLOAT)ConfigParser::ParseDouble(tokens[7].c_str(), 0.0);

				if (tokSize > 8) open = ConfigParser::ParseInt(tokens[8].c_str(), 0) == 0;
	
				if (!createShape(new Gfx::Curve(x1, y1, x2, y2, cx1, cy1, cx2, cy2,
					open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED)))
				{
					return false;
				}
			}

			// Set the 'Fill Color' to transparent for open shapes.
			// This can be overridden if an actual 'Fill Color' is defined.
			if (open) m_Shapes.back()->SetFill(Gdiplus::Color::Transparent);

			return true;
		}
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"PATH1"))
	{
		auto opt = parser.ReadString(section, shapeName.c_str(), L"");
		if (opt.empty() || !ParsePath(opt, D2D1_FILL_MODE_WINDING))
		{
			LogErrorF(this, L"Path shape has invalid parameters: %s", opt.c_str());
			return false;
		}

		return true;
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"PATH"))
	{
		auto opt = parser.ReadString(section, shapeName.c_str(), L"");
		if (opt.empty() || !ParsePath(opt, D2D1_FILL_MODE_ALTERNATE))
		{
			LogErrorF(this, L"Path shape has invalid parameters: %s", opt.c_str());
			return false;
		}

		return true;
	}
	else if (StringUtil::CaseInsensitiveCompareN(shapeName, L"COMBINE"))
	{
		// Because combined shapes need to be processed after the rest of the shapes
		// are created, we attempt to insert a 'dummy' rectangle shape here to preserve
		// the order in which the shapes are defined.

		if (!createShape(new Gfx::Rectangle(0.0, 0.0, 0.0, 0.0)))
		{
			return false;
		}

		// Set the 'combined' flag on this dummy shape so it isn't drawn in cases
		// where the combined shape is not valid (see CreateCombinedShapes)
		m_Shapes.back()->SetCombined();
		isCombined = true;
		return true;
	}

	LogErrorF(this, L"Invalid shape: %s", shapeName.c_str());
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

	std::wstring parentName = args[0].substr(8); // Remove 'Combine '
	if (StringUtil::CaseInsensitiveCompareN(parentName, L"SHAPE"))
	{
		parentId = getShapeId(parentName.c_str());
		if (parentId == shapeId)
		{
			// Cannot use myself as a parent shape
			showError(L"cannot combine with: Shape", parentName.c_str());
			return false;
		}

		if (parentId < m_Shapes.size())
		{
			Gfx::Shape* clonedShape = m_Shapes[parentId]->Clone();
			if (clonedShape)
			{
				// Delete and remove the shape from |m_Shapes|, then
				// insert the cloned shape into the position of the
				// deleted shape.

				delete m_Shapes[shapeId];
				auto iter = m_Shapes.erase(m_Shapes.begin() + shapeId);
				m_Shapes.insert(iter, clonedShape);

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
			showError(L"definition contains invalid shape reference: Shape", parentName.c_str());
			return false;
		}
	}
	else
	{
		showError(L"defintion contains invalid shape identifier: ", parentName.c_str());
		return false;
	}

	args.erase(args.begin());  // Remove Combine definition

	for (auto& option : args)
	{
		D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_FORCE_DWORD;
		if (StringUtil::CaseInsensitiveCompareN(option, L"UNION")) mode = D2D1_COMBINE_MODE_UNION;
		else if (StringUtil::CaseInsensitiveCompareN(option, L"XOR")) mode = D2D1_COMBINE_MODE_XOR;
		else if (StringUtil::CaseInsensitiveCompareN(option, L"INTERSECT")) mode = D2D1_COMBINE_MODE_INTERSECT;
		else if (StringUtil::CaseInsensitiveCompareN(option, L"EXCLUDE")) mode = D2D1_COMBINE_MODE_EXCLUDE;
		else if (ParseTransformModifers(m_Shapes[shapeId], option)) continue;
		else
		{
			showError(L"definition contains invalid combine: ", option.c_str());
			return false;
		}

		option.erase(0, 5);  // Remove 'Shape'
		size_t id = getShapeId(option.c_str());
		if (id == shapeId)
		{
			// Cannot combine with myself
			showError(L"cannot combine with: Shape", option.c_str());
			return false;
		}

		if (id < m_Shapes.size())
		{
			m_Shapes[id]->SetCombined();

			if (!m_Shapes[shapeId]->CombineWith(m_Shapes[id], mode))
			{
				showError(L"could not combine with: Shape", option.c_str());
				return false;
			}
		}
		else
		{
			showError(L"defintion contains invalid shape identifier: Shape", option.c_str());
			return false;
		}
	}

	m_Shapes[shapeId]->ValidateTransforms();
	return true;
}

void MeterShape::ParseModifiers(std::vector<std::wstring>& args, ConfigParser& parser, const WCHAR* section, bool recursive)
{
	auto parseCap = [this](std::wstring& cap) -> D2D1_CAP_STYLE
	{
		if (StringUtil::CaseInsensitiveCompareN(cap, L"FLAT")) return D2D1_CAP_STYLE_FLAT;
		else if (StringUtil::CaseInsensitiveCompareN(cap, L"SQUARE")) return D2D1_CAP_STYLE_SQUARE;
		else if (StringUtil::CaseInsensitiveCompareN(cap, L"ROUND")) return D2D1_CAP_STYLE_ROUND;
		else if (StringUtil::CaseInsensitiveCompareN(cap, L"TRIANGLE")) return D2D1_CAP_STYLE_TRIANGLE;
		else
		{
			if (!cap.empty()) LogErrorF(this, L"Invalid cap style: %s", cap.c_str());
			return D2D1_CAP_STYLE_FLAT;
		}
	};

	auto& shape = m_Shapes.back();

	for (auto& option : args)
	{
		if (StringUtil::CaseInsensitiveCompareN(option, L"FILL"))
		{
			if (StringUtil::CaseInsensitiveCompareN(option, L"COLOR"))
			{
				auto color = ConfigParser::ParseColor(option.c_str());
				shape->SetFill(color);
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"LINEARGRADIENT1"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::LinearGradient, opt.c_str(), true, false))
				{
					LogErrorF(this, L"LinearGradient1 has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"LINEARGRADIENT"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::LinearGradient, opt.c_str(), false, false))
				{
					LogErrorF(this, L"LinearGradient has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"RADIALGRADIENT1"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::RadialGradient, opt.c_str(), true, false))
				{
					LogErrorF(this, L"RadialGradient1 has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"RADIALGRADIENT"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::RadialGradient, opt.c_str(), false, false))
				{
					LogErrorF(this, L"RadialGradient has invalid parameters: %s", opt.c_str());
				}
			}
			else
			{
				LogErrorF(this, L"Fill has invalid parameters: %s", option.c_str());
			}
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKEWIDTH"))
		{
			FLOAT width = (FLOAT)ConfigParser::ParseDouble(option.c_str(), 0.0);
			if (width < 0.0f)
			{
				LogWarningF(this, L"StrokeWidth must not be negative");
				width = 0.0f;
			}

			shape->SetStrokeWidth(width);
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKESTARTCAP"))
		{
			shape->SetStrokeStartCap(parseCap(option));
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKEENDCAP"))
		{
			shape->SetStrokeEndCap(parseCap(option));
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKEDASHCAP"))
		{
			shape->SetStrokeDashCap(parseCap(option));
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKELINEJOIN"))
		{
			auto style = ConfigParser::Tokenize2(option, L',', PairedPunctuation::Parentheses);
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
				LogWarningF(this, L"StrokeLineJoin has too few parameters");
			}
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKEDASHES"))
		{
			std::vector<FLOAT> dashes;
			auto definedDashes = ConfigParser::Tokenize2(option, L',', PairedPunctuation::Parentheses);
			for (const auto& dash : definedDashes)
			{
				FLOAT value = (FLOAT)ConfigParser::ParseDouble(dash.c_str(), 0.0);
				dashes.emplace_back(value);
			}

			shape->SetStrokeDashes(dashes);
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKEDASHOFFSET"))
		{
			const WCHAR* modifier = option.c_str();
			FLOAT dashOffset = (FLOAT)ConfigParser::ParseDouble(modifier, 0.0);
			if (dashOffset < 0.0f)
			{
				LogWarningF(this, L"Invalid stroke dash offset: %s", modifier);
				dashOffset = 0.0f;
			}

			shape->SetStrokeDashOffset(dashOffset);
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"STROKE"))
		{
			if (StringUtil::CaseInsensitiveCompareN(option, L"COLOR"))
			{
				auto color = ConfigParser::ParseColor(option.c_str());
				shape->SetStrokeFill(color);
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"LINEARGRADIENT1"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::LinearGradient, opt.c_str(), true, true))
				{
					LogErrorF(this, L"LinearGradient1 has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"LINEARGRADIENT"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::LinearGradient, opt.c_str(), false, true))
				{
					LogErrorF(this, L"LinearGradient has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"RADIALGRADIENT1"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::RadialGradient, opt.c_str(), true, true))
				{
					LogErrorF(this, L"RadialGradient1 has invalid parameters: %s", opt.c_str());
				}
			}
			else if (StringUtil::CaseInsensitiveCompareN(option, L"RADIALGRADIENT"))
			{
				auto opt = parser.ReadString(section, option.c_str(), L"");
				if (opt.empty() || !ParseGradient(Gfx::BrushType::RadialGradient, opt.c_str(), false, true))
				{
					LogErrorF(this, L"RadialGradient has invalid parameters: %s", opt.c_str());
				}
			}
			else
			{
				LogErrorF(this, L"Stroke has invalid parameters: %s", option.c_str());
			}
		}
		else if (StringUtil::CaseInsensitiveCompareN(option, L"EXTEND"))
		{
			if (!recursive)
			{
				std::vector<std::wstring> extendParameters = ConfigParser::Tokenize(option, L",");
				for (auto& extend : extendParameters)
				{
					std::wstring key = parser.ReadString(section, extend.c_str(), L"");
					if (!key.empty())
					{
						auto newArgs = ConfigParser::Tokenize2(key, L'|', PairedPunctuation::Parentheses);
						ParseModifiers(newArgs, parser, section, true);
					}
				}
			}
			else
			{
				LogNoticeF(this, L"Extend cannot be used recursively");
			}
		}
		else if (!ParseTransformModifers(shape, option))
		{
			LogErrorF(this, L"Invalid shape modifier: %s", option.c_str());
		}
	}

	if (!recursive)
	{
		shape->CreateStrokeStyle();
		shape->ValidateTransforms();
	}
}

bool MeterShape::ParseTransformModifers(Gfx::Shape* shape, std::wstring& transform)
{
	if (StringUtil::CaseInsensitiveCompareN(transform, L"OFFSET"))
	{
		auto offset = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		if (offset.size() >= 2)
		{
			FLOAT x = (FLOAT)ConfigParser::ParseDouble(offset[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(offset[1].c_str(), 0.0);
			shape->SetOffset(x, y);
		}
		else
		{
			LogWarningF(this, L"Offset has too few parameters");
		}

		return true;
	}
	else if (StringUtil::CaseInsensitiveCompareN(transform, L"ROTATE"))
	{
		auto rotate = ConfigParser::Tokenize2(transform, L',', PairedPunctuation::Parentheses);
		size_t size = rotate.size();
		if (size > 0)
		{
			bool anchorDefined = false;
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			FLOAT rotation = (FLOAT)ConfigParser::ParseDouble(rotate[0].c_str(), 0.0);
			if (size > 2)
			{
				anchorX = (FLOAT)ConfigParser::ParseDouble(rotate[1].c_str(), 0.0);
				anchorY = (FLOAT)ConfigParser::ParseDouble(rotate[2].c_str(), 0.0);
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
	else if (StringUtil::CaseInsensitiveCompareN(transform, L"SCALE"))
	{
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
				anchorX = (FLOAT)ConfigParser::ParseDouble(scale[2].c_str(), 0.0);
				anchorY = (FLOAT)ConfigParser::ParseDouble(scale[3].c_str(), 0.0);
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
	else if (StringUtil::CaseInsensitiveCompareN(transform, L"SKEW"))
	{
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
				anchorX = (FLOAT)ConfigParser::ParseDouble(skew[2].c_str(), 0.0);
				anchorY = (FLOAT)ConfigParser::ParseDouble(skew[3].c_str(), 0.0);
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
	else if (StringUtil::CaseInsensitiveCompareN(transform, L"TRANSFORMORDER"))
	{
		auto order = ConfigParser::Tokenize(transform, L",");
		if (order.size() > 0)
		{
			shape->ResetTransformOrder();
			Gfx::TransformType type = Gfx::TransformType::Invalid;
			for (auto& t : order)
			{
				if (StringUtil::CaseInsensitiveCompareN(t, L"ROTATE")) type = Gfx::TransformType::Rotate;
				else if (StringUtil::CaseInsensitiveCompareN(t, L"SCALE")) type = Gfx::TransformType::Scale;
				else if (StringUtil::CaseInsensitiveCompareN(t, L"SKEW")) type = Gfx::TransformType::Skew;
				else if (StringUtil::CaseInsensitiveCompareN(t, L"OFFSET")) type = Gfx::TransformType::Offset;

				if (type == Gfx::TransformType::Invalid) LogWarningF(this, L"Invalid transform type: %s", t.c_str());
				else if (!shape->AddToTransformOrder(type)) LogWarningF(this, L"TransformOrder cannot have duplicates");
			}
		}
		else
		{
			LogWarningF(this, L"TransformOrder has too few parameters");
		}

		return true;
	}

	return false;
}

bool MeterShape::ParseGradient(Gfx::BrushType type, const WCHAR* options, bool altGamma, bool isStroke)
{
	auto& shape = m_Shapes.back();

	auto params = ConfigParser::Tokenize2(options, L'|', PairedPunctuation::Parentheses);
	size_t paramSize = params.size();
	if (paramSize < 2) return false;

	std::vector<D2D1_GRADIENT_STOP> stops(paramSize - 1);
	auto parseGradientStops = [&]() -> void
	{
		std::vector<std::wstring> tokens;
		for (size_t i = 1; i < paramSize; ++i)
		{
			tokens = ConfigParser::Tokenize2(params[i], L';', PairedPunctuation::Parentheses);
			if (tokens.size() == 2)
			{
				stops[i - 1].color = Gfx::Util::ToColorF(ConfigParser::ParseColor(tokens[0].c_str()));
				stops[i - 1].position = (FLOAT)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0);
			}
		}

		// If gradient only has 1 stop, add a transparent stop at appropriate place
		if (stops.size() == 1)
		{
			D2D1::ColorF color = { 0.0f, 0.0f, 0.0f, 0.0f };
			D2D1_GRADIENT_STOP stop = { 0.0f, color };
			if (stops[0].position < 0.5) stop.position = 1.0f;
			stops.push_back(stop);
		}
	};

	switch (type)
	{
	case Gfx::BrushType::LinearGradient:
		{
			const FLOAT angle = (FLOAT)fmod((360.0 + fmod(ConfigParser::ParseDouble(params[0].c_str(), 0.0), 360.0)), 360.0);
			parseGradientStops();

			if (isStroke)
			{
				shape->SetStrokeFill(angle, stops, altGamma);
				return true;
			}

			shape->SetFill(angle, stops, altGamma);
			return true;
		}

	case Gfx::BrushType::RadialGradient:
		{
			auto radial = ConfigParser::Tokenize2(params[0], L',', PairedPunctuation::Parentheses);
			size_t size = radial.size();

			if (size > 1)
			{
				FLOAT centerX = (FLOAT)ConfigParser::ParseDouble(radial[0].c_str(), 0.0);
				FLOAT centerY = (FLOAT)ConfigParser::ParseDouble(radial[1].c_str(), 0.0);
				FLOAT offsetX = FLT_MAX;
				FLOAT offsetY = FLT_MAX;
				FLOAT radiusX = FLT_MAX;
				FLOAT radiusY = FLT_MAX;

				if (size > 3)
				{
					offsetX = (FLOAT)ConfigParser::ParseDouble(radial[2].c_str(), 0.0);
					offsetY = (FLOAT)ConfigParser::ParseDouble(radial[3].c_str(), 0.0);
				}

				if (size > 5)
				{
					radiusX = (FLOAT)ConfigParser::ParseDouble(radial[4].c_str(), 0.0);
					radiusY = (FLOAT)ConfigParser::ParseDouble(radial[5].c_str(), 0.0);
				}

				parseGradientStops();

				if (isStroke)
				{
					shape->SetStrokeFill(
						D2D1::Point2F(offsetX, offsetY),
						D2D1::Point2F(centerX, centerY),
						D2D1::Point2F(radiusX, radiusY),
						stops,
						altGamma);

					return true;
				}

				shape->SetFill(
					D2D1::Point2F(offsetX, offsetY),
					D2D1::Point2F(centerX, centerY),
					D2D1::Point2F(radiusX, radiusY),
					stops,
					altGamma);

				return true;
			}
		}
	}

	return false;
}

bool MeterShape::ParsePath(std::wstring& options, D2D1_FILL_MODE fillMode)
{
	auto createSegmentFlags = [](bool stroke, bool round) -> D2D1_PATH_SEGMENT
	{
		D2D1_PATH_SEGMENT flags = D2D1_PATH_SEGMENT_NONE;
		if (stroke) flags |= D2D1_PATH_SEGMENT_FORCE_UNSTROKED;
		if (round) flags |= D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN;
		return flags;
	};

	auto params = ConfigParser::Tokenize2(options, L'|', PairedPunctuation::Parentheses);
	auto paramSize = params.size();
	if (paramSize < 2) return false;  // Must have a starting point and at least 1 segment

	// Parse starting point of shape
	auto stPoint = ConfigParser::Tokenize2(params[0], L',', PairedPunctuation::Parentheses);
	if (stPoint.size() < 2) return false;  // Starting point must have a x and y

	FLOAT startX = (FLOAT)ConfigParser::ParseDouble(stPoint[0].c_str(), 0.0);
	FLOAT startY = (FLOAT)ConfigParser::ParseDouble(stPoint[1].c_str(), 0.0);

	Gfx::Path* shape = new Gfx::Path(startX, startY, fillMode);

	bool error = false;
	bool open = true;
	bool setNoStroke = false;
	bool setRoundJoin = false;
	D2D1_POINT_2F currentPoint = D2D1::Point2F(startX, startY);

	for (size_t i = 1; i < paramSize; ++i)
	{
		auto& type = params[i];
		if (StringUtil::CaseInsensitiveCompareN(type, L"LINETO"))
		{
			auto lineTo = ConfigParser::Tokenize2(type, L',', PairedPunctuation::Parentheses);
			if (lineTo.size() < 2) { error = true; break; }

			FLOAT x = (FLOAT)ConfigParser::ParseDouble(lineTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(lineTo[1].c_str(), 0.0);

			shape->AddLine(x, y);

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (StringUtil::CaseInsensitiveCompareN(type, L"ARCTO"))
		{
			auto arcTo = ConfigParser::Tokenize2(type, L',', PairedPunctuation::Parentheses);
			auto arcSize = arcTo.size();
			if (arcSize < 2) { error = true; break; }

			FLOAT x = (FLOAT)ConfigParser::ParseDouble(arcTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(arcTo[1].c_str(), 0.0);
			FLOAT dx = x - currentPoint.x;
			FLOAT dy = y - currentPoint.y;
			FLOAT xRadius = std::sqrtf(dx * dx + dy * dy) / 2.0f;
			FLOAT angle = 0.0f;
			bool sweep = true;
			bool size = true;

			if (arcSize > 2) xRadius = ParseNumber(xRadius, arcTo[2].c_str(), 0.0, ConfigParser::ParseDouble);

			FLOAT yRadius = xRadius;
			if (arcSize > 3) yRadius = ParseNumber(yRadius, arcTo[3].c_str(), 0.0, ConfigParser::ParseDouble);
			if (arcSize > 4) angle = ParseNumber(angle, arcTo[4].c_str(), 0.0, ConfigParser::ParseDouble);
			if (arcSize > 5) ParseBool(sweep, arcTo[5].c_str());
			if (arcSize > 6) ParseBool(size, arcTo[6].c_str());

			shape->AddArc(x, y, xRadius, yRadius, angle,
				sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
				size ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE);

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (StringUtil::CaseInsensitiveCompareN(type, L"CURVETO"))
		{
			auto curveTo = ConfigParser::Tokenize2(type, L',', PairedPunctuation::Parentheses);
			auto curveSize = curveTo.size();
			if (curveSize < 4) { error = true; break; }

			FLOAT x = (FLOAT)ConfigParser::ParseDouble(curveTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)ConfigParser::ParseDouble(curveTo[1].c_str(), 0.0);
			FLOAT cx1 = (FLOAT)ConfigParser::ParseDouble(curveTo[2].c_str(), 0.0);
			FLOAT cy1 = (FLOAT)ConfigParser::ParseDouble(curveTo[3].c_str(), 0.0);

			if (curveSize < 6)
			{
				shape->AddQuadraticCurve(x, y, cx1, cy1);
			}
			else
			{
				FLOAT cx2 = (FLOAT)ConfigParser::ParseDouble(curveTo[4].c_str(), 0.0);
				FLOAT cy2 = (FLOAT)ConfigParser::ParseDouble(curveTo[5].c_str(), 0.0);
				
				shape->AddCubicCurve(x, y, cx1, cy1, cx2, cy2);
			}

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (StringUtil::CaseInsensitiveCompareN(type, L"SETNOSTROKE"))
		{
			setNoStroke = ConfigParser::ParseInt(type.c_str(), 0) != 0;
			shape->SetSegmentFlags(createSegmentFlags(setNoStroke, setRoundJoin));
		}
		else if (StringUtil::CaseInsensitiveCompareN(type, L"SETROUNDJOIN"))
		{
			setRoundJoin = ConfigParser::ParseInt(type.c_str(), 0) != 0;
			shape->SetSegmentFlags(createSegmentFlags(setNoStroke, setRoundJoin));
		}
		else if (StringUtil::CaseInsensitiveCompareN(type, L"CLOSEPATH"))
		{
			open = ConfigParser::ParseInt(type.c_str(), 0) == 0;
		}
		else
		{
			LogErrorF(this, L"Invalid Path type: %s", type.c_str());
			error = true;
			break;
		}
	}

	if (error)
	{
		delete shape;
		return false;
	}

	shape->Close(open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);

	// Set the 'Fill Color' to transparent for open shapes.
	// This can be overridden if an actual 'Fill Color' is defined.
	if (open) shape->SetFill(Gdiplus::Color::Transparent);

	m_Shapes.push_back(shape);

	return true;
}
