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

void Shape::BrushData::CopyFrom(const BrushData& other)
{
	type = other.type;
	color = other.color;
	linearGradientAngle = other.linearGradientAngle;
	radialGradientOffset = other.radialGradientOffset;
	radialGradientCenter = other.radialGradientCenter;
	radialGradientRadius = other.radialGradientRadius;
	gradientStops = other.gradientStops;
	gradientAltGamma = other.gradientAltGamma;
	Invalidate();
}

void Shape::BrushData::Invalidate()
{
	brush.Reset();
	changed = true;
}

void Shape::StrokeData::CopyFrom(const StrokeData& other)
{
	fill.CopyFrom(other.fill);
	style.Reset();
	customDashes = other.customDashes;
	properties = other.properties;
	width = other.width;
}

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
	m_StrokeType(StrokeType::Default),
	m_TransformModifiers(),
	m_StrokeData(),
	m_Fill(D2D1::ColorF(D2D1::ColorF::White))
{
}

Shape::~Shape()
{
}

void Shape::InvalidateDeviceResources()
{
	m_Fill.Invalidate();
	if (m_StrokeData) m_StrokeData->fill.Invalidate();
}

D2D1_MATRIX_3X2_F Shape::GetShapeMatrix()
{
	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Identity();
	if (!m_TransformModifiers) return matrix;

	const auto& modifiers = *m_TransformModifiers;

	D2D1_RECT_F bounds;
	HRESULT hr = m_Shape->GetWidenedBounds(GetStrokeWidth(), nullptr, nullptr, &bounds);
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
		GetStrokeWidth(),
		GetStrokeStyle(),
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
		GetStrokeWidth(),
		GetStrokeStyle(),
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

void Shape::SetStrokeWidth(FLOAT strokeWidth)
{
	if (strokeWidth == 0.0f)
	{
		m_StrokeData.reset();
		m_StrokeType = StrokeType::Disabled;
		return;
	}

	GetStrokeData().width = strokeWidth;
}

void Shape::CreateStrokeStyle(D2D1_STROKE_TRANSFORM_TYPE transformType)
{
	if (m_StrokeType == StrokeType::Disabled) return;

	if (!m_StrokeData)
	{
		if (transformType == D2D1_STROKE_TRANSFORM_TYPE_FIXED) return;
		GetStrokeData();
	}

	auto& stroke = *m_StrokeData;
	const FLOAT* dashes = nullptr;
	if (!stroke.customDashes.empty())
	{
		stroke.properties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
		dashes = stroke.customDashes.data();
	}

	stroke.properties.transformType = transformType;

	UINT32 dashCount = (UINT32)stroke.customDashes.size();
	HRESULT hr = Canvas::c_D2DFactory->CreateStrokeStyle(
		stroke.properties,
		dashes,
		dashCount,
		stroke.style.ReleaseAndGetAddressOf());

	// If failed, make sure stroke is null
	if (FAILED(hr)) stroke.style = nullptr;
}

void Shape::SetFill(const D2D1_COLOR_F& color)
{
	m_Fill.Set(color);
}

void Shape::SetFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_Fill.Set(angle, std::move(stops), altGamma);
}

void Shape::SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	m_Fill.Set(offset, center, radius, std::move(stops), altGamma);
}

void Shape::SetStrokeFill(const D2D1_COLOR_F& color)
{
	GetStrokeData().fill.Set(color);
}

void Shape::SetStrokeFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	GetStrokeData().fill.Set(angle, std::move(stops), altGamma);
}

void Shape::SetStrokeFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	GetStrokeData().fill.Set(offset, center, radius, std::move(stops), altGamma);
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetFillBrush(ID2D1DeviceContext* target)
{
	return GetBrush(target, m_Fill);
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetStrokeFillBrush(ID2D1DeviceContext* target)
{
	if (m_StrokeType == StrokeType::Disabled) return nullptr;
	return GetBrush(target, GetStrokeData().fill);
}

void Shape::BrushData::Set(const D2D1_COLOR_F& color)
{
	type = BrushType::Solid;
	this->color = color;
	changed = true;
}

void Shape::BrushData::Set(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	type = BrushType::LinearGradient;
	linearGradientAngle = angle;
	gradientStops = std::move(stops);
	gradientAltGamma = altGamma;
	changed = true;
}

void Shape::BrushData::Set(D2D1_POINT_2F offset, D2D1_POINT_2F center,
	D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	type = BrushType::RadialGradient;
	radialGradientOffset = offset;
	radialGradientCenter = center;
	radialGradientRadius = radius;
	gradientStops = std::move(stops);
	gradientAltGamma = altGamma;
	changed = true;
}

Microsoft::WRL::ComPtr<ID2D1Brush> Shape::GetBrush(ID2D1DeviceContext* target, BrushData& data)
{
	if (!data.changed) return data.brush;

	switch (data.type)
	{
	case BrushType::Solid:
		{
			CreateSolidBrush(target, data.brush, data.color);
		}
		break;

	case BrushType::LinearGradient:
		{
			auto collection = CreateGradientStopCollection(target, data.gradientStops, data.gradientAltGamma);
			CreateLinearGradient(target, collection, data.brush, data.linearGradientAngle);
			if (collection) collection->Release();
		}
		break;

	case BrushType::RadialGradient:
		{
			auto collection = CreateGradientStopCollection(target, data.gradientStops, data.gradientAltGamma);
			CreateRadialGradient(target, collection, data);
			if (collection) collection->Release();
		}
		break;

	default:
		return nullptr;
	}

	data.changed = false;
	return data.brush;
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

void Shape::CreateRadialGradient(
	ID2D1DeviceContext* target, ID2D1GradientStopCollection* collection, BrushData& data)
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
	center = Util::AddPoint2F(center, data.radialGradientCenter);

	// Check if offset and radii are defined
	swapIfNotDefined(offset, data.radialGradientOffset);
	swapIfNotDefined(radius, data.radialGradientRadius);

	Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> radial;
	HRESULT hr = target->CreateRadialGradientBrush(
		D2D1::RadialGradientBrushProperties(
			center,
			offset,
			radius.x,
			radius.y),
		collection,
		radial.GetAddressOf());

	if (SUCCEEDED(hr)) data.brush = std::move(radial);
}

void Shape::ResetTransformOrder()
{
	if (m_TransformModifiers) m_TransformModifiers->order.fill(TransformType::Invalid);
}

bool Shape::AddToTransformOrder(TransformType type)
{
	// Don't add if 'type' is a duplicate
	auto& order = GetTransformModifiers().order;
	for (const auto& t : order) if (t == type) return false;

	for (auto& t : order)
	{
		if (t == TransformType::Invalid)
		{
			t = type;
			return true;
		}
	}

	return false;
}

void Shape::ValidateTransforms()
{
	if (!m_TransformModifiers) return;

	// Add any missing transforms
	AddToTransformOrder(TransformType::Rotate);
	AddToTransformOrder(TransformType::Scale);
	AddToTransformOrder(TransformType::Skew);
	AddToTransformOrder(TransformType::Offset);
}

void Shape::CloneModifiers(Shape* otherShape)
{
	otherShape->m_StrokeType = m_StrokeType;
	otherShape->m_TransformModifiers.reset(
		m_TransformModifiers ? new TransformModifiers(*m_TransformModifiers) : nullptr);

	if (m_StrokeData)
	{
		otherShape->m_StrokeData.reset(new StrokeData());
		otherShape->m_StrokeData->CopyFrom(*m_StrokeData);
		otherShape->CreateStrokeStyle();
	}
	else
	{
		otherShape->m_StrokeData.reset();
	}

	otherShape->m_Fill.CopyFrom(m_Fill);
}

Shape::TransformModifiers& Shape::GetTransformModifiers()
{
	if (!m_TransformModifiers)
	{
		m_TransformModifiers.reset(new TransformModifiers());
	}

	return *m_TransformModifiers;
}

Shape::StrokeData& Shape::GetStrokeData()
{
	m_StrokeType = StrokeType::Custom;

	if (!m_StrokeData)
	{
		m_StrokeData.reset(new StrokeData());
	}

	return *m_StrokeData;
}

FLOAT Shape::GetStrokeWidth() const
{
	switch (m_StrokeType)
	{
	case StrokeType::Disabled: return 0.0f;
	case StrokeType::Custom: return m_StrokeData->width;
	default: return 1.0f;
	}
}

ID2D1StrokeStyle1* Shape::GetStrokeStyle() const
{
	return m_StrokeType == StrokeType::Custom ? m_StrokeData->style.Get() : nullptr;
}

}  // namespace Gfx
