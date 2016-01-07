/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// TagLib unity build: This file includes most TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

// The following includes have been commented out and are compiled separately due to e.g.
// mismatching symbols.
/*
#include "mpeg\id3v2\id3v2tag.cpp"
#include "toolkit\tfile.cpp"
*/

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

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
#include "toolkit\unicode.cpp"

#pragma warning(pop)
