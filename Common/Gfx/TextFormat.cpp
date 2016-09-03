/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextFormat.h"

namespace Gfx {

TextFormat::TextFormat() :
	m_HorizontalAlignment(HorizontalAlignment::Left),
	m_VerticalAlignment(VerticalAlignment::Top)
{
}

TextFormat::~TextFormat()
{
}

void TextFormat::SetHorizontalAlignment(HorizontalAlignment alignment)
{
	m_HorizontalAlignment = alignment;
}

void TextFormat::SetVerticalAlignment(VerticalAlignment alignment)
{
	m_VerticalAlignment = alignment;
}

}  // namespace Gfx
