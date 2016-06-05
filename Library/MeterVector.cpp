/* Copyright (C) 2002 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "stdafx.h"
#include "MeterVector.h"
#include <sstream>
#include "Logger.h"
#include <algorithm>    // std::remove
#include <cmath>
#define PI	(3.14159265358979323846)

using namespace Gdiplus;

MeterVector::MeterVector(Skin * skin, const WCHAR * name) : Meter(skin, name),
	m_Shapes(),
	m_Images()
{
}

MeterVector::~MeterVector()
{
	m_Images.clear();
	m_Shapes.clear();
}

void MeterVector::Initialize()
{
	Meter::Initialize();
}

bool MeterVector::Update()
{
	return Meter::Update();
}

bool MeterVector::Draw(Gfx::Canvas & canvas)
{

	bool canDraw = Meter::Draw(canvas);
	if (!canDraw) return false;
	int i = 0;
	for (const auto& shape : m_Shapes) {
		if (shape.second.m_ShapeParsed && shape.second.m_ShouldRender) {
			D2D1_POINT_2F centerPoint;
				centerPoint = D2D1::Point2F(shape.second.m_RotationCenter.x + shape.second.m_X, shape.second.m_RotationCenter.y + shape.second.m_Y);
			D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F(
				D2D1::Matrix3x2F::Rotation(shape.second.m_Rotation, centerPoint) *

				D2D1::Matrix3x2F::Skew(shape.second.m_Skew.x, shape.second.m_Skew.y, D2D1::Point2F(shape.second.m_X, shape.second.m_Y)) *
				D2D1::Matrix3x2F::Translation(shape.second.m_Offset) * D2D1::Matrix3x2F::Translation(D2D1::SizeF(GetMeterRectPadding().X, GetMeterRectPadding().Y)) * 
				D2D1::Matrix3x2F::Scale(shape.second.m_Scale, D2D1::Point2F(shape.second.m_X, shape.second.m_Y))
				);
			if (shape.second.m_ImageName.empty()) {
				canvas.DrawGeometry(shape.second, transform);

			}
			else {
				Gdiplus::Bitmap* bitmap = m_Images[shape.second.m_ImageName]->GetImage();

				int imageW = bitmap->GetWidth();
				int imageH = bitmap->GetHeight();
				canvas.DrawMaskedGeometryBitmap(bitmap, shape.second.m_ImageDstRect,
					Gdiplus::Rect(0, 0, imageW, imageH), shape.second.m_ImageRotation, shape.second, transform);
			}
		}

	}

	return true;
}

void MeterVector::ReadOptions(ConfigParser & parser, const WCHAR * section)
{

	Meter::ReadOptions(parser, section);
	std::wstring Shapeidentifier = L"Shape";
	const std::wstring& curShape = parser.ReadString(section, L"Shape", L"");
	size_t ShapeIndex = 1;
	while (!curShape.empty())
	{
		if (m_Shapes.find(Shapeidentifier) == m_Shapes.end())
			m_Shapes[Shapeidentifier] = VectorShape();
		VectorShape& shape = m_Shapes[Shapeidentifier];
		//Force all options to be loaded before the shape is handled, as some options is needed in the shape definition ;)
		bool ShapeFound = false;
		//This might create problems for options using :, should be no problem as long as they don't appear outside a '(, )' or a '[, ]' as i modifed the Tokenize method to exclude these!
		std::vector<std::wstring> Shape = CustomTokenize(curShape.c_str(), L"|:");
		//Store options that depends on other options before being able to load, this simply ensures that the order doesn't matter
		std::map<std::wstring, std::wstring> postProcessInformation;
		for (int id = 0; id < Shape.size(); id++)
		{
			std::wstring currentOption = Shape[id];
			if (Shape.size() <= id + 1) break;
			bool skip = true;
			if (_wcsicmp(currentOption.c_str(), L"Gradient") == 0 || 
				_wcsicmp(currentOption.c_str(), L"Image") == 0	||
				_wcsicmp(currentOption.c_str(), L"Rotation") == 0) { postProcessInformation[currentOption] = Shape[++id];}
			else skip = false;
			
			if (skip) 
				continue;

			if (_wcsicmp(currentOption.c_str(), L"Rectangle") == 0 ||
				_wcsicmp(currentOption.c_str(), L"RoundedRectangle") == 0 ||
				_wcsicmp(currentOption.c_str(), L"Ellipse") == 0 ||
				_wcsicmp(currentOption.c_str(), L"Arc") == 0 ||
				_wcsicmp(currentOption.c_str(), L"Curve") == 0 ||
				_wcsicmp(currentOption.c_str(), L"Pie") == 0 ||
				_wcsicmp(currentOption.c_str(), L"Custom") == 0)
			{
				if (ShapeFound) continue;
				id += 1;
				std::wstring ShapeOptions = Shape[id];;
				if (_wcsicmp(ShapeOptions.c_str(), shape.ShapeOptions.c_str()) != 0 || _wcsicmp(currentOption.c_str(), L"Custom") == 0)
				{
					shape.ShapeType = currentOption;
					shape.ShapeOptions = ShapeOptions;
					ShapeFound = true;
				}
			}
			else if (_wcsicmp(currentOption.c_str(), L"FillColor") == 0)			shape.m_FillColor = parser.ParseColor(Shape[++id].c_str());
			else if (_wcsicmp(currentOption.c_str(), L"OutlineColor") == 0)			shape.m_OutlineColor = parser.ParseColor(Shape[++id].c_str());
			else if (_wcsicmp(currentOption.c_str(), L"OutlineWidth") == 0)			shape.m_OutlineWidth = parser.ParseDouble(Shape[++id].c_str(), 1.0);
			else if (_wcsicmp(currentOption.c_str(), L"Offset") == 0)				shape.m_Offset = ParseSize(Shape[++id].c_str(), parser);
			else if (_wcsicmp(currentOption.c_str(), L"Scale") == 0)				shape.m_Scale = ParseSize(Shape[++id].c_str(), parser, 1);
			else if (_wcsicmp(currentOption.c_str(), L"Skew") == 0)					shape.m_Skew = ParsePoint(Shape[++id].c_str(), parser, 0 );
			else if (_wcsicmp(currentOption.c_str(), L"ConnectEdges") == 0)			shape.m_ConnectEdges = parser.ParseInt(Shape[++id].c_str(), 0) != 0;
			else if (_wcsicmp(currentOption.c_str(), L"CombineWith") == 0)		  { shape.m_CombineWith = Shape[++id]; ShapeFound = true; }
			else if (_wcsicmp(currentOption.c_str(), L"CombineMode") == 0)			shape.m_CombineMode = Shape[++id];
			else if (_wcsicmp(currentOption.c_str(), L"StrokeStyle") == 0)			ParseStrokeStyle(Shape[++id], shape, parser, section);
		}
		
		if (shape.ShapeType == L"" || shape.ShapeOptions == L"") break;
		if (ShapeFound) {
			if (_wcsicmp(shape.ShapeType.c_str(), L"Rectangle") == 0)				ParseRect(shape, parser.ParseRECT(shape.ShapeOptions.c_str()));
			else if (_wcsicmp(shape.ShapeType.c_str(), L"RoundedRectangle") == 0)	ParseRoundedRect(shape, shape.ShapeOptions.c_str(), parser);
			else if (_wcsicmp(shape.ShapeType.c_str(), L"Ellipse") == 0)			ParseEllipse(shape, shape.ShapeOptions.c_str(), parser);
			else if (_wcsicmp(shape.ShapeType.c_str(), L"Pie") == 0)				ParsePie(shape, shape.ShapeOptions.c_str(), parser);
			else if (_wcsicmp(shape.ShapeType.c_str(), L"Arc") == 0)				ParseArc(shape, shape.ShapeOptions.c_str(), parser);
			else if (_wcsicmp(shape.ShapeType.c_str(), L"Curve") == 0)				ParseCurve(shape, shape.ShapeOptions.c_str(), parser);
			else if (_wcsicmp(shape.ShapeType.c_str(), L"Custom") == 0)				ParseCustom(shape, shape.ShapeOptions.c_str(), parser, section);
			D2D1_RECT_F bounds;
			if (shape.m_Geometry) {
				HRESULT hr = shape.m_Geometry->GetBounds(D2D1::Matrix3x2F::Identity(), &bounds);
				if (SUCCEEDED(hr))
				{
					shape.m_X = bounds.left;
					shape.m_Y = bounds.top;
					shape.m_W = bounds.right - bounds.left + shape.m_OutlineWidth / 2;
					shape.m_H = bounds.bottom - bounds.top + shape.m_OutlineWidth / 2;
				}
			}
		}
		//Handle post options
		for (auto& optionPairs : postProcessInformation)
		{
			if (_wcsicmp(optionPairs.first.c_str(), L"Gradient") == 0)			ParseGradient(optionPairs.second, shape, parser, section);
			else if (_wcsicmp(optionPairs.first.c_str(), L"Image") == 0)		ParseImage(optionPairs.second, shape, parser, section);
			else if (_wcsicmp(optionPairs.first.c_str(), L"Rotation") == 0)		shape.m_Rotation = ParseRotation(optionPairs.second.c_str(), parser, shape);
		}
		Shapeidentifier = L"Shape" + std::to_wstring(++ShapeIndex);
		const std::wstring& curShape = parser.ReadString(section, Shapeidentifier.c_str(), L"");
	}
	//Combine shapes post readOptions to make it possible for e.g Shape2 to be combined with Shape
	for (auto& shape : m_Shapes)
	{
		if (shape.second.ShapeType == L"" || shape.second.ShapeOptions == L"" || !shape.second.m_Geometry) break;
		CombineGeometry(shape.second);
		D2D1_RECT_F bounds;
		HRESULT hr = shape.second.m_Geometry->GetBounds(D2D1::Matrix3x2F::Identity(), &bounds);
		if (SUCCEEDED(hr))
		{
			shape.second.m_X = bounds.left;
			shape.second.m_Y = bounds.top;
			shape.second.m_W = bounds.right - bounds.left + shape.second.m_OutlineWidth / 2;
			shape.second.m_H = bounds.bottom - bounds.top + shape.second.m_OutlineWidth / 2;
			if (bounds.right + shape.second.m_OutlineWidth / 2 > Meter::GetW() && !Meter::IsHidden() && !m_WDefined)
				Meter::SetW(bounds.right + shape.second.m_OutlineWidth / 2);
			if (bounds.bottom + shape.second.m_OutlineWidth / 2 > Meter::GetH() && !Meter::IsHidden() && !m_HDefined) {
				Meter::SetH(bounds.bottom + shape.second.m_OutlineWidth / 2);
				m_Skin->SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			}
		}	
	}

	if (m_Initialized && m_Measures.empty() && !m_DynamicVariables)
	{
		Initialize();
	}
}



bool MeterVector::ParseRect(VectorShape& shape, RECT& rect)
{
	D2D1_RECT_F geo_rect;
	geo_rect.left = rect.left + shape.m_OutlineWidth / 2;
	geo_rect.right = rect.right + rect.left;
	geo_rect.top = rect.top + shape.m_OutlineWidth / 2;
	geo_rect.bottom = rect.bottom + rect.top;
	shape.m_Geometry = Gfx::Canvas::CreateRectangle(geo_rect);
	shape.m_ShapeParsed = true;
	return true;
}


bool MeterVector::ParseRoundedRect(VectorShape& shape, LPCWSTR option, ConfigParser& parser)
{

	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 6)
		return false;

	D2D1_ROUNDED_RECT rect;
	rect.rect.left = parser.ParseDouble(Tokens[0].c_str(), 0) + shape.m_OutlineWidth / 2;
	rect.rect.top = parser.ParseDouble(Tokens[1].c_str(), 0) + shape.m_OutlineWidth / 2;
	rect.rect.right = parser.ParseDouble(Tokens[2].c_str(), 0) + rect.rect.left;
	rect.rect.bottom = parser.ParseDouble(Tokens[3].c_str(), 0) + rect.rect.top;
	rect.radiusX = parser.ParseDouble(Tokens[4].c_str(), 5);
	rect.radiusY = parser.ParseDouble(Tokens[5].c_str(), 5);

	shape.m_Geometry = Gfx::Canvas::CreateRoundedRectangle(rect);
	shape.m_ShapeParsed = true;
	return true;
}


bool MeterVector::ParseEllipse(VectorShape& shape, LPCWSTR option, ConfigParser& parser)
{

	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 4)
		return false;

	D2D1_ELLIPSE ellipse;
	ellipse.point.x = parser.ParseDouble(Tokens[0].c_str(), 0) + shape.m_OutlineWidth / 2;
	ellipse.point.y = parser.ParseDouble(Tokens[1].c_str(), 0) + shape.m_OutlineWidth / 2;
	ellipse.radiusX = parser.ParseDouble(Tokens[2].c_str(), 0);
	ellipse.radiusY = parser.ParseDouble(Tokens[3].c_str(), 0);


	shape.m_Geometry = Gfx::Canvas::CreateEllipse(ellipse);
	shape.m_ShapeParsed = true;

	return true;
}
bool MeterVector::ParsePie(VectorShape & shape, LPCWSTR option, ConfigParser & parser)
{
	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 5)
		return false;

	double sx = parser.ParseDouble(Tokens[0].c_str(), 0) + shape.m_OutlineWidth / 2;
	double sy = parser.ParseDouble(Tokens[1].c_str(), 0) + shape.m_OutlineWidth / 2;
	double r = parser.ParseDouble(Tokens[2].c_str(), 0);
	double startAngle = parser.ParseDouble(Tokens[3].c_str(), 0);
	double sweepAngle = parser.ParseDouble(Tokens[4].c_str(), 0);
	if (sweepAngle == 2 * PI)
	{
		D2D1_ELLIPSE ellipse;
		ellipse.point.x = sx;
		ellipse.point.y = sy;
		ellipse.radiusX = r;
		ellipse.radiusY = r;

		shape.m_Geometry = Gfx::Canvas::CreateEllipse(ellipse);
		shape.m_ShapeParsed = true;
		return true;
	}
	std::vector<Gfx::VectorPoint> points;

	//Define start point in center
	points.push_back(Gfx::VectorPoint(sx, sy));

	float p1x = sx + r * std::cos(startAngle);
	float p1y = sy + r * std::sin(startAngle);

	//Draw line to circumference 
	points.push_back(Gfx::VectorPoint(p1x, p1y));

	//Setup arc segment, simple trigonometry
	D2D1_ARC_SEGMENT segment;
	segment.rotationAngle = 0;
	segment.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
	segment.arcSize = D2D1_ARC_SIZE_SMALL;
	segment.point.x = sx + r * std::cos(startAngle + sweepAngle);
	segment.point.y = sy + r * std::sin(startAngle + sweepAngle);
	segment.size.width = r;
	segment.size.height = r;

	//If angle is > 180 degrees, change to large arc to escape weird wrapping
	if (sweepAngle > PI)
		segment.arcSize = D2D1_ARC_SIZE_LARGE;
	//Add arc to pie/sector shape
	points.push_back(Gfx::VectorPoint(segment));

	//Add shape and make edges connect, saves one line segment
	shape.m_Geometry = Gfx::Canvas::CreateCustomGeometry(points, true);
	shape.m_ShapeParsed = true;
	return true;
}
bool MeterVector::ParseArc(VectorShape & shape, LPCWSTR option, ConfigParser & parser)
{
	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() < 4 || Tokens.size() > 9)
		return false;

	D2D1_ARC_SEGMENT segment;
	segment.rotationAngle = 0;
	segment.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
	segment.arcSize = D2D1_ARC_SIZE_SMALL;

	double x1 = parser.ParseDouble(Tokens[0].c_str(), 0) + shape.m_OutlineWidth / 2;
	double y1 = parser.ParseDouble(Tokens[1].c_str(), 0) + shape.m_OutlineWidth / 2;
	double x2 = parser.ParseDouble(Tokens[2].c_str(), 0);
	double y2 = parser.ParseDouble(Tokens[3].c_str(), 0);

	segment.point.x = x2;
	segment.point.y = y2;
	if (Tokens.size() >= 6)
	{
		segment.size.width = parser.ParseDouble(Tokens[4].c_str(), 0);
		segment.size.height = parser.ParseDouble(Tokens[5].c_str(), 0);
	}
	else
	{
		double dx = x2 - x1;
		double dy = y2 - y1;
		double radius = std::sqrtf(dx*dx + dy*dy)/2;
		segment.size.width = radius;
		segment.size.height = radius;
	}
	if (Tokens.size() >= 7) segment.rotationAngle = parser.ParseDouble(Tokens[6].c_str(), 0);
	if (Tokens.size() >= 8)segment.sweepDirection = (D2D1_SWEEP_DIRECTION)(parser.ParseInt(Tokens[7].c_str(), 0) == 0 ? 0 : 1);
	if (Tokens.size() == 9)segment.arcSize = (D2D1_ARC_SIZE)(parser.ParseInt(Tokens[8].c_str(), 0) == 0 ? 0 : 1);
	shape.m_Geometry = Gfx::Canvas::CreateCustomGeometry({ Gfx::VectorPoint(x1,y1), Gfx::VectorPoint(segment)}, shape.m_ConnectEdges);
	shape.m_ShapeParsed = true;
	return true;
}
bool MeterVector::ParseCurve(VectorShape & shape, LPCWSTR option, ConfigParser & parser)
{
	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 8)
		return false;
	double x1 = parser.ParseDouble(Tokens[0].c_str(), 0) + shape.m_OutlineWidth / 2;
	double y1 = parser.ParseDouble(Tokens[1].c_str(), 0) + shape.m_OutlineWidth / 2;
	double x2 = parser.ParseDouble(Tokens[2].c_str(), 0);
	double y2 = parser.ParseDouble(Tokens[3].c_str(), 0);

	D2D1_BEZIER_SEGMENT segment;
	segment.point3.x = x2;
	segment.point3.y = y2;
	segment.point1.x = parser.ParseDouble(Tokens[4].c_str(), 0);
	segment.point1.y = parser.ParseDouble(Tokens[5].c_str(), 0);
	segment.point2.x = parser.ParseDouble(Tokens[6].c_str(), 0);
	segment.point2.y = parser.ParseDouble(Tokens[7].c_str(), 0);

	shape.m_Geometry = Gfx::Canvas::CreateCustomGeometry({ Gfx::VectorPoint(x1,y1), Gfx::VectorPoint(segment) }, shape.m_ConnectEdges);
	shape.m_ShapeParsed = true;

	return true;
}


bool MeterVector::ParseCustom(VectorShape& shape, LPCWSTR option, ConfigParser& parser, const WCHAR * section)
{
	std::wstring ShapeDef = parser.ReadString(section, option, L"");
	std::vector<std::wstring> shapePairs = parser.Tokenize(ShapeDef, L"|:");
	bool first = true;
	if (shapePairs.size() == 0)
		return false;
	std::vector<Gfx::VectorPoint> points;

	double prevDx = 0;
	double prevDy = 0;
	double prevX = 0;
	double prevY = 0;
	D2D1_BEZIER_SEGMENT* prevSegment = NULL;

	//Used to calculate the bezier curves. The reason they get handled here is because they are affected by the next segment, and i need it to calculate the bezier position
	auto bezierParser = [&](double nextX, double nextY, double nextdX, double nextdY)
	{
		if (prevSegment) {

			if (points.size() > 0) {
				double prevX = points[points.size() - 1].m_x;
				double prevY = points[points.size() - 1].m_y;
				prevSegment->point1.x = prevX - prevDx;
				prevSegment->point1.y = prevY - prevDy;
			}

			prevSegment->point2.x = prevX + nextdX;
			prevSegment->point2.y = prevY + nextdY;

			points.push_back(*prevSegment);

			delete prevSegment;
			prevSegment = NULL;
		}
	};

	for (int id = 0; id < shapePairs.size() - 1; id++)
	{
		LPCWSTR PathType = shapePairs[id].c_str();
		if (id > shapePairs.size()) continue;
		std::vector<std::wstring> options = parser.Tokenize(shapePairs[++id].c_str(), L",");
		if (options.size() < 2)
			return false;
		double x = parser.ParseDouble(options[0].c_str(), 0) + shape.m_OutlineWidth/2;
		double y = parser.ParseDouble(options[1].c_str(), 0) + shape.m_OutlineWidth/2;
		if (_wcsicmp(PathType, L"Start") == 0)
		{
			if (first) {
				shape.m_X = x;
				shape.m_Y = y;
				points.push_back(Gfx::VectorPoint(x, y));
			}
			first = false;
		}
		else if (_wcsicmp(PathType, L"LineTo") == 0)
		{
			if (points.size() > 0)
			{
				double dx = -(x - prevX) / 3*2;
				double dy = -(y - prevY) / 3*2;
				if (prevSegment)
					bezierParser(x,y, dx, dy);
				prevDx = dx;
				prevDy = dy;
			}
			points.push_back(Gfx::VectorPoint(x, y));
		}
		else if (_wcsicmp(PathType, L"ArcTo") == 0)
		{
			if (points.size() > 0)
			{
				float angle = -std::atan2(y - prevY, x - prevX) + PI / 2;
				double movedX = (x - prevX);
				double movedY = (y - prevY);
				double length = std::sqrtf(movedX * movedX + movedY * movedY);
				double dx = -length / 3*2 * std::cos(angle);
				double dy = length / 3*2 * std::sin(angle);
				if (prevSegment)
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
			if (options.size() >= 4)
			{
				segment.size.width = parser.ParseDouble(options[2].c_str(), 0);
				segment.size.height = parser.ParseDouble(options[3].c_str(), 0);
			}
			else
			{
				double dx = x - points[points.size() - 1].m_x;
				double dy = y - points[points.size() - 1].m_y;
				double radius = std::sqrtf(dx*dx + dy*dy) / 2;
				segment.size.width = radius;
				segment.size.height = radius;
			}

			if (options.size() >= 5) segment.rotationAngle = parser.ParseDouble(options[4].c_str(), 0);
			if (options.size() >= 6)segment.sweepDirection = (D2D1_SWEEP_DIRECTION)(parser.ParseInt(options[5].c_str(), 0) == 0 ? 0 : 1);
			if (options.size() == 7)segment.arcSize = (D2D1_ARC_SIZE)(parser.ParseInt(options[6].c_str(), 0) == 0 ? 0 : 1);

			points.push_back(Gfx::VectorPoint(segment));
		}
		else if (_wcsicmp(PathType, L"CurveTo") == 0)
		{
			if (options.size() > 6)
				return false;
			D2D1_BEZIER_SEGMENT* segment = new D2D1_BEZIER_SEGMENT();
			segment->point3.x = x;
			segment->point3.y = y;

			if (options.size() == 6) {
				segment->point1.x = parser.ParseDouble(options[2].c_str(), 0);
				segment->point1.y = parser.ParseDouble(options[3].c_str(), 0);
				segment->point2.x = parser.ParseDouble(options[4].c_str(), 0);
				segment->point2.y = parser.ParseDouble(options[5].c_str(), 0);
				points.push_back(Gfx::VectorPoint(*segment));
				prevDx = segment->point2.x;
				prevDy = segment->point2.y;
			}
			else if (options.size() >= 4)
			{
				D2D1_QUADRATIC_BEZIER_SEGMENT segment2;
				segment->point1.x = parser.ParseDouble(options[2].c_str(), 0);
				segment->point1.y = parser.ParseDouble(options[3].c_str(), 0);
				points.push_back(Gfx::VectorPoint(segment2));

			}
			else 
				if (points.size() > 0)
					if (prevSegment) {
						double dx = -(x - prevX) / 3*2;
						double dy = -(y - prevY) / 3*2;
						if (prevSegment)
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
		else
		{
			if (first) {
				shape.m_X = x;
				shape.m_Y = y;
				points.push_back(Gfx::VectorPoint(0, 0));
			}
			first = false;
		}
		prevX = x;
		prevY = y;
	}

	if (prevSegment)
		bezierParser(prevX, prevY, prevDx, prevDy);

	if (first)
		return false;
	shape.m_Geometry = Gfx::Canvas::CreateCustomGeometry(points, shape.m_ConnectEdges);
	shape.m_ShapeParsed = true;
	return true;
}

bool MeterVector::CombineGeometry(VectorShape & shape)
{
	if (shape.m_CombineWith != L"" && shape.m_CombineMode != L"") {
		std::vector<VectorShape*> shapes;
		double originalWidth = shape.m_W;
		for (const std::wstring with : ConfigParser::Tokenize(shape.m_CombineWith, L","))
			if (m_Shapes.find(with) != m_Shapes.end()) {
				shapes.push_back(&m_Shapes[with]);
				m_Shapes[with].m_ShouldRender = false;
			}
		auto newShape = CombineShapes(shape, shapes);
		if (newShape) {
			shape.m_Geometry = newShape;
		}
	}
	return true;
}

Microsoft::WRL::ComPtr<ID2D1Geometry> MeterVector::CombineShapes(VectorShape& shape1, std::vector<VectorShape*> shapes)
{
	Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeometry = Gfx::Canvas::CreatePathGeometry();
	D2D1_COMBINE_MODE mode;
	if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Union") == 0) {
		mode = D2D1_COMBINE_MODE_UNION;
	}
	else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Intersect") == 0) {
		mode = D2D1_COMBINE_MODE_INTERSECT;
	}
	else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"XOR") == 0) {
		mode = D2D1_COMBINE_MODE_XOR;
	}
	else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Exclude") == 0) {
		mode = D2D1_COMBINE_MODE_EXCLUDE;
	}
	else
	{
		LogErrorF(this, L"CombineMode '%s' is a valid CombineMode!", shape1.m_CombineMode.c_str());
		return NULL;
	}
	Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
	pathGeometry->Open(&sink);
	int first = 0;
	for (const auto shape : shapes)
	{
		shape1.m_Geometry->CombineWithGeometry(shape->m_Geometry.Get(), mode, NULL, sink.Get());
	}
	sink->Close();
	return pathGeometry;
}
/*
** Loads the image from disk, mostly copied from MeterImage. Transformed to store images during runtime
**
*/
TintedImage* MeterVector::LoadVectorImage(const std::wstring imageName, const WCHAR** OptionArray, VectorShape &shape, ConfigParser& parser, const WCHAR* section)
{
	if (m_Images.find(imageName) == m_Images.end()) {
		m_Images[imageName] = std::make_unique<TintedImage>(L"ImageName", nullptr, false, m_Skin);
	}
	m_Images[imageName]->ReadFromArray(parser, OptionArray);
	TintedImage* image = m_Images[imageName].get();
	image->LoadImage(imageName.c_str(), false);

	if (image->IsLoaded())
	{
		Gdiplus::Bitmap* bitmap = image->GetImage();
		shape.m_ImageW = bitmap->GetWidth();
		shape.m_ImageH = bitmap->GetHeight();
		return m_Images[imageName].get();
	}
	return nullptr;
}


void MeterVector::ParseImage(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section)
{
	std::vector<std::wstring> optionTokens = CustomTokenize(options.c_str(), L",");
	if (optionTokens.size() <= 0) return;

	shape.m_ImageDstRect = Gdiplus::Rect(shape.m_X, shape.m_Y, shape.m_W, shape.m_H);

	int next = 2;
	if (optionTokens.size() >= next + 1) shape.m_ImageDstRect.X = parser.ParseDouble(optionTokens[next++].c_str(), shape.m_X);
	if (optionTokens.size() >= next + 1) shape.m_ImageDstRect.Y = parser.ParseDouble(optionTokens[next++].c_str(), shape.m_Y);
	if (optionTokens.size() >= next + 1) shape.m_ImageDstRect.Width = parser.ParseDouble(optionTokens[next++].c_str(), shape.m_W);
	if (optionTokens.size() >= next + 1) shape.m_ImageDstRect.Height = parser.ParseDouble(optionTokens[next++].c_str(), shape.m_H);

	std::wstring imageOptions = parser.ReadString(section, options.c_str(), L"");
	std::vector<std::wstring> imageOptionPairs = CustomTokenize(imageOptions, L"|:");
	std::wstring imagePath = L"";
	for (int id = 0; id < imageOptionPairs.size(); id++) {
		std::wstring& currentOption = imageOptionPairs[id];
		if (imageOptionPairs.size() <= id + 1) break;
		if (_wcsicmp(currentOption.c_str(), L"Alpha") == 0) { c_CusomOptionArray[TintedImage::OptionIndexImageAlpha] = imageOptionPairs[++id].c_str(); }
		else if (_wcsicmp(currentOption.c_str(), L"Path") == 0) {
			if (imageOptionPairs.size() <= id + 2) break;
			imagePath = imageOptionPairs[id + 1] + L":" + imageOptionPairs[id + 2];
			id += 2;
		}
		else if (_wcsicmp(currentOption.c_str(), L"Tint") == 0) c_CusomOptionArray[TintedImage::OptionIndexImageTint] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"Rotation") == 0) { shape.m_ImageRotation = parser.ParseDouble(imageOptionPairs[++id].c_str(), 0); }
		else if (_wcsicmp(currentOption.c_str(), L"Flip") == 0) c_CusomOptionArray[TintedImage::OptionIndexImageFlip] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"Crop") == 0) c_CusomOptionArray[TintedImage::OptionIndexImageCrop] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"Grayscale") == 0) c_CusomOptionArray[TintedImage::OptionIndexGreyscale] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"UseExifOrientation") == 0) c_CusomOptionArray[TintedImage::OptionIndexUseExifOrientation] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"ColorMatrix1") == 0) c_CusomOptionArray[TintedImage::OptionIndexColorMatrix1] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"ColorMatrix2") == 0) c_CusomOptionArray[TintedImage::OptionIndexColorMatrix2] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"ColorMatrix3") == 0) c_CusomOptionArray[TintedImage::OptionIndexColorMatrix3] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"ColorMatrix4") == 0) c_CusomOptionArray[TintedImage::OptionIndexColorMatrix4] = imageOptionPairs[++id].c_str();
		else if (_wcsicmp(currentOption.c_str(), L"ColorMatrix5") == 0) c_CusomOptionArray[TintedImage::OptionIndexColorMatrix5] = imageOptionPairs[++id].c_str();
	}

	if (_wcsicmp(imagePath.c_str(), L"") != 0) {
		TintedImage* image = LoadVectorImage(imagePath, c_CusomOptionArray, shape, parser, section);
		if (image)
			shape.m_ImageName = imagePath;
	}
}
void MeterVector::ParseGradient(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section)
{
	std::vector<std::wstring> GradientOptions = CustomTokenize(options, L",");
	if (GradientOptions.size() < 1) return;
	std::wstring strokeType = GradientOptions[0];
	if (_wcsicmp(strokeType.c_str(), L"Linear") == 0) shape.m_BrushType = shape.Linear;
	else if (_wcsicmp(strokeType.c_str(), L"Radial") == 0) shape.m_BrushType = shape.Radial;
	int nextIt = 0;
	std::wstring GradientStops = L"";
	if (GradientOptions.size() < nextIt + 2) return;
	GradientStops = GradientOptions[++nextIt]; if (GradientStops == L"") return;

	shape.m_GradientStops.clear();
	std::wstring GradientStopsOption = parser.ReadString(section, GradientStops.c_str(), L"");
	std::vector<std::wstring> stops = CustomTokenize(GradientStopsOption, L"|;");
	bool foundzero = false;
	for (int id = 0; id < stops.size(); id++) {
		std::wstring& currentOption = stops[id];
		if (stops.size() < id + 2) break; //Partial gradient definition, or something else went wrong
		double position = parser.ParseDouble(stops[++id].c_str(), -1);
		if (position == 0) foundzero = true;
		Gdiplus::Color color = parser.ParseColor(currentOption.c_str());
		shape.m_GradientStops.push_back(D2D1::GradientStop(position, D2D1::ColorF(color.GetR() / 255.0, color.GetG() / 255.0, color.GetB() / 255.0, color.GetA() / 255.0)));
	}

	if (GradientOptions.size() < nextIt + 3) return;
	D2D1_POINT_2F* point;
	D2D1_POINT_2F point1 = ParsePoint((GradientOptions[nextIt+1] + L"," + GradientOptions[nextIt+2]).c_str(), parser, 0);
	nextIt += 2;
	if (shape.m_BrushType == shape.Linear) point = &shape.m_GradientProperties.m_LinearProperties.startPoint;
	else if (shape.m_BrushType == shape.Radial) point = &shape.m_GradientProperties.m_RadialProperties.center;
	point->x = shape.m_X + shape.m_W * point1.x;
	point->y = shape.m_Y + shape.m_H * point1.y;

	if (GradientOptions.size() < nextIt + 3) return;
	D2D1_POINT_2F point2 = ParsePoint((GradientOptions[nextIt+1] + L"," + GradientOptions[nextIt+2]).c_str(), parser, 0);
	nextIt += 2;
	if (shape.m_BrushType == shape.Linear) point = &shape.m_GradientProperties.m_LinearProperties.endPoint;
	else if (shape.m_BrushType == shape.Radial) point = &shape.m_GradientProperties.m_RadialProperties.gradientOriginOffset;
	point->x = shape.m_W * point2.x;
	point->y = shape.m_H * point2.y;
	if (shape.m_BrushType == shape.Linear)
	{
		point->x += shape.m_X;
		point->y += shape.m_Y;
	}
	if (shape.m_BrushType == shape.Radial) {
		if (GradientOptions.size() < nextIt + 3) return;
		D2D1_POINT_2F radius = ParsePoint((GradientOptions[++nextIt] + L"," + GradientOptions[++nextIt]).c_str(), parser, 0);
		shape.m_GradientProperties.m_RadialProperties.radiusX = radius.x;
		shape.m_GradientProperties.m_RadialProperties.radiusY = radius.y;
	}
}

void MeterVector::ParseStrokeStyle(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section)
{
	std::vector<std::wstring> StrokeOptions = CustomTokenize(options, L",");
	if (StrokeOptions.size() < 1) return;
	std::wstring strokeType = StrokeOptions[0];
	if (_wcsicmp(strokeType.c_str(), L"Solid") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_SOLID;
	else if (_wcsicmp(strokeType.c_str(), L"Dash") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_DASH;
	else if (_wcsicmp(strokeType.c_str(), L"Dot") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_DOT;
	else if (_wcsicmp(strokeType.c_str(), L"DashDot") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_DASH_DOT;
	else if (_wcsicmp(strokeType.c_str(), L"DashDotDot") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_DASH_DOT_DOT;
	else if (_wcsicmp(strokeType.c_str(), L"Custom") == 0) shape.m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
	shape.m_UseDashes = true;
	int nextIt = 0;
	std::wstring CustomDashes = L"";
	if (StrokeOptions.size() < nextIt + 2) return;
	if (shape.m_StrokeProperties.dashStyle == D2D1_DASH_STYLE_CUSTOM) {
		CustomDashes = StrokeOptions[++nextIt]; if (CustomDashes == L"") return;

		shape.m_Dashes.clear();
		std::wstring customDashOption = parser.ReadString(section, CustomDashes.c_str(), L"");
		std::vector<std::wstring> dashes = CustomTokenize(customDashOption, L"|");
		for (int id = 0; id < dashes.size(); id++)
			shape.m_Dashes.push_back(parser.ParseDouble(dashes[id].c_str(), 0));
	}
	std::wstring option;
	D2D1_CAP_STYLE* properties[3];
	properties[0] = &shape.m_StrokeProperties.startCap;
	properties[1] = &shape.m_StrokeProperties.endCap;
	properties[2] = &shape.m_StrokeProperties.dashCap;
	for (int i = 0; i < 3; i++) {
		if (StrokeOptions.size() < nextIt + 2) return;
		option = StrokeOptions[++nextIt];
		if (_wcsicmp(option.c_str(), L"Flat") == 0) *properties[1] = D2D1_CAP_STYLE_FLAT;
		else if (_wcsicmp(option.c_str(), L"Square") == 0) *properties[1] = D2D1_CAP_STYLE_SQUARE;
		else if (_wcsicmp(option.c_str(), L"Round") == 0) *properties[1] = D2D1_CAP_STYLE_ROUND;
		else if (_wcsicmp(option.c_str(), L"Triangle") == 0) *properties[1] = D2D1_CAP_STYLE_TRIANGLE;
	}
	if (_wcsicmp(option.c_str(), L"Miter") == 0) shape.m_StrokeProperties.lineJoin = D2D1_LINE_JOIN_MITER;
	else if (_wcsicmp(option.c_str(), L"Bevel") == 0) shape.m_StrokeProperties.lineJoin = D2D1_LINE_JOIN_BEVEL;
	else if (_wcsicmp(option.c_str(), L"Round") == 0) shape.m_StrokeProperties.lineJoin = D2D1_LINE_JOIN_ROUND;
	else if (_wcsicmp(option.c_str(), L"MiterOrBevel") == 0) shape.m_StrokeProperties.lineJoin = D2D1_LINE_JOIN_MITER_OR_BEVEL;
	if (StrokeOptions.size() < nextIt + 2) return;
	option = StrokeOptions[++nextIt];
	shape.m_StrokeProperties.miterLimit = parser.ParseDouble(option.c_str(), 1.0f);
	if (StrokeOptions.size() < nextIt + 2) return;
	option = StrokeOptions[++nextIt];
	shape.m_StrokeProperties.dashOffset = parser.ParseDouble(option.c_str(), 5.0f);


}

double MeterVector::ParseRotation(const LPCWSTR& string, ConfigParser& parser, VectorShape& shape)
{
	std::vector<std::wstring> tokens = CustomTokenize(string, L",");
	double rotation = 0;
	if (tokens.size() > 0)  rotation = parser.ParseDouble(tokens[0].c_str(), 0);
	if (tokens.size() > 2) {
		shape.m_RotationCenter = ParsePoint((tokens[1] + L"," + tokens[2]).c_str(), parser, FLT_MIN);
		if (shape.m_RotationCenter.x == FLT_MIN) shape.m_RotationCenter.x = shape.m_W/2;
		if (shape.m_RotationCenter.y == FLT_MIN) shape.m_RotationCenter.y = shape.m_H/2;
	}
	else {
		shape.m_RotationCenter.x = shape.m_W/2;
		shape.m_RotationCenter.y = shape.m_H/2;
	}
	return rotation;
}
/*
** Overridden method. The Vector meters need not to be bound on anything
**
*/
void MeterVector::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

//Here are the functions that could be moved to ConfigParser:
std::vector<std::wstring> MeterVector::CustomTokenize(const std::wstring& str, const std::wstring& delimiters)
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

D2D1_POINT_2F MeterVector::ParsePoint(const LPCWSTR& string, ConfigParser& parser, double defaultVal)
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
D2D1_SIZE_F MeterVector::ParseSize(const LPCWSTR& string, ConfigParser& parser, double defaultVal)
{
	D2D1_POINT_2F point = ParsePoint(string, parser, defaultVal);
	return D2D1::SizeF(point.x, point.y);
}