// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>
#include <string_view>
#include <vector>

#define REGISTER_METER_BANG(type, handler, name) \
	CommandHandler::RegisterMeterBang<type, &type::handler>(TypeID<type>(), name)

#define REGISTER_MEASURE_BANG(type, handler, name) \
	CommandHandler::RegisterMeasureBang<type, &type::handler>(TypeID<type>(), name)

#define REGISTER_SKIN_BANG(handler, name) \
	CommandHandler::RegisterSkinBang<&Skin::handler>(name)

class ConfigParser;
class Measure;
class Meter;
class Skin;

enum class Bang
{
	Refresh,
	RefreshApp,
	Redraw,
	Update,
	SetUpdate,
	ForceEnableVisibleMode,
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
	SendKey,
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
	Restart,
	EditSkin,
	LsBoxHook
};

enum class BangTarget : BYTE
{
	Default,
	Skin
};

using MeterBangFunc = void (*)(Meter* meter, std::vector<std::wstring>& args, Skin* skin);
using MeasureBangFunc = void (*)(Measure* measure, std::vector<std::wstring>& args, Skin* skin);
using SkinBangFunc = void (*)(std::vector<std::wstring>& args, Skin* skin);

// Parses and executes commands and bangs.
class CommandHandler
{
public:
	void ExecuteCommand(const WCHAR* command, Skin* skin, bool multi = true);
	void ExecuteBang(std::wstring_view name, std::vector<std::wstring>& args, Skin* skin, BangTarget target = BangTarget::Default);

	static void RunCommand(std::wstring command);
	static void RunFile(const std::wstring& file, const std::wstring& args = {});

	static std::vector<std::wstring> ParseString(const WCHAR* str, ConfigParser* parser = nullptr);

	static void RegisterMeterBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeterBangFunc handlerFunc);
	static void RegisterMeasureBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeasureBangFunc handlerFunc);

	template<typename T, void (T::*Handler)()>
	static void RegisterMeterBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeterBang(typeId, name, 0, [](Meter* meter, std::vector<std::wstring>& args, Skin* skin)
		{
			(((T*)meter)->*Handler)();
		});
	}

	template<typename T, void (T::*Handler)(const WCHAR*)>
	static void RegisterMeterBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeterBang(typeId, name, 1, [](Meter* meter, std::vector<std::wstring>& args, Skin* skin)
		{
			(((T*)meter)->*Handler)(args[0].c_str());
		});
	}

	template<typename T, void (T::*Handler)(const WCHAR*, const WCHAR*)>
	static void RegisterMeterBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeterBang(typeId, name, 2, [](Meter* meter, std::vector<std::wstring>& args, Skin* skin)
		{
			(((T*)meter)->*Handler)(args[0].c_str(), args[1].c_str());
		});
	}

	template<typename T, void (T::*Handler)()>
	static void RegisterMeasureBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeasureBang(typeId, name, 0, [](Measure* measure, std::vector<std::wstring>& args, Skin* skin)
		{
			(((T*)measure)->*Handler)();
		});
	}

	template<typename T, void (T::*Handler)(const WCHAR*)>
	static void RegisterMeasureBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeasureBang(typeId, name, 1, [](Measure* measure, std::vector<std::wstring>& args, Skin* skin)
		{
			(((T*)measure)->*Handler)(args[0].c_str());
		});
	}

	static void RegisterSkinBang(const WCHAR* name, uint8_t argCount, SkinBangFunc handlerFunc);

	template<void (Skin::*Handler)()>
	static void RegisterSkinBang(const WCHAR* name)
	{
		RegisterSkinBang(name, 0, [](std::vector<std::wstring>& args, Skin* skin)
		{
			(skin->*Handler)();
		});
	}
};
