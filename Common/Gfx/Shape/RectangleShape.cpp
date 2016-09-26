#include "StdAfx.h"
#include "RectangleShape.h"
#include "Gfx\Canvas.h"

Gfx::RectangleShape::RectangleShape(std::vector<Gdiplus::REAL> parameters)
{
	UpdateShape(parameters);
}

void Gfx::RectangleShape::UpdateShape(std::vector<Gdiplus::REAL> parameters)
{
	if (parameters.size() == 4) {
		m_Shape = Canvas::CreateRectangle(D2D1::RectF(parameters[0], parameters[1], parameters[2] + parameters[0], parameters[3] + parameters[1]));
	}
	else if (parameters.size() == 5) {
		m_Shape = Canvas::CreateRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(parameters[0], parameters[1], parameters[2] + parameters[0], parameters[3] + parameters[1]), parameters[4], parameters[4]));
	}
	else if (parameters.size() == 6) {
		m_Shape = Canvas::CreateRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(parameters[0], parameters[1], parameters[2] + parameters[0], parameters[3] + parameters[1]), parameters[4], parameters[5]));
	}
}
