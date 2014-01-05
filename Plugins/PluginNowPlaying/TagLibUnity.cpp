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

#include "taglib\fileref.cpp"
#include "taglib\tag.cpp"
#include "taglib\tagunion.cpp"

#include "taglib\flac\flacproperties.cpp"
#include "taglib\mpc\mpcproperties.cpp"
#include "taglib\mpeg\id3v1\id3v1tag.cpp"
#include "taglib\mpeg\id3v2\id3v2footer.cpp"
#include "taglib\ogg\flac\oggflacfile.cpp"

#include "taglib\ape\apefile.cpp"
#include "taglib\ape\apefooter.cpp"
#include "taglib\ape\apeitem.cpp"
#include "taglib\ape\apeproperties.cpp"
#include "taglib\ape\apetag.cpp"
#include "taglib\asf\asfattribute.cpp"
#include "taglib\asf\asffile.cpp"
#include "taglib\asf\asfpicture.cpp"
#include "taglib\asf\asfproperties.cpp"
#include "taglib\asf\asftag.cpp"
#include "taglib\audioproperties.cpp"
#include "taglib\flac\flacfile.cpp"
#include "taglib\flac\flacmetadatablock.cpp"
#include "taglib\flac\flacpicture.cpp"
#include "taglib\flac\flacunknownmetadatablock.cpp"
#include "taglib\mpc\mpcfile.cpp"
#include "taglib\mpeg\id3v1\id3v1genres.cpp"
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
#include "taglib\mpeg\id3v2\id3v2frame.cpp"
#include "taglib\mpeg\id3v2\id3v2framefactory.cpp"
#include "taglib\mpeg\id3v2\id3v2header.cpp"
#include "taglib\mpeg\id3v2\id3v2synchdata.cpp"
#include "taglib\mpeg\mpegfile.cpp"
#include "taglib\mpeg\mpegheader.cpp"
#include "taglib\mpeg\mpegproperties.cpp"
#include "taglib\mpeg\xingheader.cpp"
#include "taglib\ogg\oggfile.cpp"
#include "taglib\ogg\oggpage.cpp"
#include "taglib\ogg\oggpageheader.cpp"
#include "taglib\ogg\vorbis\vorbisfile.cpp"
#include "taglib\ogg\vorbis\vorbisproperties.cpp"
#include "taglib\ogg\xiphcomment.cpp"
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
