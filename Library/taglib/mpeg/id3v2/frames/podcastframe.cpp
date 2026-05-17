/***************************************************************************
    copyright            : (C) 2015 by Urs Fleisch
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

#include "podcastframe.h"
#include <tpropertymap.h>

using namespace TagLib;
using namespace ID3v2;

class PodcastFrame::PodcastFramePrivate
{
public:
  ByteVector fieldData;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PodcastFrame::PodcastFrame() :
  Frame("PCST"),
  d(new PodcastFramePrivate())
{
  d->fieldData = ByteVector(4, '\0');
}

PodcastFrame::~PodcastFrame()
{
  delete d;
}

String PodcastFrame::toString() const
{
  return String();
}

PropertyMap PodcastFrame::asProperties() const
{
  PropertyMap map;
  map.insert("PODCAST", StringList());
  return map;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void PodcastFrame::parseFields(const ByteVector &data)
{
  d->fieldData = data;
}

ByteVector PodcastFrame::renderFields() const
{
  return d->fieldData;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

PodcastFrame::PodcastFrame(const ByteVector &data, Header *h) :
  Frame(h),
  d(new PodcastFramePrivate())
{
  parseFields(fieldData(data));
}
