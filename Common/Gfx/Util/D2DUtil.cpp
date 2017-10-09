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

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

D2D1_RECT_F ToRectF(const Gdiplus::Rect& rect)
{
	return D2D1::RectF((FLOAT)rect.X, (FLOAT)rect.Y, (FLOAT)(rect.X + rect.Width), (FLOAT)(rect.Y + rect.Height));
}

D2D1_RECT_F ToRectF(const Gdiplus::RectF& rect)
{
	return D2D1::RectF(rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height);
}

D2D1_POINT_2F AddPoint2F(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2)
{
	return D2D1::Point2F(point1.x + point2.x, point1.y + point2.y);
}

D2D1_POINT_2F FindEdgePoint(const float theta, const float left, const float top, const float width, const float height)
{
	float theta1 = theta * (M_PI / 180.0f);

	while (theta1 < -M_PI) theta1 += (2 * M_PI);
	while (theta1 > M_PI) theta1 -= (2 * M_PI);

	const float recttan = atan2f(height, width);
	const float thetatan = tanf(theta1);

	enum Region
	{
		One,        // 315 - 45
		Two,        // 45  - 135
		Three,      // 135 - 225
		Four        // 225 - 315
	} region;

	if (theta1 > -recttan && theta1 <= recttan)
	{
		region = One;
	}
	else if (theta1 > recttan && theta1 <= (M_PI - recttan))
	{
		region = Two;
	}
	else if (theta1 > (M_PI - recttan) || theta1 <= -(M_PI - recttan))
	{
		region = Three;
	}
	else
	{
		region = Four;
	}

	float xfactor = 1.0f;
	float yfactor = -1.0f;
	switch (region)
	{
	case One: yfactor = -yfactor; break;
	case Two: yfactor = -yfactor; break;
	case Three: xfactor = -xfactor; break;
	case Four: xfactor = -xfactor; break;
	}

	D2D1_POINT_2F point = { left + (width / 2.0f), top + (height / 2.0f) };
	if (region == One || region == Three)
	{
		point.x += xfactor * (width / 2.0f);
		point.y += yfactor * (width / 2.0f) * thetatan;
	}
	else
	{
		point.x += xfactor * (height / (2.0f * thetatan));
		point.y += yfactor * (height / 2.0f);
	}

	return point;
}

}  // namespace Util
}  // namespace Gfx
