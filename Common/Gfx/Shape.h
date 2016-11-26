/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_

#include "Util/D2DUtil.h"
#include <d2d1_1.h>
#include <wrl/client.h>
#include <vector>
#include <gdiplus.h>

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
	~Shape();

	ShapeType GetShapeType() { return m_ShapeType; }

	bool DoesShapeExist() { return m_Shape != nullptr; }

	virtual Shape* Clone() = 0;

	D2D1_MATRIX_3X2_F GetShapeMatrix();
	D2D1_RECT_F GetBounds(bool useMatrix = true);
	bool IsShapeDefined();
	bool ContainsPoint(D2D1_POINT_2F point, const Gdiplus::Matrix* transformationMatrix);

	bool IsCombined() { return m_IsCombined; }
	void SetCombined() { m_IsCombined = true; }
	bool CombineWith(Shape* otherShape, D2D1_COMBINE_MODE mode);

	void SetOffset(FLOAT x, FLOAT y) { m_Offset = D2D1::SizeF(x, y); }
	void SetStrokeWidth(FLOAT strokeWidth) { m_StrokeWidth = strokeWidth; }

	void SetRotation(FLOAT rotation, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetScale(FLOAT scaleX, FLOAT scaleY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);
	void SetSkew(FLOAT skewX, FLOAT skewY, FLOAT anchorX, FLOAT anchorY, bool anchorDefined);

	void SetStrokeStartCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.startCap = cap; }
	void SetStrokeEndCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.endCap = cap; }
	void SetStrokeDashCap(D2D1_CAP_STYLE cap) { m_StrokeProperties.dashCap = cap; }
	void SetStrokeLineJoin(D2D1_LINE_JOIN join, FLOAT limit) { m_StrokeProperties.lineJoin = join; m_StrokeProperties.miterLimit = limit; }
	void SetStrokeDashes(std::vector<FLOAT> dashes) { m_StrokeCustomDashes = dashes; }
	void SetStrokeDashOffset(FLOAT offset) { m_StrokeProperties.dashOffset = offset; }
	void CreateStrokeStyle();

	void SetFill(Gdiplus::Color color);
	void SetFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	void SetFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	Microsoft::WRL::ComPtr<ID2D1Brush> GetFillBrush(ID2D1RenderTarget* target);

	void SetStrokeFill(Gdiplus::Color color);
	void SetStrokeFill(FLOAT angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	void SetStrokeFill(D2D1_POINT_2F offset, D2D1_POINT_2F center, D2D1_POINT_2F radius, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	Microsoft::WRL::ComPtr<ID2D1Brush> GetStrokeFillBrush(ID2D1RenderTarget* target);

	void ResetTransformOrder() { m_TransformOrder.clear(); }
	bool AddToTransformOrder(TransformType type);
	void ValidateTransforms();

protected:
	void CloneModifiers(Shape* otherShape);

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;

private:
	friend class Canvas;

	void CreateSolidBrush(ID2D1RenderTarget* target, Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const D2D1_COLOR_F& color);
	ID2D1GradientStopCollection* CreateGradientStopCollection(
		ID2D1RenderTarget* target, std::vector<D2D1_GRADIENT_STOP>& stops, bool altGamma);
	void CreateLinearGradient(ID2D1RenderTarget* target, ID2D1GradientStopCollection* collection,
		Microsoft::WRL::ComPtr<ID2D1Brush>& brush, const FLOAT angle);
	void CreateRadialGradient(ID2D1RenderTarget* target, ID2D1GradientStopCollection* collection,
		Microsoft::WRL::ComPtr<ID2D1Brush>& brush, bool isStroke);

	ShapeType m_ShapeType;
	bool m_IsCombined;

	std::vector<TransformType> m_TransformOrder;

	// Modifiers
	D2D1_SIZE_F m_Offset;

	FLOAT m_Rotation;
	D2D1_POINT_2F m_RotationAnchor;
	bool m_RotationAnchorDefined;

	D2D1_POINT_2F m_Skew;
	D2D1_POINT_2F m_SkewAnchor;
	bool m_SkewAnchorDefined;

	D2D1_SIZE_F m_Scale;
	D2D1_POINT_2F m_ScaleAnchor;
	bool m_ScaleAnchorDefined;

	FLOAT m_StrokeWidth;
	std::vector<FLOAT> m_StrokeCustomDashes;
	D2D1_STROKE_STYLE_PROPERTIES1 m_StrokeProperties;
	Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> m_StrokeStyle;

	// Fill options
	BrushType m_FillBrushType;
	D2D1_COLOR_F m_FillColor;
	FLOAT m_FillLinearGradientAngle;
	D2D1_POINT_2F m_FillRadialGradientOffset;
	D2D1_POINT_2F m_FillRadialGradientCenter;
	D2D1_POINT_2F m_FillRadialGradientRadius;
	std::vector<D2D1_GRADIENT_STOP> m_FillGradientStops;
	bool m_FillGradientAltGamma;
	Microsoft::WRL::ComPtr<ID2D1Brush> m_FillBrush;
	bool m_HasFillBrushChanged;

	// Stroke fill options
	BrushType m_StrokeBrushType;
	D2D1_COLOR_F m_StrokeColor;
	FLOAT m_StrokeLinearGradientAngle;
	D2D1_POINT_2F m_StrokeRadialGradientOffset;
	D2D1_POINT_2F m_StrokeRadialGradientCenter;
	D2D1_POINT_2F m_StrokeRadialGradientRadius;
	std::vector<D2D1_GRADIENT_STOP> m_StrokeGradientStops;
	bool m_StrokeGradientAltGamma;
	Microsoft::WRL::ComPtr<ID2D1Brush> m_StrokeBrush;
	bool m_HasStrokeBrushChanged;
};

} // Gfx

#endif
