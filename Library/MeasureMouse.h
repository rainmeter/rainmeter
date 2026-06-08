/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREMOUSE_H_
#define RM_LIBRARY_MEASUREMOUSE_H_

#include "Measure.h"
#include "Mouse.h"

class MeasureMouse : public Measure
{
public:
	MeasureMouse(Skin* skin, const WCHAR* name);
	virtual ~MeasureMouse();

	MeasureMouse(const MeasureMouse& other) = delete;
	MeasureMouse& operator=(MeasureMouse other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureMouse>(); }

	bool ExecuteAction(MOUSEACTION action, POINT logicalPos, POINT screenPos, MOUSEACTION fallback = MOUSEACTION_COUNT);
	void ExecuteMoveActions(POINT logicalPos, POINT screenPos);

	bool WantsCapture() const { return m_RequireDragging && m_Capturing; }
	void ClearCapture() { m_Capturing = false; }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override {}
	void Command(const std::wstring& command) override;

private:
	bool IsActive();
	bool ShouldRunMoveAction();
	void ReplaceMouseVariables(std::wstring& result, POINT logicalPos, POINT screenPos) const;

	std::wstring m_Actions[MOUSEACTION_COUNT];
	std::wstring m_MouseMoveAction;
	std::wstring m_LeftDragAction;
	std::wstring m_MiddleDragAction;
	std::wstring m_RightDragAction;
	std::wstring m_X1DragAction;
	std::wstring m_X2DragAction;

	bool m_RelativeToSkin;
	bool m_RequireDragging;
	bool m_Capturing;
	UINT m_Delay;
	ULONGLONG m_LastMoveActionTime;
};

#endif
