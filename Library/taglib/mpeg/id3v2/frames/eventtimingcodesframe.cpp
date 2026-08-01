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

#include "eventtimingcodesframe.h"
#include <tbytevectorlist.h>
#include <id3v2tag.h>
#include <tdebug.h>
#include <tpropertymap.h>

using namespace TagLib;
using namespace ID3v2;

class EventTimingCodesFrame::EventTimingCodesFramePrivate
{
public:
  EventTimingCodesFramePrivate() :
    timestampFormat(EventTimingCodesFrame::AbsoluteMilliseconds) {}
  EventTimingCodesFrame::TimestampFormat timestampFormat;
  EventTimingCodesFrame::SynchedEventList synchedEvents;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

EventTimingCodesFrame::EventTimingCodesFrame() :
  Frame("ETCO"),
  d(new EventTimingCodesFramePrivate())
{
}

EventTimingCodesFrame::EventTimingCodesFrame(const ByteVector &data) :
  Frame(data),
  d(new EventTimingCodesFramePrivate())
{
  setData(data);
}

EventTimingCodesFrame::~EventTimingCodesFrame()
{
  delete d;
}

String EventTimingCodesFrame::toString() const
{
  return String();
}

EventTimingCodesFrame::TimestampFormat
EventTimingCodesFrame::timestampFormat() const
{
  return d->timestampFormat;
}

EventTimingCodesFrame::SynchedEventList
EventTimingCodesFrame::synchedEvents() const
{
  return d->synchedEvents;
}

void EventTimingCodesFrame::setTimestampFormat(
    EventTimingCodesFrame::TimestampFormat f)
{
  d->timestampFormat = f;
}

void EventTimingCodesFrame::setSynchedEvents(
    const EventTimingCodesFrame::SynchedEventList &e)
{
  d->synchedEvents = e;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void EventTimingCodesFrame::parseFields(const ByteVector &data)
{
  const int end = data.size();
  if(end < 1) {
    debug("An event timing codes frame must contain at least 1 byte.");
    return;
  }

  d->timestampFormat = static_cast<TimestampFormat>(data[0]);

  int pos = 1;
  d->synchedEvents.clear();
  while(pos + 4 < end) {
    EventType type = static_cast<EventType>(static_cast<unsigned char>(data[pos++]));
    unsigned int time = data.toUInt(pos, true);
    pos += 4;
    d->synchedEvents.append(SynchedEvent(time, type));
  }
}

ByteVector EventTimingCodesFrame::renderFields() const
{
  ByteVector v;

  v.append(static_cast<char>(d->timestampFormat));
  for(SynchedEventList::ConstIterator it = d->synchedEvents.begin();
      it != d->synchedEvents.end();
      ++it) {
    const SynchedEvent &entry = *it;
    v.append(static_cast<char>(entry.type));
    v.append(ByteVector::fromUInt(entry.time));
  }

  return v;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

EventTimingCodesFrame::EventTimingCodesFrame(const ByteVector &data, Header *h) :
  Frame(h),
  d(new EventTimingCodesFramePrivate())
{
  parseFields(fieldData(data));
}
