// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "ogg\oggfile.cpp"
#include "ogg\oggpage.cpp"
#include "ogg\oggpageheader.cpp"
#include "ogg\xiphcomment.cpp"
#include "ogg\flac\oggflacfile.cpp"
#include "ogg\vorbis\vorbisfile.cpp"
#include "ogg\vorbis\vorbisproperties.cpp"

#pragma warning(pop)
