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
	D2D1_RECT_F GetBounds();
	bool IsShapeDefined();
	bool ContainsPoint(D2D1_POINT_2F point);

	bool IsCombined() { return m_IsCombined; }
	void SetCombined() { m_IsCombined = true; }
	bool CombineWith(Shape* otherShape, D2D1_COMBINE_MODE mode);

	void SetOffset(int x, int y) { m_Offset = D2D1::SizeF((FLOAT)x, (FLOAT)y); }
	void SetStrokeColor(Gdiplus::Color color) { m_StrokeColor = Util::ToColorF(color); }
	void SetStrokeWidth(int strokeWidth) { m_StrokeWidth = (FLOAT)strokeWidth; }

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
	void SetFill(UINT32 angle, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	void SetFill(D2D1_POINT_2F offset, std::vector<D2D1_GRADIENT_STOP> stops, bool altGamma);
	Microsoft::WRL::ComPtr<ID2D1Brush> GetFillBrush(ID2D1RenderTarget* target);

	bool AddToTransformOrder(TransformType type);
	void ValidateTransforms();

protected:
	void CloneModifiers(Shape* otherShape);

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;

private:
	friend class Canvas;

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
	D2D1_COLOR_F m_StrokeColor;
	std::vector<FLOAT> m_StrokeCustomDashes;
	D2D1_STROKE_STYLE_PROPERTIES1 m_StrokeProperties;
	Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> m_StrokeStyle;

	// Brushes are created at drawing time and cached for subsequent drawing
	// operations. They are recreated only when an option has changed.
	D2D1_COLOR_F m_FillColor;
	UINT32 m_LinearGradientAngle;
	D2D1_POINT_2F m_RadialGradientOffset;
	std::vector<D2D1_GRADIENT_STOP> m_GradientStops;
	bool m_GradientAltGamma;

	BrushType m_BrushType;
	Microsoft::WRL::ComPtr<ID2D1Brush> m_Brush;
	bool m_HasBrushChanged;
};

} // Gfx

#endif
