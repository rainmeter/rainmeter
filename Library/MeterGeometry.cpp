/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterGeometry.h"
#include "Logger.h"
#define PI	(3.14159265358979323846)

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

bool IsSegment(const WCHAR* option)
{
	return	_wcsicmp(option, L"Start") == 0 ||
		_wcsicmp(option, L"LineTo") == 0 ||
		_wcsicmp(option, L"ArcTo") == 0 ||
		_wcsicmp(option, L"CurveTo") == 0;
}

MeterGeometry::MeterGeometry(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Shapes(),
	m_MeasureModifiers(),
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

bool MeterGeometry::ReplaceModifierDef(std::wstring& option, ConfigParser& parser, const WCHAR* section)
{
	bool ModifierReplaced = false;
	size_t atLoc = option.find('@');
	while (atLoc != std::string::npos)
	{
		size_t end = option.find_first_of(L",|: ", atLoc);
		auto optionName = option.substr(atLoc+1, end - atLoc - 1);
		auto Option = parser.ReadString(section, optionName.c_str(), L"");
		if (Option != L"")
		{
			option.replace(atLoc, end - atLoc, Option.c_str());
			ModifierReplaced = true;
		}
		atLoc = option.find('@', atLoc+1);
	}
	return ModifierReplaced;
}

template <typename F>
bool MeterGeometry::MergeShapeTokens(std::wstring& endToken, const std::vector<std::wstring>& tokenArray, int & tokenId, F& tokenChecker)
{
	std::wstring fullToken = L"";
	std::wstring tokenValue;
	std::wstring tokenIdentifier = endToken;
	bool isToken = true;
	bool ChangedToken = false;
	while (isToken) {
		std::wstring segmentType;
		auto pos = tokenIdentifier.find_first_of(L":");
		if (pos != std::string::npos)
		{
			segmentType = tokenIdentifier.substr(0, pos);
			tokenValue = tokenIdentifier.substr(pos + 1);
		}
		else
		{
			segmentType = tokenIdentifier;
		}
		if (tokenChecker(segmentType.c_str()))
		{
			if (!fullToken.empty())
			{
				fullToken += L"|";
			}
			fullToken += segmentType + L":" + tokenValue;
			ChangedToken = true;
		}
		else
		{
			isToken = false;
		}
		tokenId += 1;
		if (tokenArray.size() > tokenId) {
			size_t stringPos = tokenArray[tokenId].find_first_of(L':');
			tokenIdentifier = tokenArray[tokenId].substr(0, stringPos);
			tokenValue = tokenArray[tokenId].substr(stringPos + 1);
		}
		else
		{
			break;
		}
	}
	if (ChangedToken) {
		endToken = fullToken;
		tokenId -= 2;
	}
	return ChangedToken;
}

bool MeterGeometry::ParseShape(GeometryShape& shape, const WCHAR* shapeName, const WCHAR* shapeParameters)
{
	if (_wcsicmp(shapeName, L"Rectangle") == 0)				shape.m_Shape = ParseRectangle(shape, ConfigParser::ParseRECT(shapeParameters));
	else if (_wcsicmp(shapeName, L"RoundedRectangle") == 0)	shape.m_Shape = ParseRoundedRectangle(shape, shapeParameters);
	else if (_wcsicmp(shapeName, L"Custom") == 0)			shape.m_Shape = ParseCustom(shape, shapeParameters);
	else {
		return false;
	}
	return shape.m_Shape;
}

void MeterGeometry::UpdateSize(GeometryShape& shape)
{
	if (shape.m_Shape) {
		D2D1_RECT_F& untransformedBounds = shape.m_UntransformedBounds;
		shape.m_Shape->GetWidenedBounds(shape.m_OutlineWidth, NULL, D2D1::IdentityMatrix(), &untransformedBounds);
		shape.m_Shape->GetWidenedBounds(shape.m_OutlineWidth, NULL, GetShapeMatrix(shape, &untransformedBounds), &shape.m_Bounds);
		if ((!m_X > shape.m_Bounds.left || m_X == 0))
		{
			m_X = shape.m_Bounds.left;
		}
		if ((!m_Y > shape.m_Bounds.top || m_Y == 0))
		{
			m_Y = shape.m_Bounds.top;
		}
		auto shapeWidth = std::ceil(shape.m_Bounds.right - m_X + GetWidthPadding());
		if (!m_WDefined && m_W < shapeWidth) {
			m_W = shapeWidth;
		}
		auto shapeHeight = std::ceil(shape.m_Bounds.bottom - m_Y + GetHeightPadding());
		if (!m_HDefined && m_H < shapeHeight) {
			m_H = shapeHeight;
		}
	}
}

D2D1_MATRIX_3X2_F MeterGeometry::GetShapeMatrix(const GeometryShape& shape, const D2D1_RECT_F* untransformedBounds)
{
	if (untransformedBounds) {
		D2D1_POINT_2F center = D2D1::Point2F((untransformedBounds->right - untransformedBounds->left) / 2 + untransformedBounds->left, (untransformedBounds->bottom - untransformedBounds->top) / 2 + untransformedBounds->top);
		return D2D1::Matrix3x2F(
			D2D1::Matrix3x2F::Translation(D2D1::SizeF(GetMeterRectPadding().X - m_X, GetMeterRectPadding().Y - m_Y)) *
			D2D1::Matrix3x2F::Rotation(shape.m_Rotation, center) *
			D2D1::Matrix3x2F::Skew(shape.m_Skew.x, shape.m_Skew.y, D2D1::Point2F(untransformedBounds->left, untransformedBounds->top)) *
			D2D1::Matrix3x2F::Translation(shape.m_Offset) *
			D2D1::Matrix3x2F::Scale(shape.m_Scale, center)
			);
	}
}

bool MeterGeometry::ReplaceShapeModifiers(GeometryShape& shape, const WCHAR* modifierName, const WCHAR* modifierValue)
{
	if (_wcsicmp(modifierName, L"FillColor") == 0)			shape.m_FillColor = ToColorF(ConfigParser::ParseColor(modifierValue));
	else if (_wcsicmp(modifierName, L"OutlineWidth") == 0)	shape.m_OutlineWidth = (float)ConfigParser::ParseDouble(modifierValue, 1.0f);
	else if (_wcsicmp(modifierName, L"OutlineColor") == 0)	shape.m_OutlineColor = ToColorF(ConfigParser::ParseColor(modifierValue));
	else if (_wcsicmp(modifierName, L"Offset") == 0)		shape.m_Offset = ParseSize(modifierValue, 0);
	else if (_wcsicmp(modifierName, L"Scale") == 0)			shape.m_Scale = ParseSize(modifierValue, 1);
	else if (_wcsicmp(modifierName, L"Skew") == 0)			shape.m_Skew = ParsePoint(modifierValue, 0);
	else if (_wcsicmp(modifierName, L"Rotation") == 0)		shape.m_Rotation = ParseRotation(modifierValue, 0, shape);
	else if (_wcsicmp(modifierName, L"Antialias") == 0)		shape.m_Antialias = ConfigParser::ParseInt(modifierValue, 1) == 1;
	else 
		return false;
	return true;
}

bool MeterGeometry::IsPostOption(const WCHAR* option)
{
	return _wcsicmp(option, L"Rotation") == 0;
}

bool MeterGeometry::IsShape(const WCHAR* option)
{
	return	_wcsicmp(option, L"Rectangle") == 0			||
			_wcsicmp(option, L"RoundedRectangle") == 0   ||
			_wcsicmp(option, L"Custom") == 0;
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
	m_MeasureModifiers.clear();
	size_t currentShapeId = 1;
	std::wstring shapeName = L"Shape";
	std::wstring shapeOption = parser.ReadString(section, L"Shape", L"");
	std::map<std::wstring, std::pair<std::wstring, std::wstring>> postModifiers;
	while (!shapeOption.empty()) {
		ReplaceModifierDef(shapeOption, parser, section);
		std::vector<std::wstring> shapeTokens = CustomTokenize(shapeOption.c_str(), L"|");
		m_Shapes.insert(std::pair<std::wstring, GeometryShape>(shapeName.c_str(), GeometryShape()));
		bool usingMeasures = false;

		std::wstring shapeType = L"", shapeParameters = L"";

		for (int optionId = 0; optionId < shapeTokens.size(); optionId++)
		{
			auto stringPos = shapeTokens[optionId].find_first_of(L':');
			auto optionName = shapeTokens[optionId].substr(0, stringPos);
			auto optionValue = shapeTokens[optionId].substr(stringPos + 1);

			if (_wcsicmp(optionName.c_str(), L"Custom") == 0)
			MergeShapeTokens(optionValue, shapeTokens, optionId, IsSegment);

			if (!m_Measures.empty())
				usingMeasures = ContainsMeasures(optionValue);

			//Store options to replace in update
			if (!usingMeasures) {
				if (!IsShape(optionName.c_str())) {
					if (!IsPostOption(optionName.c_str())) {
						if (!ReplaceShapeModifiers(m_Shapes[shapeName.c_str()], optionName.c_str(), optionValue.c_str()))
							break;
					}
					else {
						postModifiers[shapeName.c_str()] = std::pair<std::wstring, std::wstring>(optionName, optionValue);
					}
				}
				else
				{
						shapeType = optionName;
						shapeParameters = optionValue;
				}
			}
			else
				m_MeasureModifiers[shapeName.c_str()].push_back(std::pair<std::wstring, std::wstring>(optionName, optionValue));
			
		}
		if (shapeType.empty() || shapeParameters.empty())
			break; 
			if (!ParseShape(m_Shapes[shapeName.c_str()], shapeType.c_str(), shapeParameters.c_str()))
				break;

		shapeName = L"Shape" + std::to_wstring(++currentShapeId);
		shapeOption = parser.ReadString(section, shapeName.c_str(), L"");
	}

	for (const auto& shape : postModifiers)
	{
		const auto& modifier = shape.second;
		if (!ReplaceShapeModifiers(m_Shapes[shape.first], modifier.first.c_str(), modifier.second.c_str()))
			continue;
	}

	for (auto& shape : m_Shapes)
	{
		UpdateSize(shape.second);
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
			for (const auto& shape : m_MeasureModifiers)
			{
				for (const auto& modifier : shape.second) {
					std::wstring modifierValue = modifier.second;
					bool replaced = ReplaceMeasures(modifierValue, AUTOSCALE::AUTOSCALE_OFF, 1.0, 6, false);
					if (replaced)
						if (!IsShape(modifier.first.c_str())) {
							if (!ReplaceShapeModifiers(m_Shapes[shape.first], modifier.first.c_str(), modifierValue.c_str()))
								continue;
						}
						else
							if (!ParseShape(m_Shapes[shape.first], modifier.first.c_str(), modifierValue.c_str()))
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
			const auto transform = GetShapeMatrix(shape, &shape.m_UntransformedBounds);
			canvas.DrawGeometry(shape, transform, shape.m_Antialias);
		}
	}
	return true;
}
bool MeterGeometry::HitTest(int x, int y)
{
	bool hit = false;
	for (const auto& shape : m_Shapes)
	{
		auto matrix = GetShapeMatrix(shape.second, &shape.second.m_UntransformedBounds);
		BOOL contains = false;
		if (shape.second.m_FillColor.a != 0) {
			HRESULT result = shape.second.m_Shape->FillContainsPoint(D2D1::Point2F(x, y), matrix, &contains);
			if (SUCCEEDED(result))
			{
				hit = hit || contains;
			}
		}
		HRESULT result = shape.second.m_Shape->StrokeContainsPoint(D2D1::Point2F(x, y), shape.second.m_OutlineWidth, NULL, matrix, &contains);

		if (SUCCEEDED(result))
		{
			hit = hit || contains;
		}
	}
	return hit;
}
void MeterGeometry::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterGeometry::ParseRectangle(GeometryShape& shape, RECT& rect)
{
	D2D1_RECT_F geo_rect = D2D1::RectF(rect.left, rect.top, rect.right + rect.left, rect.bottom + rect.top);
	return Gfx::Canvas::CreateRectangle(geo_rect);
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterGeometry::ParseRoundedRectangle(GeometryShape& shape, const WCHAR* parameters)
{
	std::vector<std::wstring> Tokens = CustomTokenize(parameters, L",");
	if (Tokens.size() != 6)
		return false;

	D2D1_ROUNDED_RECT rect;
	rect.rect.left = ConfigParser::ParseDouble(Tokens[0].c_str(), 0);
	rect.rect.top = ConfigParser::ParseDouble(Tokens[1].c_str(), 0);
	rect.rect.right = ConfigParser::ParseDouble(Tokens[2].c_str(), 0) + rect.rect.left;
	rect.rect.bottom = ConfigParser::ParseDouble(Tokens[3].c_str(), 0) + rect.rect.top;
	rect.radiusX = ConfigParser::ParseDouble(Tokens[4].c_str(), 5);
	rect.radiusY = ConfigParser::ParseDouble(Tokens[5].c_str(), 5);

	return Gfx::Canvas::CreateRoundedRectangle(rect);
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterGeometry::ParseCustom(GeometryShape& shape, const WCHAR* parameters)
{
	std::vector<std::wstring> shapeParamters = CustomTokenize(parameters, L"|:");
	bool first = true;
	if (shapeParamters.size() == 0)
		return false;
	std::vector<Gfx::ShapePoint> points;

	double prevDx = 0;
	double prevDy = 0;
	double prevX = 0;
	double prevY = 0;
	double prevprevX = 0;
	double prevprevY = 0;

	D2D1_BEZIER_SEGMENT* prevSegment = NULL;

	auto bezierParser = [&](double nextX, double nextY, double nextdX, double nextdY)
	{
		if (prevSegment) {
			prevSegment->point1.x = prevprevX + prevDx;
			prevSegment->point1.y = prevprevY + prevDy;
			prevSegment->point2.x = prevX - nextdX;
			prevSegment->point2.y = prevY - nextdY;
			points.push_back(*prevSegment);

			delete prevSegment;
			prevSegment = NULL;
		}
	};

	for (int id = 0; id < shapeParamters.size() - 1; id++)
	{
		LPCWSTR PathType = shapeParamters[id].c_str();
		if (id > shapeParamters.size()) continue;
		std::vector<std::wstring> parameters = CustomTokenize(shapeParamters[++id].c_str(), L",");
		if (parameters.size() < 2)
			return false;
		double x = ConfigParser::ParseDouble(parameters[0].c_str(), 0);
		double y = ConfigParser::ParseDouble(parameters[1].c_str(), 0);
		double dx = (x - prevX) / 3;
		double dy = (y - prevY) / 3;

		if (_wcsicmp(PathType, L"Start") == 0)
		{
			if (first) {
				points.push_back(Gfx::ShapePoint(x, y));
			}
			first = false;
		}
		else if (_wcsicmp(PathType, L"LineTo") == 0)
		{
			if (points.size() > 0)
			{
				bezierParser(x, y, dx, dy);
				prevDx = dx;
				prevDy = dy;
			}
			points.push_back(Gfx::ShapePoint(x, y));
		}
		else if (_wcsicmp(PathType, L"ArcTo") == 0)
		{
			if (points.size() > 0)
			{
				float angle = -std::atan2(y - prevY, x - prevX) + PI / 2;
				double movedX = x - prevX;
				double movedY = y - prevY;
				double length = std::sqrtf(movedX * movedX + movedY * movedY);
				dx = length / 3 * std::cos(angle);
				dy = -length / 3 * std::sin(angle);
				bezierParser(x, y, dx, dy);
				prevDx = -dx;
				prevDy = -dy;
			}

			D2D1_ARC_SEGMENT segment;
			segment.rotationAngle = 0;
			segment.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
			segment.arcSize = D2D1_ARC_SIZE_SMALL;
			segment.point.x = x;
			segment.point.y = y;

			if (parameters.size() >= 4)
			{
				segment.size.width = ConfigParser::ParseDouble(parameters[2].c_str(), 0);
				segment.size.height = ConfigParser::ParseDouble(parameters[3].c_str(), 0);
			}
			else
			{
				double dx = x - prevX;
				double dy = y - prevY;
				double radius = std::sqrtf(dx*dx + dy*dy) / 2;
				segment.size.width = radius;
				segment.size.height = radius;
			}

			if (parameters.size() >= 5) segment.rotationAngle = ConfigParser::ParseDouble(parameters[4].c_str(), 0);
			if (parameters.size() >= 6)segment.sweepDirection = (D2D1_SWEEP_DIRECTION)(ConfigParser::ParseInt(parameters[5].c_str(), 0) == 0 ? 0 : 1);
			if (parameters.size() == 7)segment.arcSize = (D2D1_ARC_SIZE)(ConfigParser::ParseInt(parameters[6].c_str(), 0) == 0 ? 0 : 1);

			points.push_back(Gfx::ShapePoint(segment));
		}
		else if (_wcsicmp(PathType, L"CurveTo") == 0)
		{
			if (parameters.size() > 6)
				return false;
			D2D1_BEZIER_SEGMENT* segment = new D2D1_BEZIER_SEGMENT();
			segment->point3.x = x;
			segment->point3.y = y;

			if (parameters.size() == 6) {
				segment->point1.x = ConfigParser::ParseDouble(parameters[2].c_str(), 0);
				segment->point1.y = ConfigParser::ParseDouble(parameters[3].c_str(), 0);
				segment->point2.x = ConfigParser::ParseDouble(parameters[4].c_str(), 0);
				segment->point2.y = ConfigParser::ParseDouble(parameters[5].c_str(), 0);
				points.push_back(Gfx::ShapePoint(*segment));
				prevDx = segment->point2.x;
				prevDy = segment->point2.y;
			}
			else if (parameters.size() >= 4)
			{
				D2D1_QUADRATIC_BEZIER_SEGMENT segment2;
				segment->point1.x = ConfigParser::ParseDouble(parameters[2].c_str(), 0);
				segment->point1.y = ConfigParser::ParseDouble(parameters[3].c_str(), 0);
				points.push_back(Gfx::ShapePoint(segment2));
			}
			else {
				bezierParser(x, y, dx, dy);
				prevDx = dx;
				prevDy = dy;
			}

			if (prevSegment) {
				delete prevSegment;
				prevSegment = NULL;
			}
			prevSegment = segment;
		}
		if (first) {
			bezierParser(x, y, dx, dy);
			prevDx = dx;
			prevDy = dy;
			points.push_back(Gfx::ShapePoint(0, 0));
		}
		first = false;
		prevprevX = prevX;
		prevprevY = prevY;
		prevX = x;
		prevY = y;
	}
	bezierParser(prevX, prevY, prevDx, prevDy);

	if (first)
		return NULL;

	return Gfx::Canvas::CreateCustomGeometry(points, true);
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
			if (token.find(L'(') != std::wstring::npos || token.find(L'[') != std::wstring::npos)
			{
				//Add the next token while the order of ( and ), and [ and ] are still uneven 
				while (std::count(token.begin(), token.end(), L'(') != std::count(token.begin(), token.end(), L')') &&
					std::count(token.begin(), token.end(), L'[') != std::count(token.begin(), token.end(), L']') &&
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
D2D1_SIZE_F MeterGeometry::ParseSize(const WCHAR* string, double defaultVal)
{
	D2D1_POINT_2F point = ParsePoint(string, defaultVal);
	return D2D1::SizeF(point.x, point.y);
}

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
