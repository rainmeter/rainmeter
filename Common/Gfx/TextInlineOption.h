// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Util/D2DUtil.h"
#include <Windows.h>
#include <algorithm>
#include <string>
#include <variant>
#include <vector>
#include <d2d1_1.h>
#include <dwrite_1.h>

namespace Gfx {

enum class CaseType : BYTE
{
	Lower,
	Upper,
	Proper,
	Sentence,
	None
};

// The inline options, already parsed. Turning what a skin writes in InlineSetting into one of these
// is the caller's job - see MeterStringBase. They are values, so that the options a read ends up
// with can be compared against the ones already in use to tell whether anything changed.
namespace InlineSetting {

struct Case
{
	CaseType type;
	bool operator==(const Case& other) const = default;
};

struct CharacterSpacing
{
	// FLT_MAX leaves the spacing to the layout, and a negative advance width leaves the width to it.
	FLOAT leading;
	FLOAT trailing;
	FLOAT advanceWidth;
	bool operator==(const CharacterSpacing& other) const = default;
};

struct Color
{
	D2D1_COLOR_F color;
	bool operator==(const Color& other) const { return Util::ColorFEquals(color, other.color); }
};

struct Face
{
	std::wstring face;
	bool operator==(const Face& other) const = default;
};

struct GradientColor
{
	FLOAT angle;
	std::vector<D2D1_GRADIENT_STOP> stops;
	bool altGamma;

	bool operator==(const GradientColor& other) const
	{
		return angle == other.angle && altGamma == other.altGamma &&
			std::equal(stops.begin(), stops.end(), other.stops.begin(), other.stops.end(),
				[](const D2D1_GRADIENT_STOP& lhs, const D2D1_GRADIENT_STOP& rhs)
				{
					return lhs.position == rhs.position && Util::ColorFEquals(lhs.color, rhs.color);
				});
	}
};

struct Italic
{
	bool operator==(const Italic& other) const = default;
};

struct None
{
	bool operator==(const None& other) const = default;
};

struct Oblique
{
	bool operator==(const Oblique& other) const = default;
};

struct Shadow
{
	FLOAT blur;
	D2D1_POINT_2F offset;
	D2D1_COLOR_F color;

	bool operator==(const Shadow& other) const
	{
		return blur == other.blur && offset.x == other.offset.x && offset.y == other.offset.y &&
			Util::ColorFEquals(color, other.color);
	}
};

struct Size
{
	FLOAT size;
	bool operator==(const Size& other) const = default;
};

struct Stretch
{
	DWRITE_FONT_STRETCH stretch;
	bool operator==(const Stretch& other) const = default;
};

struct Strikethrough
{
	bool operator==(const Strikethrough& other) const = default;
};

struct Typography
{
	DWRITE_FONT_FEATURE_TAG tag;
	UINT32 parameter;
	bool operator==(const Typography& other) const = default;
};

struct Underline
{
	bool operator==(const Underline& other) const = default;
};

struct Weight
{
	DWRITE_FONT_WEIGHT weight;
	bool operator==(const Weight& other) const = default;
};

}  // namespace InlineSetting

using TextInlineSetting = std::variant<
	InlineSetting::Case,
	InlineSetting::CharacterSpacing,
	InlineSetting::Color,
	InlineSetting::Face,
	InlineSetting::GradientColor,
	InlineSetting::Italic,
	InlineSetting::None,
	InlineSetting::Oblique,
	InlineSetting::Shadow,
	InlineSetting::Size,
	InlineSetting::Stretch,
	InlineSetting::Strikethrough,
	InlineSetting::Typography,
	InlineSetting::Underline,
	InlineSetting::Weight>;

struct TextInlineOption
{
	// The text the setting applies to. Never empty - ".*" is the pattern that matches everything.
	std::wstring pattern;
	TextInlineSetting setting;

	bool operator==(const TextInlineOption& other) const = default;
};

struct TextInlineRange
{
	UINT32 start;
	UINT32 length;
};

}  // namespace Gfx
