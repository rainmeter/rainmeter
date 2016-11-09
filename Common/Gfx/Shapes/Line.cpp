/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Line.h"
#include "Gfx/Canvas.h"
#include "../Library/Logger.h"

namespace Gfx {

Line::Line(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2) : Shape(ShapeType::Line),
	m_StartPoint(D2D1::Point2F(x1, y1)),
	m_EndPoint(D2D1::Point2F(x2, y2))
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
			sink->AddLine(m_EndPoint);
			sink->EndFigure(D2D1_FIGURE_END_OPEN);
			sink->Close();

			hr = path.CopyTo(m_Shape.GetAddressOf());
			if (SUCCEEDED(hr)) return;
		}
	}

	LogErrorF(L"Could not create line object. X1=%i, Y1=%i, X2=%i, Y2=%i", (int)x1, (int)y1, (int)x2, (int)y2);
}

Line::~Line()
{
}

Shape* Line::Clone()
{
	Shape* newShape = new Line(m_StartPoint.x, m_StartPoint.y, m_EndPoint.x, m_EndPoint.y);
	CloneModifiers(newShape);
	return newShape;
}

}  // namespace Gfx
