/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_RECTANGLESHAPE_H_
#define RM_GFX_SHAPE_RECTANGLESHAPE_H_

#include "Shape.h"
#include <gdiplus.h>

namespace Gfx {

	class RectangleShape : public Shape
	{
	public:
		RectangleShape(std::vector<Gdiplus::REAL> parameters);
		void UpdateShape(std::vector<Gdiplus::REAL> parameters) override;
	};

} // Gfx

#endif