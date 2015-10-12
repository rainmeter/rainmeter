/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "taglib\mpeg\id3v2\frames\attachedpictureframe.cpp"
#include "taglib\mpeg\id3v2\frames\commentsframe.cpp"
#include "taglib\mpeg\id3v2\frames\generalencapsulatedobjectframe.cpp"
#include "taglib\mpeg\id3v2\frames\ownershipframe.cpp"
#include "taglib\mpeg\id3v2\frames\popularimeterframe.cpp"
#include "taglib\mpeg\id3v2\frames\privateframe.cpp"
#include "taglib\mpeg\id3v2\frames\relativevolumeframe.cpp"
#include "taglib\mpeg\id3v2\frames\textidentificationframe.cpp"
#include "taglib\mpeg\id3v2\frames\uniquefileidentifierframe.cpp"
#include "taglib\mpeg\id3v2\frames\unknownframe.cpp"
#include "taglib\mpeg\id3v2\frames\unsynchronizedlyricsframe.cpp"
#include "taglib\mpeg\id3v2\frames\urllinkframe.cpp"
#include "taglib\mpeg\id3v2\id3v2extendedheader.cpp"
#include "taglib\mpeg\id3v2\id3v2footer.cpp"
#include "taglib\mpeg\id3v2\id3v2frame.cpp"
#include "taglib\mpeg\id3v2\id3v2framefactory.cpp"
#include "taglib\mpeg\id3v2\id3v2header.cpp"
#include "taglib\mpeg\id3v2\id3v2synchdata.cpp"
#include "taglib\mpeg\id3v2\id3v2tag.cpp"

#pragma warning(pop)
