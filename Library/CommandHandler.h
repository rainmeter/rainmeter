/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_COMMANDHANDLER_H_
#define RM_LIBRARY_COMMANDHANDLER_H_

#include <Windows.h>
#include <string>
#include <vector>

class ConfigParser;
class Skin;

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
	FadeDuration,
	KeepOnScreen,
	AutoSelectScreen,
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
	SetFadeDurationGroup,
	KeepOnScreenGroup,
	AutoSelectScreenGroup,
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
	SkinCustomMenu,
	TrayMenu,
	ResetStats,
	Log,
	Quit,
	EditSkin,
	LsBoxHook
};

// Parses and executes commands and bangs.
class CommandHandler
{
public:
	void ExecuteCommand(const WCHAR* command, Skin* skin, bool multi = true);
	void ExecuteBang(const WCHAR* name, std::vector<std::wstring>& args, Skin* skin);

	static void RunCommand(std::wstring command);
	static void RunFile(const WCHAR* file, const WCHAR* args = nullptr);

	static std::vector<std::wstring> ParseString(const WCHAR* str, ConfigParser* parser = nullptr);

	static void DoActivateSkinBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoDeactivateSkinBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoToggleSkinBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoDeactivateSkinGroupBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoLoadLayoutBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoSetClipBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoSetWallpaperBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoAboutBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoManageBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoSkinMenuBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoTrayMenuBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoResetStatsBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoWriteKeyValueBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoLogBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoRefreshApp(std::vector<std::wstring>& args, Skin* skin);
	static void DoQuitBang(std::vector<std::wstring>& args, Skin* skin);
	static void DoEditSkinBang(std::vector<std::wstring>& args, Skin* skin);

	static void DoLsBoxHookBang(std::vector<std::wstring>& args, Skin* skin);
};

#endif
