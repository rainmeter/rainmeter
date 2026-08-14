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

std::optional<Gfx::TextInlineSetting> ParseInlineSetting(const std::vector<std::wstring>& settings, ConfigParser& parser)
{
	if (settings.empty()) return std::nullopt;

	const size_t count = settings.size();
	const WCHAR* option = settings[0].c_str();
	if (_wcsnicmp(option, L"NONE", 4) == 0)
	{
		return Gfx::InlineSetting::None{};
	}
	else if (_wcsicmp(option, L"CASE") == 0)
	{
		if (count > 1)
		{
			const WCHAR* strCase = settings[1].c_str();
			Gfx::CaseType type = Gfx::CaseType::None;

			if (_wcsicmp(strCase, L"LOWER") == 0) type = Gfx::CaseType::Lower;
			else if (_wcsicmp(strCase, L"UPPER") == 0) type = Gfx::CaseType::Upper;
			else if (_wcsicmp(strCase, L"PROPER") == 0) type = Gfx::CaseType::Proper;
			else if (_wcsicmp(strCase, L"SENTENCE") == 0) type = Gfx::CaseType::Sentence;

			// Only allow the above options.
			if (type == Gfx::CaseType::None) return std::nullopt;

			return Gfx::InlineSetting::Case{ type };
		}
	}
	else if (_wcsicmp(option, L"CHARACTERSPACING") == 0)
	{
		if (count > 1)
		{
			auto parseOptional = [&parser](const WCHAR* value) -> FLOAT
			{
				if (_wcsnicmp(value, L"*", 1) == 0) return FLT_MAX;
				return (FLOAT)parser.ParseDouble(value, FLT_MAX);
			};

			Gfx::InlineSetting::CharacterSpacing spacing = { parseOptional(settings[1].c_str()), FLT_MAX, -1.0f };

			if (count > 2)
			{
				spacing.trailing = parseOptional(settings[2].c_str());
			}

			if (count > 3)
			{
				spacing.advanceWidth = (FLOAT)parser.ParseDouble(settings[3].c_str(), -1.0f);
			}

			return spacing;
		}
	}
	else if (_wcsicmp(option, L"COLOR") == 0)
	{
		if (count > 1)
		{
			return Gfx::InlineSetting::Color{ parser.ParseColor(settings[1].c_str()) };
		}
	}
	else if (_wcsicmp(option, L"FACE") == 0)
	{
		if (count > 1)
		{
			return Gfx::InlineSetting::Face{ settings[1] };
		}
	}
	else if (_wcsnicmp(option, L"GRADIENTCOLOR", 13) == 0)
	{
		if (count >= 3)
		{
			Gfx::InlineSetting::GradientColor gradient;
			gradient.altGamma = parser.ParseInt(option + 13, 0) != 0;
			gradient.angle = (FLOAT)fmod((360.0 + fmod(parser.ParseDouble(settings[1].c_str(), 0.0), 360.0)), 360.0);

			// The first two options are the 'GRADIENTCOLOR' keyword and the angle, the rest are stops.
			gradient.stops.resize(count - 2);
			for (size_t i = 2; i < count; ++i)
			{
				const auto consumeOption = StringParser::SkipWhitespace | StringParser::SkipNestedParentheses;

				// A stop is a color and a position, "Color;Position".
				StringParser values(settings[i]);
				const auto color = values.ConsumeUntil(L';', consumeOption);
				values.ConsumeWhitespace();
				if (values.IsConsumed()) continue;

				const auto position = values.ConsumeUntilOrRest(L';', consumeOption);
				values.ConsumeWhitespace();
				if (!values.IsConsumed()) continue;

				gradient.stops[i - 2].color = parser.ParseColor(color);
				gradient.stops[i - 2].position = (FLOAT)parser.ParseDouble(position, 0.0);
			}

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
	}
	else if (_wcsicmp(option, L"ITALIC") == 0)
	{
		return Gfx::InlineSetting::Italic{};
	}
	else if (_wcsicmp(option, L"OBLIQUE") == 0)
	{
		return Gfx::InlineSetting::Oblique{};
	}
	else if (_wcsicmp(option, L"SHADOW") == 0)
	{
		if (count >= 5)
		{
			D2D1_POINT_2F offset = {
				(FLOAT)parser.ParseDouble(settings[1].c_str(), 1.0),
				(FLOAT)parser.ParseDouble(settings[2].c_str(), 1.0) };

			return Gfx::InlineSetting::Shadow{
				(FLOAT)parser.ParseDouble(settings[3].c_str(), 3.0),
				offset,
				parser.ParseColor(settings[4].c_str()) };
		}
	}
	else if (_wcsicmp(option, L"SIZE") == 0)
	{
		if (count > 1)
		{
			return Gfx::InlineSetting::Size{ (FLOAT)parser.ParseDouble(settings[1].c_str(), 10.0) };
		}
	}
	else if (_wcsicmp(option, L"STRETCH") == 0)
	{
		if (count > 1)
		{
			// DirectWrite supports 9 different stretch properties.
			return Gfx::InlineSetting::Stretch{ (DWRITE_FONT_STRETCH)
				std::clamp(parser.ParseInt(settings[1].c_str(), -1),
					(int)DWRITE_FONT_STRETCH_ULTRA_CONDENSED,
					(int)DWRITE_FONT_STRETCH_ULTRA_EXPANDED) };
		}
	}
	else if (_wcsicmp(option, L"STRIKETHROUGH") == 0)
	{
		return Gfx::InlineSetting::Strikethrough{};
	}
	else if (_wcsicmp(option, L"TYPOGRAPHY") == 0)
	{
		// Typography 'tags' need to be extactly 4 characters.
		if (count > 1 && settings[1].size() == 4)
		{
			Gfx::InlineSetting::Typography typography = { (DWRITE_FONT_FEATURE_TAG)
				DWRITE_MAKE_OPENTYPE_TAG(settings[1][0], settings[1][1], settings[1][2], settings[1][3]), 1u };

			if (count > 2)
			{
				typography.parameter = parser.ParseUInt(settings[2].c_str(), 1u);
			}

			return typography;
		}
	}
	else if (_wcsicmp(option, L"UNDERLINE") == 0)
	{
		return Gfx::InlineSetting::Underline{};
	}
	else if (_wcsicmp(option, L"WEIGHT") == 0)
	{
		if (count > 1)
		{
			// DirectWrite supports weight from 1 to 999.
			return Gfx::InlineSetting::Weight{ (DWRITE_FONT_WEIGHT)
				std::clamp(parser.ParseInt(settings[1].c_str(), -1), 1, 999) };
		}
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

void MeterStringBase::ReadOptions(ConfigParser& parser, const WCHAR* section)
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

	m_FontFace = parser.ReadString(section, L"FontFace", L"Arial");
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
	std::wstring option = parser.ReadString(section, L"InlineSetting", L"");
	std::wstring pattern = parser.ReadString(section, L"InlinePattern", L".*");
	if (pattern.empty()) pattern = L".*";

	size_t i = 1;
	while (!option.empty())
	{
		std::vector<std::wstring> settings;
		StringParser::Split(option, L'|', settings);

		// An unusable setting ends the list - the ones after it are ignored as well.
		auto setting = ParseInlineSetting(settings, parser);
		if (!setting) break;

		inlineOptions.push_back({ pattern, std::move(*setting) });

		// Check for InlineSetting2/InlinePattern2 ... etc.
		const std::wstring num = std::to_wstring(++i);

		std::wstring key = L"InlinePattern" + num;
		pattern = parser.ReadString(section, key.c_str(), L".*");
		if (pattern.empty()) pattern = L".*";

		key = L"InlineSetting" + num;
		option = parser.ReadString(section, key.c_str(), L"");
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
	ConfigParser& parser, const WCHAR* section, const WCHAR* option, TEXTSTYLE defaultStyle)
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
	ConfigParser& parser, const WCHAR* section, const WCHAR* option, TEXTCASE defaultCase)
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

bool MeterStringBase::ShouldTrim() const
{
	return m_ClipType == CLIP_ON ||
		(m_ClipType == CLIP_AUTO && (m_NeedsClipping || (m_WDefined && m_HDefined)));
}

void MeterStringBase::ApplyTextState(Gfx::Canvas& canvas, Gfx::TextFormat* format)
{
	if (!format) format = m_TextFormat.get();

	canvas.SetTextAntiAliasing(m_AntiAlias);
	format->SetTrimming(ShouldTrim());
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
		UINT32 lines = 0;
		D2D1_SIZE_F wrapped = D2D1::SizeF(w, h);
		if (canvas.MeasureTextLinesW(*str, *format, wrapped, lines) && lines != 0)
		{
			rect.right = rect.left + w;
			rect.bottom = rect.top + wrapped.height;

			if (m_HDefined || (m_ClipStringH != -1 && rect.bottom - rect.top > (FLOAT)m_ClipStringH))
			{
				rect.bottom = rect.top + (m_HDefined ? (meterRect.bottom - meterRect.top) : (FLOAT)m_ClipStringH);
			}
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
