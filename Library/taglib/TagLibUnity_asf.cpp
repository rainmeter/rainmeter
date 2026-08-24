// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "asf\asfattribute.cpp"
#include "asf\asffile.cpp"
#include "asf\asfpicture.cpp"
#include "asf\asfproperties.cpp"
#include "asf\asftag.cpp"

#pragma warning(pop)
