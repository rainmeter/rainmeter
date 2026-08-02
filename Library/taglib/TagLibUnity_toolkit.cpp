// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4018; disable: 4244; disable: 4267)

#include "toolkit\tbytevector.cpp"
#include "toolkit\tbytevectorlist.cpp"
#include "toolkit\tbytevectorstream.cpp"
#include "toolkit\tdebug.cpp"
#include "toolkit\tdebuglistener.cpp"
#include "toolkit\tfilestream.cpp"
#include "toolkit\tiostream.cpp"
#include "toolkit\tpropertymap.cpp"
#include "toolkit\trefcounter.cpp"
#include "toolkit\tstring.cpp"
#include "toolkit\tstringlist.cpp"
#include "toolkit\tzlib.cpp"

#pragma warning(pop)
