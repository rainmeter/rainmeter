/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Canvas.h"
#include "CanvasD2D.h"

namespace Gfx {

Canvas::Canvas() :
	m_W(),
	m_H(),
	m_AccurateText(false)
{
}

Canvas::~Canvas()
{
}

Canvas* Canvas::Create(Renderer renderer)
{
	if (CanvasD2D::Initialize())
	{
		return new CanvasD2D();
	}

	CanvasD2D::Finalize();
	return nullptr;
};

void Canvas::Resize(int w, int h)
{
	m_W = w;
	m_H = h;
}

}  // namespace Gfx
