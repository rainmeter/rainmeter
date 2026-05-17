/***************************************************************************
    copyright            : (C) 2013 by Lukas Krejci
    email                : krejclu6@fel.cvut.cz
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

#include <tbytevectorlist.h>
#include <tpropertymap.h>
#include <tdebug.h>
#include <stdio.h>

#include "chapterframe.h"

using namespace TagLib;
using namespace ID3v2;

class ChapterFrame::ChapterFramePrivate
{
public:
  ChapterFramePrivate() :
    tagHeader(0),
    startTime(0),
    endTime(0),
    startOffset(0),
    endOffset(0)
  {
    embeddedFrameList.setAutoDelete(true);
  }

  const ID3v2::Header *tagHeader;
  ByteVector elementID;
  unsigned int startTime;
  unsigned int endTime;
  unsigned int startOffset;
  unsigned int endOffset;
  FrameListMap embeddedFrameListMap;
  FrameList embeddedFrameList;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

ChapterFrame::ChapterFrame(const ID3v2::Header *tagHeader, const ByteVector &data) :
  ID3v2::Frame(data),
  d(new ChapterFramePrivate())
{
  d->tagHeader = tagHeader;
  setData(data);
}

ChapterFrame::ChapterFrame(const ByteVector &elementID,
                           unsigned int startTime, unsigned int endTime,
                           unsigned int startOffset, unsigned int endOffset,
                           const FrameList &embeddedFrames) :
  ID3v2::Frame("CHAP"),
  d(new ChapterFramePrivate())
{
  // setElementID has a workaround for a previously silly API where you had to
  // specifically include the null byte.

  setElementID(elementID);

  d->startTime = startTime;
  d->endTime = endTime;
  d->startOffset = startOffset;
  d->endOffset = endOffset;

  for(FrameList::ConstIterator it = embeddedFrames.begin();
      it != embeddedFrames.end(); ++it)
    addEmbeddedFrame(*it);
}

ChapterFrame::~ChapterFrame()
{
  delete d;
}

ByteVector ChapterFrame::elementID() const
{
  return d->elementID;
}

unsigned int ChapterFrame::startTime() const
{
  return d->startTime;
}

unsigned int ChapterFrame::endTime() const
{
  return d->endTime;
}

unsigned int ChapterFrame::startOffset() const
{
  return d->startOffset;
}

unsigned int ChapterFrame::endOffset() const
{
  return d->endOffset;
}

void ChapterFrame::setElementID(const ByteVector &eID)
{
  d->elementID = eID;

  if(d->elementID.endsWith(static_cast<char>(0)))
    d->elementID = d->elementID.mid(0, d->elementID.size() - 1);
}

void ChapterFrame::setStartTime(const unsigned int &sT)
{
  d->startTime = sT;
}

void ChapterFrame::setEndTime(const unsigned int &eT)
{
  d->endTime = eT;
}

void ChapterFrame::setStartOffset(const unsigned int &sO)
{
  d->startOffset = sO;
}

void ChapterFrame::setEndOffset(const unsigned int &eO)
{
  d->endOffset = eO;
}

const FrameListMap &ChapterFrame::embeddedFrameListMap() const
{
  return d->embeddedFrameListMap;
}

const FrameList &ChapterFrame::embeddedFrameList() const
{
  return d->embeddedFrameList;
}

const FrameList &ChapterFrame::embeddedFrameList(const ByteVector &frameID) const
{
  return d->embeddedFrameListMap[frameID];
}

void ChapterFrame::addEmbeddedFrame(Frame *frame)
{
  d->embeddedFrameList.append(frame);
  d->embeddedFrameListMap[frame->frameID()].append(frame);
}

void ChapterFrame::removeEmbeddedFrame(Frame *frame, bool del)
{
  // remove the frame from the frame list
  FrameList::Iterator it = d->embeddedFrameList.find(frame);
  d->embeddedFrameList.erase(it);

  // ...and from the frame list map
  it = d->embeddedFrameListMap[frame->frameID()].find(frame);
  d->embeddedFrameListMap[frame->frameID()].erase(it);

  // ...and delete as desired
  if(del)
    delete frame;
}

void ChapterFrame::removeEmbeddedFrames(const ByteVector &id)
{
  FrameList l = d->embeddedFrameListMap[id];
  for(FrameList::ConstIterator it = l.begin(); it != l.end(); ++it)
    removeEmbeddedFrame(*it, true);
}

String ChapterFrame::toString() const
{
  String s = String(d->elementID) +
             ": start time: " + String::number(d->startTime) +
             ", end time: " + String::number(d->endTime);

  if(d->startOffset != 0xFFFFFFFF)
    s += ", start offset: " + String::number(d->startOffset);

  if(d->endOffset != 0xFFFFFFFF)
    s += ", end offset: " + String::number(d->endOffset);

  if(!d->embeddedFrameList.isEmpty()) {
    StringList frameIDs;
    for(FrameList::ConstIterator it = d->embeddedFrameList.begin();
        it != d->embeddedFrameList.end(); ++it)
      frameIDs.append((*it)->frameID());
    s += ", sub-frames: [ " + frameIDs.toString(", ") + " ]";
  }

  return s;
}

PropertyMap ChapterFrame::asProperties() const
{
  PropertyMap map;

  map.unsupportedData().append(frameID() + String("/") + d->elementID);

  return map;
}

ChapterFrame *ChapterFrame::findByElementID(const ID3v2::Tag *tag, const ByteVector &eID) // static
{
  ID3v2::FrameList comments = tag->frameList("CHAP");

  for(ID3v2::FrameList::ConstIterator it = comments.begin();
      it != comments.end();
      ++it)
  {
    ChapterFrame *frame = dynamic_cast<ChapterFrame *>(*it);
    if(frame && frame->elementID() == eID)
      return frame;
  }

  return 0;
}

void ChapterFrame::parseFields(const ByteVector &data)
{
  unsigned int size = data.size();
  if(size < 18) {
    debug("A CHAP frame must contain at least 18 bytes (1 byte element ID "
          "terminated by null and 4x4 bytes for start and end time and offset).");
    return;
  }

  int pos = 0;
  unsigned int embPos = 0;
  d->elementID = readStringField(data, String::Latin1, &pos).data(String::Latin1);
  d->startTime = data.toUInt(pos, true);
  pos += 4;
  d->endTime = data.toUInt(pos, true);
  pos += 4;
  d->startOffset = data.toUInt(pos, true);
  pos += 4;
  d->endOffset = data.toUInt(pos, true);
  pos += 4;
  size -= pos;

  // Embedded frames are optional

  if(size < header()->size())
    return;

  while(embPos < size - header()->size()) {
    Frame *frame = FrameFactory::instance()->createFrame(data.mid(pos + embPos), d->tagHeader);

    if(!frame)
      return;

    // Checks to make sure that frame parsed correctly.
    if(frame->size() <= 0) {
      delete frame;
      return;
    }

    embPos += frame->size() + header()->size();
    addEmbeddedFrame(frame);
  }
}

ByteVector ChapterFrame::renderFields() const
{
  ByteVector data;

  data.append(d->elementID);
  data.append('\0');
  data.append(ByteVector::fromUInt(d->startTime, true));
  data.append(ByteVector::fromUInt(d->endTime, true));
  data.append(ByteVector::fromUInt(d->startOffset, true));
  data.append(ByteVector::fromUInt(d->endOffset, true));
  FrameList l = d->embeddedFrameList;
  for(FrameList::ConstIterator it = l.begin(); it != l.end(); ++it) {
    (*it)->header()->setVersion(header()->version());
    data.append((*it)->render());
  }

  return data;
}

ChapterFrame::ChapterFrame(const ID3v2::Header *tagHeader, const ByteVector &data, Header *h) :
  Frame(h),
  d(new ChapterFramePrivate())
{
  d->tagHeader = tagHeader;
  parseFields(fieldData(data));
}
