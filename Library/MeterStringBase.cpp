// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeterStringBase.h"
#include "Rainmeter.h"
#include "Pcre.h"
#include "../Common/Gfx/Canvas.h"
#include "../Common/StringParser.h"

namespace {

std::vector<std::vector<Gfx::TextInlineRange>> FindInlineRanges(
	const std::wstring& str, const std::vector<std::wstring>& patterns)
{
	std::vector<std::vector<Gfx::TextInlineRange>> inlineRanges;
	inlineRanges.reserve(patterns.size());

	for (const auto& pattern : patterns)
	{
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

		inlineRanges.push_back(ranges);
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
	m_NeedsClipping(false),
	m_ClipStringW(-1),
	m_ClipStringH(-1),
	m_TextFormat(skin->GetCanvas().CreateTextFormat(skin->GetMathParser())),
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
		Gfx::TextInlineOption inlineOption;
		inlineOption.pattern = pattern;
		StringParser::Split(option, L'|', inlineOption.settings);

		inlineOptions.push_back(inlineOption);

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

void MeterStringBase::ApplyTextState(Gfx::Canvas& canvas, Gfx::TextFormat* format)
{
	if (!format) format = m_TextFormat.get();

	canvas.SetTextAntiAliasing(m_AntiAlias);

	format->SetTrimming(
		m_ClipType == CLIP_ON ||
		(m_ClipType == CLIP_AUTO && (m_NeedsClipping || (m_WDefined && m_HDefined))));
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
	m_TextFormat->SetInlineRanges(FindInlineRanges(m_String, m_TextFormat->GetInlinePatterns()));
}

void MeterStringBase::UpdateAutoSize(const std::wstring* str, Gfx::TextFormat* format)
{
	if (m_WDefined && m_HDefined) return;

	D2D1_RECT_F rect = { 0 };
	if (DrawString(m_Skin->GetCanvas(), &rect, str, format))
	{
		if (!m_WDefined) m_W = (int)(rect.right - rect.left) + GetWidthPadding();
		if (!m_HDefined) m_H = (int)(rect.bottom - rect.top) + GetHeightPadding();
	}
	else
	{
		m_W = 1;
		m_H = 1;
	}
}

bool MeterStringBase::DrawString(Gfx::Canvas& canvas, D2D1_RECT_F* rect,
	const std::wstring* str, Gfx::TextFormat* format)
{
	if (!str) str = &m_String;
	if (!format) format = m_TextFormat.get();

	if (!format->IsInitialized()) return false;

	ApplyTextState(canvas, format);

	D2D1_RECT_F meterRect = GetMeterRectPadding();

	if (rect)
	{
		rect->left = meterRect.left;
		rect->top = meterRect.top;

		D2D1_SIZE_F size = D2D1::SizeF(rect->right - rect->left, rect->bottom - rect->top);
		if (canvas.MeasureTextW(*str, *format, size))
		{
			rect->right = rect->left + size.width;
			rect->bottom = rect->top + size.height;

			if (m_ClipType == CLIP_AUTO)
			{
				// Set initial clipping
				m_NeedsClipping = false;

				FLOAT w = 0.0f;
				FLOAT h = 0.0f;
				bool updateSize = true;

				if (m_WDefined)
				{
					w = meterRect.right - meterRect.left;
					h = rect->bottom - rect->top;
					m_NeedsClipping = true;
				}
				else if (m_HDefined)
				{
					if (m_ClipStringW == -1)
					{
						// Text does not fit in defined height, clip it
						if (rect->bottom - rect->top > meterRect.bottom - meterRect.top)
						{
							m_NeedsClipping = true;
						}

						rect->bottom = meterRect.bottom;
						updateSize = false;

					}
					else
					{
						if (rect->right - rect->left > (FLOAT)m_ClipStringW)
						{
							w = (FLOAT)m_ClipStringW;
							m_NeedsClipping = true;
						}
						else
						{
							w = rect->right - rect->left;
						}

						h = meterRect.bottom - meterRect.top;
					}
				}
				else
				{
					if (m_ClipStringW == -1)
					{
						// Clip text if already larger than ClipStringH
						if (m_ClipStringH != -1 && rect->bottom - rect->top > (FLOAT)m_ClipStringH)
						{
							m_NeedsClipping = true;
							rect->bottom = rect->top + (FLOAT)m_ClipStringH;
						}

						updateSize = false;
					}
					else
					{
						if (rect->right - rect->left > (FLOAT)m_ClipStringW)
						{
							w = (FLOAT)m_ClipStringW;
							m_NeedsClipping = true;
						}
						else
						{
							w = rect->right - rect->left;
						}

						h = rect->bottom - rect->top;
					}
				}

				if (updateSize)
				{
					UINT32 lines = 0;
					D2D1_SIZE_F size = D2D1::SizeF(w, h);
					if (canvas.MeasureTextLinesW(*str, *format, size, lines) && lines != 0)
					{
						rect->right = rect->left + w;
						rect->bottom = rect->top + size.height;

						if (m_HDefined || (m_ClipStringH != -1 && rect->bottom - rect->top > (FLOAT)m_ClipStringH))
						{
							rect->bottom = rect->top + (m_HDefined ? (meterRect.bottom - meterRect.top) : (FLOAT)m_ClipStringH);
						}
					}
				}
			}
		}
	}
	else
	{
		D2D1_RECT_F rcDest = GetTextRect();

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

		canvas.DrawTextW(*str, *format, rcDest, m_Color, true);
	}

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
