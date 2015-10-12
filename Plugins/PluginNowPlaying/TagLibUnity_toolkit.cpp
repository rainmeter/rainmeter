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
#include "taglib\mpeg\id3v2\id3v2tag.cpp"
#include "taglib\toolkit\tfile.cpp"
*/

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "taglib\toolkit\tbytevector.cpp"
#include "taglib\toolkit\tbytevectorlist.cpp"
#include "taglib\toolkit\tbytevectorstream.cpp"
#include "taglib\toolkit\tdebug.cpp"
#include "taglib\toolkit\tdebuglistener.cpp"
#include "taglib\toolkit\tfilestream.cpp"
#include "taglib\toolkit\tiostream.cpp"
#include "taglib\toolkit\tpropertymap.cpp"
#include "taglib\toolkit\trefcounter.cpp"
#include "taglib\toolkit\tstring.cpp"
#include "taglib\toolkit\tstringlist.cpp"
#include "taglib\toolkit\unicode.cpp"

#pragma warning(pop)
