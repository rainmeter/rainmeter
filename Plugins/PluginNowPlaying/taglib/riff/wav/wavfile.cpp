/***************************************************************************
    copyright            : (C) 2008 by Scott Wheeler
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

#include <tbytevector.h>
#include <tdebug.h>
#include <id3v2tag.h>

#include "wavfile.h"

using namespace TagLib;

class RIFF::WAV::File::FilePrivate
{
public:
  FilePrivate() :
    properties(0),
    tag(0),
    tagChunkID("ID3 ")
  {

  }

  ~FilePrivate()
  {
    delete properties;
    delete tag;
  }

  Properties *properties;
  ID3v2::Tag *tag;
  ByteVector tagChunkID;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

RIFF::WAV::File::File(FileName file, bool readProperties,
                       Properties::ReadStyle propertiesStyle) : RIFF::File(file, LittleEndian)
{
  d = new FilePrivate;
  if(isOpen())
    read(readProperties, propertiesStyle);
}

RIFF::WAV::File::~File()
{
  delete d;
}

ID3v2::Tag *RIFF::WAV::File::tag() const
{
  return d->tag;
}

RIFF::WAV::Properties *RIFF::WAV::File::audioProperties() const
{
  return d->properties;
}

bool RIFF::WAV::File::save()
{
  if(readOnly()) {
    debug("RIFF::WAV::File::save() -- File is read only.");
    return false;
  }

  if(!isValid()) {
    debug("RIFF::WAV::File::save() -- Trying to save invalid file.");
    return false;
  }

  setChunkData(d->tagChunkID, d->tag->render());

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void RIFF::WAV::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
  ByteVector formatData;
  uint streamLength = 0;
  for(uint i = 0; i < chunkCount(); i++) {
    if(chunkName(i) == "ID3 " || chunkName(i) == "id3 ") {
      d->tagChunkID = chunkName(i);
      d->tag = new ID3v2::Tag(this, chunkOffset(i));
    }
    else if(chunkName(i) == "fmt " && readProperties)
      formatData = chunkData(i);
    else if(chunkName(i) == "data" && readProperties)
      streamLength = chunkDataSize(i);
  }

  if(!formatData.isEmpty())
    d->properties = new Properties(formatData, streamLength, propertiesStyle);

  if(!d->tag)
    d->tag = new ID3v2::Tag;
}
