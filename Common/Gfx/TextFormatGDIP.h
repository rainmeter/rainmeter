/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTFORMATGDIP_H_
#define RM_GFX_TEXTFORMATGDIP_H_

#include "TextFormat.h"
#include <memory>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>

namespace Gfx {

// Provides a GDI+ implementation of TextFormat for use with CanvasGDIP.
class TextFormatGDIP : public TextFormat
{
public:
	TextFormatGDIP();
	virtual ~TextFormatGDIP();

	TextFormatGDIP(const TextFormatGDIP& other) = delete;
	TextFormatGDIP& operator=(TextFormatGDIP other) = delete;

	virtual bool IsInitialized() const override { return m_Font != nullptr; }

	virtual void SetProperties(
		const WCHAR* fontFamily, int size, bool bold, bool italic,
		const FontCollection* fontCollection) override;

	virtual void SetTrimming(bool trim) override;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment) override;
	virtual void SetVerticalAlignment(VerticalAlignment alignment) override;

	// Inline options only available with D2D at this time.
	virtual void ReadInlineOptions(ConfigParser& parser, const WCHAR* section) override;
	virtual void FindInlineRanges(const std::wstring& str) override;

private:
	friend class CanvasGDIP;

	std::unique_ptr<Gdiplus::Font> m_Font;
	std::unique_ptr<Gdiplus::FontFamily> m_FontFamily;
	Gdiplus::StringFormat m_StringFormat;
};

}  // namespace Gfx

#endif
