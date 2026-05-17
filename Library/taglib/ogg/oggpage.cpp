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

#include <algorithm>

#include <tstring.h>
#include <tdebug.h>

#include "oggpage.h"
#include "oggpageheader.h"
#include "oggfile.h"

using namespace TagLib;

class Ogg::Page::PagePrivate
{
public:
  PagePrivate(File *f = 0, long pageOffset = -1) :
    file(f),
    fileOffset(pageOffset),
    header(f, pageOffset),
    firstPacketIndex(-1) {}

  File *file;
  long fileOffset;
  PageHeader header;
  int firstPacketIndex;
  ByteVectorList packets;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Ogg::Page::Page(Ogg::File *file, long pageOffset) :
  d(new PagePrivate(file, pageOffset))
{
}

Ogg::Page::~Page()
{
  delete d;
}

long Ogg::Page::fileOffset() const
{
  return d->fileOffset;
}

const Ogg::PageHeader *Ogg::Page::header() const
{
  return &d->header;
}

int Ogg::Page::pageSequenceNumber() const
{
  return d->header.pageSequenceNumber();
}

void Ogg::Page::setPageSequenceNumber(int sequenceNumber)
{
  d->header.setPageSequenceNumber(sequenceNumber);
}

int Ogg::Page::firstPacketIndex() const
{
  return d->firstPacketIndex;
}

void Ogg::Page::setFirstPacketIndex(int index)
{
  d->firstPacketIndex = index;
}

Ogg::Page::ContainsPacketFlags Ogg::Page::containsPacket(int index) const
{
  const int lastPacketIndex = d->firstPacketIndex + packetCount() - 1;
  if(index < d->firstPacketIndex || index > lastPacketIndex)
    return DoesNotContainPacket;

  ContainsPacketFlags flags = DoesNotContainPacket;

  if(index == d->firstPacketIndex)
    flags = static_cast<ContainsPacketFlags>(flags | BeginsWithPacket);

  if(index == lastPacketIndex)
    flags = static_cast<ContainsPacketFlags>(flags | EndsWithPacket);

  // If there's only one page and it's complete:

  if(packetCount() == 1 &&
     !d->header.firstPacketContinued() &&
     d->header.lastPacketCompleted())
  {
    flags = static_cast<ContainsPacketFlags>(flags | CompletePacket);
  }

  // Or if there is more than one page and the page is
  // (a) the first page and it's complete or
  // (b) the last page and it's complete or
  // (c) a page in the middle.
  else if(packetCount() > 1 &&
          ((flags & BeginsWithPacket && !d->header.firstPacketContinued()) ||
           (flags & EndsWithPacket && d->header.lastPacketCompleted()) ||
           (!(flags & BeginsWithPacket) && !(flags & EndsWithPacket))))
  {
    flags = static_cast<ContainsPacketFlags>(flags | CompletePacket);
  }

  return flags;
}

unsigned int Ogg::Page::packetCount() const
{
  return d->header.packetSizes().size();
}

ByteVectorList Ogg::Page::packets() const
{
  if(!d->packets.isEmpty())
    return d->packets;

  ByteVectorList l;

  if(d->file && d->header.isValid()) {

    d->file->seek(d->fileOffset + d->header.size());

    List<int> packetSizes = d->header.packetSizes();

    List<int>::ConstIterator it = packetSizes.begin();
    for(; it != packetSizes.end(); ++it)
      l.append(d->file->readBlock(*it));
  }
  else
    debug("Ogg::Page::packets() -- attempting to read packets from an invalid page.");

  return l;
}

int Ogg::Page::size() const
{
  return d->header.size() + d->header.dataSize();
}

ByteVector Ogg::Page::render() const
{
  ByteVector data;

  data.append(d->header.render());

  if(d->packets.isEmpty()) {
    if(d->file) {
      d->file->seek(d->fileOffset + d->header.size());
      data.append(d->file->readBlock(d->header.dataSize()));
    }
    else
      debug("Ogg::Page::render() -- this page is empty!");
  }
  else {
    ByteVectorList::ConstIterator it = d->packets.begin();
    for(; it != d->packets.end(); ++it)
      data.append(*it);
  }

  // Compute and set the checksum for the Ogg page.  The checksum is taken over
  // the entire page with the 4 bytes reserved for the checksum zeroed and then
  // inserted in bytes 22-25 of the page header.

  const ByteVector checksum = ByteVector::fromUInt(data.checksum(), false);
  std::copy(checksum.begin(), checksum.end(), data.begin() + 22);

  return data;
}

List<Ogg::Page *> Ogg::Page::paginate(const ByteVectorList &packets,
                                      PaginationStrategy strategy,
                                      unsigned int streamSerialNumber,
                                      int firstPage,
                                      bool firstPacketContinued,
                                      bool lastPacketCompleted,
                                      bool containsLastPacket)
{
  // SplitSize must be a multiple of 255 in order to get the lacing values right
  // create pages of about 8KB each

  static const unsigned int SplitSize = 32 * 255;

  // Force repagination if the segment table will exceed the size limit.

  if(strategy != Repaginate) {

    size_t tableSize = 0;
    for(ByteVectorList::ConstIterator it = packets.begin(); it != packets.end(); ++it)
      tableSize += it->size() / 255 + 1;

    if(tableSize > 255)
      strategy = Repaginate;
  }

  List<Page *> l;

  // Handle creation of multiple pages with appropriate pagination.

  if(strategy == Repaginate) {

    int pageIndex = firstPage;

    for(ByteVectorList::ConstIterator it = packets.begin(); it != packets.end(); ++it) {

      const bool lastPacketInList = (it == --packets.end());

      // mark very first packet?

      bool continued = (firstPacketContinued && it == packets.begin());
      unsigned int pos = 0;

      while(pos < it->size()) {

        const bool lastSplit = (pos + SplitSize >= it->size());

        ByteVectorList packetList;
        packetList.append(it->mid(pos, SplitSize));

        l.append(new Page(packetList,
                          streamSerialNumber,
                          pageIndex,
                          continued,
                          lastSplit && (lastPacketInList ? lastPacketCompleted : true),
                          lastSplit && (containsLastPacket && lastPacketInList)));
        pageIndex++;
        continued = true;

        pos += SplitSize;
      }
    }
  }
  else {
    l.append(new Page(packets,
                      streamSerialNumber,
                      firstPage,
                      firstPacketContinued,
                      lastPacketCompleted,
                      containsLastPacket));
  }

  return l;
}

Ogg::Page* Ogg::Page::getCopyWithNewPageSequenceNumber(int /*sequenceNumber*/)
{
  debug("Ogg::Page::getCopyWithNewPageSequenceNumber() -- This function is obsolete. Returning null.");
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

Ogg::Page::Page(const ByteVectorList &packets,
                unsigned int streamSerialNumber,
                int pageNumber,
                bool firstPacketContinued,
                bool lastPacketCompleted,
                bool containsLastPacket) :
  d(new PagePrivate())
{
  d->header.setFirstPageOfStream(pageNumber == 0 && !firstPacketContinued);
  d->header.setLastPageOfStream(containsLastPacket);
  d->header.setFirstPacketContinued(firstPacketContinued);
  d->header.setLastPacketCompleted(lastPacketCompleted);
  d->header.setStreamSerialNumber(streamSerialNumber);
  d->header.setPageSequenceNumber(pageNumber);

  // Build a page from the list of packets.

  ByteVector data;
  List<int> packetSizes;

  for(ByteVectorList::ConstIterator it = packets.begin(); it != packets.end(); ++it) {
    packetSizes.append((*it).size());
    data.append(*it);
  }
  d->packets = packets;
  d->header.setPacketSizes(packetSizes);

  // https://xiph.org/ogg/doc/framing.html, absolute granule position:
  // A special value of '-1' (in two's complement) indicates that no packets
  // finish on this page.
  if(!lastPacketCompleted && packets.size() <= 1)
    d->header.setAbsoluteGranularPosition(-1);
}
