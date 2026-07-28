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

protected:
	void CloneModifiers(Shape* otherShape);

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;

private:
	friend class Canvas;

	enum class StrokeType : BYTE
	{
		Default = 0,
		Custom,
		Disabled
	};

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

	struct StrokeData
	{
		void CopyFrom(const StrokeData& other);

		BrushData fill{ D2D1::ColorF(D2D1::ColorF::Black) };
		Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> style;
		std::vector<FLOAT> customDashes;
		D2D1_STROKE_STYLE_PROPERTIES1 properties = D2D1::StrokeStyleProperties1();
		FLOAT width = 1.0f;
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
	FLOAT GetStrokeWidth() const;
	ID2D1StrokeStyle1* GetStrokeStyle() const;

	ShapeType m_ShapeType;
	bool m_IsCombined = false;
	StrokeType m_StrokeType = StrokeType::Default;

	std::unique_ptr<TransformModifiers> m_TransformModifiers{};
	std::unique_ptr<StrokeData> m_StrokeData{};

	BrushData m_Fill{ D2D1::ColorF(D2D1::ColorF::White) };
};

} // Gfx

#endif
