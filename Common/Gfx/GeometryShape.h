#pragma once
#ifndef RM_GFX_GEOMETRYSHAPE_H_
#define RM_GFX_GEOMETRYSHAPE_H_

namespace Gfx {


	struct GeometryShape {

		GeometryShape() : m_OutlineWidth(1),
			m_OutlineColor(Gdiplus::Color::Black),
			m_FillColor(Gdiplus::Color::White),
			m_GradientStops(),
			m_GradientProperties(),
			m_StrokeProperties()
		{}

		enum GeometryType {
			Line,
			Arc,
			Bezier,
			QuadBezier
		};

		Microsoft::WRL::ComPtr<ID2D1Geometry> m_Geometry;
		int m_OutlineWidth;
		Gdiplus::Color m_OutlineColor;
		Gdiplus::Color m_FillColor;

		std::vector<D2D1_GRADIENT_STOP> m_GradientStops;
		union GradientProperties {
			D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES m_LinearProperties;
			D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES m_RadialProperties;
		} m_GradientProperties;
		enum BrushType {
			Linear,
			Radial,
			Solid
		} m_BrushType;

		bool m_UseDashes = false;
		std::vector<float> m_Dashes;
		D2D1_STROKE_STYLE_PROPERTIES m_StrokeProperties;


	};
	struct VectorPoint
	{

		VectorPoint(double x, double y)
		{
			m_Geometry.lineSegment = D2D1::Point2F(x, y);
			m_type = GeometryShape::Line;
			m_x = x;
			m_y = y;
		}
		VectorPoint(D2D1_ARC_SEGMENT& segment)
		{
			m_Geometry.arcSegment = segment;
			m_type = GeometryShape::Arc;
			m_x = segment.point.x;
			m_y = segment.point.y;
		}
		VectorPoint(D2D1_BEZIER_SEGMENT& segment)
		{
			m_Geometry.bezierSegment = segment;
			m_type = GeometryShape::Bezier;
			m_x = segment.point3.x;
			m_y = segment.point3.y;
		}
		VectorPoint(D2D1_QUADRATIC_BEZIER_SEGMENT& segment)
		{
			m_Geometry.quadBezierSegment = segment;
			m_type = GeometryShape::QuadBezier;
			m_x = segment.point2.x;
			m_y = segment.point2.y;
		}
		union Geometry {
			D2D1_ARC_SEGMENT arcSegment;
			D2D1_POINT_2F lineSegment;
			D2D1_BEZIER_SEGMENT bezierSegment;
			D2D1_QUADRATIC_BEZIER_SEGMENT quadBezierSegment;

		} m_Geometry;
		double m_x, m_y;
		GeometryShape::GeometryType m_type;
	};
}
#endif