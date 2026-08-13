// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>
#include <string_view>
#include <vector>

class ConfigParser;
class Skin;

enum class Bang
{
	Refresh,
	RefreshApp,
	Redraw,
	Update,
	SetUpdate,
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
	FocusMeter,
	DisableMouseAction,
	ClearMouseAction,
	EnableMouseAction,
	ToggleMouseAction,
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
	SetWindowPosition,
	SetAnchor,
	SetZoomFactor,
	ZPos,
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
	DisableMouseActionGroup,
	ClearMouseActionGroup,
	EnableMouseActionGroup,
	ToggleMouseActionGroup,
	DisableMouseActionSkinGroup,
	ClearMouseActionSkinGroup,
	EnableMouseActionSkinGroup,
	ToggleMouseActionSkinGroup,
	DisableMeasureGroup,
	EnableMeasureGroup,
	ToggleMeasureGroup,
	PauseMeasureGroup,
	UnpauseMeasureGroup,
	TogglePauseMeasureGroup,
	UpdateMeasureGroup,
	CommandMeasureGroup,
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
	Debug,
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
	void ExecuteBang(std::wstring_view name, std::vector<std::wstring>& args, Skin* skin);

	static void RunCommand(std::wstring command);
	static void RunFile(const WCHAR* file, const WCHAR* args = nullptr);

	static std::vector<std::wstring> ParseString(const WCHAR* str, ConfigParser* parser = nullptr);
};
