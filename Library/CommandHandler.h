/*
  Copyright (C) 2013 Rainmeter Team

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

#ifndef RM_LIBRARY_COMMANDHANDLER_H_
#define RM_LIBRARY_COMMANDHANDLER_H_

#include <Windows.h>
#include <string>
#include <vector>

class ConfigParser;
class MeterWindow;

enum class Bang
{
	Refresh,
	RefreshApp,
	Redraw,
	Update,
	Hide,
	Show,
	Toggle,
	HideFade,
	ShowFade,
	ToggleFade,
	HideMeter,
	ShowMeter,
	ToggleMeter,
	MoveMeter,
	UpdateMeter,
	DisableMeasure,
	EnableMeasure,
	ToggleMeasure,
	PauseMeasure,
	UnpauseMeasure,
	TogglePauseMeasure,
	UpdateMeasure,
	CommandMeasure,
	PluginBang,
	ShowBlur,
	HideBlur,
	ToggleBlur,
	AddBlur,
	RemoveBlur,
	ActivateConfig,
	DeactivateConfig,
	ToggleConfig,
	Move,
	ZPos,
	ChangeZPos,
	ClickThrough,
	Draggable,
	SnapEdges,
	KeepOnScreen,
	SetTransparency,
	SetVariable,
	SetOption,
	RefreshGroup,
	UpdateGroup,
	RedrawGroup,
	HideGroup,
	ShowGroup,
	ToggleGroup,
	HideFadeGroup,
	ShowFadeGroup,
	ToggleFadeGroup,
	HideMeterGroup,
	ShowMeterGroup,
	ToggleMeterGroup,
	UpdateMeterGroup,
	DisableMeasureGroup,
	EnableMeasureGroup,
	ToggleMeasureGroup,
	PauseMeasureGroup,
	UnpauseMeasureGroup,
	TogglePauseMeasureGroup,
	UpdateMeasureGroup,
	DeactivateConfigGroup,
	ZPosGroup,
	ClickThroughGroup,
	DraggableGroup,
	SnapEdgesGroup,
	KeepOnScreenGroup,
	SetTransparencyGroup,
	SetVariableGroup,
	SetOptionGroup,
	WriteKeyValue,
	LoadLayout,
	SetClip,
	SetWallpaper,
	About,
	Manage,
	SkinMenu,
	TrayMenu,
	ResetStats,
	Log,
	Quit,
	LsBoxHook
};

// Parses and executes commands and bangs.
class CommandHandler
{
public:
	void ExecuteCommand(const WCHAR* command, MeterWindow* skin, bool multi = true);
	void ExecuteBang(const WCHAR* name, std::vector<std::wstring>& args, MeterWindow* skin);

	static void RunCommand(std::wstring command);
	static void RunFile(const WCHAR* file, const WCHAR* args = nullptr);

	static std::vector<std::wstring> ParseString(const WCHAR* str, ConfigParser* parser = nullptr);

	static void DoActivateSkinBang(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoDeactivateSkinBang(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoToggleSkinBang(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoDeactivateSkinGroupBang(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoLoadLayoutBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoSetClipBang(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoSetWallpaperBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoAboutBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoManageBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoSkinMenuBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoTrayMenuBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoResetStatsBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoWriteKeyValueBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoLogBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoRefreshApp(std::vector<std::wstring>& args, MeterWindow* skin);
	static void DoQuitBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
	static void DoLsBoxHookBang(std::vector<std::wstring>& args, MeterWindow* meterWindow);
};

#endif
