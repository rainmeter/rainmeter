/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREWINDOWMESSAGE_H_
#define RM_LIBRARY_MEASUREWINDOWMESSAGE_H_

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
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
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

#endif
