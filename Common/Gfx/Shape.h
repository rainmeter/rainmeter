/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_H_
#define RM_GFX_SHAPE_H_

#include <gdiplus.h>

namespace Gfx {

struct Shape
{
	Shape() :
		m_FillColor(D2D1::ColorF(D2D1::ColorF::White)),
		m_OutlineWidth(1),
		m_OutlineColor(D2D1::ColorF(D2D1::ColorF::Black))
	{
	}

	Microsoft::WRL::ComPtr<ID2D1Geometry> m_Shape;
	D2D1_COLOR_F m_FillColor;
	double m_OutlineWidth;
	D2D1_COLOR_F m_OutlineColor;
};

}
#endif