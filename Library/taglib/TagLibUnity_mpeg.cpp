// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "mpeg\mpegheader.cpp"

#include "mpeg\mpegfile.cpp"
#include "mpeg\mpegproperties.cpp"
#include "mpeg\xingheader.cpp"

#include "mpeg\id3v1\id3v1genres.cpp"
#include "mpeg\id3v1\id3v1tag.cpp"

#pragma warning(pop)
