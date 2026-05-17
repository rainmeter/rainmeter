/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
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
#include <tagunion.h>
#include <tdebug.h>
#include <tpropertymap.h>
#include <tagutils.h>

#include "mpcfile.h"
#include "id3v1tag.h"
#include "id3v2header.h"
#include "apetag.h"
#include "apefooter.h"

using namespace TagLib;

namespace
{
  enum { MPCAPEIndex = 0, MPCID3v1Index = 1 };
} // namespace

class MPC::File::FilePrivate
{
public:
  FilePrivate() :
    APELocation(-1),
    APESize(0),
    ID3v1Location(-1),
    ID3v2Header(0),
    ID3v2Location(-1),
    ID3v2Size(0),
    properties(0) {}

  ~FilePrivate()
  {
    delete ID3v2Header;
    delete properties;
  }

  long APELocation;
  long APESize;

  long ID3v1Location;

  ID3v2::Header *ID3v2Header;
  long ID3v2Location;
  long ID3v2Size;

  TagUnion tag;

  Properties *properties;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool MPC::File::isSupported(IOStream *stream)
{
  // A newer MPC file has to start with "MPCK" or "MP+", but older files don't
  // have keys to do a quick check.

  const ByteVector id = Utils::readHeader(stream, 4, false);
  return (id == "MPCK" || id.startsWith("MP+"));
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MPC::File::File(FileName file, bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

MPC::File::File(IOStream *stream, bool readProperties, Properties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

MPC::File::~File()
{
  delete d;
}

TagLib::Tag *MPC::File::tag() const
{
  return &d->tag;
}

PropertyMap MPC::File::properties() const
{
  return d->tag.properties();
}

void MPC::File::removeUnsupportedProperties(const StringList &properties)
{
  d->tag.removeUnsupportedProperties(properties);
}

PropertyMap MPC::File::setProperties(const PropertyMap &properties)
{
  if(ID3v1Tag())
    ID3v1Tag()->setProperties(properties);

  return APETag(true)->setProperties(properties);
}

MPC::Properties *MPC::File::audioProperties() const
{
  return d->properties;
}

bool MPC::File::save()
{
  if(readOnly()) {
    debug("MPC::File::save() -- File is read only.");
    return false;
  }

  // Possibly strip ID3v2 tag

  if(!d->ID3v2Header && d->ID3v2Location >= 0) {
    removeBlock(d->ID3v2Location, d->ID3v2Size);

    if(d->APELocation >= 0)
      d->APELocation -= d->ID3v2Size;

    if(d->ID3v1Location >= 0)
      d->ID3v1Location -= d->ID3v2Size;

    d->ID3v2Location = -1;
    d->ID3v2Size = 0;
  }

  // Update ID3v1 tag

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

  // Update APE tag

  if(APETag() && !APETag()->isEmpty()) {

    // APE tag is not empty. Update the old one or create a new one.

    if(d->APELocation < 0) {
      if(d->ID3v1Location >= 0)
        d->APELocation = d->ID3v1Location;
      else
        d->APELocation = length();
    }

    const ByteVector data = APETag()->render();
    insert(data, d->APELocation, d->APESize);

    if(d->ID3v1Location >= 0)
      d->ID3v1Location += (static_cast<long>(data.size()) - d->APESize);

    d->APESize = data.size();
  }
  else {

    // APE tag is empty. Remove the old one.

    if(d->APELocation >= 0) {
      removeBlock(d->APELocation, d->APESize);

      if(d->ID3v1Location >= 0)
        d->ID3v1Location -= d->APESize;

      d->APELocation = -1;
      d->APESize = 0;
    }
  }

  return true;
}

ID3v1::Tag *MPC::File::ID3v1Tag(bool create)
{
  return d->tag.access<ID3v1::Tag>(MPCID3v1Index, create);
}

APE::Tag *MPC::File::APETag(bool create)
{
  return d->tag.access<APE::Tag>(MPCAPEIndex, create);
}

void MPC::File::strip(int tags)
{
  if(tags & ID3v1)
    d->tag.set(MPCID3v1Index, 0);

  if(tags & APE)
    d->tag.set(MPCAPEIndex, 0);

  if(!ID3v1Tag())
    APETag(true);

  if(tags & ID3v2) {
    delete d->ID3v2Header;
    d->ID3v2Header = 0;
  }
}

void MPC::File::remove(int tags)
{
  strip(tags);
}

bool MPC::File::hasID3v1Tag() const
{
  return (d->ID3v1Location >= 0);
}

bool MPC::File::hasAPETag() const
{
  return (d->APELocation >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void MPC::File::read(bool readProperties)
{
  // Look for an ID3v2 tag

  d->ID3v2Location = Utils::findID3v2(this);

  if(d->ID3v2Location >= 0) {
    seek(d->ID3v2Location);
    d->ID3v2Header = new ID3v2::Header(readBlock(ID3v2::Header::size()));
    d->ID3v2Size = d->ID3v2Header->completeTagSize();
  }

  // Look for an ID3v1 tag

  d->ID3v1Location = Utils::findID3v1(this);

  if(d->ID3v1Location >= 0)
    d->tag.set(MPCID3v1Index, new ID3v1::Tag(this, d->ID3v1Location));

  // Look for an APE tag

  d->APELocation = Utils::findAPE(this, d->ID3v1Location);

  if(d->APELocation >= 0) {
    d->tag.set(MPCAPEIndex, new APE::Tag(this, d->APELocation));
    d->APESize = APETag()->footer()->completeTagSize();
    d->APELocation = d->APELocation + APE::Footer::size() - d->APESize;
  }

  if(d->ID3v1Location < 0)
    APETag(true);

  // Look for MPC metadata

  if(readProperties) {

    long streamLength;

    if(d->APELocation >= 0)
      streamLength = d->APELocation;
    else if(d->ID3v1Location >= 0)
      streamLength = d->ID3v1Location;
    else
      streamLength = length();

    if(d->ID3v2Location >= 0) {
      seek(d->ID3v2Location + d->ID3v2Size);
      streamLength -= (d->ID3v2Location + d->ID3v2Size);
    }
    else {
      seek(0);
    }

    d->properties = new Properties(this, streamLength);
  }
}
