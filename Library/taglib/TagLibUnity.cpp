// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

// The files have been separated into several TagLibUnity_*.cpp files in order help the optimizer.
// Including everything here increases the binary size.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "fileref.cpp"
#include "tag.cpp"
#include "tagunion.cpp"
#include "audioproperties.cpp"
#include "tagutils.cpp"

#include "mpc\mpcfile.cpp"
#include "mpc\mpcproperties.cpp"

// Included here due to mismatches.
#include "toolkit\tfile.cpp"

#pragma warning(pop)
