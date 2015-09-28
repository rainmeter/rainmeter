/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "FontCollectionGDIP.h"

namespace Gfx {

FontCollectionGDIP::FontCollectionGDIP() : FontCollection(),
	m_PrivateCollection()
{
}

FontCollectionGDIP::~FontCollectionGDIP()
{
	Dispose();
}

void FontCollectionGDIP::Dispose()
{
	if (m_PrivateCollection)
	{
		delete m_PrivateCollection;
		m_PrivateCollection = nullptr;
	}
}

bool FontCollectionGDIP::AddFile(const WCHAR* file)
{
	if (!m_PrivateCollection)
	{
		m_PrivateCollection = new Gdiplus::PrivateFontCollection();
	}

	const Gdiplus::Status status = m_PrivateCollection->AddFontFile(file);
	return status == Gdiplus::Ok;
}

}  // namespace Gfx
