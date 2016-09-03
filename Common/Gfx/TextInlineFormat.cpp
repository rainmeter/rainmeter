/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormat.h"

namespace Gfx {

TextInlineFormat::TextInlineFormat(std::wstring pattern) :
	m_Pattern(std::move(pattern))
{
}

TextInlineFormat::~TextInlineFormat()
{
}

}  // namespace Gfx
