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

#include <iostream>

#include "id3v2synchdata.h"

using namespace TagLib;
using namespace ID3v2;

unsigned int SynchData::toUInt(const ByteVector &data)
{
  unsigned int sum = 0;
  bool notSynchSafe = false;
  int last = data.size() > 4 ? 3 : data.size() - 1;

  for(int i = 0; i <= last; i++) {
    if(data[i] & 0x80) {
      notSynchSafe = true;
      break;
    }

    sum |= (data[i] & 0x7f) << ((last - i) * 7);
  }

  if(notSynchSafe) {
    // Invalid data; assume this was created by some buggy software that just
    // put normal integers here rather than syncsafe ones, and try it that
    // way.
    if(data.size() >= 4) {
      sum = data.toUInt(0, true);
    }
    else {
      ByteVector tmp(data);
      tmp.resize(4);
      sum = tmp.toUInt(0, true);
    }
  }

  return sum;
}

ByteVector SynchData::fromUInt(unsigned int value)
{
  ByteVector v(4, 0);

  for(int i = 0; i < 4; i++)
    v[i] = static_cast<unsigned char>(value >> ((3 - i) * 7) & 0x7f);

  return v;
}

ByteVector SynchData::decode(const ByteVector &data)
{
  if (data.size() == 0) {
    return ByteVector();
  }

  // We have this optimized method instead of using ByteVector::replace(),
  // since it makes a great difference when decoding huge unsynchronized frames.

  ByteVector result(data.size());

  ByteVector::ConstIterator src = data.begin();
  ByteVector::Iterator dst = result.begin();

  while(src < data.end() - 1) {
    *dst++ = *src++;

    if(*(src - 1) == '\xff' && *src == '\x00')
      src++;
  }

  if(src < data.end())
    *dst++ = *src++;

  result.resize(static_cast<unsigned int>(dst - result.begin()));

  return result;
}
