/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureWindowMessage.h"
#include "ConfigParser.h"
#include "Logger.h"

MeasureWindowMessage::MeasureWindowMessage(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_WindowName(),
	m_WindowClass(),
	m_StringValue(),
	m_WParam(0),
	m_LParam(0),
	m_Message(0U)
{
}

MeasureWindowMessage::~MeasureWindowMessage()
{
}

void MeasureWindowMessage::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_WindowName = parser.ReadString(section, L"WindowName", L"");
	m_WindowClass = parser.ReadString(section, L"WindowClass", L"");

	m_Message = 0U;
	m_WParam = 0;
	m_LParam = 0;

	UINT message = 0U;
	UINT wParam = 0U;
	UINT lParam = 0U;
	const std::wstring& windowMessage = parser.ReadString(section, L"WindowMessage", L"");
	if (swscanf_s(windowMessage.c_str(), L"%u %u %u", &message, &wParam, &lParam) == 3)
	{
		m_Message = message;
		m_WParam = wParam;
		m_LParam = lParam;
	}
}

void MeasureWindowMessage::UpdateValue()
{
	m_Value = 0.0;

	HWND hwnd = FindTargetWindow();
	if (hwnd)
	{
		if (m_Message == 0U)
		{
			WCHAR buffer[256] = { 0 };
			GetWindowText(hwnd, buffer, _countof(buffer));
			m_StringValue = buffer;
		}
		else
		{
			m_Value = (double)SendMessage(hwnd, m_Message, m_WParam, m_LParam);
		}
	}
	else if (m_Message == 0U)
	{
		m_StringValue.clear();
	}
}

const WCHAR* MeasureWindowMessage::GetStringValue()
{
	if (m_Message == 0U)
	{
		return CheckSubstitute(m_StringValue.c_str());
	}

	return nullptr;
}

void MeasureWindowMessage::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();
	const WCHAR* pos = wcschr(args, L' ');
	if (pos)
	{
		const size_t len = pos - args;
		if (_wcsnicmp(args, L"SendMessage", len) == 0)
		{
			++pos;

			UINT message = 0U;
			UINT wParam = 0U;
			UINT lParam = 0U;
			if (swscanf_s(pos, L"%u %u %u", &message, &wParam, &lParam) == 3)
			{
				HWND hwnd = FindTargetWindow();
				if (hwnd)
				{
					PostMessage(hwnd, message, wParam, lParam);
				}
				else
				{
					LogErrorF(this, L"Unable to find window");
				}
			}
			else
			{
				LogWarningF(this, L"Incorrect number of arguments for bang");
			}

			return;
		}
	}

	LogWarningF(this, L"Unknown bang");
}

HWND MeasureWindowMessage::FindTargetWindow() const
{
	return FindWindow(
		m_WindowClass.empty() ? nullptr : m_WindowClass.c_str(),
		m_WindowName.empty() ? nullptr : m_WindowName.c_str());
}
