/*
Copyright (C) 2014 Birunthan Mohanathas

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
