/***************************************************************************
    copyright            : (C) 2014 by Urs Fleisch
    email                : ufleisch@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
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

#include "synchronizedlyricsframe.h"
#include <tbytevectorlist.h>
#include <id3v2tag.h>
#include <tdebug.h>
#include <tpropertymap.h>

using namespace TagLib;
using namespace ID3v2;

class SynchronizedLyricsFrame::SynchronizedLyricsFramePrivate
{
public:
  SynchronizedLyricsFramePrivate() :
    textEncoding(String::Latin1),
    timestampFormat(SynchronizedLyricsFrame::AbsoluteMilliseconds),
    type(SynchronizedLyricsFrame::Lyrics) {}
  String::Type textEncoding;
  ByteVector language;
  SynchronizedLyricsFrame::TimestampFormat timestampFormat;
  SynchronizedLyricsFrame::Type type;
  String description;
  SynchronizedLyricsFrame::SynchedTextList synchedText;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

SynchronizedLyricsFrame::SynchronizedLyricsFrame(String::Type encoding) :
  Frame("SYLT"),
  d(new SynchronizedLyricsFramePrivate())
{
  d->textEncoding = encoding;
}

SynchronizedLyricsFrame::SynchronizedLyricsFrame(const ByteVector &data) :
  Frame(data),
  d(new SynchronizedLyricsFramePrivate())
{
  setData(data);
}

SynchronizedLyricsFrame::~SynchronizedLyricsFrame()
{
  delete d;
}

String SynchronizedLyricsFrame::toString() const
{
  return d->description;
}

String::Type SynchronizedLyricsFrame::textEncoding() const
{
  return d->textEncoding;
}

ByteVector SynchronizedLyricsFrame::language() const
{
  return d->language;
}

SynchronizedLyricsFrame::TimestampFormat
SynchronizedLyricsFrame::timestampFormat() const
{
  return d->timestampFormat;
}

SynchronizedLyricsFrame::Type SynchronizedLyricsFrame::type() const
{
  return d->type;
}

String SynchronizedLyricsFrame::description() const
{
  return d->description;
}

SynchronizedLyricsFrame::SynchedTextList
SynchronizedLyricsFrame::synchedText() const
{
  return d->synchedText;
}

void SynchronizedLyricsFrame::setTextEncoding(String::Type encoding)
{
  d->textEncoding = encoding;
}

void SynchronizedLyricsFrame::setLanguage(const ByteVector &languageEncoding)
{
  d->language = languageEncoding.mid(0, 3);
}

void SynchronizedLyricsFrame::setTimestampFormat(SynchronizedLyricsFrame::TimestampFormat f)
{
  d->timestampFormat = f;
}

void SynchronizedLyricsFrame::setType(SynchronizedLyricsFrame::Type t)
{
  d->type = t;
}

void SynchronizedLyricsFrame::setDescription(const String &s)
{
  d->description = s;
}

void SynchronizedLyricsFrame::setSynchedText(
    const SynchronizedLyricsFrame::SynchedTextList &t)
{
  d->synchedText = t;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void SynchronizedLyricsFrame::parseFields(const ByteVector &data)
{
  const int end = data.size();
  if(end < 7) {
    debug("A synchronized lyrics frame must contain at least 7 bytes.");
    return;
  }

  d->textEncoding = static_cast<String::Type>(data[0]);
  d->language = data.mid(1, 3);
  d->timestampFormat = static_cast<TimestampFormat>(data[4]);
  d->type = static_cast<Type>(data[5]);

  int pos = 6;

  d->description = readStringField(data, d->textEncoding, &pos);
  if(pos == 6)
    return;

  /*
   * If UTF16 strings are found in SYLT frames, a BOM may only be
   * present in the first string (content descriptor), and the strings of
   * the synchronized text have no BOM. Here the BOM is read from
   * the first string to have a specific encoding with endianness for the
   * case of strings without BOM so that readStringField() will work.
   */
  String::Type encWithEndianness = d->textEncoding;
  if(d->textEncoding == String::UTF16) {
    unsigned short bom = data.toUShort(6, true);
    if(bom == 0xfffe) {
      encWithEndianness = String::UTF16LE;
    } else if(bom == 0xfeff) {
      encWithEndianness = String::UTF16BE;
    }
  }

  d->synchedText.clear();
  while(pos < end) {
    String::Type enc = d->textEncoding;
    // If a UTF16 string has no BOM, use the encoding found above.
    if(enc == String::UTF16 && pos + 1 < end) {
      unsigned short bom = data.toUShort(pos, true);
      if(bom != 0xfffe && bom != 0xfeff) {
        enc = encWithEndianness;
      }
    }
    String text = readStringField(data, enc, &pos);
    if(pos + 4 > end)
      return;

    unsigned int time = data.toUInt(pos, true);
    pos += 4;

    d->synchedText.append(SynchedText(time, text));
  }
}

ByteVector SynchronizedLyricsFrame::renderFields() const
{
  ByteVector v;

  String::Type encoding = d->textEncoding;

  encoding = checkTextEncoding(d->description, encoding);
  for(SynchedTextList::ConstIterator it = d->synchedText.begin();
      it != d->synchedText.end();
      ++it) {
    encoding = checkTextEncoding(it->text, encoding);
  }

  v.append(static_cast<char>(encoding));
  v.append(d->language.size() == 3 ? d->language : "XXX");
  v.append(static_cast<char>(d->timestampFormat));
  v.append(static_cast<char>(d->type));
  v.append(d->description.data(encoding));
  v.append(textDelimiter(encoding));
  for(SynchedTextList::ConstIterator it = d->synchedText.begin();
      it != d->synchedText.end();
      ++it) {
    const SynchedText &entry = *it;
    v.append(entry.text.data(encoding));
    v.append(textDelimiter(encoding));
    v.append(ByteVector::fromUInt(entry.time));
  }

  return v;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

SynchronizedLyricsFrame::SynchronizedLyricsFrame(const ByteVector &data, Header *h) :
  Frame(h),
  d(new SynchronizedLyricsFramePrivate())
{
  parseFields(fieldData(data));
}
