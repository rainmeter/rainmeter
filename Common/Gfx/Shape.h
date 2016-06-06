#pragma once
#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_
#include <gdiplus.h>
namespace Gfx {
	struct Shape
	{
		Shape() :
			m_FillColor(Gdiplus::Color::White),
			m_OutlineWidth(1),
			m_OutlineColor(Gdiplus::Color::Black)
		{}

		Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;
		Gdiplus::Color m_FillColor;
		double m_OutlineWidth;
		Gdiplus::Color m_OutlineColor;
	};
}
#endif