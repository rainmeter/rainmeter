/**************************************************************************
    copyright            : (C) 2007 by Lukáš Lalinský
    email                : lalinsky@gmail.com
 **************************************************************************/

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

#include <tdebug.h>
#include <tstring.h>
#include <tpropertymap.h>
#include <tagutils.h>

#include "mp4atom.h"
#include "mp4tag.h"
#include "mp4file.h"

using namespace TagLib;

namespace
{
  bool checkValid(const MP4::AtomList &list)
  {
    for(MP4::AtomList::ConstIterator it = list.begin(); it != list.end(); ++it) {

      if((*it)->length == 0)
        return false;

      if(!checkValid((*it)->children))
        return false;
    }

    return true;
  }
}  // namespace

class MP4::File::FilePrivate
{
public:
  FilePrivate() :
    tag(0),
    atoms(0),
    properties(0) {}

  ~FilePrivate()
  {
    delete atoms;
    delete tag;
    delete properties;
  }

  MP4::Tag        *tag;
  MP4::Atoms      *atoms;
  MP4::Properties *properties;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool MP4::File::isSupported(IOStream *stream)
{
  // An MP4 file has to have an "ftyp" box first.

  const ByteVector id = Utils::readHeader(stream, 8, false);
  return id.containsAt("ftyp", 4);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MP4::File::File(FileName file, bool readProperties, AudioProperties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

MP4::File::File(IOStream *stream, bool readProperties, AudioProperties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

MP4::File::~File()
{
  delete d;
}

MP4::Tag *
MP4::File::tag() const
{
  return d->tag;
}

PropertyMap MP4::File::properties() const
{
  return d->tag->properties();
}

void MP4::File::removeUnsupportedProperties(const StringList &properties)
{
  d->tag->removeUnsupportedProperties(properties);
}

PropertyMap MP4::File::setProperties(const PropertyMap &properties)
{
  return d->tag->setProperties(properties);
}

MP4::Properties *
MP4::File::audioProperties() const
{
  return d->properties;
}

void
MP4::File::read(bool readProperties)
{
  if(!isValid())
    return;

  d->atoms = new Atoms(this);
  if(!checkValid(d->atoms->atoms)) {
    setValid(false);
    return;
  }

  // must have a moov atom, otherwise consider it invalid
  if(!d->atoms->find("moov")) {
    setValid(false);
    return;
  }

  d->tag = new Tag(this, d->atoms);
  if(readProperties) {
    d->properties = new Properties(this, d->atoms);
  }
}

bool
MP4::File::save()
{
  if(readOnly()) {
    debug("MP4::File::save() -- File is read only.");
    return false;
  }

  if(!isValid()) {
    debug("MP4::File::save() -- Trying to save invalid file.");
    return false;
  }

  return d->tag->save();
}

bool
MP4::File::strip(int tags)
{
  if(readOnly()) {
    debug("MP4::File::strip() - Cannot strip tags from a read only file.");
    return false;
  }

  if(!isValid()) {
    debug("MP4::File::strip() -- Cannot strip tags from an invalid file.");
    return false;
  }

  if(tags & MP4) {
    return d->tag->strip();
  }

  return true;
}

bool
MP4::File::hasMP4Tag() const
{
  return (d->atoms->find("moov", "udta", "meta", "ilst") != 0);
}
