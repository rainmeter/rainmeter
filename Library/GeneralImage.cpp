/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "GeneralImage.h"

GeneralImage::GeneralImage(Skin* skin) : 
	m_Bitmap(nullptr), 
	m_Skin(skin)
{
}

GeneralImage::~GeneralImage()
{
	if (m_Bitmap) delete m_Bitmap;
}

void GeneralImage::ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath)
{
	// TODO: Read and process general image options
}

bool GeneralImage::LoadImage(const std::wstring& imageName)
{
	if (m_Skin == nullptr) return false;
	if (m_Bitmap) delete m_Bitmap;
	m_Bitmap = new Gfx::D2DBitmap(imageName);
	if (m_Bitmap->Load(m_Skin->GetCanvas())) return true;
	if (m_Bitmap) delete m_Bitmap;
	m_Bitmap = nullptr;
	return false;
}
