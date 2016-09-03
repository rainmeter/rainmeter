/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __COVER_H__
#define __COVER_H__

// TagLib
#include "fileref.h"
#include "apefile.h"
#include "apetag.h"
#include "asffile.h"
#include "attachedpictureframe.h"
#include "commentsframe.h"
#include "flacfile.h"
#include "id3v1genres.h"
#include "id3v2tag.h"
#include "mpcfile.h"
#include "mpegfile.h"
#include "mp4file.h"
#include "tag.h"
#include "taglib.h"
#include "textidentificationframe.h"
#include "tstring.h"
#include "vorbisfile.h"

class CCover
{
public:
	static bool GetCached(std::wstring& path);
	static bool GetLocal(std::wstring filename, const std::wstring& folder, std::wstring& target);
	static bool GetEmbedded(const TagLib::FileRef& fr, const std::wstring& target);
	static std::wstring GetFileFolder(const std::wstring& file);
};

#endif
