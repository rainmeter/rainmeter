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

#include <tbytevectorlist.h>
#include <tmap.h>
#include <tstring.h>
#include <tdebug.h>

#include "oggfile.h"
#include "oggpage.h"
#include "oggpageheader.h"

using namespace TagLib;

namespace
{
  // Returns the first packet index of the right next page to the given one.
  unsigned int nextPacketIndex(const Ogg::Page *page)
  {
    if(page->header()->lastPacketCompleted())
      return page->firstPacketIndex() + page->packetCount();
    return page->firstPacketIndex() + page->packetCount() - 1;
  }
}  // namespace

class Ogg::File::FilePrivate
{
public:
  FilePrivate() :
    firstPageHeader(0),
    lastPageHeader(0)
  {
    pages.setAutoDelete(true);
  }

  ~FilePrivate()
  {
    delete firstPageHeader;
    delete lastPageHeader;
  }

  unsigned int streamSerialNumber;
  List<Page *> pages;
  PageHeader *firstPageHeader;
  PageHeader *lastPageHeader;
  Map<unsigned int, ByteVector> dirtyPackets;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Ogg::File::~File()
{
  delete d;
}

ByteVector Ogg::File::packet(unsigned int i)
{
  // Check to see if we're called setPacket() for this packet since the last
  // save:

  if(d->dirtyPackets.contains(i))
    return d->dirtyPackets[i];

  // If we haven't indexed the page where the packet we're interested in starts,
  // begin reading pages until we have.

  if(!readPages(i)) {
    debug("Ogg::File::packet() -- Could not find the requested packet.");
    return ByteVector();
  }

  // Look for the first page in which the requested packet starts.

  List<Page *>::ConstIterator it = d->pages.begin();
  while((*it)->containsPacket(i) == Page::DoesNotContainPacket)
    ++it;

  // If the packet is completely contained in the first page that it's in.

  // If the packet is *not* completely contained in the first page that it's a
  // part of then that packet trails off the end of the page.  Continue appending
  // the pages' packet data until we hit a page that either does not end with the
  // packet that we're fetching or where the last packet is complete.

  ByteVector packet = (*it)->packets()[i - (*it)->firstPacketIndex()];

  while(nextPacketIndex(*it) <= i) {
    ++it;
    packet.append((*it)->packets().front());
  }

  return packet;
}

void Ogg::File::setPacket(unsigned int i, const ByteVector &p)
{
  if(!readPages(i)) {
    debug("Ogg::File::setPacket() -- Could not set the requested packet.");
    return;
  }

  d->dirtyPackets[i] = p;
}

const Ogg::PageHeader *Ogg::File::firstPageHeader()
{
  if(!d->firstPageHeader) {
    const long firstPageHeaderOffset = find("OggS");
    if(firstPageHeaderOffset < 0)
      return 0;

    d->firstPageHeader = new PageHeader(this, firstPageHeaderOffset);
  }

  return d->firstPageHeader->isValid() ? d->firstPageHeader : 0;
}

const Ogg::PageHeader *Ogg::File::lastPageHeader()
{
  if(!d->lastPageHeader) {
    const long lastPageHeaderOffset = rfind("OggS");
    if(lastPageHeaderOffset < 0)
      return 0;

    d->lastPageHeader = new PageHeader(this, lastPageHeaderOffset);
  }

  return d->lastPageHeader->isValid() ? d->lastPageHeader : 0;
}

bool Ogg::File::save()
{
  if(readOnly()) {
    debug("Ogg::File::save() - Cannot save to a read only file.");
    return false;
  }

  Map<unsigned int, ByteVector>::ConstIterator it;
  for(it = d->dirtyPackets.begin(); it != d->dirtyPackets.end(); ++it)
    writePacket(it->first, it->second);

  d->dirtyPackets.clear();

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

Ogg::File::File(FileName file) :
  TagLib::File(file),
  d(new FilePrivate())
{
}

Ogg::File::File(IOStream *stream) :
  TagLib::File(stream),
  d(new FilePrivate())
{
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

bool Ogg::File::readPages(unsigned int i)
{
  while(true) {
    unsigned int packetIndex;
    long offset;

    if(d->pages.isEmpty()) {
      packetIndex = 0;
      offset = find("OggS");
      if(offset < 0)
        return false;
    }
    else {
      const Page *page = d->pages.back();
      packetIndex = nextPacketIndex(page);
      offset = page->fileOffset() + page->size();
    }

    // Enough pages have been fetched.

    if(packetIndex > i)
      return true;

    // Read the next page and add it to the page list.

    Page *nextPage = new Page(this, offset);
    if(!nextPage->header()->isValid()) {
      delete nextPage;
      return false;
    }

    nextPage->setFirstPacketIndex(packetIndex);
    d->pages.append(nextPage);

    if(nextPage->header()->lastPageOfStream())
      return false;
  }
}

void Ogg::File::writePacket(unsigned int i, const ByteVector &packet)
{
  if(!readPages(i)) {
    debug("Ogg::File::writePacket() -- Could not find the requested packet.");
    return;
  }

  // Look for the pages where the requested packet should belong to.

  List<Page *>::ConstIterator it = d->pages.begin();
  while((*it)->containsPacket(i) == Page::DoesNotContainPacket)
    ++it;

  const Page *firstPage = *it;

  while(nextPacketIndex(*it) <= i)
    ++it;

  const Page *lastPage = *it;

  // Replace the requested packet and create new pages to replace the located pages.

  ByteVectorList packets = firstPage->packets();
  packets[i - firstPage->firstPacketIndex()] = packet;

  if(firstPage != lastPage && lastPage->packetCount() > 1) {
    ByteVectorList lastPagePackets = lastPage->packets();
    lastPagePackets.erase(lastPagePackets.begin());
    packets.append(lastPagePackets);
  }

  // TODO: This pagination method isn't accurate for what's being done here.
  // This should account for real possibilities like non-aligned packets and such.

  List<Page *> pages = Page::paginate(packets,
                                      Page::SinglePagePerGroup,
                                      firstPage->header()->streamSerialNumber(),
                                      firstPage->pageSequenceNumber(),
                                      firstPage->header()->firstPacketContinued(),
                                      lastPage->header()->lastPacketCompleted());
  pages.setAutoDelete(true);

  // Write the pages.

  ByteVector data;
  for(it = pages.begin(); it != pages.end(); ++it)
    data.append((*it)->render());

  const unsigned long originalOffset = firstPage->fileOffset();
  const unsigned long originalLength = lastPage->fileOffset() + lastPage->size() - originalOffset;

  insert(data, originalOffset, originalLength);

  // Renumber the following pages if the pages have been split or merged.

  const int numberOfNewPages
    = pages.back()->pageSequenceNumber() - lastPage->pageSequenceNumber();

  if(numberOfNewPages != 0) {
    long pageOffset = originalOffset + data.size();

    while(true) {
      Page page(this, pageOffset);
      if(!page.header()->isValid())
        break;

      page.setPageSequenceNumber(page.pageSequenceNumber() + numberOfNewPages);
      const ByteVector data = page.render();

      seek(pageOffset + 18);
      writeBlock(data.mid(18, 8));

      if(page.header()->lastPageOfStream())
        break;

      pageOffset += page.size();
    }
  }

  // Discard all the pages to keep them up-to-date by fetching them again.

  d->pages.clear();
}
