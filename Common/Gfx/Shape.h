/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_

#include "Util/D2DUtil.h"
#include <array>
#include <d2d1_1.h>
#include <memory>
#include <wrl/client.h>
#include <vector>

namespace Gfx {

class Canvas;

enum class ShapeType : BYTE
{
	None = 0,
	Rectangle = 100,
	RoundedRectangle,
	Ellipse,
	Line,
	Arc,
	Curve,
	QuadraticCurve,
	Path
};

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

class __declspec(novtable) Shape
{
public:
	Shape(ShapeType type);
	virtual ~Shape();

	void InvalidateDeviceResources();

	ShapeType GetShapeType() { return m_ShapeType; }

	bool DoesShapeExist() { return m_Shape != nullptr; }

	virtual Shape* Clone() = 0;

	D2D1_MATRIX_3X2_F GetShapeMatrix();
	D2D1_RECT_F GetBounds(bool useMatrix = true);
	bool IsShapeDefined();
	bool ContainsPoint(D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F& transformationMatrix = D2D1::Matrix3x2F::Identity());

	bool IsCombined() { return m_IsCombined; }
	void SetCombined() { m_IsCombined = true; }
	bool CombineWith(Shape* otherShape, D2D1_COMBINE_MODE mode);

	void SetOffset(FLOAT x, FLOAT y) { GetTransformModifiers().offset = D2D1::SizeF(x, y); }
	void SetStrokeWidth(FLOAT strokeWidth) { m_StrokeWidth = strokeWidth; }

	void SetRotation(FLOAT rotation, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetScale(FLOAT scaleX, FLOAT scaleY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetSkew(FLOAT skewX, FLOAT skewY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);

	void SetStrokeStartCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.startCap = cap; }
	void SetStrokeEndCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.endCap = cap; }
	void SetStrokeDashCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.dashCap = cap; }
	void SetStrokeLineJoin(D2D1_LINE_JOIN join, FLOAT limit) { m_StrokeProperties.lineJoin = join; m_StrokeProperties.miterLimit = limit; }
	void SetStrokeDashes(std::vector<FLOAT> dashes) { m_StrokeCustomDashes = std::move(dashes); }
	void SetStrokeDashOffset(FLOAT offset) { m_StrokeProperties.dashOffset = offset; }
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

protected:
	void CloneModifiers(Shape* otherShape);

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;

private:
	friend class Canvas;

	struct BrushData
	{
		BrushData(const D2D1_COLOR_F& defaultColor) : color(defaultColor) {}
		void CopyFrom(const BrushData& other);
		void Invalidate();
		void Set(const D2D1_COLOR_F& color);
		void Set(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
		void Set(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius,
			std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);

		BrushType type = BrushType::Solid;
		D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::White);
		FLOAT linearGradientAngle = 0.0f;
		D2D1_POINT_2F radialGradientOffset = D2D1::Point2F(0.0f, 0.0f);
		D2D1_POINT_2F radialGradientCenter = D2D1::Point2F(0.0f, 0.0f);
		D2D1_POINT_2F radialGradientRadius = D2D1::Point2F(0.0f, 0.0f);
		std::vector<D2D1_GRADIENT_STOP> gradientStops;
		bool gradientAltGamma = false;
		Microsoft::WRL::ComPtr<ID2D1Brush> brush;
		bool changed = true;
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
		TransformModifiers();

		std::array<TransformType, (size_t)TransformType::MAX - 1> order;
		D2D1_SIZE_F offset;

		FLOAT rotation;
		D2D1_POINT_2F rotationAnchor;
		bool rotationAnchorDefined;

		D2D1_POINT_2F skew;
		D2D1_POINT_2F skewAnchor;
		bool skewAnchorDefined;

		D2D1_SIZE_F scale;
		D2D1_POINT_2F scaleAnchor;
		bool scaleAnchorDefined;
	};

	TransformModifiers& GetTransformModifiers();

	ShapeType m_ShapeType;
	bool m_IsCombined;

	std::unique_ptr<TransformModifiers> m_TransformModifiers;

	FLOAT m_StrokeWidth;
	std::vector<FLOAT> m_StrokeCustomDashes;
	D2D1_STROKE_STYLE_PROPERTIES1 m_StrokeProperties;
	Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> m_StrokeStyle;

	BrushData m_Fill;
	BrushData m_StrokeFill;
};

} // Gfx

#endif
