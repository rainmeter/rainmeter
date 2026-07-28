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

Shape::TransformModifiers::TransformModifiers() :
	order(),
	offset(D2D1::SizeF(0.0f, 0.0f)),
	rotation(0.0f),
	rotationAnchor(D2D1::Point2F(0.0f, 0.0f)),
	rotationAnchorDefined(false),
	skew(D2D1::Point2F(0.0f, 0.0f)),
	skewAnchor(D2D1::Point2F(0.0f, 0.0f)),
	skewAnchorDefined(false),
	scale(D2D1::SizeF(1.0f, 1.0f)),
	scaleAnchor(D2D1::Point2F(0.0f, 0.0f)),
	scaleAnchorDefined(false)
{
}

Shape::Shape(ShapeType type) :
	m_ShapeType(type),
	m_IsCombined(false),
	m_TransformModifiers(),
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
}

Shape::~Shape()
{
}

void Shape::InvalidateDeviceResources()
{
	m_FillBrush.Reset();
	m_StrokeBrush.Reset();
	m_HasFillBrushChanged = true;
	m_HasStrokeBrushChanged = true;
}

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix()
{
	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
	if (!m_TransformModifiers) return matrix;

	const auto& modifiers = *m_TransformModifiers;

	D2D1_RECT_F bounds;
	HRESULT hr = m_Shape->GetWidenedBounds(m_StrokeWidth, nullptr, nullptr, &bounds);
	if (FAILED(hr)) return matrix;

	D2D1_POINT_2F point = D2D1::Point2F(bounds.left, bounds.top);

	// Use the center of the shape as the default anchor point for all transforms
	D2D1_POINT_2F center = D2D1::Point2F((bounds.right - bounds.left) / 2.0f, (bounds.bottom - bounds.top) / 2.0f);

	D2D1_POINT_2F rotationPoint = modifiers.rotationAnchorDefined ? modifiers.rotationAnchor : center;
	rotationPoint = Util::AddPoint2F(point, rotationPoint);

	D2D1_POINT_2F scalePoint = modifiers.scaleAnchorDefined ? modifiers.scaleAnchor : center;
	scalePoint = Util::AddPoint2F(point, scalePoint);

	D2D1_POINT_2F skewPoint = modifiers.skewAnchorDefined ? modifiers.skewAnchor : center;
	skewPoint = Util::AddPoint2F(point, skewPoint);

	for (const auto& type : modifiers.order)
	{
		switch (type)
		{
		case TransformType::Rotate:
			matrix = matrix * D2D1::Matrix3x2F::Rotation(modifiers.rotation, rotationPoint);
			break;

		case TransformType::Scale:
			matrix = matrix * D2D1::Matrix3x2F::Scale(modifiers.scale, scalePoint);
			break;

		case TransformType::Skew:
			matrix = matrix * D2D1::Matrix3x2F::Skew(modifiers.skew.x, modifiers.skew.y, skewPoint);
			break;

		case TransformType::Offset:
			matrix = matrix * D2D1::Matrix3x2F::Translation(modifiers.offset);
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

bool Shape::ContainsPoint(D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F& transformationMatrix)
{
	D2D1_MATRIX_3X2_F matrix = transformationMatrix;

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

		m_Shape = std::move(path);

		return true;
	}

	static const D2D1_RECT_F rect = { 0.0f, 0.0f, 0.0f, 0.0f };
	Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> emptyShape;
	hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, emptyShape.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = emptyShape->CombineWithGeometry(m_Shape.Get(), mode, GetShapeMatrix(), sink.Get());

	sink->Close();

	if (FAILED(hr)) return false;

	m_Shape = std::move(path);

	m_TransformModifiers.reset();

	return true;
}

void Shape::SetRotation(FLOAT rotation, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	auto& modifiers = GetTransformModifiers();
	modifiers.rotation = rotation;

	modifiers.rotationAnchor.x = anchorX;
	modifiers.rotationAnchor.y = anchorY;
	modifiers.rotationAnchorDefined = anchorDefined;
}

void Shape::SetScale(FLOAT scaleX, FLOAT scaleY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	auto& modifiers = GetTransformModifiers();
	modifiers.scale.width = scaleX;
	modifiers.scale.height = scaleY;

	modifiers.scaleAnchor.x = anchorX;
	modifiers.scaleAnchor.y = anchorY;
	modifiers.scaleAnchorDefined = anchorDefined;
}

void Shape::SetSkew(FLOAT skewX, FLOAT skewY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined)
{
	auto& modifiers = GetTransformModifiers();
	modifiers.skew.x = skewX;
	modifiers.skew.y = skewY;

	modifiers.skewAnchor.x = anchorX;
	modifiers.skewAnchor.y = anchorY;
	modifiers.skewAnchorDefined = anchorDefined;
}

void Shape::CreateStrokeStyle(D2D1_STROKE_TRANSFORM_TYPE transformType)
{
	const FLOAT* dashes = nullptr;
	if (!m_StrokeCustomDashes.empty())
	{
		m_StrokeProperties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
		dashes = m_StrokeCustomDashes.data();
	}

	m_StrokeProperties.transformType = transformType;

	UINT32 dashCount = (UINT32)m_StrokeCustomDashes.size();
	HRESULT hr = Canvas::c_D2DFactory->CreateStrokeStyle(
		m_StrokeProperties,
		dashes,
		dashCount,
		m_StrokeStyle.ReleaseAndGetAddressOf());

	// If failed, make sure stroke is null
	if (FAILED(hr)) m_StrokeStyle = nullptr;
}

void Shape::SetFill(const D2D1_COLOR_F& color)
{
	m_FillBrushType = BrushType::Solid;
	m_FillColor = color;
	m_HasFillBrushChanged = true;
}

void Shape::SetFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_FillBrushType = BrushType::LinearGradient;
	m_FillLinearGradientAngle = angle;
	m_FillGradientStops = std::move(stops);
	m_FillGradientAltGamma = altGamma;
	m_HasFillBrushChanged = true;
}

void Shape::SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_FillBrushType = BrushType::RadialGradient;
	m_FillRadialGradientOffset = offset;
	m_FillRadialGradientCenter = center;
	m_FillRadialGradientRadius = radius;
	m_FillGradientStops = std::move(stops);
	m_FillGradientAltGamma = altGamma;
	m_HasFillBrushChanged = true;
}

void Shape::SetStrokeFill(const D2D1_COLOR_F& color)
{
	m_StrokeBrushType = BrushType::Solid;
	m_StrokeColor = color;
	m_HasStrokeBrushChanged = true;
}

void Shape::SetStrokeFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_StrokeBrushType = BrushType::LinearGradient;
	m_StrokeLinearGradientAngle = angle;
	m_StrokeGradientStops = std::move(stops);
	m_StrokeGradientAltGamma = altGamma;
	m_HasStrokeBrushChanged = true;
}

void Shape::SetStrokeFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_StrokeBrushType = BrushType::RadialGradient;
	m_StrokeRadialGradientOffset = offset;
	m_StrokeRadialGradientCenter = center;
	m_StrokeRadialGradientRadius = radius;
	m_StrokeGradientStops = std::move(stops);
	m_StrokeGradientAltGamma = altGamma;
	m_HasStrokeBrushChanged = true;
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetFillBrush(ID2D1DeviceContext* target)
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

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetStrokeFillBrush(ID2D1DeviceContext* target)
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

ID2D1GradientStopCollection* Shape::CreateGradientStopCollection(ID2D1DeviceContext* target,
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

void Shape::CreateSolidBrush(ID2D1DeviceContext* target, Microsoft::WRL::ComPtr<ID2D1Brush>& brush,
	const D2D1_COLOR_F& color)
{
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solid;
	HRESULT hr = target->CreateSolidColorBrush(color, solid.GetAddressOf());

	if (SUCCEEDED(hr)) brush = std::move(solid);
}

void Shape::CreateLinearGradient(ID2D1DeviceContext* target, ID2D1GradientStopCollection* collection,
	Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const FLOAT angle)
{
	auto bounds = GetBounds(false);
	D2D1_POINT_2F start = Util::FindEdgePoint(angle,
		bounds.left, bounds.top, bounds.right, bounds.bottom);
	D2D1_POINT_2F end = Util::FindEdgePoint(angle + 180.0f,
		bounds.left, bounds.top, bounds.right, bounds.bottom);

	Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> linear;
	HRESULT hr = target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(start, end),
		collection,
		linear.GetAddressOf());

	if (SUCCEEDED(hr)) brush = std::move(linear);
}

void Shape::CreateRadialGradient(ID2D1DeviceContext* target, ID2D1GradientStopCollection* collection,
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

	if (SUCCEEDED(hr)) brush = std::move(radial);
}

void Shape::ResetTransformOrder()
{
	if (m_TransformModifiers) m_TransformModifiers->order.clear();
}

bool Shape::AddToTransformOrder(TransformType type)
{
	// Don't add if 'type' is a duplicate
	if (m_TransformModifiers)
	{
		for (const auto& t : m_TransformModifiers->order) if (t == type) return false;
	}

	GetTransformModifiers().order.emplace_back(type);
	return true;
}

void Shape::ValidateTransforms()
{
	if (!m_TransformModifiers) return;

	// There should be no duplicates, but make sure the order is not larger
	// than the defined amount of transforms
	auto& order = m_TransformModifiers->order;
	while (order.size() >= (size_t)TransformType::MAX) order.pop_back();

	// Add any missing transforms
	AddToTransformOrder(TransformType::Rotate);
	AddToTransformOrder(TransformType::Scale);
	AddToTransformOrder(TransformType::Skew);
	AddToTransformOrder(TransformType::Offset);
}

void Shape::CloneModifiers(Shape* otherShape)
{
	otherShape->m_StrokeWidth = m_StrokeWidth;
	otherShape->m_StrokeProperties = m_StrokeProperties;
	otherShape->m_StrokeCustomDashes = m_StrokeCustomDashes;
	otherShape->m_TransformModifiers.reset(
		m_TransformModifiers ? new TransformModifiers(*m_TransformModifiers) : nullptr);

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

Shape::TransformModifiers& Shape::GetTransformModifiers()
{
	if (!m_TransformModifiers)
	{
		m_TransformModifiers.reset(new TransformModifiers());
	}

	return *m_TransformModifiers;
}

}  // namespace Gfx
