/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Curve.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

namespace Gfx {

Curve::Curve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2,
	FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2, D2D1_FIGURE_END ending) : Shape(ShapeType::Curve),
	m_StartPoint(D2D1::Point2F(x1, y1)),
	m_BezierSegment(D2D1::BezierSegment(
		D2D1::Point2F(cx1, cy1),
		D2D1::Point2F(cx2, cy2),
		D2D1::Point2F(x2, y2))),
	m_ShapeEnding(ending)
{
	Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
	HRESULT hr = Canvas::c_D2DFactory->CreatePathGeometry(path.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		hr = path->Open(sink.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			sink->BeginFigure(m_StartPoint, D2D1_FIGURE_BEGIN_FILLED);
			sink->AddBezier(m_BezierSegment);
			sink->EndFigure(m_ShapeEnding);
			sink->Close();

			hr = path.CopyTo(m_Shape.GetAddressOf());
			if (SUCCEEDED(hr)) return;
		}
	}

	LogErrorF(L"Could not create curve object. X1=%i, Y1=%i, X2=%i, Y2=%i", (int)x1, (int)y1, (int)x2, (int)y2);
}

Curve::~Curve()
{
}

Shape* Curve::Clone()
{
	Shape* newShape = new Curve(
		m_StartPoint.x,
		m_StartPoint.y,
		m_BezierSegment.point3.x,
		m_BezierSegment.point3.y,
		m_BezierSegment.point1.x,
		m_BezierSegment.point1.y,
		m_BezierSegment.point2.x,
		m_BezierSegment.point2.y,
		m_ShapeEnding);
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
