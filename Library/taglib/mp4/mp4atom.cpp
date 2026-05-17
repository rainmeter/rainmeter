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

#include <climits>

#include <tdebug.h>
#include <tstring.h>
#include "mp4atom.h"

using namespace TagLib;

const char *const MP4::Atom::containers[11] = {
    "moov", "udta", "mdia", "meta", "ilst",
    "stbl", "minf", "moof", "traf", "trak",
    "stsd"
};

MP4::Atom::Atom(File *file)
  : offset(file->tell())
{
  children.setAutoDelete(true);

  ByteVector header = file->readBlock(8);
  if(header.size() != 8) {
    // The atom header must be 8 bytes long, otherwise there is either
    // trailing garbage or the file is truncated
    debug("MP4: Couldn't read 8 bytes of data for atom header");
    length = 0;
    file->seek(0, File::End);
    return;
  }

  length = header.toUInt();

  if(length == 0) {
    // The last atom which extends to the end of the file.
    length = file->length() - offset;
  }
  else if(length == 1) {
    // The atom has a 64-bit length.
    const long long longLength = file->readBlock(8).toLongLong();
    if(longLength <= LONG_MAX) {
      // The actual length fits in long. That's always the case if long is 64-bit.
      length = static_cast<long>(longLength);
    }
    else {
      debug("MP4: 64-bit atoms are not supported");
      length = 0;
      file->seek(0, File::End);
      return;
    }
  }

  if(length < 8 || length > file->length() - offset) {
    debug("MP4: Invalid atom size");
    length = 0;
    file->seek(0, File::End);
    return;
  }

  name = header.mid(4, 4);
  for(int i = 0; i < 4; ++i) {
    const char ch = name.at(i);
    if((ch < ' ' || ch > '~') && ch != '\251') {
      debug("MP4: Invalid atom type");
      length = 0;
      file->seek(0, File::End);
    }
  }

  for(int i = 0; i < numContainers; i++) {
    if(name == containers[i]) {
      if(name == "meta") {
        long posAfterMeta = file->tell();
        ByteVector nextSize = file->readBlock(8).mid(4, 4);
        static const char *const metaChildrenNames[] = {
            "hdlr", "ilst", "mhdr", "ctry", "lang"
        };
        bool metaIsFullAtom = true;
        for(size_t j = 0;
            j < sizeof(metaChildrenNames) / sizeof(metaChildrenNames[0]);
            ++j) {
          if(nextSize == metaChildrenNames[j]) {
            // meta is not a full atom (i.e. not followed by version, flags). It
            // is followed by the size and type of the first child atom.
            metaIsFullAtom = false;
            break;
          }
        }
        // Only skip next four bytes, which contain version and flags, if meta
        // is a full atom.
        file->seek(posAfterMeta + (metaIsFullAtom ? 4 : 0));
      }
      else if(name == "stsd") {
        file->seek(8, File::Current);
      }
      while(file->tell() < offset + length) {
        MP4::Atom *child = new MP4::Atom(file);
        children.append(child);
        if(child->length == 0)
          return;
      }
      return;
    }
  }

  file->seek(offset + length);
}

MP4::Atom::~Atom()
{
}

MP4::Atom *
MP4::Atom::find(const char *name1, const char *name2, const char *name3, const char *name4)
{
  if(name1 == 0) {
    return this;
  }
  for(AtomList::ConstIterator it = children.begin(); it != children.end(); ++it) {
    if((*it)->name == name1) {
      return (*it)->find(name2, name3, name4);
    }
  }
  return 0;
}

MP4::AtomList
MP4::Atom::findall(const char *name, bool recursive)
{
  MP4::AtomList result;
  for(AtomList::ConstIterator it = children.begin(); it != children.end(); ++it) {
    if((*it)->name == name) {
      result.append(*it);
    }
    if(recursive) {
      result.append((*it)->findall(name, recursive));
    }
  }
  return result;
}

bool
MP4::Atom::path(MP4::AtomList &path, const char *name1, const char *name2, const char *name3)
{
  path.append(this);
  if(name1 == 0) {
    return true;
  }
  for(AtomList::ConstIterator it = children.begin(); it != children.end(); ++it) {
    if((*it)->name == name1) {
      return (*it)->path(path, name2, name3);
    }
  }
  return false;
}

MP4::Atoms::Atoms(File *file)
{
  atoms.setAutoDelete(true);

  file->seek(0, File::End);
  long end = file->tell();
  file->seek(0);
  while(file->tell() + 8 <= end) {
    MP4::Atom *atom = new MP4::Atom(file);
    atoms.append(atom);
    if (atom->length == 0)
      break;
  }
}

MP4::Atoms::~Atoms()
{
}

MP4::Atom *
MP4::Atoms::find(const char *name1, const char *name2, const char *name3, const char *name4)
{
  for(AtomList::ConstIterator it = atoms.begin(); it != atoms.end(); ++it) {
    if((*it)->name == name1) {
      return (*it)->find(name2, name3, name4);
    }
  }
  return 0;
}

MP4::AtomList
MP4::Atoms::path(const char *name1, const char *name2, const char *name3, const char *name4)
{
  MP4::AtomList path;
  for(AtomList::ConstIterator it = atoms.begin(); it != atoms.end(); ++it) {
    if((*it)->name == name1) {
      if(!(*it)->path(path, name2, name3, name4)) {
        path.clear();
      }
      return path;
    }
  }
  return path;
}
