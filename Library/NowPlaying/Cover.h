// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
	static std::optional<std::wstring> GetLocal(std::wstring_view filename, std::wstring_view folder);
	static bool GetEmbedded(const TagLib::FileRef& fr, const std::wstring& target);
	static std::wstring GetFileFolder(const std::wstring& file);
};
