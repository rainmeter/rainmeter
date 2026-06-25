/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINSELECTIONOVERLAY_H_
#define RM_LIBRARY_SKINSELECTIONOVERLAY_H_

class Skin;
class SkinZoomDrag;

class SkinSelectionOverlay
{
public:
	SkinSelectionOverlay(Skin* skin);
	~SkinSelectionOverlay();

	void Update();

private:
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ForwardMessageToSkin(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Create();
	void Destroy();
	int HitTestZoomDrag(POINT screenPos) const;
	bool SetZoomDragCursor(int hit);
	void BeginZoomDrag(int hit, POINT screenPos);
	void ApplyZoomDrag();
	void CommitZoomDrag();

	struct ZoomDragStartState
	{
		Skin* skin = nullptr;
		POINT pos = {};
		float zoom = 1.0f;
	};

	Skin* m_Skin;
	HWND m_Window;
	std::unique_ptr<SkinZoomDrag> m_ZoomDrag;
	std::vector<ZoomDragStartState> m_ZoomDragStartStates;
};

#endif
