/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "D2DUtil.h"

namespace Gfx {
namespace Util {

static const float M_PI = 3.14159265358979323846f;

D2D1_RECT_F ToRectF(FLOAT x, FLOAT y, FLOAT w, FLOAT h)
{
	return D2D1::RectF(x, y, x + w, y + h);
}

D2D1_POINT_2F AddPoint2F(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2)
{
	return D2D1::Point2F(point1.x + point2.x, point1.y + point2.y);
}

D2D1_POINT_2F FindEdgePoint(const float theta, const float left, const float top, const float right, const float bottom)
{
	float base_angle = theta;  // In degrees

	// Restrict |base_angle| to [0..360] degrees
	while (base_angle < 0.0f) base_angle += 360.0f;
	base_angle = fmodf(base_angle, 360.0f);

	// Convert |base_angle| from degrees to radians
	const float base_radians = base_angle * (M_PI / 180.0f);

	// Get the shape area diagonal
	const float width = right - left;
	const float height = bottom - top;
	const float rectangle_tangent = atan2f(height, width);

	// Find the quadrant the |base_angle| is in
	const int quadrant = (int)fmodf(base_angle / 90.0f, 4.0f) + 1;

	// Get the gradient axis angle based on quadrant the |base_radians| is in
	const float axis_angle = [&]() -> float
	{
		switch (quadrant)
		{
		default:
		case 1: return base_radians - M_PI * 0.0f;
		case 2: return M_PI * 1.0f - base_radians;
		case 3: return base_radians - M_PI * 1.0f;
		case 4: return M_PI * 2.0f - base_radians;
		}
	}();

	// Calculate the point by:
	// x = center of shape width  + half of shape area * cos(axis_angle - rectangle_tangent) * cos(base_angle)
	// y = center of shape height + half of shape area * cos(axis angle - rectangle_tangent) * sin(base_angle)
	const float half_area = sqrtf(powf(width, 2.0f) + powf(height, 2.0f)) / 2.0f;
	const float cos_axis = cosf(fabsf(axis_angle - rectangle_tangent));

	return D2D1::Point2F(
		left + (width / 2.0f) + (half_area * cos_axis * cosf(base_radians)),
		top + (height / 2.0f) + (half_area * cos_axis * sinf(base_radians)));
}

bool RectContains(const D2D1_RECT_F& rect, const D2D1_POINT_2F& point)
{
	return rect.left <= point.x && rect.right >= point.x && rect.top <= point.y && rect.bottom >= point.y;
}

bool ColorFEquals(const D2D1_COLOR_F& color1, const D2D1_COLOR_F& color2)
{
	return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b && color1.a == color2.a;
}

}  // namespace Util
}  // namespace Gfx
