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

#include <tdebug.h>
#include <tfile.h>

#include "id3v1tag.h"
#include "id3v1genres.h"

using namespace TagLib;
using namespace ID3v1;

namespace
{
  const ID3v1::StringHandler defaultStringHandler;
  const ID3v1::StringHandler *stringHandler = &defaultStringHandler;
} // namespace

class ID3v1::Tag::TagPrivate
{
public:
  TagPrivate() :
    file(0),
    tagOffset(0),
    track(0),
    genre(255) {}

  File *file;
  long tagOffset;

  String title;
  String artist;
  String album;
  String year;
  String comment;
  unsigned char track;
  unsigned char genre;
};

////////////////////////////////////////////////////////////////////////////////
// StringHandler implementation
////////////////////////////////////////////////////////////////////////////////

StringHandler::StringHandler()
{
}

String ID3v1::StringHandler::parse(const ByteVector &data) const
{
  return String(data, String::Latin1).stripWhiteSpace();
}

ByteVector ID3v1::StringHandler::render(const String &s) const
{
  if(s.isLatin1())
    return s.data(String::Latin1);
  return ByteVector();
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

ID3v1::Tag::Tag() :
  d(new TagPrivate())
{
}

ID3v1::Tag::Tag(File *file, long tagOffset) :
  d(new TagPrivate())
{
  d->file = file;
  d->tagOffset = tagOffset;

  read();
}

ID3v1::Tag::~Tag()
{
  delete d;
}

ByteVector ID3v1::Tag::render() const
{
  ByteVector data;

  data.append(fileIdentifier());
  data.append(stringHandler->render(d->title).resize(30));
  data.append(stringHandler->render(d->artist).resize(30));
  data.append(stringHandler->render(d->album).resize(30));
  data.append(stringHandler->render(d->year).resize(4));
  data.append(stringHandler->render(d->comment).resize(28));
  data.append(static_cast<char>(0));
  data.append(static_cast<char>(d->track));
  data.append(static_cast<char>(d->genre));

  return data;
}

ByteVector ID3v1::Tag::fileIdentifier()
{
  return ByteVector::fromCString("TAG");
}

String ID3v1::Tag::title() const
{
  return d->title;
}

String ID3v1::Tag::artist() const
{
  return d->artist;
}

String ID3v1::Tag::album() const
{
  return d->album;
}

String ID3v1::Tag::comment() const
{
  return d->comment;
}

String ID3v1::Tag::genre() const
{
  return ID3v1::genre(d->genre);
}

unsigned int ID3v1::Tag::year() const
{
  return d->year.toInt();
}

unsigned int ID3v1::Tag::track() const
{
  return d->track;
}

void ID3v1::Tag::setTitle(const String &s)
{
  d->title = s;
}

void ID3v1::Tag::setArtist(const String &s)
{
  d->artist = s;
}

void ID3v1::Tag::setAlbum(const String &s)
{
  d->album = s;
}

void ID3v1::Tag::setComment(const String &s)
{
  d->comment = s;
}

void ID3v1::Tag::setGenre(const String &s)
{
  d->genre = ID3v1::genreIndex(s);
}

void ID3v1::Tag::setYear(unsigned int i)
{
  d->year = i > 0 ? String::number(i) : String();
}

void ID3v1::Tag::setTrack(unsigned int i)
{
  d->track = i < 256 ? i : 0;
}

unsigned int ID3v1::Tag::genreNumber() const
{
  return d->genre;
}

void ID3v1::Tag::setGenreNumber(unsigned int i)
{
  d->genre = i < 256 ? i : 255;
}

void ID3v1::Tag::setStringHandler(const StringHandler *handler)
{
  if(handler)
    stringHandler = handler;
  else
    stringHandler = &defaultStringHandler;
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void ID3v1::Tag::read()
{
  if(d->file && d->file->isValid()) {
    d->file->seek(d->tagOffset);
    // read the tag -- always 128 bytes
    const ByteVector data = d->file->readBlock(128);

    // some initial sanity checking
    if(data.size() == 128 && data.startsWith("TAG"))
      parse(data);
    else
      debug("ID3v1 tag is not valid or could not be read at the specified offset.");
  }
}

void ID3v1::Tag::parse(const ByteVector &data)
{
  int offset = 3;

  d->title = stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->artist = stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->album = stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->year = stringHandler->parse(data.mid(offset, 4));
  offset += 4;

  // Check for ID3v1.1 -- Note that ID3v1 *does not* support "track zero" -- this
  // is not a bug in TagLib.  Since a zeroed byte is what we would expect to
  // indicate the end of a C-String, specifically the comment string, a value of
  // zero must be assumed to be just that.

  if(data[offset + 28] == 0 && data[offset + 29] != 0) {
    // ID3v1.1 detected

    d->comment = stringHandler->parse(data.mid(offset, 28));
    d->track   = static_cast<unsigned char>(data[offset + 29]);
  }
  else
    d->comment = data.mid(offset, 30);

  offset += 30;

  d->genre = static_cast<unsigned char>(data[offset]);
}
