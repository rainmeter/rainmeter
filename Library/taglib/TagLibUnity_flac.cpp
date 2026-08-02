// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "flac\flacfile.cpp"
#include "flac\flacmetadatablock.cpp"
#include "flac\flacpicture.cpp"
#include "flac\flacunknownmetadatablock.cpp"
#include "flac\flacproperties.cpp"

#pragma warning(pop)
