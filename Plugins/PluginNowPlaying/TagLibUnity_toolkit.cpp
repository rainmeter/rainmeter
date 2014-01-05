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
