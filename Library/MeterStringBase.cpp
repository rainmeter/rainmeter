// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterStringBase.h"
#include "Rainmeter.h"
#include "Pcre.h"
#include "../Common/Gfx/Canvas.h"
#include "../Common/StringParser.h"
#include "../Common/StringUtil.h"
#include <algorithm>
#include <cfloat>

namespace {

// Turns an InlineSetting value into the option it names. The value is a '|' separated list of the
// option's name and the arguments that go with it. The name has to be a whole argument of its own:
// "Size|10" is the Size option, "Sized|10" is not. An argument that is there but is not a number
// reads as zero, while the default for what it sets is what an argument that is not there reads as.
// Returns nothing when the option is not recognized or does not have all the arguments it needs.
std::optional<Gfx::TextInlineSetting> ParseInlineSetting(std::wstring_view setting, ConfigParser& parser)
{
	const auto skipWS = StringParser::SkipWhitespace;
	const MathParser& mathParser = parser.GetMathParser();
	StringParser strParser(setting);

	// Only ConsumeUntil() trims what it matched, so the whitespace an argument ends with is left
	// for the separator to step over.
	auto consumeSeparator = [&strParser, skipWS]() { return strParser.Consume(L'|', skipWS); };

	// |true| while there is anything left to read.
	auto hasNextValue = [&strParser]() { strParser.ConsumeWhitespace(); return !strParser.IsConsumed(); };

	// The next argument, or an empty value when there are none left. Empty arguments are not
	// arguments at all, so "Color||FF0000" reads the same as "Color|FF0000".
	auto consumeNextValue = [&strParser, skipWS]()
	{
		while (!strParser.IsConsumed())
		{
			const auto value = strParser.ConsumeUntilOrRest(L'|', skipWS);
			if (!value.empty()) return value;
		}

		return std::wstring_view();
	};

	// Numbers are read from the argument as a whole, so that what follows an unreadable one is
	// still read as the argument it was written as. A value that is not a number reads as zero.
	auto toDouble = [&mathParser, skipWS](std::wstring_view value)
	{
		StringParser valueParser(value);
		return valueParser.ConsumeDoubleOrFormula(mathParser, skipWS).value_or(0.0);
	};

	auto toInt = [&mathParser, skipWS](std::wstring_view value)
	{
		StringParser valueParser(value);
		return valueParser.ConsumeIntOrFormula(mathParser, skipWS).value_or(0);
	};

	auto toUInt = [&mathParser, skipWS](std::wstring_view value)
	{
		StringParser valueParser(value);
		return valueParser.ConsumeUIntOrFormula(mathParser, skipWS).value_or(0u);
	};

	// An option that takes no arguments still has to be a whole argument of its own, while any
	// argument written after it is ignored.
	auto isWholeValue = [&hasNextValue, &consumeSeparator]() { return !hasNextValue() || consumeSeparator(); };

	if (strParser.Consume(L"None", skipWS))
	{
		return Gfx::InlineSetting::None{};
	}
	else if (strParser.Consume(L"Case", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		StringParser caseParser(consumeNextValue());
		if (caseParser.ConsumeRest(L"Lower", skipWS)) return Gfx::InlineSetting::Case{ Gfx::CaseType::Lower };
		if (caseParser.ConsumeRest(L"Upper", skipWS)) return Gfx::InlineSetting::Case{ Gfx::CaseType::Upper };
		if (caseParser.ConsumeRest(L"Proper", skipWS)) return Gfx::InlineSetting::Case{ Gfx::CaseType::Proper };
		if (caseParser.ConsumeRest(L"Sentence", skipWS)) return Gfx::InlineSetting::Case{ Gfx::CaseType::Sentence };

		// Only allow the above options.
		return std::nullopt;
	}
	else if (strParser.Consume(L"CharacterSpacing", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		// A spacing of "*" leaves that side to DirectWrite, and so does one that is not there.
		auto toSpacing = [&toDouble](std::wstring_view value)
		{
			if (value.empty() || value.starts_with(L'*')) return FLT_MAX;
			return (FLOAT)toDouble(value);
		};

		const auto leading = consumeNextValue();
		if (leading.empty()) return std::nullopt;

		Gfx::InlineSetting::CharacterSpacing spacing = {};
		spacing.leading = toSpacing(leading);
		spacing.trailing = toSpacing(consumeNextValue());

		const auto advanceWidth = consumeNextValue();
		spacing.advanceWidth = advanceWidth.empty() ? -1.0f : (FLOAT)toDouble(advanceWidth);
		return spacing;
	}
	else if (strParser.Consume(L"Color", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto color = consumeNextValue();
		if (color.empty()) return std::nullopt;

		return Gfx::InlineSetting::Color{ parser.ParseColor(color) };
	}
	else if (strParser.Consume(L"Face", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto face = consumeNextValue();
		if (face.empty()) return std::nullopt;

		return Gfx::InlineSetting::Face{ std::wstring(face) };
	}
	else if (strParser.Consume(L"GradientColor", skipWS))
	{
		// The name carries the gamma with it, as in "GradientColor1". ConsumeUntil() leaves nothing
		// behind when the name is all there is, which the angle is then missing from.
		const auto altGamma = strParser.ConsumeUntil(L'|', skipWS);

		const auto angle = consumeNextValue();
		if (angle.empty()) return std::nullopt;

		Gfx::InlineSetting::GradientColor gradient = {};
		gradient.altGamma = toInt(altGamma) != 0;
		gradient.angle = (FLOAT)fmod((360.0 + fmod(toDouble(angle), 360.0)), 360.0);

		const auto skipWSAndParens = StringParser::SkipWhitespace | StringParser::SkipNestedParentheses;

		while (!strParser.IsConsumed())
		{
			const auto value = consumeNextValue();
			if (value.empty()) break;

			// A stop that cannot be read is left zero initialized, so that the stops after it keep
			// the position they were written in.
			auto& stop = gradient.stops.emplace_back();

			// A stop is a color and a position, "Color;Position".
			StringParser stopParser(value);
			const auto color = stopParser.ConsumeUntil(L';', skipWSAndParens);
			stopParser.ConsumeWhitespace();
			if (stopParser.IsConsumed()) continue;

			const auto position = stopParser.ConsumeUntilOrRest(L';', skipWSAndParens);
			stopParser.ConsumeWhitespace();
			if (!stopParser.IsConsumed()) continue;

			stop.color = parser.ParseColor(color);
			stop.position = (FLOAT)toDouble(position);
		}

		if (gradient.stops.empty()) return std::nullopt;

		// If gradient only has 1 stop, add a transparent stop at appropriate place
		if (gradient.stops.size() == 1)
		{
			D2D1::ColorF color = { 0.0f, 0.0f, 0.0f, 0.0f };
			D2D1_GRADIENT_STOP stop = { 0.0f, color };
			if (gradient.stops[0].position < 0.5f)
			{
				stop.position = 1.0f;
			}

			gradient.stops.push_back(stop);
		}

		return gradient;
	}
	else if (strParser.Consume(L"Italic", skipWS))
	{
		if (!isWholeValue()) return std::nullopt;

		return Gfx::InlineSetting::Italic{};
	}
	else if (strParser.Consume(L"Oblique", skipWS))
	{
		if (!isWholeValue()) return std::nullopt;

		return Gfx::InlineSetting::Oblique{};
	}
	else if (strParser.Consume(L"Shadow", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto x = consumeNextValue();
		const auto y = consumeNextValue();
		const auto blur = consumeNextValue();
		const auto color = consumeNextValue();
		if (x.empty() || y.empty() || blur.empty() || color.empty()) return std::nullopt;

		return Gfx::InlineSetting::Shadow{
			(FLOAT)toDouble(blur),
			{ (FLOAT)toDouble(x), (FLOAT)toDouble(y) },
			parser.ParseColor(color) };
	}
	else if (strParser.Consume(L"Size", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto size = consumeNextValue();
		if (size.empty()) return std::nullopt;

		return Gfx::InlineSetting::Size{ (FLOAT)toDouble(size) };
	}
	else if (strParser.Consume(L"Stretch", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto stretch = consumeNextValue();
		if (stretch.empty()) return std::nullopt;

		// DirectWrite supports 9 different stretch properties.
		return Gfx::InlineSetting::Stretch{ (DWRITE_FONT_STRETCH)std::clamp(toInt(stretch),
			(int)DWRITE_FONT_STRETCH_ULTRA_CONDENSED, (int)DWRITE_FONT_STRETCH_ULTRA_EXPANDED) };
	}
	else if (strParser.Consume(L"Strikethrough", skipWS))
	{
		if (!isWholeValue()) return std::nullopt;

		return Gfx::InlineSetting::Strikethrough{};
	}
	else if (strParser.Consume(L"Typography", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		// Typography 'tags' need to be extactly 4 characters.
		const auto tag = consumeNextValue();
		if (tag.size() != 4) return std::nullopt;

		const auto tagValue = (DWRITE_FONT_FEATURE_TAG)
			DWRITE_MAKE_OPENTYPE_TAG(tag[0], tag[1], tag[2], tag[3]);
		// A parameter that is not there is a parameter of 1.
		const auto parameter = consumeNextValue();
		return Gfx::InlineSetting::Typography{ tagValue, parameter.empty() ? 1u : toUInt(parameter) };
	}
	else if (strParser.Consume(L"Underline", skipWS))
	{
		if (!isWholeValue()) return std::nullopt;

		return Gfx::InlineSetting::Underline{};
	}
	else if (strParser.Consume(L"Weight", skipWS))
	{
		if (!consumeSeparator()) return std::nullopt;

		const auto weight = consumeNextValue();
		if (weight.empty()) return std::nullopt;

		// DirectWrite supports weight from 1 to 999.
		return Gfx::InlineSetting::Weight{ (DWRITE_FONT_WEIGHT)std::clamp(toInt(weight), 1, 999) };
	}

	return std::nullopt;
}

std::vector<std::vector<Gfx::TextInlineRange>> FindInlineRanges(
	const std::wstring& str, const Gfx::TextFormat& format)
{
	const size_t count = format.GetInlineOptionCount();

	std::vector<std::vector<Gfx::TextInlineRange>> inlineRanges;
	inlineRanges.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		const std::wstring& pattern = format.GetInlinePattern(i);
		if (pattern == L".*")
		{
			// Empty string is not a valid match.
			if (str.empty())
			{
				inlineRanges.emplace_back();
			}
			else
			{
				inlineRanges.push_back({ { 0, (UINT32)str.length() } });
			}

			continue;
		}

		std::vector<Gfx::TextInlineRange> ranges;
		int ovector[300];
		const char* error;
		Pcre re(pattern.c_str(), &error);
		if (!re)
		{
			//LogNoticeF(this, L"InlinePattern%i error at offset %d: %S", re.GetErrorOffset(), error);
		}
		else
		{
			do
			{
				// Empty string is not a valid match.
				const int rc = re.Execute(str, PCRE_NOTEMPTY, ovector, (int)_countof(ovector));
				if (rc <= 0)
				{
					break;
				}

				const UINT32 start = ovector[0];
				const UINT32 length = ovector[1] - ovector[0];

				// No captures found, but the rest of the text is still 'found'.
				if (rc == 1)
				{
					Gfx::TextInlineRange range = { start, length };
					ranges.push_back(range);
				}
				else if (rc > 1)	// Captures found.
				{
					for (int j = rc - 1; j > 0; --j)
					{
						const int newStart = ovector[2 * j];
						const int newEnd = ovector[2 * j + 1];
						if (newStart < 0 || newEnd < 0) break;	// Match was not found, so skip to the next item

						Gfx::TextInlineRange range = { (UINT32)newStart, (UINT32)(newEnd - newStart) };
						ranges.push_back(range);
					}
				}

				re.SetOffset(start + length);

			} while (true);
		}

		inlineRanges.push_back(std::move(ranges));
	}

	return inlineRanges;
}

}  // namespace

MeterStringBase::MeterStringBase(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Color(D2D1::ColorF(D2D1::ColorF::White)),
	m_EffectColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_Style(NORMAL),
	m_Effect(EFFECT_NONE),
	m_FontSize(10.0f),
	m_ClipType(CLIP_OFF),
	m_Case(TEXTCASE_NONE),
	m_NeedsClipping(false),
	m_ClipStringW(-1),
	m_ClipStringH(-1),
	m_TextFormat(skin->GetCanvas().CreateTextFormat()),
	m_FontWeight(-1),
	m_TextOffset()
{
}

MeterStringBase::~MeterStringBase()
{
}

void MeterStringBase::InvalidateDeviceResources()
{
	Meter::InvalidateDeviceResources();
	if (m_TextFormat)
	{
		m_TextFormat->InvalidateDeviceResources();
	}
}

int MeterStringBase::GetX(bool abs)
{
	int x = Meter::GetX();

	if (!abs)
	{
		switch (m_TextFormat->GetHorizontalAlignment())
		{
		case Gfx::HorizontalAlignment::Center:
			x -= m_W / 2;
			break;

		case Gfx::HorizontalAlignment::Right:
			x -= m_W;
			break;
		}
	}

	return x;
}

int MeterStringBase::GetY(bool abs)
{
	int y = Meter::GetY();

	if (!abs)
	{
		switch (m_TextFormat->GetVerticalAlignment())
		{
		case Gfx::VerticalAlignment::Center:
			y -= m_H / 2;
			break;

		case Gfx::VerticalAlignment::Bottom:
			y -= m_H;
			break;
		}
	}

	return y;
}

void MeterStringBase::Initialize()
{
	Meter::Initialize();

	m_TextFormat->SetProperties(
		m_FontFace.c_str(),
		m_FontSize,
		(m_Style & BOLD) != 0,
		(m_Style & ITALIC) != 0,
		m_Skin->GetFontCollection());
}

void MeterStringBase::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	FLOAT oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;
	Gfx::HorizontalAlignment oldHAlign = m_TextFormat->GetHorizontalAlignment();
	Gfx::VerticalAlignment oldVAlign = m_TextFormat->GetVerticalAlignment();

	Meter::ReadOptions(parser, section);

	m_Color = parser.ReadColor(section, L"FontColor", D2D1::ColorF(D2D1::ColorF::Black));
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", D2D1::ColorF(D2D1::ColorF::Black));

	int clipping = parser.ReadInt(section, L"ClipString", 0);
	switch (clipping)
	{
	case 2:
		m_ClipType = CLIP_AUTO;

		m_ClipStringW = parser.ReadInt(section, L"ClipStringW", -1);
		m_ClipStringH = parser.ReadInt(section, L"ClipStringH", -1);
		break;

	case 1:
		m_ClipType = CLIP_ON;
		break;

	case 0:
		m_ClipType = CLIP_OFF;
		break;

	default:
		LogErrorF(this, L"ClipString=%i is not valid", clipping);
	}

	parser.ReadString(m_FontFace, section, L"FontFace", L"Arial");
	if (m_FontFace.empty())
	{
		m_FontFace = L"Arial";
	}

	m_FontSize = (FLOAT)parser.ReadFloat(section, L"FontSize", 10.0);
	if (m_FontSize < 0.0f)
	{
		m_FontSize = 10.0f;
	}

	const WCHAR* hAlign = parser.ReadString(section, L"StringAlign", L"LEFT").c_str();
	const WCHAR* vAlign = nullptr;
	if (_wcsnicmp(hAlign, L"LEFT", 4) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Left);
		vAlign = hAlign + 4;
	}
	else if (_wcsnicmp(hAlign, L"RIGHT", 5) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Right);
		vAlign = hAlign + 5;
	}
	else if (_wcsnicmp(hAlign, L"CENTER", 6) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Center);
		vAlign = hAlign + 6;
	}
	else if (_wcsnicmp(hAlign, L"JUSTIFY", 7) == 0)
	{
		m_TextFormat->SetHorizontalAlignment(Gfx::HorizontalAlignment::Justify);
		vAlign = hAlign + 7;
	}

	if (!vAlign || _wcsicmp(vAlign, L"TOP") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Top);
	}
	else if (_wcsicmp(vAlign, L"BOTTOM") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Bottom);
	}
	else if (_wcsicmp(vAlign, L"CENTER") == 0)
	{
		m_TextFormat->SetVerticalAlignment(Gfx::VerticalAlignment::Center);
	}

	m_Style = ReadStringStyle(parser, section, L"StringStyle", NORMAL);

	m_Case = ReadStringCase(parser, section, L"StringCase", TEXTCASE_NONE);

	int weight = parser.ReadInt(section, L"FontWeight", -1);
	if (parser.GetLastValueDefined())
	{
		if (weight > 0 && weight < 1000)
		{
			m_FontWeight = weight;
		}
		else
		{
			LogErrorF(this, L"Invalid FontWeight: %i", weight);
		}
	}

	const WCHAR* effect = parser.ReadString(section, L"StringEffect", L"NONE").c_str();
	if (_wcsicmp(effect, L"NONE") == 0)
	{
		m_Effect = EFFECT_NONE;
	}
	else if (_wcsicmp(effect, L"SHADOW") == 0)
	{
		m_Effect = EFFECT_SHADOW;
	}
	else if (_wcsicmp(effect, L"BORDER") == 0)
	{
		m_Effect = EFFECT_BORDER;
	}
	else
	{
		LogErrorF(this, L"StringEffect=%s is not valid", effect);
	}

	std::vector<Gfx::TextInlineOption> inlineOptions;
	inlineOptions.reserve(m_TextFormat->GetInlineOptionCount());

	for (int i = 1; ; ++i)
	{
		WCHAR num[8] = { 0 };
		if (i > 1) _itow_s(i, num, 10);

		WCHAR settingKey[32] = L"InlineSetting";
		wcscat_s(settingKey, num);
		const auto& option = parser.ReadString(section, settingKey, L"");
		if (option.empty()) break;

		auto setting = ParseInlineSetting(option, parser);
		if (!setting) break;

		WCHAR patternKey[32] = L"InlinePattern";
		wcscat_s(patternKey, num);
		std::wstring pattern = parser.ReadString(section, patternKey, L"");
		if (pattern.empty()) pattern = L".*";

		inlineOptions.push_back({ std::move(pattern), std::move(*setting) });
	}

	m_TextFormat->SetInlineOptions(inlineOptions);

	if (m_Initialized &&
		(wcscmp(oldFontFace.c_str(), m_FontFace.c_str()) != 0 ||
		oldFontSize != m_FontSize ||
		oldStyle != m_Style ||
		oldHAlign != m_TextFormat->GetHorizontalAlignment() ||
		oldVAlign != m_TextFormat->GetVerticalAlignment()))
	{
		Initialize();	// Recreate the font
	}
}

MeterStringBase::TEXTSTYLE MeterStringBase::ReadStringStyle(
	ConfigParser& parser, std::wstring_view section, const WCHAR* option, TEXTSTYLE defaultStyle)
{
	const WCHAR* value = parser.ReadString(section, option, L"").c_str();
	if (!*value) return defaultStyle;

	if (_wcsicmp(value, L"NORMAL") == 0) return NORMAL;
	if (_wcsicmp(value, L"BOLD") == 0) return BOLD;
	if (_wcsicmp(value, L"ITALIC") == 0) return ITALIC;
	if (_wcsicmp(value, L"BOLDITALIC") == 0) return BOLDITALIC;

	LogErrorF(this, L"%s=%s is not valid", option, value);
	return defaultStyle;
}

MeterStringBase::TEXTCASE MeterStringBase::ReadStringCase(
	ConfigParser& parser, std::wstring_view section, const WCHAR* option, TEXTCASE defaultCase)
{
	const WCHAR* value = parser.ReadString(section, option, L"").c_str();
	if (!*value) return defaultCase;

	if (_wcsicmp(value, L"NONE") == 0) return TEXTCASE_NONE;
	if (_wcsicmp(value, L"UPPER") == 0) return TEXTCASE_UPPER;
	if (_wcsicmp(value, L"LOWER") == 0) return TEXTCASE_LOWER;
	if (_wcsicmp(value, L"PROPER") == 0) return TEXTCASE_PROPER;

	LogErrorF(this, L"%s=%s is not valid", option, value);
	return defaultCase;
}

void MeterStringBase::ApplyCase(std::wstring& text, TEXTCASE textCase) const
{
	if (text.empty()) return;

	switch (textCase)
	{
	case TEXTCASE_UPPER:
		StringUtil::ToUpperCase(text);
		break;
	case TEXTCASE_LOWER:
		StringUtil::ToLowerCase(text);
		break;
	case TEXTCASE_PROPER:
		StringUtil::ToProperCase(text);
		break;
	}
}

bool MeterStringBase::ShouldClip() const
{
	return m_ClipType == CLIP_ON ||
		(m_ClipType == CLIP_AUTO && (m_NeedsClipping || (m_WDefined && m_HDefined)));
}

void MeterStringBase::ApplyTextState(Gfx::Canvas& canvas, Gfx::TextFormat* format)
{
	if (!format) format = m_TextFormat.get();

	canvas.SetTextAntiAliasing(m_AntiAlias);

	// The two follow ClipString together for String. A TextEdit takes the wrapping and leaves the
	// ellipsis, since it scrolls to the text that does not fit rather than announcing it, and a
	// single-line one leaves the wrapping as well.
	format->SetTrimming(ShouldTrim(), ShouldClip() && CanWrap());
}

D2D1_RECT_F MeterStringBase::GetTextRect()
{
	D2D1_RECT_F rect = GetMeterRectPadding();
	rect.left -= m_TextOffset.x;
	rect.right -= m_TextOffset.x;
	rect.top -= m_TextOffset.y;
	rect.bottom -= m_TextOffset.y;
	return rect;
}

void MeterStringBase::UpdateTextFormat()
{
	m_TextFormat->SetFontWeight(m_FontWeight);
	m_TextFormat->SetInlineRanges(FindInlineRanges(m_String, *m_TextFormat));
}

void MeterStringBase::UpdateAutoSize(const std::wstring* str, Gfx::TextFormat* format)
{
	if (m_WDefined && m_HDefined) return;

	const int oldW = m_W;
	const int oldH = m_H;

	const auto rect = MeasureStringBounds(m_Skin->GetCanvas(), str, format);
	if (rect)
	{
		if (!m_WDefined) m_W = (int)(rect->right - rect->left) + GetWidthPadding();
		if (!m_HDefined) m_H = (int)(rect->bottom - rect->top) + GetHeightPadding();
	}
	else
	{
		m_W = 1;
		m_H = 1;
	}

	if (m_W != oldW || m_H != oldH)
	{
		m_Skin->RequestWindowSizeCheck();
	}
}

std::optional<D2D1_RECT_F> MeterStringBase::MeasureStringBounds(Gfx::Canvas& canvas, const std::wstring* str, Gfx::TextFormat* format)
{
	if (!str) str = &m_String;
	if (!format) format = m_TextFormat.get();

	if (!format->IsInitialized()) return std::nullopt;

	ApplyTextState(canvas, format);

	const D2D1_RECT_F meterRect = GetMeterRectPadding();

	D2D1_SIZE_F size = D2D1::SizeF();
	if (!canvas.MeasureTextW(*str, *format, size)) return std::nullopt;

	D2D1_RECT_F rect = D2D1::RectF(
		meterRect.left,
		meterRect.top,
		meterRect.left + size.width,
		meterRect.top + size.height);

	if (m_ClipType != CLIP_AUTO) return rect;

	// Set initial clipping
	m_NeedsClipping = false;

	FLOAT w = 0.0f;
	FLOAT h = 0.0f;
	bool updateSize = true;

	if (m_WDefined)
	{
		w = meterRect.right - meterRect.left;
		h = rect.bottom - rect.top;
		m_NeedsClipping = true;
	}
	else if (m_HDefined)
	{
		if (m_ClipStringW == -1)
		{
			// Text does not fit in defined height, clip it
			if (rect.bottom - rect.top > meterRect.bottom - meterRect.top)
			{
				m_NeedsClipping = true;
			}

			rect.bottom = meterRect.bottom;
			updateSize = false;

		}
		else
		{
			if (rect.right - rect.left > (FLOAT)m_ClipStringW)
			{
				w = (FLOAT)m_ClipStringW;
				m_NeedsClipping = true;
			}
			else
			{
				w = rect.right - rect.left;
			}

			h = meterRect.bottom - meterRect.top;
		}
	}
	else
	{
		if (m_ClipStringW == -1)
		{
			// Clip text if already larger than ClipStringH
			if (m_ClipStringH != -1 && rect.bottom - rect.top > (FLOAT)m_ClipStringH)
			{
				m_NeedsClipping = true;
				rect.bottom = rect.top + (FLOAT)m_ClipStringH;
			}

			updateSize = false;
		}
		else
		{
			if (rect.right - rect.left > (FLOAT)m_ClipStringW)
			{
				w = (FLOAT)m_ClipStringW;
				m_NeedsClipping = true;
			}
			else
			{
				w = rect.right - rect.left;
			}

			h = rect.bottom - rect.top;
		}
	}

	if (updateSize)
	{
		if (CanWrap())
		{
			UINT32 lines = 0;
			D2D1_SIZE_F wrapped = D2D1::SizeF(w, h);
			if (canvas.MeasureTextLinesW(*str, *format, wrapped, lines) && lines != 0)
			{
				rect.right = rect.left + w;
				rect.bottom = rect.top + wrapped.height;
			}
		}
		else
		{
			// MeasureTextLinesW() reflows the text to |w| whatever the format says, so a meter that
			// cannot wrap has to keep out of it: the height it needs is the one line MeasureTextW()
			// already measured, and all the clipping decides for it is the width it is given.
			rect.right = rect.left + w;
		}

		if (m_HDefined || (m_ClipStringH != -1 && rect.bottom - rect.top > (FLOAT)m_ClipStringH))
		{
			rect.bottom = rect.top + (m_HDefined ? (meterRect.bottom - meterRect.top) : (FLOAT)m_ClipStringH);
		}
	}

	return rect;
}

bool MeterStringBase::DrawString(Gfx::Canvas& canvas, const std::wstring* str, Gfx::TextFormat* format, const D2D1_COLOR_F* color)
{
	if (!str) str = &m_String;
	if (!format) format = m_TextFormat.get();
	if (!color) color = &m_Color;

	if (!format->IsInitialized()) return false;

	ApplyTextState(canvas, format);

	const D2D1_RECT_F rcDest = GetTextRect();

	if (m_Effect != EFFECT_NONE)
	{
		D2D1_RECT_F rcEffect = rcDest;

		auto offsetEffect = [&](FLOAT x, FLOAT y)
		{
			rcEffect.left += x;
			rcEffect.right += x;
			rcEffect.top += y;
			rcEffect.bottom += y;
		};

		if (m_Effect == EFFECT_SHADOW)
		{
			offsetEffect(1.0f, 1.0f);
			canvas.DrawTextW(*str, *format, rcEffect, m_EffectColor);
		}
		else  //if (m_Effect == EFFECT_BORDER)
		{
			offsetEffect(0.0f, 1.0f);
			canvas.DrawTextW(*str, *format, rcEffect, m_EffectColor);
			offsetEffect(1.0f, -1.0f);
			canvas.DrawTextW(*str, *format, rcEffect, m_EffectColor);
			offsetEffect(-1.0f, -1.0f);
			canvas.DrawTextW(*str, *format, rcEffect, m_EffectColor);
			offsetEffect(-1.0f, 1.0f);
			canvas.DrawTextW(*str, *format, rcEffect, m_EffectColor);
		}
	}

	canvas.DrawTextW(*str, *format, rcDest, *color, true);
	return true;
}

void MeterStringBase::InitializeStatic()
{
	if (GetRainmeter().GetDebug())
	{
		LogDebug(L"------------------------------");

		UINT32 familyCount = 0;
		std::wstring families;
		bool success = Gfx::Canvas::EnumerateInstalledFontFamilies(familyCount, families);
		LogDebugF(L"* Font families: Count=%i", familyCount);
		if (success)
		{
			LogDebug(families.c_str());
		}
		else
		{
			LogError(families.c_str());
		}

		LogDebug(L"------------------------------");
	}
}

void MeterStringBase::FinalizeStatic()
{
}
