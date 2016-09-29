/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_RECTANGLE_H_
#define RM_GFX_SHAPE_RECTANGLE_H_

#include "../Shape.h"

namespace Gfx {

class Rectangle final : public Shape
{
public:
	Rectangle(float x, float y, float width, float height);
	~Rectangle();
	
private:
	Rectangle(const Rectangle& other) = delete;
	Rectangle& operator=(Rectangle other) = delete;

	float m_X;
	float m_Y;
	float m_Width;
	float m_Height;
};

} // Gfx

#endif
