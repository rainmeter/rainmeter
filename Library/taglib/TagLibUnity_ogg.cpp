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

#include "ogg\flac\oggflacfile.cpp"
#include "ogg\oggfile.cpp"
#include "ogg\oggpage.cpp"
#include "ogg\oggpageheader.cpp"
#include "ogg\vorbis\vorbisfile.cpp"
#include "ogg\vorbis\vorbisproperties.cpp"
#include "ogg\xiphcomment.cpp"

#pragma warning(pop)
