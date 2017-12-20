/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __GENERALIMAGE_H__
#define __GENERALIMAGE_H__

#include "../Common/Gfx/D2DBitmap.h"
#include <string>
#include "Skin.h"

class GeneralImage
{
public:
	GeneralImage(Skin* skin);
	~GeneralImage();

	bool IsLoaded() const { return m_Bitmap != nullptr; }
	Gfx::D2DBitmap* GetImage() { return m_Bitmap; }

	void ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath = L"");
	bool LoadImage(const std::wstring& imageName);

private:
	Gfx::D2DBitmap* m_Bitmap;
	Skin* m_Skin;
};

#endif