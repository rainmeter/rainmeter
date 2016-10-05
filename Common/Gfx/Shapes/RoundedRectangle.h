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
	RoundedRectangle(FLOAT x, FLOAT y, FLOAT width, FLOAT height, FLOAT xRadius, FLOAT yRadius);
	~RoundedRectangle();

	virtual Shape* Clone() override;

private:
	RoundedRectangle(const RoundedRectangle& other) = delete;
	RoundedRectangle& operator=(RoundedRectangle other) = delete;

	FLOAT m_X;
	FLOAT m_Y;
	FLOAT m_Width;
	FLOAT m_Height;
	FLOAT m_XRadius;
	FLOAT m_YRadius;
};

} // Gfx

#endif
