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
	gradient.reset(other.gradient ? new GradientData(*other.gradient) : nullptr);
	Invalidate();
}

Shape::GradientData& Shape::BrushData::GetGradientData()
{
	if (!gradient)
	{
		gradient.reset(new GradientData());
	}

	return *gradient;
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

Shape::Shape(ShapeType type) :
	m_ShapeType(type)
{
}

Shape::Shape(const Shape& other) :
	m_ShapeType(other.m_ShapeType),
	m_IsCombined(other.m_IsCombined),
	m_StrokeType(other.m_StrokeType),
	m_TransformModifiers(other.m_TransformModifiers ? new TransformModifiers(*other.m_TransformModifiers) : nullptr),
	m_StrokeData(other.m_StrokeData ? new StrokeData() : nullptr),
	m_PathData(other.m_PathData ? new PathData(*other.m_PathData) : nullptr),
	m_Geometry(other.m_Geometry)
{
	m_Fill.CopyFrom(other.m_Fill);
	if (m_StrokeData)
	{
		m_StrokeData->CopyFrom(*other.m_StrokeData);
		CreateStrokeStyle();
	}
}

Shape::~Shape()
{
}

std::optional<Shape> Shape::Rectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height)
{
	Shape shape(ShapeType::Rectangle);
	const D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
	HRESULT hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, (ID2D1RectangleGeometry**)shape.m_Geometry.GetAddressOf());
	if (FAILED(hr)) return std::nullopt;
	return shape;
}

std::optional<Shape> Shape::RoundedRectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT xRadius, FLOAT yRadius)
{
	Shape shape(ShapeType::RoundedRectangle);
	const D2D1_ROUNDED_RECT rect = { D2D1::RectF(x, y, x + width, y + height), xRadius, yRadius };
	HRESULT hr = Canvas::c_D2DFactory->CreateRoundedRectangleGeometry(
		rect, (ID2D1RoundedRectangleGeometry**)shape.m_Geometry.GetAddressOf());
	if (FAILED(hr)) return std::nullopt;
	return shape;
}

std::optional<Shape> Shape::Ellipse(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius)
{
	Shape shape(ShapeType::Ellipse);
	const D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), xRadius, yRadius);
	HRESULT hr = Canvas::c_D2DFactory->CreateEllipseGeometry(ellipse, (ID2D1EllipseGeometry**)shape.m_Geometry.GetAddressOf());
	if (FAILED(hr)) return std::nullopt;
	return shape;
}

std::optional<Shape> Shape::Line(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2)
{
	auto shape = Path(x1, y1, D2D1_FILL_MODE_ALTERNATE);
	if (!shape) return std::nullopt;
	shape->m_ShapeType = ShapeType::Line;
	shape->AddPathLine(x2, y2);
	shape->ClosePath(D2D1_FIGURE_END_OPEN);
	return shape;
}

std::optional<Shape> Shape::Arc(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT xRadius, FLOAT yRadius, FLOAT angle, D2D1_SWEEP_DIRECTION sweep, D2D1_ARC_SIZE size, D2D1_FIGURE_END ending)
{
	auto shape = Path(x1, y1, D2D1_FILL_MODE_ALTERNATE);
	if (!shape) return std::nullopt;
	shape->m_ShapeType = ShapeType::Arc;
	shape->AddPathArc(x2, y2, xRadius, yRadius, angle, sweep, size);
	shape->ClosePath(ending);
	return shape;
}

std::optional<Shape> Shape::Curve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2, D2D1_FIGURE_END ending)
{
	auto shape = Path(x1, y1, D2D1_FILL_MODE_ALTERNATE);
	if (!shape) return std::nullopt;
	shape->m_ShapeType = ShapeType::Curve;
	shape->AddPathCubicCurve(x2, y2, cx1, cy1, cx2, cy2);
	shape->ClosePath(ending);
	return shape;
}

std::optional<Shape> Shape::QuadraticCurve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT cx, FLOAT cy, D2D1_FIGURE_END ending)
{
	auto shape = Path(x1, y1, D2D1_FILL_MODE_ALTERNATE);
	if (!shape) return std::nullopt;
	shape->m_ShapeType = ShapeType::QuadraticCurve;
	shape->AddPathQuadraticCurve(x2, y2, cx, cy);
	shape->ClosePath(ending);
	return shape;
}

std::optional<Shape> Shape::Path(FLOAT x, FLOAT y, D2D1_FILL_MODE fillMode)
{
	Shape shape(ShapeType::Path);
	auto& data = shape.GetPathData();
	HRESULT hr = Canvas::c_D2DFactory->CreatePathGeometry(data.path.GetAddressOf());
	if (FAILED(hr)) return std::nullopt;

	hr = data.path->Open(data.sink.GetAddressOf());
	if (FAILED(hr)) return std::nullopt;

	data.sink->SetFillMode(fillMode);
	data.sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED);

	return shape;
}

void Shape::AddPathLine(FLOAT x, FLOAT y)
{
	if (m_PathData && m_PathData->path && m_PathData->sink) m_PathData->sink->AddLine(D2D1::Point2F(x, y));
}

void Shape::AddPathArc(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius, FLOAT angle, D2D1_SWEEP_DIRECTION direction, D2D1_ARC_SIZE arcSize)
{
	xRadius = xRadius < 0.0f ? 0.0f : xRadius;
	yRadius = yRadius < 0.0f ? 0.0f : yRadius;
	if (m_PathData && m_PathData->path && m_PathData->sink)
	{
		m_PathData->sink->AddArc(D2D1::ArcSegment(
			D2D1::Point2F(x, y), D2D1::SizeF(xRadius, yRadius), angle, direction, arcSize));
	}
}

void Shape::AddPathQuadraticCurve(FLOAT x, FLOAT y, FLOAT cx, FLOAT cy)
{
	if (m_PathData && m_PathData->path && m_PathData->sink)
	{
		m_PathData->sink->AddQuadraticBezier(
			D2D1::QuadraticBezierSegment(D2D1::Point2F(cx, cy), D2D1::Point2F(x, y)));
	}
}

void Shape::AddPathCubicCurve(FLOAT x, FLOAT y, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2)
{
	if (m_PathData && m_PathData->path && m_PathData->sink)
	{
		m_PathData->sink->AddBezier(D2D1::BezierSegment(
			D2D1::Point2F(cx1, cy1), D2D1::Point2F(cx2, cy2), D2D1::Point2F(x, y)));
	}
}

void Shape::SetPathSegmentFlags(D2D1_PATH_SEGMENT flags)
{
	if (m_PathData && m_PathData->path && m_PathData->sink) m_PathData->sink->SetSegmentFlags(flags);
}

void Shape::ClosePath(D2D1_FIGURE_END ending)
{
	if (!m_PathData || !m_PathData->path || !m_PathData->sink) return;
	m_PathData->sink->EndFigure(ending);
	m_PathData->sink->Close();
	m_Geometry = std::move(m_PathData->path);
	m_PathData.reset();
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
	HRESULT hr = m_Geometry->GetWidenedBounds(GetStrokeWidth(), nullptr, nullptr, &bounds);
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

	HRESULT hr = m_Geometry->GetWidenedBounds(
		GetStrokeWidth(),
		GetStrokeStyle(),
		matrix,
		&strokedBounds);
	if (FAILED(hr)) return D2D1::RectF();

	hr = m_Geometry->GetBounds(matrix, &fillBounds);
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
	return m_Geometry;
}

bool Shape::ContainsPoint(D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F& transformationMatrix)
{
	D2D1_MATRIX_3X2_F matrix = transformationMatrix;

	matrix = matrix * GetShapeMatrix();

	BOOL contains = FALSE;
	HRESULT hr = m_Geometry->StrokeContainsPoint(
		point,
		GetStrokeWidth(),
		GetStrokeStyle(),
		matrix,
		&contains);
	if (SUCCEEDED(hr) && contains) return true;

	hr = m_Geometry->FillContainsPoint(point, matrix, &contains);
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
		hr = m_Geometry->CombineWithGeometry(
			otherShape->m_Geometry.Get(),
			mode,
			otherShape->GetShapeMatrix(),
			sink.Get());
		if (FAILED(hr)) return false;

		sink->Close();

		m_Geometry = std::move(path);

		return true;
	}

	static const D2D1_RECT_F rect = { 0.0f, 0.0f, 0.0f, 0.0f };
	Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> emptyShape;
	hr = Canvas::c_D2DFactory->CreateRectangleGeometry(rect, emptyShape.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = emptyShape->CombineWithGeometry(m_Geometry.Get(), mode, GetShapeMatrix(), sink.Get());

	sink->Close();

	if (FAILED(hr)) return false;

	m_Geometry = std::move(path);

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

	auto& stroke = GetStrokeData();
	stroke.width = strokeWidth;
	m_StrokeType = StrokeType::Custom;
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
	gradient.reset();
	changed = true;
}

void Shape::BrushData::Set(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	auto& gradient = GetGradientData();
	type = BrushType::LinearGradient;
	gradient.linearAngle = angle;
	gradient.stops = std::move(stops);
	gradient.altGamma = altGamma;
	changed = true;
}

void Shape::BrushData::Set(D2D1_POINT_2F offset, D2D1_POINT_2F center,
	D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma)
{
	auto& gradient = GetGradientData();
	type = BrushType::RadialGradient;
	gradient.radialOffset = offset;
	gradient.radialCenter = center;
	gradient.radialRadius = radius;
	gradient.stops = std::move(stops);
	gradient.altGamma = altGamma;
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
			auto& gradient = *data.gradient;
			auto collection = CreateGradientStopCollection(target, gradient.stops, gradient.altGamma);
			CreateLinearGradient(target, collection, data.brush, gradient.linearAngle);
			if (collection) collection->Release();
		}
		break;

	case BrushType::RadialGradient:
		{
			auto& gradient = *data.gradient;
			auto collection = CreateGradientStopCollection(target, gradient.stops, gradient.altGamma);
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
	const auto& gradient = *data.gradient;
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
	center = Util::AddPoint2F(center, gradient.radialCenter);

	// Check if offset and radii are defined
	swapIfNotDefined(offset, gradient.radialOffset);
	swapIfNotDefined(radius, gradient.radialRadius);

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
	if (m_StrokeType == StrokeType::Default)
	{
		m_StrokeType = StrokeType::Custom;
	}

	if (!m_StrokeData)
	{
		m_StrokeData.reset(new StrokeData());
	}

	return *m_StrokeData;
}

Shape::PathData& Shape::GetPathData()
{
	if (!m_PathData)
	{
		m_PathData.reset(new PathData());
	}

	return *m_PathData;
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
