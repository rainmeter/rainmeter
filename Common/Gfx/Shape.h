/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_

#include <d2d1_1.h>
#include <wrl/client.h>

namespace Gfx {

struct ShapePoint
{
	ShapePoint(double x, double y)
	{
		m_Geometry.lineSegment = D2D1::Point2F(x, y);
		m_Type = Line;
		m_X = x;
		m_Y = y;
	}

	ShapePoint(D2D1_ARC_SEGMENT& segment)
	{
		m_Geometry.arcSegment = segment;
		m_Type = Arc;
		m_X = segment.point.x;
		m_Y = segment.point.y;
	}

	ShapePoint(D2D1_BEZIER_SEGMENT& segment)
	{
		m_Geometry.bezierSegment = segment;
		m_Type = Bezier;
		m_X = segment.point3.x;
		m_Y = segment.point3.y;
	}

	ShapePoint(D2D1_QUADRATIC_BEZIER_SEGMENT& segment)
	{
		m_Geometry.quadBezierSegment = segment;
		m_Type = QuadBezier;
		m_X = segment.point2.x;
		m_Y = segment.point2.y;
	}

	union Geometry {
		D2D1_ARC_SEGMENT arcSegment;
		D2D1_POINT_2F lineSegment;
		D2D1_BEZIER_SEGMENT bezierSegment;
		D2D1_QUADRATIC_BEZIER_SEGMENT quadBezierSegment;

	} m_Geometry;
	double m_X, m_Y;
	enum SegmentType {
		Line,
		Arc,
		Bezier,
		QuadBezier
	} m_Type;
};

struct Shape
{
	Shape() :
		m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
		m_OutlineWidth(1),
		m_OutlineColor(D2D1::ColorF(D2D1::ColorF::Black))
	{
	}

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;
	D2D1_COLOR_F m_FillColor;
	double m_OutlineWidth;
	D2D1_COLOR_F m_OutlineColor;
};

} // Gfx

#endif