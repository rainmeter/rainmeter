/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Shape.h"
#include "Canvas.h"
#include "Gfx/Util/D2DUtil.h"

namespace Gfx {

Shape::Shape(ShapeType type) :
	m_ShapeType(type),
	m_TransformOrder(),
	m_IsCombined(false),
	m_Offset(D2D1::SizeF(0.0f, 0.0f)),
	m_Rotation(0.0f),
	m_RotationAnchor(D2D1::Point2F(0.0f, 0.0f)),
	m_RotationAnchorDefined(false),
	m_Scale(D2D1::SizeF(1.0f, 1.0f)),
	m_ScaleAnchor(D2D1::Point2F(0.0f, 0.0f)),
	m_ScaleAnchorDefined(false),
	m_Skew(D2D1::Point2F(0.0f, 0.0f)),
	m_SkewAnchor(D2D1::Point2F(0.0f, 0.0f)),
	m_SkewAnchorDefined(false),
	m_StrokeWidth(1.0f),
	m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_StrokeCustomDashes(),
	m_StrokeProperties(D2D1::StrokeStyleProperties1()),
	m_BrushType(BrushType::Solid),
	m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
	m_LinearGradientAngle(0),
	m_RadialGradientOffset(D2D1::Point2F(0.0f, 0.0f)),
	m_GradientAltGamma(false),
	m_HasBrushChanged(true)
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

	D2D1_POINT_2F point = D2D1::Point2F(bounds.left, bounds.top);

	// If the rotation anchor is not defined, use the center of the shape
	D2D1_POINT_2F rotationPoint = m_RotationAnchorDefined ?
		m_RotationAnchor :
		D2D1::Point2F((bounds.right - bounds.left) / 2.0f, (bounds.bottom - bounds.top) / 2.0f);

	rotationPoint = Util::AddPoint2F(point, rotationPoint);

	D2D1_POINT_2F scalePoint = m_ScaleAnchorDefined ?
		m_ScaleAnchor :
		D2D1::Point2F(0.0f, 0.0f);

	scalePoint = Util::AddPoint2F(point, scalePoint);

	D2D1_POINT_2F skewPoint = m_SkewAnchorDefined ?
		m_SkewAnchor :
		D2D1::Point2F(0.0f, 0.0f);

	skewPoint = Util::AddPoint2F(point, skewPoint);

	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
	for (const auto& type : m_TransformOrder)
	{
		switch (type)
		{
		case TransformType::Rotate:
			matrix = matrix * D2D1::Matrix3x2F::Rotation(m_Rotation, rotationPoint);
			break;

		case TransformType::Scale:
			matrix = matrix * D2D1::Matrix3x2F::Scale(m_Scale, scalePoint);
			break;

		case TransformType::Skew:
			matrix = matrix * D2D1::Matrix3x2F::Skew(m_Skew.x, m_Skew.y, skewPoint);
			break;

		case TransformType::Offset:
			matrix = matrix * D2D1::Matrix3x2F::Translation(m_Offset);
			break;
		}
	}

	return matrix;
}

D2D1_RECT_F Shape::GetBounds(bool useMatrix)
{
	D2D1_RECT_F bounds;
	D2D1_MATRIX_3X2_F matrix = useMatrix ? GetShapeMatrix() : D2D1::Matrix3x2F::Identity();

	HRESULT result = m_Shape->GetWidenedBounds(
		m_StrokeWidth,
		m_StrokeStyle.Get(),
		matrix,
		&bounds);
	if (FAILED(result)) return D2D1::RectF();

	return bounds;
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
		HRESULT result = m_Shape->StrokeContainsPoint(
			point,
			m_StrokeWidth,
			m_StrokeStyle.Get(),
			GetShapeMatrix(),
			&contains);
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
	m_RotationAnchor = D2D1::Point2F();
	m_RotationAnchorDefined = false;
	m_Scale = D2D1::SizeF(1.0f, 1.0f);
	m_ScaleAnchor = D2D1::Point2F();
	m_ScaleAnchorDefined = false;
	m_Skew = D2D1::Point2F();
	m_SkewAnchor = D2D1::Point2F();
	m_SkewAnchorDefined = false;
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

void Shape::SetScale(FLOAT scaleX, FLOAT scaleY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	m_Scale.width = scaleX;
	m_Scale.height = scaleY;

	m_ScaleAnchor.x = anchorX;
	m_ScaleAnchor.y = anchorY;
	m_ScaleAnchorDefined = anchorDefined;
}

void Shape::SetSkew(FLOAT skewX, FLOAT skewY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	m_Skew.x = skewX;
	m_Skew.y = skewY;

	m_SkewAnchor.x = anchorX;
	m_SkewAnchor.y = anchorY;
	m_SkewAnchorDefined = anchorDefined;
}

void Shape::CreateStrokeStyle()
{
	const FLOAT* dashes = nullptr;
	if (!m_StrokeCustomDashes.empty())
	{
		m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
		dashes = m_StrokeCustomDashes.data();
	}

	UINT32 dashCount = (UINT32)m_StrokeCustomDashes.size();
	HRESULT hr = Canvas::c_D2DFactory->CreateStrokeStyle(
		m_StrokeProperties,
		dashes,
		dashCount,
		m_StrokeStyle.ReleaseAndGetAddressOf());

	// If failed, make sure stroke is null
	if (FAILED(hr)) m_StrokeStyle = nullptr;
}

void Shape::SetFill(Gdiplus::Color color)
{
	m_BrushType = BrushType::Solid;
	m_FillColor = Util::ToColorF(color);
	m_HasBrushChanged = true;
}

void Shape::SetFill(UINT32 angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_BrushType = BrushType::LinearGradient;
	m_LinearGradientAngle = angle;
	m_GradientStops = stops;
	m_GradientAltGamma = altGamma;
	m_HasBrushChanged = true;
}

void Shape::SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_BrushType = BrushType::RadialGradient;
	m_RadialGradientOffset = offset;
	m_RadialGradientCenter = center;
	m_RadialGradientRadius = radius;
	m_GradientStops = stops;
	m_GradientAltGamma = altGamma;
	m_HasBrushChanged = true;
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetFillBrush(ID2D1RenderTarget* target)
{
	// If the brush hasn't changed, return current brush
	if (!m_HasBrushChanged) return m_Brush;

	HRESULT hr = E_FAIL;
	Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> collection;

	if (m_BrushType == BrushType::LinearGradient ||
		m_BrushType == BrushType::RadialGradient)
	{
		hr = target->CreateGradientStopCollection(
			&m_GradientStops[0],
			(UINT32)m_GradientStops.size(),
			m_GradientAltGamma ? D2D1_GAMMA_1_0 : D2D1_GAMMA_2_2,
			D2D1_EXTEND_MODE_CLAMP,
			collection.GetAddressOf());

		if (FAILED(hr)) return nullptr;
	}

	switch (m_BrushType)
	{
	case BrushType::Solid:
		{
			Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solid;
			hr = target->CreateSolidColorBrush(m_FillColor, solid.GetAddressOf());
			if (FAILED(hr)) return nullptr;
			solid.CopyTo(m_Brush.GetAddressOf());
		}
		break;

	case BrushType::LinearGradient:
		{
			auto bounds = GetBounds(false);
			D2D1_POINT_2F start = Util::FindEdgePoint(m_LinearGradientAngle,
				bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
			D2D1_POINT_2F end = Util::FindEdgePoint(m_LinearGradientAngle + 180,
				bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);

			Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> linear;
			hr = target->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(start, end),
				collection.Get(),
				linear.GetAddressOf());
			if (FAILED(hr)) return nullptr;
			linear.CopyTo(m_Brush.GetAddressOf());
		}
		break;

	case BrushType::RadialGradient:
		{
			auto swapIfNotDefined = [](D2D1_POINT_2F& pt1, const D2D1_POINT_2F pt2) -> void
			{
				if (pt2.x != FLT_MAX) pt1.x = pt2.x;
				if (pt2.y != FLT_MAX) pt1.y = pt2.y;
			};

			auto bounds = GetBounds(false);
			D2D1_POINT_2F offset = D2D1::Point2F();
			D2D1_POINT_2F center = D2D1::Point2F(((bounds.left + bounds.right) / 2.0f), ((bounds.top + bounds.bottom) / 2.0f));
			D2D1_POINT_2F radius = D2D1::Point2F((bounds.right - bounds.left) / 2.0f, (bounds.bottom - bounds.top) / 2.0f);

			// Offset from actual center of shape
			center = Util::AddPoint2F(center, m_RadialGradientCenter);

			// Check if offset and radii are defined
			swapIfNotDefined(offset, m_RadialGradientOffset);
			swapIfNotDefined(radius, m_RadialGradientRadius);

			Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> radial;
			hr = target->CreateRadialGradientBrush(
				D2D1::RadialGradientBrushProperties(
					center,
					offset,
					radius.x,
					radius.y),
				collection.Get(),
				radial.GetAddressOf());
			if (FAILED(hr)) return nullptr;
			radial.CopyTo(m_Brush.GetAddressOf());
		}
		break;

	default:
		return nullptr;
	}

	m_HasBrushChanged = false;
	return m_Brush;
}

bool Shape::AddToTransformOrder(TransformType type)
{
	// Don't add if 'type' is a duplicate
	for (const auto& t : m_TransformOrder) if (t == type) return false;

	m_TransformOrder.emplace_back(type);
	return true;
}

void Shape::ValidateTransforms()
{
	// There should be no duplicates, but make sure the order is not larger
	// than the defined amount of transforms
	while (m_TransformOrder.size() >= (size_t)TransformType::MAX) m_TransformOrder.pop_back();

	// Add any missing transforms
	AddToTransformOrder(TransformType::Rotate);
	AddToTransformOrder(TransformType::Scale);
	AddToTransformOrder(TransformType::Skew);
	AddToTransformOrder(TransformType::Offset);
}

void Shape::CloneModifiers(Shape* otherShape)
{
	otherShape->m_Offset = m_Offset;
	otherShape->m_StrokeColor = m_StrokeColor;
	otherShape->m_StrokeWidth = m_StrokeWidth;
	otherShape->m_Rotation = m_Rotation;
	otherShape->m_RotationAnchor = m_RotationAnchor;
	otherShape->m_RotationAnchorDefined;
	otherShape->m_Scale = m_Scale;
	otherShape->m_ScaleAnchor = m_ScaleAnchor;
	otherShape->m_ScaleAnchorDefined = m_ScaleAnchorDefined;
	otherShape->m_Skew = m_Skew;
	otherShape->m_SkewAnchor = m_SkewAnchor;
	otherShape->m_SkewAnchorDefined = m_SkewAnchorDefined;
	otherShape->m_StrokeProperties = m_StrokeProperties;
	otherShape->m_StrokeCustomDashes = m_StrokeCustomDashes;
	otherShape->m_TransformOrder = m_TransformOrder;

	otherShape->CreateStrokeStyle();

	otherShape->m_BrushType = m_BrushType;
	otherShape->m_FillColor = m_FillColor;
	otherShape->m_LinearGradientAngle = m_LinearGradientAngle;
	otherShape->m_RadialGradientOffset = m_RadialGradientOffset;
	otherShape->m_GradientStops = m_GradientStops;
	otherShape->m_GradientAltGamma = m_GradientAltGamma;
	
	// Re-create brush on next draw
	otherShape->m_HasBrushChanged = true;
}

}  // namespace Gfx
