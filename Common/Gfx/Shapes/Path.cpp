/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Path.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

namespace Gfx {

Path::Path(FLOAT x, FLOAT y, D2D1_FILL_MODE fillMode, bool isCloned) : Shape(ShapeType::Path),
	m_StartPoint(D2D1::Point2F(x, y)),
	m_FillMode(fillMode)
{
	// If cloned, copy |m_Path| instead of re-creating it.
	if (isCloned) return;

	HRESULT hr = Canvas::c_D2DFactory->CreatePathGeometry(m_Path.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		hr = m_Path->Open(m_Sink.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			m_Sink->SetFillMode(m_FillMode);
			m_Sink->BeginFigure(m_StartPoint, D2D1_FIGURE_BEGIN_FILLED);
			return;
		}
	}

	LogErrorF(L"Could not create path object. X1=%i, Y1=%i", (int)x, (int)y);
}

Path::~Path()
{
}

void Path::Dispose()
{
	if (m_Path) m_Path.Reset();
	if (m_Sink) m_Sink.Reset();
}

Shape* Path::Clone()
{
	Path* newShape = new Path(m_StartPoint.x, m_StartPoint.y, m_FillMode, true);

	m_Shape.CopyTo(newShape->m_Shape.GetAddressOf());

	CloneModifiers(newShape);

	return newShape;
}

void Path::AddLine(FLOAT x, FLOAT y)
{
	if (!m_Path || !m_Sink) return;
	m_Sink->AddLine(D2D1::Point2F(x, y));
}

void Path::AddArc(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius, FLOAT angle,
	D2D1_SWEEP_DIRECTION direction, D2D1_ARC_SIZE arcSize)
{
	if (!m_Path || !m_Sink) return;
	D2D1_ARC_SEGMENT segment = D2D1::ArcSegment(
		D2D1::Point2F(x, y),
		D2D1::SizeF(xRadius, yRadius),
		angle,
		direction,
		arcSize);
	m_Sink->AddArc(segment);
}

void Path::AddQuadraticCurve(FLOAT x, FLOAT y, FLOAT cx, FLOAT cy)
{
	if (!m_Path || !m_Sink) return;
	D2D1_QUADRATIC_BEZIER_SEGMENT segment = D2D1::QuadraticBezierSegment(
		D2D1::Point2F(cx, cy),
		D2D1::Point2F(x, y));
	m_Sink->AddQuadraticBezier(segment);
}

void Path::AddCubicCurve(FLOAT x, FLOAT y, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2)
{
	if (!m_Path || !m_Sink) return;
	D2D1_BEZIER_SEGMENT segment = D2D1::BezierSegment(
		D2D1::Point2F(cx1, cy1),
		D2D1::Point2F(cx2, cy2),
		D2D1::Point2F(x, y));
	m_Sink->AddBezier(segment);
}

void Path::SetSegmentFlags(D2D1_PATH_SEGMENT flags)
{
	if (!m_Path || !m_Sink) return;
	m_Sink->SetSegmentFlags(flags);
}

void Path::Close(D2D1_FIGURE_END ending)
{
	if (!m_Path || !m_Sink) return;

	m_Sink->EndFigure(ending);
	m_Sink->Close();

	m_Path.CopyTo(m_Shape.GetAddressOf());
	Dispose();
}

}  // namespace Gfx
