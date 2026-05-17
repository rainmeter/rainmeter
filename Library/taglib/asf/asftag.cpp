/**************************************************************************
    copyright            : (C) 2005-2007 by Lukáš Lalinský
    email                : lalinsky@gmail.com
 **************************************************************************/

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

#include <tpropertymap.h>
#include "asftag.h"

using namespace TagLib;

class ASF::Tag::TagPrivate
{
public:
  String title;
  String artist;
  String copyright;
  String comment;
  String rating;
  AttributeListMap attributeListMap;
};

ASF::Tag::Tag() :
  d(new TagPrivate())
{
}

ASF::Tag::~Tag()
{
  delete d;
}

String ASF::Tag::title() const
{
  return d->title;
}

String ASF::Tag::artist() const
{
  return d->artist;
}

String ASF::Tag::album() const
{
  if(d->attributeListMap.contains("WM/AlbumTitle"))
    return d->attributeListMap["WM/AlbumTitle"][0].toString();
  return String();
}

String ASF::Tag::copyright() const
{
  return d->copyright;
}

String ASF::Tag::comment() const
{
  return d->comment;
}

String ASF::Tag::rating() const
{
  return d->rating;
}

unsigned int ASF::Tag::year() const
{
  if(d->attributeListMap.contains("WM/Year"))
    return d->attributeListMap["WM/Year"][0].toString().toInt();
  return 0;
}

unsigned int ASF::Tag::track() const
{
  if(d->attributeListMap.contains("WM/TrackNumber")) {
    const ASF::Attribute attr = d->attributeListMap["WM/TrackNumber"][0];
    if(attr.type() == ASF::Attribute::DWordType)
      return attr.toUInt();
    return attr.toString().toInt();
  }
  if(d->attributeListMap.contains("WM/Track"))
    return d->attributeListMap["WM/Track"][0].toUInt();
  return 0;
}

String ASF::Tag::genre() const
{
  if(d->attributeListMap.contains("WM/Genre"))
    return d->attributeListMap["WM/Genre"][0].toString();
  return String();
}

void ASF::Tag::setTitle(const String &value)
{
  d->title = value;
}

void ASF::Tag::setArtist(const String &value)
{
  d->artist = value;
}

void ASF::Tag::setCopyright(const String &value)
{
  d->copyright = value;
}

void ASF::Tag::setComment(const String &value)
{
  d->comment = value;
}

void ASF::Tag::setRating(const String &value)
{
  d->rating = value;
}

void ASF::Tag::setAlbum(const String &value)
{
  setAttribute("WM/AlbumTitle", value);
}

void ASF::Tag::setGenre(const String &value)
{
  setAttribute("WM/Genre", value);
}

void ASF::Tag::setYear(unsigned int value)
{
  setAttribute("WM/Year", String::number(value));
}

void ASF::Tag::setTrack(unsigned int value)
{
  setAttribute("WM/TrackNumber", String::number(value));
}

ASF::AttributeListMap& ASF::Tag::attributeListMap()
{
  return d->attributeListMap;
}

const ASF::AttributeListMap &ASF::Tag::attributeListMap() const
{
  return d->attributeListMap;
}

bool ASF::Tag::contains(const String &key) const
{
  return d->attributeListMap.contains(key);
}

void ASF::Tag::removeItem(const String &key)
{
  d->attributeListMap.erase(key);
}

ASF::AttributeList ASF::Tag::attribute(const String &name) const
{
  return d->attributeListMap[name];
}

void ASF::Tag::setAttribute(const String &name, const Attribute &attribute)
{
  AttributeList value;
  value.append(attribute);
  d->attributeListMap.insert(name, value);
}

void ASF::Tag::setAttribute(const String &name, const AttributeList &values)
{
  d->attributeListMap.insert(name, values);
}

void ASF::Tag::addAttribute(const String &name, const Attribute &attribute)
{
  if(d->attributeListMap.contains(name)) {
    d->attributeListMap[name].append(attribute);
  }
  else {
    setAttribute(name, attribute);
  }
}

bool ASF::Tag::isEmpty() const
{
  return TagLib::Tag::isEmpty() &&
         copyright().isEmpty() &&
         rating().isEmpty() &&
         d->attributeListMap.isEmpty();
}

namespace
{
  const std::pair<const char *, const char *> keyTranslation[] = {
    std::make_pair("WM/AlbumTitle", "ALBUM"),
    std::make_pair("WM/AlbumArtist", "ALBUMARTIST"),
    std::make_pair("WM/Composer", "COMPOSER"),
    std::make_pair("WM/Writer", "LYRICIST"),
    std::make_pair("WM/Conductor", "CONDUCTOR"),
    std::make_pair("WM/ModifiedBy", "REMIXER"),
    std::make_pair("WM/Year", "DATE"),
    std::make_pair("WM/OriginalReleaseYear", "ORIGINALDATE"),
    std::make_pair("WM/Producer", "PRODUCER"),
    std::make_pair("WM/ContentGroupDescription", "WORK"),
    std::make_pair("WM/SubTitle", "SUBTITLE"),
    std::make_pair("WM/SetSubTitle", "DISCSUBTITLE"),
    std::make_pair("WM/TrackNumber", "TRACKNUMBER"),
    std::make_pair("WM/PartOfSet", "DISCNUMBER"),
    std::make_pair("WM/Genre", "GENRE"),
    std::make_pair("WM/BeatsPerMinute", "BPM"),
    std::make_pair("WM/Mood", "MOOD"),
    std::make_pair("WM/ISRC", "ISRC"),
    std::make_pair("WM/Lyrics", "LYRICS"),
    std::make_pair("WM/Media", "MEDIA"),
    std::make_pair("WM/Publisher", "LABEL"),
    std::make_pair("WM/CatalogNo", "CATALOGNUMBER"),
    std::make_pair("WM/Barcode", "BARCODE"),
    std::make_pair("WM/EncodedBy", "ENCODEDBY"),
    std::make_pair("WM/AlbumSortOrder", "ALBUMSORT"),
    std::make_pair("WM/AlbumArtistSortOrder", "ALBUMARTISTSORT"),
    std::make_pair("WM/ArtistSortOrder", "ARTISTSORT"),
    std::make_pair("WM/TitleSortOrder", "TITLESORT"),
    std::make_pair("WM/Script", "SCRIPT"),
    std::make_pair("WM/Language", "LANGUAGE"),
    std::make_pair("WM/ARTISTS", "ARTISTS"),
    std::make_pair("ASIN", "ASIN"),
    std::make_pair("MusicBrainz/Track Id", "MUSICBRAINZ_TRACKID"),
    std::make_pair("MusicBrainz/Artist Id", "MUSICBRAINZ_ARTISTID"),
    std::make_pair("MusicBrainz/Album Id", "MUSICBRAINZ_ALBUMID"),
    std::make_pair("MusicBrainz/Album Artist Id", "MUSICBRAINZ_ALBUMARTISTID"),
    std::make_pair("MusicBrainz/Album Release Country", "RELEASECOUNTRY"),
    std::make_pair("MusicBrainz/Album Status", "RELEASESTATUS"),
    std::make_pair("MusicBrainz/Album Type", "RELEASETYPE"),
    std::make_pair("MusicBrainz/Release Group Id", "MUSICBRAINZ_RELEASEGROUPID"),
    std::make_pair("MusicBrainz/Release Track Id", "MUSICBRAINZ_RELEASETRACKID"),
    std::make_pair("MusicBrainz/Work Id", "MUSICBRAINZ_WORKID"),
    std::make_pair("MusicIP/PUID", "MUSICIP_PUID"),
    std::make_pair("Acoustid/Id", "ACOUSTID_ID"),
    std::make_pair("Acoustid/Fingerprint", "ACOUSTID_FINGERPRINT"),
  };
  const size_t keyTranslationSize = sizeof(keyTranslation) / sizeof(keyTranslation[0]);

  String translateKey(const String &key)
  {
    for(size_t i = 0; i < keyTranslationSize; ++i) {
      if(key == keyTranslation[i].first)
        return keyTranslation[i].second;
    }

    return String();
  }
}  // namespace

PropertyMap ASF::Tag::properties() const
{
  PropertyMap props;

  if(!d->title.isEmpty()) {
    props["TITLE"] = d->title;
  }
  if(!d->artist.isEmpty()) {
    props["ARTIST"] = d->artist;
  }
  if(!d->copyright.isEmpty()) {
    props["COPYRIGHT"] = d->copyright;
  }
  if(!d->comment.isEmpty()) {
    props["COMMENT"] = d->comment;
  }

  ASF::AttributeListMap::ConstIterator it = d->attributeListMap.begin();
  for(; it != d->attributeListMap.end(); ++it) {
    const String key = translateKey(it->first);
    if(!key.isEmpty()) {
      AttributeList::ConstIterator it2 = it->second.begin();
      for(; it2 != it->second.end(); ++it2) {
        if(key == "TRACKNUMBER") {
          if(it2->type() == ASF::Attribute::DWordType)
            props.insert(key, String::number(it2->toUInt()));
          else
            props.insert(key, it2->toString());
        }
        else {
          props.insert(key, it2->toString());
        }
      }
    }
    else {
      props.unsupportedData().append(it->first);
    }
  }
  return props;
}

void ASF::Tag::removeUnsupportedProperties(const StringList &props)
{
  StringList::ConstIterator it = props.begin();
  for(; it != props.end(); ++it)
    d->attributeListMap.erase(*it);
}

PropertyMap ASF::Tag::setProperties(const PropertyMap &props)
{
  static Map<String, String> reverseKeyMap;
  if(reverseKeyMap.isEmpty()) {
    for(size_t i = 0; i < keyTranslationSize; i++) {
      reverseKeyMap[keyTranslation[i].second] = keyTranslation[i].first;
    }
  }

  PropertyMap origProps = properties();
  PropertyMap::ConstIterator it = origProps.begin();
  for(; it != origProps.end(); ++it) {
    if(!props.contains(it->first) || props[it->first].isEmpty()) {
      if(it->first == "TITLE") {
        d->title.clear();
      }
      else if(it->first == "ARTIST") {
        d->artist.clear();
      }
      else if(it->first == "COMMENT") {
        d->comment.clear();
      }
      else if(it->first == "COPYRIGHT") {
        d->copyright.clear();
      }
      else {
        d->attributeListMap.erase(reverseKeyMap[it->first]);
      }
    }
  }

  PropertyMap ignoredProps;
  it = props.begin();
  for(; it != props.end(); ++it) {
    if(reverseKeyMap.contains(it->first)) {
      String name = reverseKeyMap[it->first];
      removeItem(name);
      StringList::ConstIterator it2 = it->second.begin();
      for(; it2 != it->second.end(); ++it2) {
        addAttribute(name, *it2);
      }
    }
    else if(it->first == "TITLE") {
      d->title = it->second.toString();
    }
    else if(it->first == "ARTIST") {
      d->artist = it->second.toString();
    }
    else if(it->first == "COMMENT") {
      d->comment = it->second.toString();
    }
    else if(it->first == "COPYRIGHT") {
      d->copyright = it->second.toString();
    }
    else {
      ignoredProps.insert(it->first, it->second);
    }
  }

  return ignoredProps;
}
