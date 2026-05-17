/***************************************************************************
    copyright            : (C) 2003-2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.org
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
#include <tstring.h>
#include <tlist.h>
#include <tdebug.h>
#include <tagunion.h>
#include <tpropertymap.h>
#include <tagutils.h>

#include <id3v2header.h>
#include <id3v2tag.h>
#include <id3v1tag.h>
#include <xiphcomment.h>

#include "flacpicture.h"
#include "flacfile.h"
#include "flacmetadatablock.h"
#include "flacunknownmetadatablock.h"

using namespace TagLib;

namespace
{
  typedef List<FLAC::MetadataBlock *> BlockList;
  typedef BlockList::Iterator BlockIterator;
  typedef BlockList::Iterator BlockConstIterator;

  enum { FlacXiphIndex = 0, FlacID3v2Index = 1, FlacID3v1Index = 2 };

  const long MinPaddingLength = 4096;
  const long MaxPaddingLegnth = 1024 * 1024;

  const char LastBlockFlag = '\x80';
}  // namespace

class FLAC::File::FilePrivate
{
public:
  FilePrivate(const ID3v2::FrameFactory *frameFactory = ID3v2::FrameFactory::instance()) :
    ID3v2FrameFactory(frameFactory),
    ID3v2Location(-1),
    ID3v2OriginalSize(0),
    ID3v1Location(-1),
    properties(0),
    flacStart(0),
    streamStart(0),
    scanned(false)
  {
    blocks.setAutoDelete(true);
  }

  ~FilePrivate()
  {
    delete properties;
  }

  const ID3v2::FrameFactory *ID3v2FrameFactory;
  long ID3v2Location;
  long ID3v2OriginalSize;

  long ID3v1Location;

  TagUnion tag;

  Properties *properties;
  ByteVector xiphCommentData;
  BlockList blocks;

  long flacStart;
  long streamStart;
  bool scanned;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool FLAC::File::isSupported(IOStream *stream)
{
  // A FLAC file has an ID "fLaC" somewhere. An ID3v2 tag may precede.

  const ByteVector buffer = Utils::readHeader(stream, bufferSize(), true);
  return (buffer.find("fLaC") >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FLAC::File::File(FileName file, bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

FLAC::File::File(FileName file, ID3v2::FrameFactory *frameFactory,
                 bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate(frameFactory))
{
  if(isOpen())
    read(readProperties);
}

FLAC::File::File(IOStream *stream, ID3v2::FrameFactory *frameFactory,
                 bool readProperties, Properties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate(frameFactory))
{
  if(isOpen())
    read(readProperties);
}

FLAC::File::~File()
{
  delete d;
}

TagLib::Tag *FLAC::File::tag() const
{
  return &d->tag;
}

PropertyMap FLAC::File::properties() const
{
  return d->tag.properties();
}

void FLAC::File::removeUnsupportedProperties(const StringList &unsupported)
{
  d->tag.removeUnsupportedProperties(unsupported);
}

PropertyMap FLAC::File::setProperties(const PropertyMap &properties)
{
  return xiphComment(true)->setProperties(properties);
}

FLAC::Properties *FLAC::File::audioProperties() const
{
  return d->properties;
}

bool FLAC::File::save()
{
  if(readOnly()) {
    debug("FLAC::File::save() - Cannot save to a read only file.");
    return false;
  }

  if(!isValid()) {
    debug("FLAC::File::save() -- Trying to save invalid file.");
    return false;
  }

  // Create new vorbis comments
  if(!hasXiphComment())
    Tag::duplicate(&d->tag, xiphComment(true), false);

  d->xiphCommentData = xiphComment()->render(false);

  // Replace metadata blocks

  MetadataBlock *commentBlock =
      new UnknownMetadataBlock(MetadataBlock::VorbisComment, d->xiphCommentData);
  for(BlockIterator it = d->blocks.begin(); it != d->blocks.end();) {
    if((*it)->code() == MetadataBlock::VorbisComment) {
      // Remove the old Vorbis Comment block
      delete *it;
      it = d->blocks.erase(it);
      continue;
    }
    if(commentBlock && (*it)->code() == MetadataBlock::Picture) {
      // Set the new Vorbis Comment block before the first picture block
      d->blocks.insert(it, commentBlock);
      commentBlock = 0;
    }
    ++it;
  }
  if(commentBlock)
    d->blocks.append(commentBlock);

  // Render data for the metadata blocks

  ByteVector data;
  for(BlockConstIterator it = d->blocks.begin(); it != d->blocks.end(); ++it) {
    ByteVector blockData = (*it)->render();
    ByteVector blockHeader = ByteVector::fromUInt(blockData.size());
    blockHeader[0] = (*it)->code();
    data.append(blockHeader);
    data.append(blockData);
  }

  // Compute the amount of padding, and append that to data.

  long originalLength = d->streamStart - d->flacStart;
  long paddingLength = originalLength - data.size() - 4;

  if(paddingLength <= 0) {
    paddingLength = MinPaddingLength;
  }
  else {
    // Padding won't increase beyond 1% of the file size or 1MB.

    long threshold = length() / 100;
    threshold = std::max(threshold, MinPaddingLength);
    threshold = std::min(threshold, MaxPaddingLegnth);

    if(paddingLength > threshold)
      paddingLength = MinPaddingLength;
  }

  ByteVector paddingHeader = ByteVector::fromUInt(paddingLength);
  paddingHeader[0] = static_cast<char>(MetadataBlock::Padding | LastBlockFlag);
  data.append(paddingHeader);
  data.resize(static_cast<unsigned int>(data.size() + paddingLength));

  // Write the data to the file

  insert(data, d->flacStart, originalLength);

  d->streamStart += (static_cast<long>(data.size()) - originalLength);

  if(d->ID3v1Location >= 0)
    d->ID3v1Location += (static_cast<long>(data.size()) - originalLength);

  // Update ID3 tags

  if(ID3v2Tag() && !ID3v2Tag()->isEmpty()) {

    // ID3v2 tag is not empty. Update the old one or create a new one.

    if(d->ID3v2Location < 0)
      d->ID3v2Location = 0;

    data = ID3v2Tag()->render();
    insert(data, d->ID3v2Location, d->ID3v2OriginalSize);

    d->flacStart   += (static_cast<long>(data.size()) - d->ID3v2OriginalSize);
    d->streamStart += (static_cast<long>(data.size()) - d->ID3v2OriginalSize);

    if(d->ID3v1Location >= 0)
      d->ID3v1Location += (static_cast<long>(data.size()) - d->ID3v2OriginalSize);

    d->ID3v2OriginalSize = data.size();
  }
  else {

    // ID3v2 tag is empty. Remove the old one.

    if(d->ID3v2Location >= 0) {
      removeBlock(d->ID3v2Location, d->ID3v2OriginalSize);

      d->flacStart   -= d->ID3v2OriginalSize;
      d->streamStart -= d->ID3v2OriginalSize;

      if(d->ID3v1Location >= 0)
        d->ID3v1Location -= d->ID3v2OriginalSize;

      d->ID3v2Location = -1;
      d->ID3v2OriginalSize = 0;
    }
  }

  if(ID3v1Tag() && !ID3v1Tag()->isEmpty()) {

    // ID3v1 tag is not empty. Update the old one or create a new one.

    if(d->ID3v1Location >= 0) {
      seek(d->ID3v1Location);
    }
    else {
      seek(0, End);
      d->ID3v1Location = tell();
    }

    writeBlock(ID3v1Tag()->render());
  }
  else {

    // ID3v1 tag is empty. Remove the old one.

    if(d->ID3v1Location >= 0) {
      truncate(d->ID3v1Location);
      d->ID3v1Location = -1;
    }
  }

  return true;
}

ID3v2::Tag *FLAC::File::ID3v2Tag(bool create)
{
  return d->tag.access<ID3v2::Tag>(FlacID3v2Index, create);
}

ID3v1::Tag *FLAC::File::ID3v1Tag(bool create)
{
  return d->tag.access<ID3v1::Tag>(FlacID3v1Index, create);
}

Ogg::XiphComment *FLAC::File::xiphComment(bool create)
{
  return d->tag.access<Ogg::XiphComment>(FlacXiphIndex, create);
}

void FLAC::File::setID3v2FrameFactory(const ID3v2::FrameFactory *factory)
{
  d->ID3v2FrameFactory = factory;
}

ByteVector FLAC::File::streamInfoData()
{
  debug("FLAC::File::streamInfoData() -- This function is obsolete. Returning an empty ByteVector.");
  return ByteVector();
}

long FLAC::File::streamLength()
{
  debug("FLAC::File::streamLength() -- This function is obsolete. Returning zero.");
  return 0;
}

List<FLAC::Picture *> FLAC::File::pictureList()
{
  List<Picture *> pictures;
  for(BlockConstIterator it = d->blocks.begin(); it != d->blocks.end(); ++it) {
    Picture *picture = dynamic_cast<Picture *>(*it);
    if(picture) {
      pictures.append(picture);
    }
  }
  return pictures;
}

void FLAC::File::addPicture(Picture *picture)
{
  d->blocks.append(picture);
}

void FLAC::File::removePicture(Picture *picture, bool del)
{
  BlockIterator it = d->blocks.find(picture);
  if(it != d->blocks.end())
    d->blocks.erase(it);

  if(del)
    delete picture;
}

void FLAC::File::removePictures()
{
  for(BlockIterator it = d->blocks.begin(); it != d->blocks.end(); ) {
    if(dynamic_cast<Picture *>(*it)) {
      delete *it;
      it = d->blocks.erase(it);
    }
    else {
      ++it;
    }
  }
}

void FLAC::File::strip(int tags)
{
  if(tags & ID3v1)
    d->tag.set(FlacID3v1Index, 0);

  if(tags & ID3v2)
    d->tag.set(FlacID3v2Index, 0);

  if(tags & XiphComment) {
    xiphComment()->removeAllFields();
    xiphComment()->removeAllPictures();
  }
}

bool FLAC::File::hasXiphComment() const
{
  return !d->xiphCommentData.isEmpty();
}

bool FLAC::File::hasID3v1Tag() const
{
  return (d->ID3v1Location >= 0);
}

bool FLAC::File::hasID3v2Tag() const
{
  return (d->ID3v2Location >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void FLAC::File::read(bool readProperties)
{
  // Look for an ID3v2 tag

  d->ID3v2Location = Utils::findID3v2(this);

  if(d->ID3v2Location >= 0) {
    d->tag.set(FlacID3v2Index, new ID3v2::Tag(this, d->ID3v2Location, d->ID3v2FrameFactory));
    d->ID3v2OriginalSize = ID3v2Tag()->header()->completeTagSize();
  }

  // Look for an ID3v1 tag

  d->ID3v1Location = Utils::findID3v1(this);

  if(d->ID3v1Location >= 0)
    d->tag.set(FlacID3v1Index, new ID3v1::Tag(this, d->ID3v1Location));

  // Look for FLAC metadata, including vorbis comments

  scan();

  if(!isValid())
    return;

  if(!d->xiphCommentData.isEmpty())
    d->tag.set(FlacXiphIndex, new Ogg::XiphComment(d->xiphCommentData));
  else
    d->tag.set(FlacXiphIndex, new Ogg::XiphComment());

  if(readProperties) {

    // First block should be the stream_info metadata

    const ByteVector infoData = d->blocks.front()->render();

    long streamLength;

    if(d->ID3v1Location >= 0)
      streamLength = d->ID3v1Location - d->streamStart;
    else
      streamLength = length() - d->streamStart;

    d->properties = new Properties(infoData, streamLength);
  }
}

void FLAC::File::scan()
{
  // Scan the metadata pages

  if(d->scanned)
    return;

  if(!isValid())
    return;

  long nextBlockOffset;

  if(d->ID3v2Location >= 0)
    nextBlockOffset = find("fLaC", d->ID3v2Location + d->ID3v2OriginalSize);
  else
    nextBlockOffset = find("fLaC");

  if(nextBlockOffset < 0) {
    debug("FLAC::File::scan() -- FLAC stream not found");
    setValid(false);
    return;
  }

  nextBlockOffset += 4;
  d->flacStart = nextBlockOffset;

  while(true) {

    seek(nextBlockOffset);
    const ByteVector header = readBlock(4);
    if(header.size() != 4) {
      debug("FLAC::File::scan() -- Failed to read a block header");
      setValid(false);
      return;
    }

    // Header format (from spec):
    // <1> Last-metadata-block flag
    // <7> BLOCK_TYPE
    //    0 : STREAMINFO
    //    1 : PADDING
    //    ..
    //    4 : VORBIS_COMMENT
    //    ..
    //    6 : PICTURE
    //    ..
    // <24> Length of metadata to follow

    const char blockType = header[0] & ~LastBlockFlag;
    const bool isLastBlock = (header[0] & LastBlockFlag) != 0;
    const unsigned int blockLength = header.toUInt(1U, 3U);

    // First block should be the stream_info metadata

    if(d->blocks.isEmpty() && blockType != MetadataBlock::StreamInfo) {
      debug("FLAC::File::scan() -- First block should be the stream_info metadata");
      setValid(false);
      return;
    }

    if(blockLength == 0
      && blockType != MetadataBlock::Padding && blockType != MetadataBlock::SeekTable)
    {
      debug("FLAC::File::scan() -- Zero-sized metadata block found");
      setValid(false);
      return;
    }

    const ByteVector data = readBlock(blockLength);
    if(data.size() != blockLength) {
      debug("FLAC::File::scan() -- Failed to read a metadata block");
      setValid(false);
      return;
    }

    MetadataBlock *block = 0;

    // Found the vorbis-comment
    if(blockType == MetadataBlock::VorbisComment) {
      if(d->xiphCommentData.isEmpty()) {
        d->xiphCommentData = data;
        block = new UnknownMetadataBlock(MetadataBlock::VorbisComment, data);
      }
      else {
        debug("FLAC::File::scan() -- multiple Vorbis Comment blocks found, discarding");
      }
    }
    else if(blockType == MetadataBlock::Picture) {
      FLAC::Picture *picture = new FLAC::Picture();
      if(picture->parse(data)) {
        block = picture;
      }
      else {
        debug("FLAC::File::scan() -- invalid picture found, discarding");
        delete picture;
      }
    }
    else if(blockType == MetadataBlock::Padding) {
      // Skip all padding blocks.
    }
    else {
      block = new UnknownMetadataBlock(blockType, data);
    }

    if(block)
      d->blocks.append(block);

    nextBlockOffset += blockLength + 4;

    if(isLastBlock)
      break;
  }

  // End of metadata, now comes the datastream

  d->streamStart = nextBlockOffset;

  d->scanned = true;
}
