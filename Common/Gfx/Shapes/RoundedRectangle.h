/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_ROUNDRECTANGLE_H_
#define RM_GFX_SHAPE_ROUNDRECTANGLE_H_

#include "../Shape.h"

namespace Gfx {

class RoundedRectangle final : public Shape
{
public:
	RoundedRectangle(float x, float y, float width, float height, float xRadius, float yRadius);
	~RoundedRectangle();

private:
	RoundedRectangle(const RoundedRectangle& other) = delete;
	RoundedRectangle& operator=(RoundedRectangle other) = delete;

	float m_X;
	float m_Y;
	float m_Width;
	float m_Height;
	float m_XRadius;
	float m_YRadius;
};

} // Gfx

#endif
