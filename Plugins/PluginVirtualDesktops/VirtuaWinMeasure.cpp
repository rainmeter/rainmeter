/*
  Copyright (C) 2010 Patrick Dubbert

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "VirtuaWinMeasure.h"

#include <tchar.h>

#include "VirtuaWinMessages.h"
#include "../../Library/Export.h"

HWND VirtuaWinMeasure::vwHandle = nullptr;
std::map<std::wstring, VirtuaWinMeasure::MeasureType> VirtuaWinMeasure::StringToType;

VirtuaWinMeasure::VirtuaWinMeasure(HMODULE instance, UINT id) : VDMeasure(instance, id)
{
	if (StringToType.size() == 0)
	{
		StringToType.insert(std::make_pair(std::wstring(L"VDMActive"), VDMActive));
		StringToType.insert(std::make_pair(std::wstring(L"DesktopCount"), DesktopCountTotal));
		StringToType.insert(std::make_pair(std::wstring(L"CurrentDesktop"), CurrentDesktop));
		StringToType.insert(std::make_pair(std::wstring(L"SwitchDesktop"), SwitchDesktop));
	}
}

VirtuaWinMeasure::~VirtuaWinMeasure(void)
{
}

UINT VirtuaWinMeasure::Initialize(LPCTSTR iniFile, LPCTSTR section)
{
	std::wstring TypeString(ReadConfigString(section, _T("VDMeasureType"), _T("")));
	std::map<std::wstring, MeasureType>::iterator i = StringToType.find(TypeString);
	if (i != StringToType.end())
	{
		Type = i->second;
	}
	else
	{
		Type = Unknown;
	}

	switch(Type)
	{
	case VDMActive:
		return 1;

	case DesktopCountTotal:
		{
			LPCTSTR CountType = ReadConfigString(section, _T("VDDesktopCount"), _T(""));
			if (_tcsicmp(CountType, _T("X")) == 0) Type = DesktopCountColumns;
			else if (_tcsicmp(CountType, _T("Y")) == 0) Type = DesktopCountRows;
			if (FindVirtuaWinWindow())
			{
				return (UINT) SendMessage(vwHandle, VW_DESKTOP_SIZE, 0, 0);
			}
			break;
		}
	}

	return 0;
}

void VirtuaWinMeasure::Finalize()
{
}

UINT VirtuaWinMeasure::Update()
{
	if (!FindVirtuaWinWindow())
	{
		return 0;
	}

	switch(Type)
	{
	case VDMActive:
		return 1;

	case DesktopCountColumns:
		return (UINT) SendMessage(vwHandle, VW_DESKX, 0, 0);

	case DesktopCountRows:
		return (UINT) SendMessage(vwHandle, VW_DESKY, 0, 0);

	case DesktopCountTotal:
		return (UINT) SendMessage(vwHandle, VW_DESKX, 0, 0) * (UINT) SendMessage(vwHandle, VW_DESKY, 0, 0);

	case CurrentDesktop:
		return (UINT) SendMessage(vwHandle, VW_CURDESK, 0, 0);
	}

	return 0;
}

LPCTSTR VirtuaWinMeasure::GetString(UINT flags)
{
	static TCHAR Buffer[MAX_PATH];
	Buffer[0] = _T('\0');

	switch(Type)
	{
	case VDMActive:
	case DesktopCountColumns:
	case DesktopCountRows:
	case DesktopCountTotal:
	case CurrentDesktop:
		swprintf_s(Buffer, MAX_PATH, L"%i", Update());
		break;
	}

	return Buffer;
}

void VirtuaWinMeasure::ExecuteBang(LPCTSTR args)
{
	INT32 Desktop;

	if (!FindVirtuaWinWindow()) return;

	switch(Type)
	{
	case SwitchDesktop:
		Desktop = _wtoi(args);
		SendNotifyMessage(vwHandle, VW_CHANGEDESK, Desktop, 0);
	}
}

BOOL VirtuaWinMeasure::FindVirtuaWinWindow()
{
	if (IsWindow(vwHandle)) return TRUE;
	vwHandle = FindWindow(_T("VirtuaWinMainClass"), _T("VirtuaWinMainClass"));
	return vwHandle != nullptr;
}
