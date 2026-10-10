// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <type_traits>
#include <utility>

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
class Section;

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

struct BangNumber
{
	double value;
	bool relative;
};

struct BangInteger
{
	int value;
	bool relative;
};

// Parses and executes commands and bangs.
class CommandHandler
{
public:
	void ExecuteCommand(const WCHAR* command, Skin* skin, bool multi = true);
	void ExecuteBang(std::wstring_view name, std::vector<std::wstring>& args, Skin* skin, BangTarget target = BangTarget::Default);

	static void RunCommand(std::wstring command);
	static void RunFile(const std::wstring& file, const std::wstring& args = {});

	static std::vector<std::wstring> ParseString(const WCHAR* str, ConfigParser* parser = nullptr);

	// A leading sign selects relative mode only when allowed. Parenthesized signs are part of the formula.
	static std::optional<BangNumber> ParseBangNumber(const ConfigParser& parser, std::wstring_view argument, bool allowRelative = false);
	static std::optional<BangInteger> ParseBangInteger(const ConfigParser& parser, std::wstring_view argument, bool allowRelative = false);

	static void RegisterMeterBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeterBangFunc handlerFunc);
	static void RegisterMeasureBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeasureBangFunc handlerFunc);

	// BangNumber and BangInteger allow relative values; plain numeric arguments are absolute.
	template<typename T, auto Handler>
	static void RegisterMeterBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeterBang(typeId, name, GetBangArgumentCount(Handler), [](Meter* meter, std::vector<std::wstring>& args, Skin* skin)
		{
			InvokeBang((T*)meter, args, skin, Handler);
		});
	}

	template<typename T, auto Handler>
	static void RegisterMeasureBang(UINT typeId, const WCHAR* name)
	{
		RegisterMeasureBang(typeId, name, GetBangArgumentCount(Handler), [](Measure* measure, std::vector<std::wstring>& args, Skin* skin)
		{
			InvokeBang((T*)measure, args, skin, Handler);
		});
	}

	static void RegisterSkinBang(const WCHAR* name, uint8_t argCount, SkinBangFunc handlerFunc);

	template<auto Handler>
	static void RegisterSkinBang(const WCHAR* name)
	{
		RegisterSkinBang(name, GetBangArgumentCount(Handler), [](std::vector<std::wstring>& args, Skin* skin)
		{
			InvokeBang(skin, args, skin, Handler);
		});
	}

private:
	static const ConfigParser& GetBangParser(Skin* skin);
	static void ReportInvalidBangArgument(Section* section, size_t index, const std::wstring& argument);
	static void ReportInvalidBangArgument(Skin* skin, size_t index, const std::wstring& argument);

	template<typename T, typename... Args>
	static constexpr uint8_t GetBangArgumentCount(void (T::*handler)(Args...))
	{
		static_assert(sizeof...(Args) <= 255);
		return (uint8_t)sizeof...(Args);
	}

	template<typename T>
	static std::optional<T> ParseBangArgument(Skin* skin, const std::wstring& argument)
	{
		static_assert(std::is_same_v<T, const WCHAR*> || std::is_same_v<T, int> || std::is_same_v<T, double> || std::is_same_v<T, BangInteger> || std::is_same_v<T, BangNumber>, "Unsupported bang argument type");
		if constexpr (std::is_same_v<T, const WCHAR*>)
		{
			return argument.c_str();
		}
		else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, BangInteger>)
		{
			const auto number = ParseBangInteger(GetBangParser(skin), argument, std::is_same_v<T, BangInteger>);
			if (!number) return std::nullopt;

			if constexpr (std::is_same_v<T, int>)
			{
				return number->value;
			}
			else
			{
				return number;
			}
		}
		else
		{
			const auto number = ParseBangNumber(GetBangParser(skin), argument, std::is_same_v<T, BangNumber>);
			if (!number) return std::nullopt;

			if constexpr (std::is_same_v<T, double>)
			{
				return number->value;
			}
			else
			{
				return number;
			}
		}
	}

	template<typename SectionType, typename T, typename... Args>
	static void InvokeBang(SectionType* section, std::vector<std::wstring>& args, Skin* skin, void (T::*handler)(Args...))
	{
		InvokeBang(section, args, skin, handler, std::index_sequence_for<Args...>{});
	}

	template<typename SectionType, typename T, typename... Args, size_t... Indices>
	static void InvokeBang(SectionType* section, std::vector<std::wstring>& args, Skin* skin, void (T::*handler)(Args...), std::index_sequence<Indices...>)
	{
		const std::tuple<std::optional<Args>...> parsed{ ParseBangArgument<Args>(skin, args[Indices])... };
		if ((std::get<Indices>(parsed).has_value() && ...))
		{
			(section->*handler)(*std::get<Indices>(parsed)...);
		}
		else
		{
			((std::get<Indices>(parsed).has_value() ? (void)0 : ReportInvalidBangArgument(section, Indices, args[Indices])), ...);
		}
	}
};
