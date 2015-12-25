/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

#include "mpc\mpcfile.cpp"
#include "mpc\mpcproperties.cpp"

// Included here due to mismatch with toolkit\tfilestream.cpp.
#include "toolkit\tfile.cpp"

#pragma warning(pop)
