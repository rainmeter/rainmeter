/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#ifndef RM_GFX_CANVAS_H_
#define RM_GFX_CANVAS_H_

#include "FontCollectionD2D.h"
#include "TextFormatD2D.h"
#include "Util/WICBitmapDIB.h"
#include <memory>
#include <string>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>

#include <sstream>
#include "../../Library/Logger.h"

/*



*/

struct GeometryShape {

	GeometryShape() : m_OutlineWidth(1),
		m_OutlineColor(Gdiplus::Color::Black),
		m_FillColor(Gdiplus::Color::White),
		m_GradientStops(),
		m_GradientProperties(),
		m_StrokeProperties()
	{}


	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Geometry;
	int m_OutlineWidth;
	Gdiplus::Color m_OutlineColor;
	Gdiplus::Color m_FillColor;

	std::vector<D2D1_GRADIENT_STOP> m_GradientStops;
	union GradientProperties {
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES m_LinearProperties;
		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES m_RadialProperties;
	} m_GradientProperties;
	enum BrushType {
		Linear,
		Radial,
		Solid
	} m_BrushType;

	bool m_UseDashes = false;
	std::vector<float> m_Dashes;
	D2D1_STROKE_STYLE_PROPERTIES m_StrokeProperties;

};


namespace Gfx {

// Wraps Direct2D/DirectWrite.
class Canvas
{
public:
	Canvas();
	~Canvas();

	static bool Initialize();
	static void Finalize();

	int GetW() const { return m_W; }
	int GetH() const { return m_H; }

	void SetAccurateText(bool option) { m_AccurateText = option; }

	// Resize the draw area of the Canvas. This function must not be called if BeginDraw() has been
	// called and has not yet been matched by a correspoding call to EndDraw.
	void Resize(int w, int h);

	bool BeginDraw();
	void EndDraw();

	Gdiplus::Graphics& BeginGdiplusContext();
	void EndGdiplusContext();

	HDC GetDC();
	void ReleaseDC(HDC dc);
	
	FontCollection* CreateFontCollection() { return new FontCollectionD2D(); }
	TextFormat* CreateTextFormat() { return new TextFormatD2D(); }

	bool IsTransparentPixel(int x, int y);

	void SetTransform(const Gdiplus::Matrix& matrix);
	void ResetTransform();
	void RotateTransform(float angle, float x, float y, float dx, float dy);

	void SetAntiAliasing(bool enable);
	void SetTextAntiAliasing(bool enable);

	void Clear(const Gdiplus::Color& color = Gdiplus::Color(0, 0, 0, 0));

	void DrawTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect,
		const Gdiplus::SolidBrush& brush, bool applyInlineFormatting = false);
	bool MeasureTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect);
	bool MeasureTextLinesW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines);

	void DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect);
	void DrawMaskedBitmap(Gdiplus::Bitmap* bitmap, Gdiplus::Bitmap* maskBitmap, const Gdiplus::Rect& dstRect,
		const Gdiplus::Rect& srcRect, const Gdiplus::Rect& srcRect2);
	enum GeometryType {
		Line,
		Arc,
		Bezier,
		QuadBezier
	};

	struct VectorPoint
	{
		VectorPoint(double x, double y)
		{
			m_Geometry.lineSegment = D2D1::Point2F(x, y);
			m_type = Line;
			m_x = x;
			m_y = y;
		}
		VectorPoint(D2D1_ARC_SEGMENT& segment) 
		{
			m_Geometry.arcSegment = segment;
			m_type = Arc;
			m_x = segment.point.x;
			m_y = segment.point.y;
		}
		VectorPoint(D2D1_BEZIER_SEGMENT& segment)
		{
			m_Geometry.bezierSegment = segment;
			m_type = Bezier;
			m_x = segment.point3.x;
			m_y = segment.point3.y;
		}
		VectorPoint(D2D1_QUADRATIC_BEZIER_SEGMENT& segment)
		{
			m_Geometry.quadBezierSegment = segment;
			m_type = QuadBezier;
			m_x = segment.point2.x;
			m_y = segment.point2.y;
		}
		union Geometry {
			D2D1_ARC_SEGMENT arcSegment;
			D2D1_POINT_2F lineSegment;
			D2D1_BEZIER_SEGMENT bezierSegment;
			D2D1_QUADRATIC_BEZIER_SEGMENT quadBezierSegment;

		} m_Geometry;
		double m_x, m_y;
		GeometryType m_type;
	};


	static Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> CreateRectangle(D2D1_RECT_F rectangle);
	static Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> CreateRoundedRectangle(D2D1_ROUNDED_RECT rectangle);
	static Microsoft::WRL::ComPtr<ID2D1EllipseGeometry> CreateEllipse(D2D1_ELLIPSE rectangle);
	static Microsoft::WRL::ComPtr<ID2D1PathGeometry> CreatePathGeometry();
	static Microsoft::WRL::ComPtr<ID2D1PathGeometry> CreateCustomGeometry(const std::vector<VectorPoint>& points, bool ConnectEdges);


	void DrawGeometry(const GeometryShape& shape, D2D1_MATRIX_3X2_F& transform);
	void DrawMaskedGeometryBitmap(Gdiplus::Bitmap* bitmap, Gdiplus::Rect& dstRect, Gdiplus::Rect& srcRect, double imageRotation, const GeometryShape& shape, D2D1_MATRIX_3X2_F& transform);

	static Microsoft::WRL::ComPtr<ID2D1PathGeometry> CombineGeometry(ID2D1Geometry* geometry1, ID2D1Geometry* geometry2, D2D1_COMBINE_MODE mode)
	{
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeometry = CreatePathGeometry();
		Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
		pathGeometry->Open(&sink);
		geometry1->CombineWithGeometry(geometry2, mode, NULL, sink.Get());
		sink->Close();

		return pathGeometry;
	}
	//void DrawPathGeometry(const std::vector<GeometryPoint>& points, const Gdiplus::SolidBrush& fillBrush, const Gdiplus::Color& outlineColor, bool renderBackground, float lineWidth, bool connectEdges);

	void FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush);

private:
	friend class Canvas;
	friend class FontCollectionD2D;
	friend class TextFormatD2D;
	friend class TextInlineFormat_Typography;

	Canvas(const Canvas& other) = delete;
	Canvas& operator=(Canvas other) = delete;

	bool BeginTargetDraw();
	void EndTargetDraw();

	// Sets the |m_Target| transformation to be equal to that of |m_GdipGraphics|.
	void UpdateTargetTransform();

	int m_W;
	int m_H;

	// GDI+, by default, includes padding around the string and also has a larger character spacing
	// compared to DirectWrite. In order to minimize diffeences between the text renderers,
	// an option is provided to enable accurate (typographic) text rendering. If set to |true|,
	// it is expected that there is no padding around the text and that the output is similar to
	// the default DirectWrite output. Otherwise, the expected result should be similar to that of
	// non-typographic GDI+.
	bool m_AccurateText;

	Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_Target;


	// Underlying pixel data shared by both m_Target and m_GdipBitmap.
	Util::WICBitmapDIB m_Bitmap;

	// GDI+ objects that share the pixel data of m_Bitmap.
	std::unique_ptr<Gdiplus::Graphics> m_GdipGraphics;
	std::unique_ptr<Gdiplus::Bitmap> m_GdipBitmap;



	bool m_TextAntiAliasing;

	// |true| if PushAxisAlignedClip()/PopAxisAlignedClip() can be used.
	bool m_CanUseAxisAlignClip;

	static UINT c_Instances;
	static Microsoft::WRL::ComPtr<ID2D1Factory1> c_D2DFactory;
	static Microsoft::WRL::ComPtr<IDWriteFactory1> c_DWFactory;
	static Microsoft::WRL::ComPtr<IDWriteGdiInterop> c_DWGDIInterop;
	static Microsoft::WRL::ComPtr<IWICImagingFactory> c_WICFactory;
};

}  // namespace Gfx

#endif
