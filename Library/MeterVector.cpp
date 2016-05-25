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

using namespace Gdiplus;

MeterVector::MeterVector(Skin * skin, const WCHAR * name) : Meter(skin, name),
	m_Shapes(),
	m_Skin(skin)
{
}

MeterVector::~MeterVector()
{
	for (auto& item : m_Images)
		delete item.second;
}

void MeterVector::Initialize()
{
	Meter::Initialize();
}

bool MeterVector::Update()
{
	if (Meter::Update())
	{

		return true;
	}
	return false;
}

bool MeterVector::Draw(Gfx::Canvas & canvas)
{

	bool canDraw = Meter::Draw(canvas);
	if (!canDraw) return false;
	int i = 0;
	for (const auto& shape : m_Shapes) {
		if(shape.second.m_ShapeParsed)
			if (!shape.second.m_Image) {
				canvas.DrawGeometry(shape.second.m_Geometry.Get(), shape.second.m_FillColor, shape.second.m_OutlineColor,
					shape.second.m_OutlineWidth, shape.second.m_Rotation, D2D1::Point2F(shape.second.m_X + (shape.second.m_W) / 2, shape.second.m_Y + (shape.second.m_H) / 2));
			}
			else {
				Gdiplus::Bitmap* bitmap = shape.second.m_Image->GetImage();

				int imageW = bitmap->GetWidth();
				int imageH = bitmap->GetHeight();
				canvas.DrawMaskedGeometryBitmap(shape.second.m_Image->GetImage(), Gdiplus::Rect(0, 0, imageW, imageH), Gdiplus::Rect(0, 0, imageW, imageH), shape.second.m_Geometry.Get(), shape.second.m_FillColor, shape.second.m_OutlineColor,
					shape.second.m_OutlineWidth, shape.second.m_Rotation, D2D1::Point2F(shape.second.m_X + (shape.second.m_W) / 2, shape.second.m_Y + (shape.second.m_H) / 2));
			}
				//canvas.DrawCustomGeometry(D2D1::Point2F(20, 20), shape.m_Points, shape.m_FillColor, shape.m_OutlineColor, shape.m_OutlineWidth, true);

			

		
	}
	//canvas.DrawPathGeometry(m_points, solid, m_outlineColor, m_Solid, m_lineWidth, m_connectedEdges);

	return true;
}

void MeterVector::ReadOptions(ConfigParser & parser, const WCHAR * section)
{


	Meter::ReadOptions(parser, section);
		std::wstring Shapeidentifier = L"Shape";
		const std::wstring& curShape = parser.ReadString(section, Shapeidentifier.c_str(), L"");
		std::vector<std::wstring> Shape = parser.Tokenize(curShape.c_str(), L"|");
		size_t ShapeIndex = 1;
		;
			while (!curShape.empty())
			{

				VectorShape shape;
				//Force all options to be loaded before the shape is handled, as some options is needed in the shape ;)
				std::wstring ShapeType = L"";
				std::wstring ShapeOptions = L"";
				bool ShapeFound = false;

				Shape = parser.Tokenize(curShape.c_str(), L"|:");

				for (int id = 0; id < Shape.size(); id++)
				{
					std::wstring& currentOption = Shape[id];

					if (!ShapeFound)
					{
						if (_wcsicmp(currentOption.c_str(), L"Rectangle") == 0 ||
							_wcsicmp(currentOption.c_str(), L"RoundedRectangle") == 0 ||
							_wcsicmp(currentOption.c_str(), L"Ellipse") == 0 ||
							_wcsicmp(currentOption.c_str(), L"Arc") == 0 ||
							_wcsicmp(currentOption.c_str(), L"Curve") == 0 ||
							_wcsicmp(currentOption.c_str(), L"Custom") == 0)
						{
							ShapeType = currentOption;
							ShapeOptions = Shape[++id].c_str();
						}
					}

					if (_wcsicmp(currentOption.c_str(), L"FillColor") == 0)				shape.m_FillColor = parser.ParseColor(Shape[++id].c_str());
					else if (_wcsicmp(currentOption.c_str(), L"OutlineColor") == 0)		shape.m_OutlineColor = parser.ParseColor(Shape[++id].c_str());
					else if (_wcsicmp(currentOption.c_str(), L"OutlineWidth") == 0)		shape.m_OutlineWidth = parser.ParseDouble(Shape[++id].c_str(), 1.0);
					else if (_wcsicmp(currentOption.c_str(), L"Rotation") == 0)			shape.m_Rotation = parser.ParseDouble(Shape[++id].c_str(), 0);
					else if (_wcsicmp(currentOption.c_str(), L"ConnectEdges") == 0)		shape.m_ConnectEdges = parser.ParseInt(Shape[++id].c_str(), 0) != 0;
					else if (_wcsicmp(currentOption.c_str(), L"CombineWith") == 0)			shape.m_CombineWith = Shape[++id];
					else if (_wcsicmp(currentOption.c_str(), L"CombineMode") == 0)			shape.m_CombineMode = Shape[++id];
					else if (_wcsicmp(currentOption.c_str(), L"ImageName") == 0) { shape.m_Image = LoadImage(Shape[id + 1] + L":" + Shape[id + 2], false, shape); id += 2; }
					else if (_wcsicmp(currentOption.c_str(), L"ImageRect") == 0)			shape.m_ImageDstRect = parser.ParseRect(Shape[++id].c_str());
				}


				if (ShapeType == L"" || ShapeOptions == L"") break;

				if (_wcsicmp(ShapeType.c_str(), L"Rectangle") == 0) ParseRect(shape, parser.ParseRECT(ShapeOptions.c_str()));
				else if (_wcsicmp(ShapeType.c_str(), L"RoundedRectangle") == 0) ParseRoundedRect(shape, ShapeOptions.c_str(), parser);
				else if (_wcsicmp(ShapeType.c_str(), L"Ellipse") == 0) ParseEllipse(shape, ShapeOptions.c_str(), parser);
				else if (_wcsicmp(ShapeType.c_str(), L"Arc") == 0);
				else if (_wcsicmp(ShapeType.c_str(), L"Curve") == 0);
				else if (_wcsicmp(ShapeType.c_str(), L"Custom") == 0) ParseCustom(shape, ShapeOptions.c_str(), parser, section);
				m_Shapes[Shapeidentifier] = shape;
				Shapeidentifier = L"Shape" + std::to_wstring(++ShapeIndex);
				const std::wstring& curShape = parser.ReadString(section, Shapeidentifier.c_str(), L"");
			}

		End:;
	
	int i = 0;

	if (m_Initialized && m_Measures.empty() && !m_DynamicVariables)
	{
		Initialize();
		//m_NeedsRedraw = true;
	}
}

/*
** Overridden method. The Image meters need not to be bound on anything
**
*/
void MeterVector::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, true))
	{
		BindSecondaryMeasures(parser, section);
	}
}

void MeterVector::ParseRect(VectorShape& shape, RECT& rect)
{
	D2D1_RECT_F geo_rect;
	geo_rect.left = rect.left;
	geo_rect.right = rect.right + rect.left;
	geo_rect.top = rect.top;
	geo_rect.bottom = rect.bottom + rect.top;

	shape.m_X = geo_rect.left;
	shape.m_Y = geo_rect.top;
	shape.m_W = geo_rect.right - geo_rect.left;
	shape.m_H = geo_rect.bottom - geo_rect.top;
	shape.m_Geometry = Gfx::Canvas::CreateRectangle(geo_rect);
	shape.m_ShapeParsed = true;
	if (shape.m_CombineWith != L"" && shape.m_CombineMode != L"" && m_Shapes.find(shape.m_CombineWith) != m_Shapes.end()) {
		auto newShape = CombineShapes(shape, m_Shapes[shape.m_CombineWith]);
		if (newShape) {
			shape.m_Geometry = newShape;
			m_Shapes.erase(shape.m_CombineWith);
		}
	}
}


bool MeterVector::ParseRoundedRect(VectorShape& shape, LPCWSTR option, ConfigParser& parser)
{

	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 6)
		return false;

	D2D1_ROUNDED_RECT rect;
	rect.rect.left = parser.ParseDouble(Tokens[0].c_str(), 0);
	rect.rect.top = parser.ParseDouble(Tokens[1].c_str(), 0);
	rect.rect.right = parser.ParseDouble(Tokens[2].c_str(), 0) + rect.rect.left;
	rect.rect.bottom = parser.ParseDouble(Tokens[3].c_str(), 0) + rect.rect.top;
	rect.radiusX = parser.ParseDouble(Tokens[4].c_str(), 5);
	rect.radiusY = parser.ParseDouble(Tokens[5].c_str(), 5);
	shape.m_X = rect.rect.left;
	shape.m_Y = rect.rect.top;
	shape.m_W = rect.rect.right - rect.rect.left;
	shape.m_H = rect.rect.bottom - rect.rect.top;
	shape.m_Geometry = Gfx::Canvas::CreateRoundedRectangle(rect);
	shape.m_ShapeParsed = true;
	if (shape.m_CombineWith != L"" && shape.m_CombineMode != L"" && m_Shapes.find(shape.m_CombineWith) != m_Shapes.end()) {
		auto newShape = CombineShapes(shape, m_Shapes[shape.m_CombineWith]);
		if (newShape) {
			shape.m_Geometry = newShape;
			m_Shapes.erase(shape.m_CombineWith);
		}
	}
	return true;
}


bool MeterVector::ParseEllipse(VectorShape& shape, LPCWSTR option, ConfigParser& parser)
{

	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 4)
		return false;

	D2D1_ELLIPSE ellipse;
	ellipse.point.x = parser.ParseDouble(Tokens[0].c_str(), 0);
	ellipse.point.y = parser.ParseDouble(Tokens[1].c_str(), 0);
	ellipse.radiusX = parser.ParseDouble(Tokens[2].c_str(), 0);
	ellipse.radiusY = parser.ParseDouble(Tokens[3].c_str(), 0);
	shape.m_X = ellipse.point.x - ellipse.radiusX;
	shape.m_Y = ellipse.point.y - ellipse.radiusY;
	shape.m_W = ellipse.point.x + ellipse.radiusX;
	shape.m_H = ellipse.point.y + ellipse.radiusY;
	shape.m_Geometry = Gfx::Canvas::CreateEllipse(ellipse);
	shape.m_ShapeParsed = true;
	if (shape.m_CombineWith != L"" && shape.m_CombineMode != L"" && m_Shapes.find(shape.m_CombineWith) != m_Shapes.end()) {
		auto newShape = CombineShapes(shape, m_Shapes[shape.m_CombineWith]);
		if (newShape) {
			shape.m_Geometry = newShape;
				m_Shapes.erase(shape.m_CombineWith);
		}
	}
	return true;
}

bool MeterVector::ParseCustom(VectorShape& shape, LPCWSTR option, ConfigParser& parser, const WCHAR * section)
{

	std::vector<std::wstring> Tokens = parser.Tokenize(option, L",");
	if (Tokens.size() != 1)
		return false;

	std::wstring ShapeDef = parser.ReadString(section, Tokens[0].c_str(), L"");
	auto shapePairs = parser.Tokenize(ShapeDef, L"|");

	std::vector<Gfx::Canvas::VectorPoint> points;
	bool first = true;
	for (const std::wstring& pair : shapePairs)
	{
		auto well = parser.Tokenize(pair, L":");
		if (well.size() < 2)
			break;

		std::wstring fullOption = L"";
		for (int i = 1; i < well.size(); i++) {
			if(i > 1)
			fullOption += L":" + well[i];
			else
				fullOption += well[i];
		}
		auto well2 = parser.Tokenize(fullOption.c_str(), L",");
		well2 = CombineOptions(well2);
		if (well2.size() < 2) break;
		double x = parser.ParseDouble(well2[0].c_str(), 0);
		double y = parser.ParseDouble(well2[1].c_str(), 0);
		if (first) {
			if (_wcsicmp(well[0].c_str(), L"Start") == 0)
			{
				if (well2.size() != 2) break;
				shape.m_X = x;
				shape.m_Y = y;
				shape.m_W = x;
				shape.m_H = y;
				points.push_back(Gfx::Canvas::VectorPoint(x, y));
			}
			else
			{
				shape.m_X = x;
				shape.m_Y = y;
				points.push_back(Gfx::Canvas::VectorPoint(0, 0));
			}
			first = false;
		}


		if (_wcsicmp(well[0].c_str(), L"LineTo") == 0)
		{
			if (well2.size() != 2) break;
			points.push_back(Gfx::Canvas::VectorPoint(x, y));
		}
		else if (_wcsicmp(well[0].c_str(), L"ArcTo") == 0)
		{
			if (well2.size() < 4) break;
			D2D1_ARC_SEGMENT segment;

			segment.rotationAngle = 0;
			segment.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
			segment.arcSize = D2D1_ARC_SIZE_SMALL;

			segment.point.x = x;
			segment.point.y = y;
			segment.size.width= parser.ParseDouble(well2[2].c_str(), 0);
			segment.size.height= parser.ParseDouble(well2[3].c_str(), 0);

			if(well2.size() >= 5) segment.rotationAngle = parser.ParseDouble(well2[4].c_str(), 0);
			//if (well2.size() >= 5) segment.sweepDirection = parser.ParseDouble(well2[4].c_str(), 0);
			//if (well2.size() >= 5) segment.arcSize = parser.ParseDouble(well2[4].c_str(), 0);

			points.push_back(Gfx::Canvas::VectorPoint(segment));
		}
		else if (_wcsicmp(well[0].c_str(), L"CurveTo") == 0)
		{
			if (well2.size() != 6) break;
			D2D1_BEZIER_SEGMENT segment;
			segment.point3.x = x;
			segment.point3.y = y;
			segment.point1.x = parser.ParseDouble(well2[2].c_str(), 0);
			segment.point1.y = parser.ParseDouble(well2[3].c_str(), 0);
			segment.point2.x = parser.ParseDouble(well2[4].c_str(), 0);
			segment.point2.y = parser.ParseDouble(well2[5].c_str(), 0);

			points.push_back(Gfx::Canvas::VectorPoint(segment));
		}

		if (x < shape.m_X)
			shape.m_X = x;
		if (y < shape.m_Y)
			shape.m_Y = y;
		if (x > shape.m_W)
			shape.m_W = x;
		if (y > shape.m_H)
			shape.m_H = y;
	}
	if (first)
		return false;
	shape.m_Geometry = Gfx::Canvas::CreateCustomGeometry(points, shape.m_ConnectEdges);

	if (shape.m_CombineWith != L"" && shape.m_CombineMode != L"" && m_Shapes.find(shape.m_CombineWith) != m_Shapes.end()) {
		shape.m_Geometry = CombineShapes(shape, m_Shapes[shape.m_CombineWith]);
		m_Shapes.erase(shape.m_CombineWith);
	}

	shape.m_ShapeParsed = true;
	return true;
}