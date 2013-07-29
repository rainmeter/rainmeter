/*
  Copyright (C) 2013 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Canvas.h"
#include "CanvasD2D.h"
#include "CanvasGDIP.h"
#include "../Platform.h"

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
	if (renderer == Renderer::GDIP)
	{
		return new CanvasGDIP();
	}
	else if (renderer == Renderer::D2D && Platform::IsAtLeastWin7())
	{
		if (CanvasD2D::Initialize())
		{
			return new CanvasD2D();
		}

		CanvasD2D::Finalize();
	}
	else if (renderer == Renderer::PreferD2D)
	{
		if (Canvas* canvas = Create(Renderer::D2D))
		{
			return canvas;
		}

		return Create(Renderer::GDIP);
	}

	return nullptr;
};

void Canvas::Resize(int w, int h)
{
	m_W = w;
	m_H = h;
}

}  // namespace Gfx
