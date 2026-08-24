// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Util/D2DUtil.h"
#include <array>
#include <d2d1_1.h>
#include <memory>
#include <optional>
#include <wrl/client.h>
#include <vector>

namespace Gfx {

class Canvas;

enum class TransformType : BYTE
{
	Invalid = 0,
	Rotate,
	Scale,
	Skew,
	Offset,

	MAX  // number of transforms
};

enum class BrushType : BYTE
{
	None = 0,
	Solid,
	LinearGradient,
	RadialGradient,
	//Image
};

class Shape
{
public:
	Shape(const Shape& other);
	Shape(Shape&& other) = default;
	Shape& operator=(Shape&& other) = default;
	~Shape();

	static Shape None();
	static std::optional<Shape> Rectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height);
	static std::optional<Shape> RoundedRectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT xRadius, FLOAT yRadius);
	static std::optional<Shape> Ellipse(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius);
	static std::optional<Shape> Line(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2);
	static std::optional<Shape> Arc(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT xRadius, FLOAT yRadius, FLOAT angle, D2D1_SWEEP_DIRECTION sweep, D2D1_ARC_SIZE size, D2D1_FIGURE_END ending);
	static std::optional<Shape> Curve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2, D2D1_FIGURE_END ending);
	static std::optional<Shape> QuadraticCurve(FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT cx, FLOAT cy, D2D1_FIGURE_END ending);
	static std::optional<Shape> Path(FLOAT x, FLOAT y, D2D1_FILL_MODE fillMode);

	void InvalidateDeviceResources();

	D2D1_MATRIX_3X2_F GetShapeMatrix() const;
	D2D1_RECT_F GetBounds(bool useMatrix = true) const;
	bool IsShapeDefined();
	bool ContainsPoint(D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F& transformationMatrix = D2D1::Matrix3x2F::Identity());

	bool IsCombined() const { return m_IsCombined; }
	void SetCombined(bool combined = true) { m_IsCombined = combined; }
	bool CombineWith(Shape* otherShape, D2D1_COMBINE_MODE mode);

	void SetOffset(FLOAT x, FLOAT y) { GetTransformModifiers().offset = D2D1::SizeF(x, y); }
	void SetStrokeWidth(FLOAT strokeWidth);

	void SetRotation(FLOAT rotation, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetScale(FLOAT scaleX, FLOAT scaleY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetSkew(FLOAT skewX, FLOAT skewY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);

	D2D1_STROKE_STYLE_PROPERTIES1& GetStrokeProperties() { return GetStrokeData().properties; }
	void SetStrokeDashes(std::vector<FLOAT> dashes) { GetStrokeData().customDashes = std::move(dashes); }
	void CreateStrokeStyle(D2D1_STROKE_TRANSFORM_TYPE transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED);

	void SetFill(const D2D1_COLOR_F& color);
	void SetFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	void SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	Microsoft::WRL::ComPtr<ID2D1Brush> GetFillBrush(ID2D1DeviceContext* target);

	void SetStrokeFill(const D2D1_COLOR_F& color);
	void SetStrokeFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	void SetStrokeFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	Microsoft::WRL::ComPtr<ID2D1Brush> GetStrokeFillBrush(ID2D1DeviceContext* target);

	void ResetTransformOrder();
	bool AddToTransformOrder(TransformType type);
	void ValidateTransforms();

	// Only valid for path shapes:
	void AddPathLine(FLOAT x, FLOAT y);
	void AddPathArc(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius, FLOAT angle, D2D1_SWEEP_DIRECTION direction, D2D1_ARC_SIZE arcSize);
	void AddPathQuadraticCurve(FLOAT x, FLOAT y, FLOAT cx, FLOAT cy);
	void AddPathCubicCurve(FLOAT x, FLOAT y, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2);
	void SetPathSegmentFlags(D2D1_PATH_SEGMENT flags);
	void ClosePath(D2D1_FIGURE_END ending);

private:
	friend class Canvas;

	Shape() = default;
	Shape& operator=(const Shape& other) = delete;

	enum class StrokeType : BYTE
	{
		Default = 0,
		Custom,
		Disabled
	};

	struct GradientData
	{
		FLOAT linearAngle = 0.0f;
		D2D1_POINT_2F radialOffset = D2D1::Point2F(0.0f, 0.0f);
		D2D1_POINT_2F radialCenter = D2D1::Point2F(0.0f, 0.0f);
		D2D1_POINT_2F radialRadius = D2D1::Point2F(0.0f, 0.0f);
		std::vector<D2D1_GRADIENT_STOP> stops;
		bool altGamma = false;
	};

	struct BrushData
	{
		BrushData(const D2D1_COLOR_F& defaultColor) : color(defaultColor) {}
		void CopyFrom(const BrushData& other);
		GradientData& GetGradientData();
		void Invalidate();
		void Set(const D2D1_COLOR_F& color);
		void Set(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
		void Set(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius,
			std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);

		BrushType type = BrushType::Solid;
		D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::White);
		std::unique_ptr<GradientData> gradient{};
		Microsoft::WRL::ComPtr<ID2D1Brush> brush;
		bool changed = true;
	};

	struct StrokeData
	{
		void CopyFrom(const StrokeData& other);

		BrushData fill{ D2D1::ColorF(D2D1::ColorF::Black) };
		Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> style;
		std::vector<FLOAT> customDashes;
		D2D1_STROKE_STYLE_PROPERTIES1 properties = D2D1::StrokeStyleProperties1();
		FLOAT width = 1.0f;
	};

	struct PathData
	{
		Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
	};

	Microsoft::WRL::ComPtr<ID2D1Brush> GetBrush(ID2D1DeviceContext* target, BrushData& data);

	void CreateSolidBrush(ID2D1DeviceContext* target, Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const D2D1_COLOR_F& color);
	ID2D1GradientStopCollection* CreateGradientStopCollection(
		ID2D1DeviceContext* target, std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma);
	void CreateLinearGradient(ID2D1DeviceContext* target, ID2D1GradientStopCollection* collection,
		Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const FLOAT angle);
	void CreateRadialGradient(
		ID2D1DeviceContext* target, ID2D1GradientStopCollection* collection, BrushData& data);

	struct TransformModifiers
	{
		std::array<TransformType, (size_t)TransformType::MAX - 1> order{};
		D2D1_SIZE_F offset = D2D1::SizeF(0.0f, 0.0f);

		FLOAT rotation = 0.0f;
		D2D1_POINT_2F rotationAnchor = D2D1::Point2F(0.0f, 0.0f);
		bool rotationAnchorDefined = false;

		D2D1_POINT_2F skew = D2D1::Point2F(0.0f, 0.0f);
		D2D1_POINT_2F skewAnchor = D2D1::Point2F(0.0f, 0.0f);
		bool skewAnchorDefined = false;

		D2D1_SIZE_F scale = D2D1::SizeF(1.0f, 1.0f);
		D2D1_POINT_2F scaleAnchor = D2D1::Point2F(0.0f, 0.0f);
		bool scaleAnchorDefined = false;
	};

	TransformModifiers& GetTransformModifiers();
	StrokeData& GetStrokeData();
	PathData& GetPathData();
	FLOAT GetStrokeWidth() const;
	ID2D1StrokeStyle1* GetStrokeStyle() const;

	bool m_IsCombined = false;
	StrokeType m_StrokeType = StrokeType::Default;

	std::unique_ptr<TransformModifiers> m_TransformModifiers{};
	std::unique_ptr<StrokeData> m_StrokeData{};
	std::unique_ptr<PathData> m_PathData{};

	BrushData m_Fill{ D2D1::ColorF(D2D1::ColorF::White) };
	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Geometry;
};

} // Gfx
