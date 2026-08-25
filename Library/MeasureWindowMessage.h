// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureWindowMessage : public Measure
{
public:
	MeasureWindowMessage(Skin* skin, const WCHAR* name);
	virtual ~MeasureWindowMessage();

	MeasureWindowMessage(const MeasureWindowMessage& other) = delete;
	MeasureWindowMessage& operator=(MeasureWindowMessage other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureWindowMessage>(); }

	virtual const WCHAR* GetStringValue();

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();
	virtual void Command(const std::wstring& command);

private:
	HWND FindTargetWindow() const;

	std::wstring m_WindowName;
	std::wstring m_WindowClass;
	std::wstring m_StringValue;
	WPARAM m_WParam;
	LPARAM m_LParam;
	UINT m_Message;
};
