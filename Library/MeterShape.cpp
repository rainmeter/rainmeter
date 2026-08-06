// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterShape.h"
#include "Logger.h"
#include "../Common/StringUtil.h"
#include "../Common/Gfx/Util/D2DUtil.h"
#include "../Common/Gfx/Shape.h"

namespace {

// A shape option is a '|' separated list of a shape definition and its modifiers, any of which
// may contain a formula. Returns the next non-empty option, or an empty view once consumed.
std::wstring_view ConsumeOption(StringParser& options)
{
	while (!options.IsConsumed())
	{
		const auto option = options.ConsumeUntilOrRest(
			L'|', StringParser::SkipWhitespace | StringParser::SkipNestedParentheses);
		if (!option.empty()) return option;
	}

	return {};
}

// The values of a shape definition are comma separated, and may contain formulas with commas of
// their own. Returns nullopt once every value has been consumed.
std::optional<std::wstring_view> ConsumeValue(StringParser& values)
{
	// A trailing value containing only whitespace is not a value.
	values.ConsumeWhitespace();
	if (values.IsConsumed()) return std::nullopt;

	return values.ConsumeUntilOrRest(
		L',', StringParser::SkipWhitespace | StringParser::SkipNestedParentheses);
}

// Reads the next value of a shape definition. Returns false if there is none, leaving |value|
// unchanged.
bool ReadValue(ConfigParser& parser, StringParser& values, FLOAT& value, double defValue = 0.0)
{
	const auto next = ConsumeValue(values);
	if (!next) return false;

	value = (FLOAT)parser.ParseDouble(*next, defValue);
	return true;
}

// Reads the two values of a transform anchor. Both are left unchanged unless both are read.
bool ReadAnchor(ConfigParser& parser, StringParser& values, FLOAT& x, FLOAT& y)
{
	FLOAT anchorX = 0.0f;
	FLOAT anchorY = 0.0f;
	if (!ReadValue(parser, values, anchorX) || !ReadValue(parser, values, anchorY)) return false;

	x = anchorX;
	y = anchorY;
	return true;
}

// As above, but "*" also leaves |value| unchanged, which is how the default of a value derived
// from the earlier ones is requested.
bool ReadValueOrDefault(ConfigParser& parser, StringParser& values, FLOAT& value)
{
	const auto next = ConsumeValue(values);
	if (!next) return false;

	if (!next->starts_with(L'*')) value = (FLOAT)parser.ParseDouble(*next, 0.0);
	return true;
}

bool ReadFlag(ConfigParser& parser, StringParser& values, bool& flag)
{
	const auto next = ConsumeValue(values);
	if (!next) return false;

	flag = parser.ParseInt(*next, 0) == 0;
	return true;
}

bool ReadFlagOrDefault(ConfigParser& parser, StringParser& values, bool& flag)
{
	const auto next = ConsumeValue(values);
	if (!next) return false;

	if (!next->starts_with(L'*')) flag = parser.ParseInt(*next, 0) == 0;
	return true;
}

bool CompareAndStrip(std::wstring& str, const WCHAR* prefix)
{
	const size_t len = wcslen(prefix);
	if (_wcsnicmp(str.c_str(), prefix, len) == 0)
	{
		str.erase(0, len);
		str.erase(0, str.find_first_not_of(L" \t\r\n"));
		return true;
	}

	return false;
}

// Helpers to allow default values to be used instead of needing to be defined
auto ParseNumber = [](ConfigParser& parser, auto var, const WCHAR* value, auto defValue) -> decltype(var)
{
	if (_wcsnicmp(value, L"*", 1) == 0) return var;
	return (decltype(var))parser.ParseDouble(value, defValue);
};

auto ParseBool = [](ConfigParser& parser, auto& var, const WCHAR* value)
{
	if (_wcsnicmp(value, L"*", 1) != 0) var = (parser.ParseInt(value, 0) == 0);
};

}  // namespace

MeterShape::MeterShape(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Shapes(),
	m_ShapeOptions()
{
	Meter::Initialize();
}

MeterShape::~MeterShape()
{
	Dispose();
}

void MeterShape::Dispose()
{
	m_Shapes.clear();
}

void MeterShape::InvalidateDeviceResources()
{
	Meter::InvalidateDeviceResources();

	for (auto& shape : m_Shapes)
	{
		shape.InvalidateDeviceResources();
	}
}

void MeterShape::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	bool shapeOptionsChanged = m_ShapeOptions.empty();
	for (auto& option : m_ShapeOptions)
	{
		const auto& value = parser.ReadString(section, option.first.c_str(), L"");
		if (value != option.second) shapeOptionsChanged = true;
		option.second = value;
	}

	if (shapeOptionsChanged)
	{
		Dispose();

		std::map<size_t, std::wstring> combinedShapes;

		WCHAR key[32] = L"Shape";
		for (size_t i = 1; ; ++i)
		{
			if (i > 1) _snwprintf_s(key, _TRUNCATE, L"Shape%zu", i);
			// Cache the first empty ShapeN option so newly added shapes are detected.
			auto shape = ReadShapeOption(parser, section, key);
			if (shape.empty()) break;

			StringParser options(shape);
			const auto definition = ConsumeOption(options);

			bool isCombined = false;
			if (!CreateShape(definition, parser, section, isCombined, i - 1)) break;

			// If the shape is combined with another, process later once all shapes have been read.
			if (isCombined)
			{
				combinedShapes.emplace(i - 1, std::move(shape));
			}
			else
			{
				ParseModifiers(m_Shapes[i - 1], options, parser, section);
			}
		}

		for (auto& shape : combinedShapes)
		{
			if (!CreateCombinedShape(parser, shape.first, shape.second)) break;
		}
	}

	// Adjust width/height if necessary
	if (!m_WDefined || !m_HDefined)
	{
		int newW = 0;
		int newH = 0;

		for (const auto& shape : m_Shapes)
		{
			if (shape.IsCombined()) continue;

			D2D1_RECT_F bounds = shape.GetBounds();
			int shapeW = (int)ceil(bounds.right);  // Account for 'half-pixels'
			int shapeH = (int)ceil(bounds.bottom);
			if (newW < shapeW) newW = shapeW;
			if (newH < shapeH) newH = shapeH;
		}

		m_W = newW + GetWidthPadding();
		m_H = newH + GetHeightPadding();
	}
}

std::wstring MeterShape::ReadShapeOption(ConfigParser& parser, const WCHAR* section, std::wstring key)
{
	StringUtil::ToUpperCase(key);

	auto iter = m_ShapeOptions.find(key);
	if (iter != m_ShapeOptions.end()) return iter->second;

	const auto& value = parser.ReadString(section, key.c_str(), L"");
	m_ShapeOptions.emplace(std::move(key), value);
	return value;
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

	canvas.SetAntiAliasing(true);  // Temporary

	for (auto& shape : m_Shapes)
	{
		if (!shape.IsCombined())
		{
			canvas.DrawGeometry(shape, (int)padding.left, (int)padding.top);
		}
	}

	return true;
}

bool MeterShape::HitTest(int x, int y)
{
	if (!Meter::HitTestContainer(x, y))
	{
		return false;
	}

	const D2D1_MATRIX_3X2_F& matrix = GetTransformationMatrix();

	D2D1_POINT_2F point = D2D1::Point2F((FLOAT)(x - Meter::GetX()), (FLOAT)(y - Meter::GetY()));
	for (auto& shape : m_Shapes)
	{
		if (!shape.IsCombined() && shape.ContainsPoint(point, matrix))
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

bool MeterShape::CreateShape(std::wstring_view definition, ConfigParser& parser,
	const WCHAR* section, bool& isCombined, size_t keyId)
{
	auto addShape = [&](std::optional<Gfx::Shape> shape) -> bool
	{
		if (shape)
		{
			m_Shapes.emplace_back(std::move(*shape));
			return true;
		}

		std::wstring id = keyId == 0 ? L"" : std::to_wstring(keyId);
		LogErrorF(this, L"Could not create shape: Shape%s", id.c_str());
		return false;
	};

	StringParser values(definition);
	if (values.Consume(L"Rectangle"))
	{
		FLOAT x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
		if (!ReadValue(parser, values, x) || !ReadValue(parser, values, y) ||
			!ReadValue(parser, values, w) || !ReadValue(parser, values, h))
		{
			LogErrorF(this, L"Rectangle has too few parameters");
			return false;
		}

		FLOAT xRadius = 0.0f;
		if (!ReadValue(parser, values, xRadius))
		{
			return addShape(Gfx::Shape::Rectangle(x, y, w, h));
		}

		FLOAT yRadius = xRadius;
		ReadValue(parser, values, yRadius);

		return addShape(Gfx::Shape::RoundedRectangle(x, y, w, h, xRadius, yRadius));
	}
	else if (values.Consume(L"Ellipse"))
	{
		FLOAT x = 0.0f, y = 0.0f, xRadius = 0.0f;
		if (!ReadValue(parser, values, x) || !ReadValue(parser, values, y) ||
			!ReadValue(parser, values, xRadius))
		{
			LogErrorF(this, L"Ellipse has too few parameters");
			return false;
		}

		FLOAT yRadius = xRadius;
		ReadValue(parser, values, yRadius);

		return addShape(Gfx::Shape::Ellipse(x, y, xRadius, yRadius));
	}
	else if (values.Consume(L"Line"))
	{
		FLOAT x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
		if (!ReadValue(parser, values, x1) || !ReadValue(parser, values, y1) ||
			!ReadValue(parser, values, x2) || !ReadValue(parser, values, y2))
		{
			LogErrorF(this, L"Line has too few parameters");
			return false;
		}

		return addShape(Gfx::Shape::Line(x1, y1, x2, y2));
	}
	else if (values.Consume(L"Arc"))
	{
		FLOAT x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
		if (!ReadValue(parser, values, x1) || !ReadValue(parser, values, y1) ||
			!ReadValue(parser, values, x2) || !ReadValue(parser, values, y2))
		{
			LogErrorF(this, L"Arc has too few parameters");
			return false;
		}

		const FLOAT dx = x2 - x1;
		const FLOAT dy = y2 - y1;
		FLOAT xRadius = std::sqrtf(dx * dx + dy * dy) / 2.0f;
		FLOAT angle = 0.0f;
		bool sweep = true;
		bool size = true;
		bool open = true;

		ReadValueOrDefault(parser, values, xRadius);

		FLOAT yRadius = xRadius;
		ReadValueOrDefault(parser, values, yRadius);
		ReadValueOrDefault(parser, values, angle);
		ReadFlagOrDefault(parser, values, sweep);
		ReadFlagOrDefault(parser, values, size);
		ReadFlagOrDefault(parser, values, open);

		const auto sweepDirection = sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
		const auto arcSize = size ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE;
		const auto figureEnd = open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED;
		auto shape = Gfx::Shape::Arc(x1, y1, x2, y2, xRadius, yRadius, angle, sweepDirection, arcSize, figureEnd);

		// Set the 'Fill Color' to transparent for open shapes.
		// This can be overridden if an actual 'Fill Color' is defined.
		if (open && shape) shape->SetFill(Gfx::Util::c_Transparent_Color_F);

		return addShape(std::move(shape));
	}
	else if (values.Consume(L"Curve"))
	{
		FLOAT x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, cx1 = 0.0f, cy1 = 0.0f;
		if (!ReadValue(parser, values, x1) || !ReadValue(parser, values, y1) ||
			!ReadValue(parser, values, x2) || !ReadValue(parser, values, y2) ||
			!ReadValue(parser, values, cx1) || !ReadValue(parser, values, cy1))
		{
			LogErrorF(this, L"Curve has too few parameters");
			return false;
		}

		bool open = true;

		auto addCurve = [&](std::optional<Gfx::Shape> shape) -> bool
		{
			// Set the 'Fill Color' to transparent for open shapes.
			// This can be overridden if an actual 'Fill Color' is defined.
			if (open && shape) shape->SetFill(Gfx::Util::c_Transparent_Color_F);

			return addShape(std::move(shape));
		};

		// A 7th value is the figure end of a quadratic curve, unless an 8th follows it, in which
		// case the two are the second control point of a cubic curve.
		const auto controlPoint = ConsumeValue(values);
		if (!controlPoint)
		{
			return addCurve(Gfx::Shape::QuadraticCurve(x1, y1, x2, y2, cx1, cy1, D2D1_FIGURE_END_OPEN));
		}

		FLOAT cy2 = 0.0f;
		if (!ReadValue(parser, values, cy2))
		{
			open = parser.ParseInt(*controlPoint, 0) == 0;
			return addCurve(Gfx::Shape::QuadraticCurve(x1, y1, x2, y2, cx1, cy1,
				open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED));
		}

		const FLOAT cx2 = (FLOAT)parser.ParseDouble(*controlPoint, 0.0);
		ReadFlag(parser, values, open);

		return addCurve(Gfx::Shape::Curve(x1, y1, x2, y2, cx1, cy1, cx2, cy2,
			open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED));
	}
	else if (values.Consume(L"Path1"))
	{
		values.ConsumeWhitespace();
		auto opt = ReadShapeOption(parser, section, std::wstring(values.Remaining()));
		if (opt.empty() || !ParsePath(parser, opt, D2D1_FILL_MODE_WINDING))
		{
			LogErrorF(this, L"Path shape has invalid parameters: %s", opt.c_str());
			return false;
		}

		return true;
	}
	else if (values.Consume(L"Path"))
	{
		values.ConsumeWhitespace();
		auto opt = ReadShapeOption(parser, section, std::wstring(values.Remaining()));
		if (opt.empty() || !ParsePath(parser, opt, D2D1_FILL_MODE_ALTERNATE))
		{
			LogErrorF(this, L"Path shape has invalid parameters: %s", opt.c_str());
			return false;
		}

		return true;
	}
	else if (values.Consume(L"Combine"))
	{
		// Reserve this position until combined shapes are processed.
		auto shape = Gfx::Shape::None();
		shape.SetCombined();
		isCombined = true;
		return addShape(std::move(shape));
	}

	LogErrorF(this, L"Invalid shape: %s", std::wstring(definition).c_str());
	return false;
}

bool MeterShape::CreateCombinedShape(ConfigParser& parser, size_t shapeId, std::wstring& options)
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

	StringParser combine(options);
	const auto definition = ConsumeOption(combine);
	if (definition.length() < 8)
	{
		std::wstring key = L"Shape";
		key += std::to_wstring(shapeId + 1);
		LogErrorF(this, L"%s definition contains no shape identifiers", key.c_str());
		return false;
	}

	std::wstring parentName(definition.substr(8));  // Remove 'Combine '
	if (CompareAndStrip(parentName, L"SHAPE"))
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
			m_Shapes[shapeId] = Gfx::Shape(m_Shapes[parentId]);
			m_Shapes[shapeId].SetCombined(false);

			m_Shapes[parentId].SetCombined();

			// Bake the parent shape's transform into the copied geometry.
			m_Shapes[shapeId].CombineWith(nullptr, D2D1_COMBINE_MODE_UNION);
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

	while (!combine.IsConsumed())
	{
		std::wstring option(ConsumeOption(combine));
		if (option.empty()) continue;

		D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_FORCE_DWORD;
		if (CompareAndStrip(option, L"UNION")) mode = D2D1_COMBINE_MODE_UNION;
		else if (CompareAndStrip(option, L"XOR")) mode = D2D1_COMBINE_MODE_XOR;
		else if (CompareAndStrip(option, L"INTERSECT")) mode = D2D1_COMBINE_MODE_INTERSECT;
		else if (CompareAndStrip(option, L"EXCLUDE")) mode = D2D1_COMBINE_MODE_EXCLUDE;
		else
		{
			StringParser transform(option);
			if (ParseTransformModifers(parser, m_Shapes[shapeId], transform)) continue;

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
			m_Shapes[id].SetCombined();

			if (!m_Shapes[shapeId].CombineWith(&m_Shapes[id], mode))
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

	m_Shapes[shapeId].ValidateTransforms();
	return true;
}

void MeterShape::ParseModifiers(Gfx::Shape& shape, StringParser& modifiers, ConfigParser& parser, const WCHAR* section, bool recursive)
{
	auto parseCap = [this](StringParser& cap) -> D2D1_CAP_STYLE
	{
		if (cap.Consume(L"Flat", StringParser::SkipWhitespace)) return D2D1_CAP_STYLE_FLAT;
		else if (cap.Consume(L"Square", StringParser::SkipWhitespace)) return D2D1_CAP_STYLE_SQUARE;
		else if (cap.Consume(L"Round", StringParser::SkipWhitespace)) return D2D1_CAP_STYLE_ROUND;
		else if (cap.Consume(L"Triangle", StringParser::SkipWhitespace)) return D2D1_CAP_STYLE_TRIANGLE;
		else
		{
			const std::wstring style(cap.ConsumeRest(StringParser::SkipWhitespace));
			if (!style.empty()) LogErrorF(this, L"Invalid cap style: %s", style.c_str());
			return D2D1_CAP_STYLE_FLAT;
		}
	};

	// The rest of the modifier names the option holding the gradient.
	auto parseGradient = [&](StringParser& option, Gfx::BrushType type, const WCHAR* name, bool altGamma, bool isStroke) -> void
	{
		auto opt = ReadShapeOption(parser, section, std::wstring(option.ConsumeRest(StringParser::SkipWhitespace)));
		if (opt.empty() || !ParseGradient(shape, parser, type, opt.c_str(), altGamma, isStroke))
		{
			LogErrorF(this, L"%s has invalid parameters: %s", name, opt.c_str());
		}
	};

	while (!modifiers.IsConsumed())
	{
		StringParser option(ConsumeOption(modifiers));
		if (option.IsConsumed()) continue;

		if (option.Consume(L"Fill"))
		{
			if (option.Consume(L"Color", StringParser::SkipWhitespace))
			{
				shape.SetFill(parser.ParseColor(option.ConsumeRest(StringParser::SkipWhitespace)));
			}
			else if (option.Consume(L"LinearGradient1", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::LinearGradient, L"LinearGradient1", true, false);
			}
			else if (option.Consume(L"LinearGradient", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::LinearGradient, L"LinearGradient", false, false);
			}
			else if (option.Consume(L"RadialGradient1", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::RadialGradient, L"RadialGradient1", true, false);
			}
			else if (option.Consume(L"RadialGradient", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::RadialGradient, L"RadialGradient", false, false);
			}
			else
			{
				LogErrorF(this, L"Fill has invalid parameters: %s",
					std::wstring(option.ConsumeRest(StringParser::SkipWhitespace)).c_str());
			}
		}
		else if (option.Consume(L"StrokeWidth"))
		{
			FLOAT width = 0.0f;
			ReadValue(parser, option, width);
			if (width < 0.0f)
			{
				LogWarningF(this, L"StrokeWidth must not be negative");
				width = 0.0f;
			}

			shape.SetStrokeWidth(width);
		}
		else if (option.Consume(L"StrokeStartCap"))
		{
			shape.GetStrokeProperties().startCap = parseCap(option);
		}
		else if (option.Consume(L"StrokeEndCap"))
		{
			shape.GetStrokeProperties().endCap = parseCap(option);
		}
		else if (option.Consume(L"StrokeDashCap"))
		{
			shape.GetStrokeProperties().dashCap = parseCap(option);
		}
		else if (option.Consume(L"StrokeLineJoin"))
		{
			const auto style = ConsumeValue(option);
			if (!style)
			{
				LogWarningF(this, L"StrokeLineJoin has too few parameters");
				continue;
			}

			D2D1_LINE_JOIN join = D2D1_LINE_JOIN_MITER;
			StringParser joinStyle(*style);
			if (joinStyle.ConsumeRest(L"Miter")) join = D2D1_LINE_JOIN_MITER;
			else if (joinStyle.ConsumeRest(L"Bevel")) join = D2D1_LINE_JOIN_BEVEL;
			else if (joinStyle.ConsumeRest(L"Round")) join = D2D1_LINE_JOIN_ROUND;
			else if (joinStyle.ConsumeRest(L"MiterOrBevel")) join = D2D1_LINE_JOIN_MITER_OR_BEVEL;
			else
			{
				LogWarningF(this, L"Invalid line join style: %s", std::wstring(*style).c_str());
			}

			FLOAT limit = 10.0f;
			ReadValue(parser, option, limit, 10.0);
			if (limit < 0.0f)
			{
				LogWarningF(this, L"Miter limit must be positive");
				limit = 10.0f;
			}

			auto& properties = shape.GetStrokeProperties();
			properties.lineJoin = join;
			properties.miterLimit = limit;
		}
		else if (option.Consume(L"StrokeDashes"))
		{
			std::vector<FLOAT> dashes;
			while (const auto dash = ConsumeValue(option))
			{
				dashes.emplace_back((FLOAT)parser.ParseDouble(*dash, 0.0));
			}

			shape.SetStrokeDashes(dashes);
		}
		else if (option.Consume(L"StrokeDashOffset"))
		{
			const std::wstring modifier(option.ConsumeRest(StringParser::SkipWhitespace));
			FLOAT dashOffset = (FLOAT)parser.ParseDouble(modifier, 0.0);
			if (dashOffset < 0.0f)
			{
				LogWarningF(this, L"Invalid stroke dash offset: %s", modifier.c_str());
				dashOffset = 0.0f;
			}

			shape.GetStrokeProperties().dashOffset = dashOffset;
		}
		else if (option.Consume(L"Stroke"))
		{
			if (option.Consume(L"Color", StringParser::SkipWhitespace))
			{
				shape.SetStrokeFill(parser.ParseColor(option.ConsumeRest(StringParser::SkipWhitespace)));
			}
			else if (option.Consume(L"LinearGradient1", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::LinearGradient, L"LinearGradient1", true, true);
			}
			else if (option.Consume(L"LinearGradient", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::LinearGradient, L"LinearGradient", false, true);
			}
			else if (option.Consume(L"RadialGradient1", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::RadialGradient, L"RadialGradient1", true, true);
			}
			else if (option.Consume(L"RadialGradient", StringParser::SkipWhitespace))
			{
				parseGradient(option, Gfx::BrushType::RadialGradient, L"RadialGradient", false, true);
			}
			else
			{
				LogErrorF(this, L"Stroke has invalid parameters: %s",
					std::wstring(option.ConsumeRest(StringParser::SkipWhitespace)).c_str());
			}
		}
		else if (option.Consume(L"Extend"))
		{
			if (recursive)
			{
				LogNoticeF(this, L"Extend cannot be used recursively");
				continue;
			}

			while (const auto extend = ConsumeValue(option))
			{
				if (extend->empty()) continue;

				std::wstring key = ReadShapeOption(parser, section, std::wstring(*extend));
				if (!key.empty())
				{
					StringParser extendedModifiers(key);
					ParseModifiers(shape, extendedModifiers, parser, section, true);
				}
			}
		}
		else if (!ParseTransformModifers(parser, shape, option))
		{
			LogErrorF(this, L"Invalid shape modifier: %s",
				std::wstring(option.ConsumeRest(StringParser::SkipWhitespace)).c_str());
		}
	}

	if (!recursive)
	{
		shape.CreateStrokeStyle();
		shape.ValidateTransforms();
	}
}

bool MeterShape::ParseTransformModifers(ConfigParser& parser, Gfx::Shape& shape, StringParser& transform)
{
	if (transform.Consume(L"Offset"))
	{
		FLOAT x = 0.0f;
		FLOAT y = 0.0f;
		if (ReadValue(parser, transform, x) && ReadValue(parser, transform, y))
		{
			shape.SetOffset(x, y);
		}
		else
		{
			LogWarningF(this, L"Offset has too few parameters");
		}

		return true;
	}
	else if (transform.Consume(L"Rotate"))
	{
		FLOAT rotation = 0.0f;
		if (ReadValue(parser, transform, rotation))
		{
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			const bool anchorDefined = ReadAnchor(parser, transform, anchorX, anchorY);

			shape.SetRotation(rotation, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Rotate has too few parameters");
		}

		return true;
	}
	else if (transform.Consume(L"Scale"))
	{
		FLOAT scaleX = 1.0f;
		FLOAT scaleY = 1.0f;
		if (ReadValue(parser, transform, scaleX, 1.0) && ReadValue(parser, transform, scaleY, 1.0))
		{
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			const bool anchorDefined = ReadAnchor(parser, transform, anchorX, anchorY);

			shape.SetScale(scaleX, scaleY, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Scale has too few parameters");
		}

		return true;
	}
	else if (transform.Consume(L"Skew"))
	{
		FLOAT skewX = 1.0f;
		FLOAT skewY = 1.0f;
		if (ReadValue(parser, transform, skewX, 1.0) && ReadValue(parser, transform, skewY, 1.0))
		{
			FLOAT anchorX = 0.0f;
			FLOAT anchorY = 0.0f;
			const bool anchorDefined = ReadAnchor(parser, transform, anchorX, anchorY);

			shape.SetSkew(skewX, skewY, anchorX, anchorY, anchorDefined);
		}
		else
		{
			LogWarningF(this, L"Skew has too few parameters");
		}

		return true;
	}
	else if (transform.Consume(L"TransformOrder"))
	{
		// Note that the type is deliberately not reset for each value, so an invalid type after a
		// valid one repeats the previous type.
		Gfx::TransformType type = Gfx::TransformType::Invalid;
		bool ordered = false;

		while (const auto value = ConsumeValue(transform))
		{
			if (value->empty()) continue;

			if (!ordered)
			{
				shape.ResetTransformOrder();
				ordered = true;
			}

			StringParser transformType(*value);
			if (transformType.Consume(L"Rotate")) type = Gfx::TransformType::Rotate;
			else if (transformType.Consume(L"Scale")) type = Gfx::TransformType::Scale;
			else if (transformType.Consume(L"Skew")) type = Gfx::TransformType::Skew;
			else if (transformType.Consume(L"Offset")) type = Gfx::TransformType::Offset;

			if (type == Gfx::TransformType::Invalid) LogWarningF(this, L"Invalid transform type: %s", std::wstring(*value).c_str());
			else if (!shape.AddToTransformOrder(type)) LogWarningF(this, L"TransformOrder cannot have duplicates");
		}

		if (!ordered) LogWarningF(this, L"TransformOrder has too few parameters");

		return true;
	}

	return false;
}

bool MeterShape::ParseGradient(Gfx::Shape& shape, ConfigParser& parser, Gfx::BrushType type, const WCHAR* options, bool altGamma, bool isStroke)
{
	auto params = ConfigParser::TokenizeWithPairedPunctuation(options, L'|', PairedPunctuation::Parentheses);
	size_t paramSize = params.size();
	if (paramSize < 2) return false;

	std::vector<D2D1_GRADIENT_STOP> stops(paramSize - 1);
	auto parseGradientStops = [&]() -> void
	{
		std::vector<std::wstring> tokens;
		for (size_t i = 1; i < paramSize; ++i)
		{
			tokens = ConfigParser::TokenizeWithPairedPunctuation(params[i], L';', PairedPunctuation::Parentheses);
			if (tokens.size() == 2)
			{
				stops[i - 1].color = parser.ParseColor(tokens[0].c_str());
				stops[i - 1].position = (FLOAT)parser.ParseDouble(tokens[1].c_str(), 0.0);
			}
		}

		// If gradient only has 1 stop, add a transparent stop at appropriate place
		if (stops.size() == 1)
		{
			D2D1_GRADIENT_STOP stop = D2D1::GradientStop(0.0f, Gfx::Util::c_Transparent_Color_F);
			if (stops[0].position < 0.5f) stop.position = 1.0f;
			stops.push_back(stop);
		}
	};

	switch (type)
	{
	case Gfx::BrushType::LinearGradient:
		{
			const FLOAT angle = (FLOAT)fmod((360.0 + fmod(parser.ParseDouble(params[0].c_str(), 0.0), 360.0)), 360.0);
			parseGradientStops();

			if (isStroke)
			{
				shape.SetStrokeFill(angle, stops, altGamma);
				return true;
			}

			shape.SetFill(angle, stops, altGamma);
			return true;
		}

	case Gfx::BrushType::RadialGradient:
		{
			auto radial = ConfigParser::TokenizeWithPairedPunctuation(params[0], L',', PairedPunctuation::Parentheses);
			size_t size = radial.size();

			if (size > 1)
			{
				FLOAT centerX = (FLOAT)parser.ParseDouble(radial[0].c_str(), 0.0);
				FLOAT centerY = (FLOAT)parser.ParseDouble(radial[1].c_str(), 0.0);
				FLOAT offsetX = FLT_MAX;
				FLOAT offsetY = FLT_MAX;
				FLOAT radiusX = FLT_MAX;
				FLOAT radiusY = FLT_MAX;

				if (size > 3)
				{
					offsetX = (FLOAT)parser.ParseDouble(radial[2].c_str(), 0.0);
					offsetY = (FLOAT)parser.ParseDouble(radial[3].c_str(), 0.0);
				}

				if (size > 5)
				{
					radiusX = (FLOAT)parser.ParseDouble(radial[4].c_str(), 0.0);
					radiusY = (FLOAT)parser.ParseDouble(radial[5].c_str(), 0.0);
				}

				parseGradientStops();

				if (isStroke)
				{
					shape.SetStrokeFill(
						D2D1::Point2F(offsetX, offsetY),
						D2D1::Point2F(centerX, centerY),
						D2D1::Point2F(radiusX, radiusY),
						stops,
						altGamma);

					return true;
				}

				shape.SetFill(
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

bool MeterShape::ParsePath(ConfigParser& parser, std::wstring& options, D2D1_FILL_MODE fillMode)
{
	auto createSegmentFlags = [](bool stroke, bool round) -> D2D1_PATH_SEGMENT
	{
		D2D1_PATH_SEGMENT flags = D2D1_PATH_SEGMENT_NONE;
		if (stroke) flags |= D2D1_PATH_SEGMENT_FORCE_UNSTROKED;
		if (round) flags |= D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN;
		return flags;
	};

	auto params = ConfigParser::TokenizeWithPairedPunctuation(options, L'|', PairedPunctuation::Parentheses);
	auto paramSize = params.size();
	if (paramSize < 2) return false;  // Must have a starting point and at least 1 segment

	// Parse starting point of shape
	auto stPoint = ConfigParser::TokenizeWithPairedPunctuation(params[0], L',', PairedPunctuation::Parentheses);
	if (stPoint.size() < 2) return false;  // Starting point must have a x and y

	FLOAT startX = (FLOAT)parser.ParseDouble(stPoint[0].c_str(), 0.0);
	FLOAT startY = (FLOAT)parser.ParseDouble(stPoint[1].c_str(), 0.0);

	auto shape = Gfx::Shape::Path(startX, startY, fillMode);
	if (!shape) return false;

	bool error = false;
	bool open = true;
	bool setNoStroke = false;
	bool setRoundJoin = false;
	D2D1_POINT_2F currentPoint = D2D1::Point2F(startX, startY);

	for (size_t i = 1; i < paramSize; ++i)
	{
		auto& type = params[i];
		if (CompareAndStrip(type, L"LINETO"))
		{
			auto lineTo = ConfigParser::TokenizeWithPairedPunctuation(type, L',', PairedPunctuation::Parentheses);
			if (lineTo.size() < 2) { error = true; break; }

			FLOAT x = (FLOAT)parser.ParseDouble(lineTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)parser.ParseDouble(lineTo[1].c_str(), 0.0);

			shape->AddPathLine(x, y);

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (CompareAndStrip(type, L"ARCTO"))
		{
			auto arcTo = ConfigParser::TokenizeWithPairedPunctuation(type, L',', PairedPunctuation::Parentheses);
			auto arcSize = arcTo.size();
			if (arcSize < 2) { error = true; break; }

			FLOAT x = (FLOAT)parser.ParseDouble(arcTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)parser.ParseDouble(arcTo[1].c_str(), 0.0);
			FLOAT dx = x - currentPoint.x;
			FLOAT dy = y - currentPoint.y;
			FLOAT xRadius = std::sqrtf(dx * dx + dy * dy) / 2.0f;
			FLOAT angle = 0.0f;
			bool sweep = true;
			bool size = true;

			if (arcSize > 2) xRadius = ParseNumber(parser, xRadius, arcTo[2].c_str(), 0.0);

			FLOAT yRadius = xRadius;
			if (arcSize > 3) yRadius = ParseNumber(parser, yRadius, arcTo[3].c_str(), 0.0);
			if (arcSize > 4) angle = ParseNumber(parser, angle, arcTo[4].c_str(), 0.0);
			if (arcSize > 5) ParseBool(parser, sweep, arcTo[5].c_str());
			if (arcSize > 6) ParseBool(parser, size, arcTo[6].c_str());

			shape->AddPathArc(x, y, xRadius, yRadius, angle,
				sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
				size ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE);

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (CompareAndStrip(type, L"CURVETO"))
		{
			auto curveTo = ConfigParser::TokenizeWithPairedPunctuation(type, L',', PairedPunctuation::Parentheses);
			auto curveSize = curveTo.size();
			if (curveSize < 4) { error = true; break; }

			FLOAT x = (FLOAT)parser.ParseDouble(curveTo[0].c_str(), 0.0);
			FLOAT y = (FLOAT)parser.ParseDouble(curveTo[1].c_str(), 0.0);
			FLOAT cx1 = (FLOAT)parser.ParseDouble(curveTo[2].c_str(), 0.0);
			FLOAT cy1 = (FLOAT)parser.ParseDouble(curveTo[3].c_str(), 0.0);

			if (curveSize < 6)
			{
				shape->AddPathQuadraticCurve(x, y, cx1, cy1);
			}
			else
			{
				FLOAT cx2 = (FLOAT)parser.ParseDouble(curveTo[4].c_str(), 0.0);
				FLOAT cy2 = (FLOAT)parser.ParseDouble(curveTo[5].c_str(), 0.0);

				shape->AddPathCubicCurve(x, y, cx1, cy1, cx2, cy2);
			}

			currentPoint = D2D1::Point2F(x, y);
		}
		else if (CompareAndStrip(type, L"SETNOSTROKE"))
		{
			setNoStroke = parser.ParseInt(type.c_str(), 0) != 0;
			shape->SetPathSegmentFlags(createSegmentFlags(setNoStroke, setRoundJoin));
		}
		else if (CompareAndStrip(type, L"SETROUNDJOIN"))
		{
			setRoundJoin = parser.ParseInt(type.c_str(), 0) != 0;
			shape->SetPathSegmentFlags(createSegmentFlags(setNoStroke, setRoundJoin));
		}
		else if (CompareAndStrip(type, L"CLOSEPATH"))
		{
			open = parser.ParseInt(type.c_str(), 0) == 0;
		}
		else
		{
			LogErrorF(this, L"Invalid Path type: %s", type.c_str());
			error = true;
			break;
		}
	}

	if (error) return false;

	shape->ClosePath(open ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);

	// Set the 'Fill Color' to transparent for open shapes.
	// This can be overridden if an actual 'Fill Color' is defined.
	if (open) shape->SetFill(Gfx::Util::c_Transparent_Color_F);

	m_Shapes.emplace_back(std::move(*shape));

	return true;
}
