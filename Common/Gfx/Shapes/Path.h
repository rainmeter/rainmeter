/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_SHAPE_PATH_H_
#define RM_GFX_SHAPE_PATH_H_

#include "../Shape.h"

namespace Gfx {

class Path final : public Shape
{
public:
	Path(FLOAT x, FLOAT y, D2D1_FILL_MODE fillMode, bool isCloned = false);
	~Path();

	void AddLine(FLOAT x, FLOAT y);
	void AddArc(FLOAT x, FLOAT y, FLOAT xRadius, FLOAT yRadius, FLOAT angle,
		D2D1_SWEEP_DIRECTION direction, D2D1_ARC_SIZE arcSize);
	void AddQuadraticCurve(FLOAT x, FLOAT y, FLOAT cx, FLOAT cy);
	void AddCubicCurve(FLOAT x, FLOAT y, FLOAT cx1, FLOAT cy1, FLOAT cx2, FLOAT cy2);
	void SetSegmentFlags(D2D1_PATH_SEGMENT flags);
	void Close(D2D1_FIGURE_END ending);

	virtual Shape* Clone() override;
	
private:
	Path(const Path& other) = delete;
	Path& operator=(Path other) = delete;

	void Dispose();

	D2D1_POINT_2F m_StartPoint;
	D2D1_FILL_MODE m_FillMode;

	Microsoft::WRL::ComPtr<ID2D1GeometrySink> m_Sink;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry> m_Path;
};

} // Gfx

#endif
