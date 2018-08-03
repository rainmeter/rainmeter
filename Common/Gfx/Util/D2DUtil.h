/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_D2D_UTIL_H_
#define RM_GFX_D2D_UTIL_H_

#include <d2d1.h>

namespace Gfx {
namespace Util {

const D2D1_COLOR_F c_Transparent_Color_F = D2D1::ColorF(D2D1::ColorF::Black, 0.0f);

D2D1_RECT_F ToRectF(FLOAT x, FLOAT y, FLOAT w, FLOAT h);

D2D1_POINT_2F AddPoint2F(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2);

D2D1_POINT_2F FindEdgePoint(const float theta, const float left, const float top, const float right, const float bottom);

bool RectContains(const D2D1_RECT_F& rect, const D2D1_POINT_2F& point);
bool ColorFEquals(const D2D1_COLOR_F& color1, const D2D1_COLOR_F& color2);

}  // namespace Util
}  // namespace Gfx

#endif
