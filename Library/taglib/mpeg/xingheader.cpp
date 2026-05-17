/***************************************************************************
    copyright            : (C) 2003 by Ismael Orenstein
    email                : orenstein@kde.org
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
#include <tdebug.h>

#include "xingheader.h"
#include "mpegfile.h"

using namespace TagLib;

class MPEG::XingHeader::XingHeaderPrivate
{
public:
  XingHeaderPrivate() :
    frames(0),
    size(0),
    type(MPEG::XingHeader::Invalid) {}

  unsigned int frames;
  unsigned int size;

  MPEG::XingHeader::HeaderType type;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MPEG::XingHeader::XingHeader(const ByteVector &data) :
  d(new XingHeaderPrivate())
{
  parse(data);
}

MPEG::XingHeader::~XingHeader()
{
  delete d;
}

bool MPEG::XingHeader::isValid() const
{
  return (d->type != Invalid && d->frames > 0 && d->size > 0);
}

unsigned int MPEG::XingHeader::totalFrames() const
{
  return d->frames;
}

unsigned int MPEG::XingHeader::totalSize() const
{
  return d->size;
}

MPEG::XingHeader::HeaderType MPEG::XingHeader::type() const
{
  return d->type;
}

int MPEG::XingHeader::xingHeaderOffset(TagLib::MPEG::Header::Version /*v*/,
                                       TagLib::MPEG::Header::ChannelMode /*c*/)
{
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void MPEG::XingHeader::parse(const ByteVector &data)
{
  // Look for a Xing header.

  long offset = data.find("Xing");
  if(offset < 0)
    offset = data.find("Info");

  if(offset >= 0) {

    // Xing header found.

    if(data.size() < static_cast<unsigned long>(offset + 16)) {
      debug("MPEG::XingHeader::parse() -- Xing header found but too short.");
      return;
    }

    if((data[offset + 7] & 0x03) != 0x03) {
      debug("MPEG::XingHeader::parse() -- Xing header doesn't contain the required information.");
      return;
    }

    d->frames = data.toUInt(offset + 8,  true);
    d->size   = data.toUInt(offset + 12, true);
    d->type   = Xing;
  }
  else {

    // Xing header not found. Then look for a VBRI header.

    offset = data.find("VBRI");

    if(offset >= 0) {

      // VBRI header found.

      if(data.size() < static_cast<unsigned long>(offset + 32)) {
        debug("MPEG::XingHeader::parse() -- VBRI header found but too short.");
        return;
      }

      d->frames = data.toUInt(offset + 14, true);
      d->size   = data.toUInt(offset + 10, true);
      d->type   = VBRI;
    }
  }
}
