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
	m_StrokeCustomDashes(),
	m_StrokeProperties(D2D1::StrokeStyleProperties1()),
	m_FillBrushType(BrushType::Solid),
	m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
	m_FillLinearGradientAngle(0),
	m_FillRadialGradientOffset(D2D1::Point2F(0.0f, 0.0f)),
	m_FillRadialGradientCenter(D2D1::Point2F(0.0f, 0.0f)),
	m_FillRadialGradientRadius(D2D1::Point2F(0.0f, 0.0f)),
	m_FillGradientAltGamma(false),
	m_HasFillBrushChanged(true),
	m_StrokeBrushType(BrushType::Solid),
	m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_StrokeLinearGradientAngle(0),
	m_StrokeRadialGradientOffset(D2D1::Point2F(0.0f, 0.0f)),
	m_StrokeRadialGradientCenter(D2D1::Point2F(0.0f, 0.0f)),
	m_StrokeRadialGradientRadius(D2D1::Point2F(0.0f, 0.0f)),
	m_StrokeGradientAltGamma(false),
	m_HasStrokeBrushChanged(true)
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
	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();

	D2D1_RECT_F bounds;
	HRESULT hr = m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, nullptr, &bounds);
	if (FAILED(hr)) return matrix;

	D2D1_POINT_2F point = D2D1::Point2F(bounds.left, bounds.top);

	// Use the center of the shape as the default anchor point for all transforms
	D2D1_POINT_2F center = D2D1::Point2F((bounds.right - bounds.left) / 2.0f, (bounds.bottom - bounds.top) / 2.0f);

	D2D1_POINT_2F rotationPoint = m_RotationAnchorDefined ? m_RotationAnchor : center;
	rotationPoint = Util::AddPoint2F(point, rotationPoint);

	D2D1_POINT_2F scalePoint = m_ScaleAnchorDefined ? m_ScaleAnchor : center;
	scalePoint = Util::AddPoint2F(point, scalePoint);

	D2D1_POINT_2F skewPoint = m_SkewAnchorDefined ? m_SkewAnchor : center;
	skewPoint = Util::AddPoint2F(point, skewPoint);

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
	D2D1_RECT_F strokedBounds;
	D2D1_RECT_F fillBounds;
	D2D1_MATRIX_3X2_F matrix = useMatrix ? GetShapeMatrix() : D2D1::Matrix3x2F::Identity();

	HRESULT hr = m_Shape->GetWidenedBounds(
		m_StrokeWidth,
		m_StrokeStyle.Get(),
		matrix,
		&strokedBounds);
	if (FAILED(hr)) return D2D1::RectF();

	hr = m_Shape->GetBounds(matrix, &fillBounds);
	if (FAILED(hr)) return D2D1::RectF();

	// The 'Path' shape can have un-stroked segments, so we need to also
	// check the bounds of the fill to see which bounds are greater.
	if (fillBounds.left < strokedBounds.left) strokedBounds.left = fillBounds.left;
	if (fillBounds.top < strokedBounds.top) strokedBounds.top = fillBounds.top;
	if (fillBounds.right > strokedBounds.right) strokedBounds.right = fillBounds.right;
	if (fillBounds.bottom > strokedBounds.bottom) strokedBounds.bottom = fillBounds.bottom;

	return strokedBounds;
}

bool Shape::IsShapeDefined()
{
	return m_Shape;
}

bool Shape::ContainsPoint(D2D1_POINT_2F point, const Gdiplus::Matrix* transformationMatrix)
{
	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();

	// Apply TransformationMatrix if available
	if (transformationMatrix) transformationMatrix->GetElements((Gdiplus::REAL*)&matrix);

	matrix = matrix * GetShapeMatrix();

	BOOL contains = FALSE;
	HRESULT hr = m_Shape->StrokeContainsPoint(
		point,
		m_StrokeWidth,
		m_StrokeStyle.Get(),
		matrix,
		&contains);
	if (SUCCEEDED(hr) && contains) return true;

	hr = m_Shape->FillContainsPoint(point, matrix, &contains);
	if (SUCCEEDED(hr) && contains) return true;

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

	static const D2D1_RECT_F rect = { 0.0f, 0.0f, 0.0f, 0.0f };
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
	m_FillBrushType = BrushType::Solid;
	m_FillColor = Util::ToColorF(color);
	m_HasFillBrushChanged = true;
}

void Shape::SetFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_FillBrushType = BrushType::LinearGradient;
	m_FillLinearGradientAngle = angle;
	m_FillGradientStops = stops;
	m_FillGradientAltGamma = altGamma;
	m_HasFillBrushChanged = true;
}

void Shape::SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_FillBrushType = BrushType::RadialGradient;
	m_FillRadialGradientOffset = offset;
	m_FillRadialGradientCenter = center;
	m_FillRadialGradientRadius = radius;
	m_FillGradientStops = stops;
	m_FillGradientAltGamma = altGamma;
	m_HasFillBrushChanged = true;
}

void Shape::SetStrokeFill(Gdiplus::Color color)
{
	m_StrokeBrushType = BrushType::Solid;
	m_StrokeColor = Util::ToColorF(color);
	m_HasStrokeBrushChanged = true;
}

void Shape::SetStrokeFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_StrokeBrushType = BrushType::LinearGradient;
	m_StrokeLinearGradientAngle = angle;
	m_StrokeGradientStops = stops;
	m_StrokeGradientAltGamma = altGamma;
	m_HasStrokeBrushChanged = true;
}

void Shape::SetStrokeFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_StrokeBrushType = BrushType::RadialGradient;
	m_StrokeRadialGradientOffset = offset;
	m_StrokeRadialGradientCenter = center;
	m_StrokeRadialGradientRadius = radius;
	m_StrokeGradientStops = stops;
	m_StrokeGradientAltGamma = altGamma;
	m_HasStrokeBrushChanged = true;
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetFillBrush(ID2D1RenderTarget* target)
{
	// If the brush hasn't changed, return current fill brush
	if (!m_HasFillBrushChanged) return m_FillBrush;

	switch (m_FillBrushType)
	{
	case BrushType::Solid:
		{
			CreateSolidBrush(target, m_FillBrush, m_FillColor);
		}
		break;

	case BrushType::LinearGradient:
		{
			auto collection = CreateGradientStopCollection(target, m_FillGradientStops, m_FillGradientAltGamma);
			CreateLinearGradient(target, collection, m_FillBrush, m_FillLinearGradientAngle);
			if (collection) collection->Release();
		}
		break;

	case BrushType::RadialGradient:
		{
			auto collection = CreateGradientStopCollection(target, m_FillGradientStops, m_FillGradientAltGamma);
			CreateRadialGradient(target, collection, m_FillBrush, false);
			if (collection) collection->Release();
		}
		break;

	default:
		return nullptr;
	}

	m_HasFillBrushChanged = false;
	return m_FillBrush;
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetStrokeFillBrush(ID2D1RenderTarget* target)
{
	// If the brush hasn't changed, return current stroke brush
	if (!m_HasStrokeBrushChanged) return m_StrokeBrush;

	switch (m_StrokeBrushType)
	{
	case BrushType::Solid:
		{
			CreateSolidBrush(target, m_StrokeBrush, m_StrokeColor);
		}
		break;

	case BrushType::LinearGradient:
		{
			auto collection = CreateGradientStopCollection(target, m_StrokeGradientStops, m_StrokeGradientAltGamma);
			CreateLinearGradient(target, collection, m_StrokeBrush, m_StrokeLinearGradientAngle);
			if (collection) collection->Release();
		}
		break;

	case BrushType::RadialGradient:
		{
			auto collection = CreateGradientStopCollection(target, m_StrokeGradientStops, m_StrokeGradientAltGamma);
			CreateRadialGradient(target, collection, m_StrokeBrush, true);
			if (collection) collection->Release();
		}
		break;

	default:
		return nullptr;
	}

	m_HasStrokeBrushChanged = false;
	return m_StrokeBrush;
}

ID2D1GradientStopCollection* Shape::CreateGradientStopCollection(ID2D1RenderTarget* target,
	std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma)
{
	if (stops.empty()) return nullptr;

	ID2D1GradientStopCollection* collection;
	HRESULT hr = target->CreateGradientStopCollection(
		&stops[0],
		(UINT32)stops.size(),
		altGamma ? D2D1_GAMMA_1_0 : D2D1_GAMMA_2_2,
		D2D1_EXTEND_MODE_CLAMP,
		&collection);
	if (FAILED(hr)) return nullptr;

	return collection;
}

void Shape::CreateSolidBrush(ID2D1RenderTarget* target, Microsoft::WRL::ComPtr<ID2D1Brush>& brush,
	const D2D1_COLOR_F& color)
{
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solid;
	HRESULT hr = target->CreateSolidColorBrush(color, solid.GetAddressOf());

	if (SUCCEEDED(hr)) solid.CopyTo(brush.ReleaseAndGetAddressOf());
}

void Shape::CreateLinearGradient(ID2D1RenderTarget* target, ID2D1GradientStopCollection* collection,
	Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const FLOAT angle)
{
	auto bounds = GetBounds(false);
	D2D1_POINT_2F start = Util::FindEdgePoint(angle,
		bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
	D2D1_POINT_2F end = Util::FindEdgePoint(angle + 180.0f,
		bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);

	Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> linear;
	HRESULT hr = target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(start, end),
		collection,
		linear.GetAddressOf());

	if (SUCCEEDED(hr)) linear.CopyTo(brush.ReleaseAndGetAddressOf());
}

void Shape::CreateRadialGradient(ID2D1RenderTarget* target, ID2D1GradientStopCollection* collection,
	Microsoft::WRL::ComPtr<ID2D1Brush>& brush, bool isStroke)
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
	center = Util::AddPoint2F(center, isStroke ? m_StrokeRadialGradientCenter : m_FillRadialGradientCenter);

	// Check if offset and radii are defined
	swapIfNotDefined(offset, isStroke ? m_StrokeRadialGradientOffset : m_FillRadialGradientOffset);
	swapIfNotDefined(radius, isStroke ? m_StrokeRadialGradientRadius : m_FillRadialGradientRadius);

	Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> radial;
	HRESULT hr = target->CreateRadialGradientBrush(
		D2D1::RadialGradientBrushProperties(
			center,
			offset,
			radius.x,
			radius.y),
		collection,
		radial.GetAddressOf());

	if (SUCCEEDED(hr)) radial.CopyTo(brush.ReleaseAndGetAddressOf());
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

	otherShape->m_FillBrushType = m_FillBrushType;
	otherShape->m_FillColor = m_FillColor;
	otherShape->m_FillLinearGradientAngle = m_FillLinearGradientAngle;
	otherShape->m_FillRadialGradientOffset = m_FillRadialGradientOffset;
	otherShape->m_FillRadialGradientCenter = m_FillRadialGradientCenter;
	otherShape->m_FillRadialGradientRadius = m_FillRadialGradientRadius;
	otherShape->m_FillGradientStops = m_FillGradientStops;
	otherShape->m_FillGradientAltGamma = m_FillGradientAltGamma;

	otherShape->m_StrokeBrushType = m_StrokeBrushType;
	otherShape->m_StrokeColor = m_StrokeColor;
	otherShape->m_StrokeLinearGradientAngle = m_StrokeLinearGradientAngle;
	otherShape->m_StrokeRadialGradientOffset = m_StrokeRadialGradientOffset;
	otherShape->m_StrokeRadialGradientCenter = m_StrokeRadialGradientCenter;
	otherShape->m_StrokeRadialGradientRadius = m_StrokeRadialGradientRadius;
	otherShape->m_StrokeGradientStops = m_StrokeGradientStops;
	otherShape->m_StrokeGradientAltGamma = m_StrokeGradientAltGamma;
	
	// Re-create brushes on next draw
	otherShape->m_HasFillBrushChanged = true;
	otherShape->m_HasStrokeBrushChanged = true;
}

}  // namespace Gfx
