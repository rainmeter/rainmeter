/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include "id3v1genres.h"

using namespace TagLib;

namespace
{
  const wchar_t *const genres[] = {
    L"Blues",
    L"Classic Rock",
    L"Country",
    L"Dance",
    L"Disco",
    L"Funk",
    L"Grunge",
    L"Hip-Hop",
    L"Jazz",
    L"Metal",
    L"New Age",
    L"Oldies",
    L"Other",
    L"Pop",
    L"R&B",
    L"Rap",
    L"Reggae",
    L"Rock",
    L"Techno",
    L"Industrial",
    L"Alternative",
    L"Ska",
    L"Death Metal",
    L"Pranks",
    L"Soundtrack",
    L"Euro-Techno",
    L"Ambient",
    L"Trip-Hop",
    L"Vocal",
    L"Jazz-Funk",
    L"Fusion",
    L"Trance",
    L"Classical",
    L"Instrumental",
    L"Acid",
    L"House",
    L"Game",
    L"Sound Clip",
    L"Gospel",
    L"Noise",
    L"Alternative Rock",
    L"Bass",
    L"Soul",
    L"Punk",
    L"Space",
    L"Meditative",
    L"Instrumental Pop",
    L"Instrumental Rock",
    L"Ethnic",
    L"Gothic",
    L"Darkwave",
    L"Techno-Industrial",
    L"Electronic",
    L"Pop-Folk",
    L"Eurodance",
    L"Dream",
    L"Southern Rock",
    L"Comedy",
    L"Cult",
    L"Gangsta",
    L"Top 40",
    L"Christian Rap",
    L"Pop/Funk",
    L"Jungle",
    L"Native American",
    L"Cabaret",
    L"New Wave",
    L"Psychedelic",
    L"Rave",
    L"Showtunes",
    L"Trailer",
    L"Lo-Fi",
    L"Tribal",
    L"Acid Punk",
    L"Acid Jazz",
    L"Polka",
    L"Retro",
    L"Musical",
    L"Rock & Roll",
    L"Hard Rock",
    L"Folk",
    L"Folk Rock",
    L"National Folk",
    L"Swing",
    L"Fast Fusion",
    L"Bebop",
    L"Latin",
    L"Revival",
    L"Celtic",
    L"Bluegrass",
    L"Avant-garde",
    L"Gothic Rock",
    L"Progressive Rock",
    L"Psychedelic Rock",
    L"Symphonic Rock",
    L"Slow Rock",
    L"Big Band",
    L"Chorus",
    L"Easy Listening",
    L"Acoustic",
    L"Humour",
    L"Speech",
    L"Chanson",
    L"Opera",
    L"Chamber Music",
    L"Sonata",
    L"Symphony",
    L"Booty Bass",
    L"Primus",
    L"Porn Groove",
    L"Satire",
    L"Slow Jam",
    L"Club",
    L"Tango",
    L"Samba",
    L"Folklore",
    L"Ballad",
    L"Power Ballad",
    L"Rhythmic Soul",
    L"Freestyle",
    L"Duet",
    L"Punk Rock",
    L"Drum Solo",
    L"A Cappella",
    L"Euro-House",
    L"Dancehall",
    L"Goa",
    L"Drum & Bass",
    L"Club-House",
    L"Hardcore Techno",
    L"Terror",
    L"Indie",
    L"Britpop",
    L"Worldbeat",
    L"Polsk Punk",
    L"Beat",
    L"Christian Gangsta Rap",
    L"Heavy Metal",
    L"Black Metal",
    L"Crossover",
    L"Contemporary Christian",
    L"Christian Rock",
    L"Merengue",
    L"Salsa",
    L"Thrash Metal",
    L"Anime",
    L"Jpop",
    L"Synthpop",
    L"Abstract",
    L"Art Rock",
    L"Baroque",
    L"Bhangra",
    L"Big Beat",
    L"Breakbeat",
    L"Chillout",
    L"Downtempo",
    L"Dub",
    L"EBM",
    L"Eclectic",
    L"Electro",
    L"Electroclash",
    L"Emo",
    L"Experimental",
    L"Garage",
    L"Global",
    L"IDM",
    L"Illbient",
    L"Industro-Goth",
    L"Jam Band",
    L"Krautrock",
    L"Leftfield",
    L"Lounge",
    L"Math Rock",
    L"New Romantic",
    L"Nu-Breakz",
    L"Post-Punk",
    L"Post-Rock",
    L"Psytrance",
    L"Shoegaze",
    L"Space Rock",
    L"Trop Rock",
    L"World Music",
    L"Neoclassical",
    L"Audiobook",
    L"Audio Theatre",
    L"Neue Deutsche Welle",
    L"Podcast",
    L"Indie Rock",
    L"G-Funk",
    L"Dubstep",
    L"Garage Rock",
    L"Psybient"
  };
  const int genresSize = sizeof(genres) / sizeof(genres[0]);
}  // namespace

StringList ID3v1::genreList()
{
  StringList l;
  for(int i = 0; i < genresSize; i++) {
    l.append(genres[i]);
  }

  return l;
}

ID3v1::GenreMap ID3v1::genreMap()
{
  GenreMap m;
  for(int i = 0; i < genresSize; i++) {
    m.insert(genres[i], i);
  }

  return m;
}

String ID3v1::genre(int i)
{
  if(i >= 0 && i < genresSize)
    return String(genres[i]); // always make a copy
  return String();
}

int ID3v1::genreIndex(const String &name)
{
  for(int i = 0; i < genresSize; ++i) {
    if(name == genres[i])
      return i;
  }

  // If the name was not found, try the names which have been changed
  static const struct {
    const wchar_t *genre;
    int code;
  } fixUpGenres[] = {
    { L"Jazz+Funk", 29 },
    { L"Folk/Rock", 81 },
    { L"Bebob", 85 },
    { L"Avantgarde", 90 },
    { L"Dance Hall", 125 },
    { L"Hardcore", 129 },
    { L"BritPop", 132 },
    { L"Negerpunk", 133 }
  };
  static const int fixUpGenresSize =
      sizeof(fixUpGenres) / sizeof(fixUpGenres[0]);
  for(int i = 0; i < fixUpGenresSize; ++i) {
    if(name == fixUpGenres[i].genre)
      return fixUpGenres[i].code;
  }

  return 255;
}
