// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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

	bool ExecuteAction(MOUSEACTION action, POINT screenPos, MOUSEACTION fallback = MOUSEACTION_NONE);
	void ExecuteMoveActions(POINT screenPos);

	bool WantsCapture() const { return m_RequireDragging && m_Capturing; }
	void ClearCapture() { m_Capturing = false; }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override {}
	void Command(const std::wstring& command) override;

private:
	bool IsActive();
	bool ShouldRunMoveAction();
	void ReplaceMouseVariables(std::wstring& result, POINT screenPos) const;

	Mouse m_Mouse;
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
