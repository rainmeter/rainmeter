/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "QuadraticCurve.h"
#include "Gfx/Canvas.h"

namespace Gfx {

QuadraticCurve::QuadraticCurve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT cx, FLOAT cy,
	D2D1_FIGURE_END ending) : Shape(ShapeType::QuadraticCurve),
	m_StartPoint(D2D1::Point2F(x1, y1)),
	m_QuadraticBezierSegment(D2D1::QuadraticBezierSegment(
		D2D1::Point2F(cx, cy),
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
			sink->AddQuadraticBezier(m_QuadraticBezierSegment);
			sink->EndFigure(m_ShapeEnding);
			sink->Close();

			m_Shape = std::move(path);
		}
	}
}

QuadraticCurve::~QuadraticCurve()
{
}


}  // namespace Gfx
