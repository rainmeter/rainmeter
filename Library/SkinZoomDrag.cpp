/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinZoomDrag.h"

const int g_ZoomDragMinPercent = 10;
const int g_ZoomDragMaxPercent = 500;
const int g_ZoomDragEdgeSize = 10;
const int g_ZoomDragCornerSize = 20;

static bool IsLeftHit(int hit)
{
	return hit == HTLEFT || hit == HTTOPLEFT || hit == HTBOTTOMLEFT;
}

static bool IsRightHit(int hit)
{
	return hit == HTRIGHT || hit == HTTOPRIGHT || hit == HTBOTTOMRIGHT;
}

static bool IsTopHit(int hit)
{
	return hit == HTTOP || hit == HTTOPLEFT || hit == HTTOPRIGHT;
}

static bool IsBottomHit(int hit)
{
	return hit == HTBOTTOM || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT;
}

HCURSOR SkinZoomDrag::GetCursorForHit(int hit)
{
	switch (hit)
	{
	case HTLEFT:
	case HTRIGHT:
		return LoadCursor(nullptr, IDC_SIZEWE);

	case HTTOP:
	case HTBOTTOM:
		return LoadCursor(nullptr, IDC_SIZENS);

	case HTTOPLEFT:
	case HTBOTTOMRIGHT:
		return LoadCursor(nullptr, IDC_SIZENWSE);

	case HTTOPRIGHT:
	case HTBOTTOMLEFT:
		return LoadCursor(nullptr, IDC_SIZENESW);
	}

	return nullptr;
}

int SkinZoomDrag::HitTest(const RECT& rect, POINT screenPos)
{
	if (!PtInRect(&rect, screenPos)) return HTCLIENT;

	const auto w = rect.right - rect.left;
	const auto h = rect.bottom - rect.top;
	const int edgeX = min(g_ZoomDragEdgeSize, max(1, w / 2));
	const int edgeY = min(g_ZoomDragEdgeSize, max(1, h / 2));
	const int cornerX = min(g_ZoomDragCornerSize, max(1, w / 2));
	const int cornerY = min(g_ZoomDragCornerSize, max(1, h / 2));
	const int leftDistance = screenPos.x - rect.left;
	const int rightDistance = rect.right - screenPos.x - 1;
	const int topDistance = screenPos.y - rect.top;
	const int bottomDistance = rect.bottom - screenPos.y - 1;

	bool left = leftDistance < edgeX;
	bool right = rightDistance < edgeX;
	bool top = topDistance < edgeY;
	bool bottom = bottomDistance < edgeY;

	if (left && right)
	{
		if (leftDistance <= rightDistance)
		{
			right = false;
		}
		else
		{
			left = false;
		}
	}

	if (top && bottom)
	{
		if (topDistance <= bottomDistance)
		{
			bottom = false;
		}
		else
		{
			top = false;
		}
	}

	const bool nearLeftCorner = leftDistance < cornerX;
	const bool nearRightCorner = rightDistance < cornerX;
	const bool nearTopCorner = topDistance < cornerY;
	const bool nearBottomCorner = bottomDistance < cornerY;

	if ((left || top) && nearLeftCorner && nearTopCorner && leftDistance <= rightDistance && topDistance <= bottomDistance) return HTTOPLEFT;
	if ((right || top) && nearRightCorner && nearTopCorner && rightDistance < leftDistance && topDistance <= bottomDistance) return HTTOPRIGHT;
	if ((left || bottom) && nearLeftCorner && nearBottomCorner && leftDistance <= rightDistance && bottomDistance < topDistance) return HTBOTTOMLEFT;
	if ((right || bottom) && nearRightCorner && nearBottomCorner && rightDistance < leftDistance && bottomDistance < topDistance) return HTBOTTOMRIGHT;

	if (left) return HTLEFT;
	if (right) return HTRIGHT;
	if (top) return HTTOP;
	if (bottom) return HTBOTTOM;

	return HTCLIENT;
}

SkinZoomDrag::SkinZoomDrag(int hit, RECT startRect, POINT startPoint, float startZoom) :
	m_InitialHit(hit),
	m_StartRect(startRect),
	m_StartPoint(startPoint),
	m_StartZoom(startZoom)
{
}

SkinZoomDrag::UpdateResult SkinZoomDrag::Update(POINT screenPos, int windowW, int windowH, float dpiScale, float currentZoom, POINT currentPos)
{
	const int startW = max(1, m_StartRect.right - m_StartRect.left);
	const int startH = max(1, m_StartRect.bottom - m_StartRect.top);
	const bool horizontal = IsLeftHit(m_InitialHit) || IsRightHit(m_InitialHit);
	const bool vertical = IsTopHit(m_InitialHit) || IsBottomHit(m_InitialHit);

	int draggedW = startW;
	int draggedH = startH;
	const int dx = screenPos.x - m_StartPoint.x;
	const int dy = screenPos.y - m_StartPoint.y;

	if (IsLeftHit(m_InitialHit))
	{
		draggedW = startW - dx;
	}
	else if (IsRightHit(m_InitialHit))
	{
		draggedW = startW + dx;
	}

	if (IsTopHit(m_InitialHit))
	{
		draggedH = startH - dy;
	}
	else if (IsBottomHit(m_InitialHit))
	{
		draggedH = startH + dy;
	}

	draggedW = max(1, draggedW);
	draggedH = max(1, draggedH);

	float zoom = m_StartZoom;
	if (horizontal && vertical)
	{
		const float widthRatio = (float)draggedW / (float)startW;
		const float heightRatio = (float)draggedH / (float)startH;
		zoom *= (fabsf(widthRatio - 1.0f) >= fabsf(heightRatio - 1.0f)) ? widthRatio : heightRatio;
	}
	else if (horizontal)
	{
		zoom *= (float)draggedW / (float)startW;
	}
	else if (vertical)
	{
		zoom *= (float)draggedH / (float)startH;
	}

	zoom = max((float)g_ZoomDragMinPercent / 100.0f, min((float)g_ZoomDragMaxPercent / 100.0f, zoom));
	zoom = floorf(zoom * 100.0f + 0.5f) / 100.0f;

	int x = m_StartRect.left;
	int y = m_StartRect.top;

	if (IsLeftHit(m_InitialHit))
	{
		const int widthAtZoom = (int)((float)windowW * dpiScale * zoom);
		x = m_StartRect.right - widthAtZoom;
	}
	if (IsTopHit(m_InitialHit))
	{
		const int heightAtZoom = (int)((float)windowH * dpiScale * zoom);
		y = m_StartRect.bottom - heightAtZoom;
	}

	UpdateResult result;
	result.zoom = zoom;
	result.zoomDelta = zoom - m_StartZoom;
	result.deltaX = m_StartRect.left - x;
	result.deltaY = m_StartRect.top - y;
	result.positionChanged = (currentPos.x + result.deltaX) != m_StartRect.left || (currentPos.y + result.deltaY) != m_StartRect.top;
	result.changed = fabsf(currentZoom - zoom) > 0.0001f || currentPos.x != x || currentPos.y != y;

	if (result.changed)
	{
		m_Moved = true;
		m_PositionChanged = m_PositionChanged || result.positionChanged;
	}

	return result;
}

