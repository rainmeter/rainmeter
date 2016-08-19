/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_

#include <d2d1_1.h>
#include <wrl/client.h>
#include <vector>
#include <gdiplus.h>

namespace Gfx {

class Canvas;

class Shape
{
public:
	Shape() :
		m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
		m_StrokeWidth(1),
		m_StrokeColor(D2D1::ColorF(D2D1::ColorF::Black)),
		m_Rotation(),
		m_RotationCenter(),
		m_Skew(),
		m_Scale(D2D1::SizeF(1, 1)),
		m_Offset(),
		m_Antialias(true)
	{
	}

	D2D1_MATRIX_3X2_F GetShapeMatrix() const;
	virtual void UpdateShape(std::vector<Gdiplus::REAL> parameters) = 0;
	bool IsShapeDefined();

	void SetFillColor(D2D1_COLOR_F fillColor);
	void SetStrokeWidth(float strokeWidth);
	void SetStrokeColor(D2D1_COLOR_F strokeColor);
	void SetRotation(float rotation);
	void SetRotationCenter(D2D1_POINT_2F rotationCenter);
	void SetSkew(D2D1_POINT_2F skew);
	void SetScale(D2D1_SIZE_F scale);
	void SetOffset(D2D1_SIZE_F offset);
	void SetAntialias(bool antialias);
protected:

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;
	D2D1_COLOR_F m_FillColor;
	float m_StrokeWidth;
	D2D1_COLOR_F m_StrokeColor;
	float m_Rotation;
	D2D1_POINT_2F m_RotationCenter;
	D2D1_POINT_2F m_Skew;
	D2D1_SIZE_F m_Scale;
	D2D1_SIZE_F m_Offset;
	bool m_Antialias;
	D2D1_RECT_F m_Bounds;
	D2D1_RECT_F m_UntransformedBounds;

	friend class Canvas;
};

} // Gfx

#endif