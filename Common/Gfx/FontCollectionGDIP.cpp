/*
  Copyright (C) 2013 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "FontCollectionGDIP.h"
#include <GdiPlus.h>

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
