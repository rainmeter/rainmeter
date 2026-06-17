/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINZOOMDRAG_H_
#define RM_LIBRARY_SKINZOOMDRAG_H_

class SkinZoomDrag
{
public:
	struct UpdateResult
	{
		float zoom = 1.0f;
		int deltaX = 0;
		int deltaY = 0;
		bool changed = false;
		bool positionChanged = false;
	};

	static int HitTest(const RECT& rect, POINT screenPos);
	static HCURSOR GetCursorForHit(int hitTest);

	SkinZoomDrag(int hit, RECT startRect, POINT startPoint, float startZoom);
	SkinZoomDrag(const SkinZoomDrag& other) = delete;
	SkinZoomDrag& operator=(const SkinZoomDrag& other) = delete;

	int GetInitialHit() const { return m_InitialHit; }
	bool HasMoved() const { return m_Moved; }
	bool HasPositionChanged() const { return m_PositionChanged; }
	UpdateResult Update(POINT screenPos, int windowW, int windowH, float dpiScale, float currentZoom, POINT currentPos);

private:
	int m_InitialHit = HTCLIENT;
	RECT m_StartRect = {};
	POINT m_StartPoint = {};
	float m_StartZoom = 1.0f;
	bool m_Moved = false;
	bool m_PositionChanged = false;
};

#endif
