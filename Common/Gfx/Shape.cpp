/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"
#include "Canvas.h"

namespace Gfx {

Shape::Shape(ShapeType type) :
	m_ShapeType(type),
	m_IsCombined(false),
	m_Offset(D2D1::SizeF(0.0f, 0.0f)),
	m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
	m_Rotation(0.0f),
	m_RotationAnchor(D2D1::Point2F(0.0f, 0.0f)),
	m_RotationAnchorDefined(false),
	m_StrokeWidth(1.0f),
	m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_StrokeCustomDashes(),
	m_StrokeProperties(D2D1::StrokeStyleProperties1())
{
	// Make sure the stroke width is exact, not altered by other
	// transforms like Scale or Rotation
	m_StrokeProperties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED;
}

Shape::~Shape()
{
}

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix()
{
	D2D1_RECT_F bounds;
	m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, nullptr, &bounds);

	// If the rotation anchor is not defined, use the center of the shape
	D2D1_POINT_2F rotationPoint = m_RotationAnchorDefined ?
		m_RotationAnchor :
		D2D1::Point2F((bounds.right - bounds.left) / 2.0f, (bounds.bottom - bounds.top) / 2.0f);

	// Offset rotation point by the shapes bounds
	rotationPoint.x += bounds.left;
	rotationPoint.y += bounds.top;

	return D2D1::Matrix3x2F(
		D2D1::Matrix3x2F::Rotation(m_Rotation, rotationPoint) *
		D2D1::Matrix3x2F::Translation(m_Offset)
	);
}

D2D1_RECT_F Shape::GetBounds()
{
	D2D1_RECT_F bounds;
	if (m_Shape)
	{
		HRESULT result = m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, GetShapeMatrix(), &bounds);
		if (SUCCEEDED(result)) return bounds;
	}

	return D2D1::RectF();
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
}

bool Shape::ContainsPoint(D2D1_POINT_2F point)
{
	if (m_Shape)
	{
		BOOL contains = FALSE;
		HRESULT result = m_Shape->StrokeContainsPoint(point, m_StrokeWidth, nullptr, GetShapeMatrix(), &contains);
		if (SUCCEEDED(result) && contains) return true;

		result = m_Shape->FillContainsPoint(point, GetShapeMatrix(), &contains);
		if (SUCCEEDED(result) && contains) return true;
	}

	return false;
}

bool Shape::CombineWith(Shape* otherShape, D2D1_COMBINE_MODE mode)
{
	Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
	HRESULT hr = Canvas::c_D2DFactory->CreatePathGeometry(path.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = path->Open(sink.GetAddressOf());
	if (FAILED(hr)) return false;

	if (otherShape)
	{
		hr = m_Shape->CombineWithGeometry(
			otherShape->m_Shape.Get(),
			mode,
			otherShape->GetShapeMatrix(),
			sink.Get());
		if (FAILED(hr)) return false;

		sink->Close();

		hr = path.CopyTo(m_Shape.ReleaseAndGetAddressOf());
		if (FAILED(hr)) return false;

		return true;
	}

	const D2D1_RECT_F rect = { 0, 0, 0, 0 };
	Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> emptyShape;
	hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, emptyShape.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = emptyShape->CombineWithGeometry(m_Shape.Get(), mode, GetShapeMatrix(), sink.Get());

	sink->Close();

	if (FAILED(hr)) return false;

	hr = path.CopyTo(m_Shape.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return false;

	m_Rotation = 0.0f;
	m_Offset = D2D1::SizeF(0.0f, 0.0f);

	return true;
}

void Shape::SetRotation(FLOAT rotation, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	m_Rotation = rotation;

	m_RotationAnchor.x = anchorX;
	m_RotationAnchor.y = anchorY;
	m_RotationAnchorDefined = anchorDefined;
}

void Shape::CloneModifiers(Shape* otherShape)
{
	otherShape->m_Offset = m_Offset;
	otherShape->m_FillColor = m_FillColor;
	otherShape->m_StrokeColor = m_StrokeColor;
	otherShape->m_StrokeWidth = m_StrokeWidth;
	otherShape->m_Rotation = m_Rotation;
	otherShape->m_StrokeProperties = m_StrokeProperties;
	otherShape->m_StrokeCustomDashes = m_StrokeCustomDashes;
}

}  // namespace Gfx
