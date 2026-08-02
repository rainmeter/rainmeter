// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

class Skin;

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

	struct ZoomDragResult
	{
		float zoom = 1.0f;
		float zoomDelta = 0.0f;
		int deltaX = 0;
		int deltaY = 0;
		bool changed = false;
		bool positionChanged = false;
	};

	ZoomDragResult UpdateZoomDrag(POINT screenPos, int windowW, int windowH, float dpiScale, float currentZoom, POINT currentPos);

	struct ZoomDragState
	{
		enum class CornerAxis : BYTE
		{
			None = 0,
			Horizontal,
			Vertical
		};

		int initialHit = HTCLIENT;
		RECT startRect = {};
		POINT startPoint = {};
		float startZoom = 1.0f;
		CornerAxis cornerAxis = CornerAxis::None;
		bool thresholdReached = false;
		bool moved = false;
		bool positionChanged = false;
	};

	struct ZoomDragStartState
	{
		Skin* skin = nullptr;
		POINT pos = {};
		float zoom = 1.0f;
	};

	Skin* m_Skin;
	HWND m_Window;
	std::optional<ZoomDragState> m_ZoomDrag;
	std::vector<ZoomDragStartState> m_ZoomDragStartStates;
};
