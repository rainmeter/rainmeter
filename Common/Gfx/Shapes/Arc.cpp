/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Arc.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

namespace Gfx {

Arc::Arc(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT xRadius, FLOAT yRadius, FLOAT angle,
	D2D1_SWEEP_DIRECTION sweep, D2D1_ARC_SIZE size, D2D1_FIGURE_END ending) : Shape(ShapeType::Arc),
	m_StartPoint(D2D1::Point2F(x1, y1)),
	m_ArcSegment(D2D1::ArcSegment(
		D2D1::Point2F(x2, y2),
		D2D1::SizeF(xRadius, yRadius),
		angle,
		sweep,
		size)),
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
			sink->AddArc(m_ArcSegment);
			sink->EndFigure(m_ShapeEnding);
			sink->Close();

			hr = path.CopyTo(m_Shape.GetAddressOf());
			if (SUCCEEDED(hr)) return;
		}
	}

	LogErrorF(L"Could not create arc object. X1=%i, Y1=%i, X2=%i, Y2=%i, XRadius=%i, YRadius=%i, Angle=%i",
		(int)x1, (int)y1, (int)x2, (int)y2, (int)xRadius, (int)yRadius, (int)angle);
}

Arc::~Arc()
{
}

Shape* Arc::Clone()
{
	Shape* newShape = new Arc(
		m_StartPoint.x,
		m_StartPoint.y,
		m_ArcSegment.point.x,
		m_ArcSegment.point.y,
		m_ArcSegment.size.width,
		m_ArcSegment.size.height,
		m_ArcSegment.rotationAngle,
		m_ArcSegment.sweepDirection,
		m_ArcSegment.arcSize,
		m_ShapeEnding);
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
