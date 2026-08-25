// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "mp4\mp4atom.cpp"
#include "mp4\mp4coverart.cpp"
#include "mp4\mp4file.cpp"
#include "mp4\mp4item.cpp"
#include "mp4\mp4properties.cpp"
#include "mp4\mp4tag.cpp"

#pragma warning(pop)
