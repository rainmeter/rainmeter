/***************************************************************************
    copyright            : (C) 2015 by Tsuda Kageyu
    email                : tsuda.kageyu@gmail.com
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

#ifndef TAGLIB_ASFUTILS_H
#define TAGLIB_ASFUTILS_H

// THIS FILE IS NOT A PART OF THE TAGLIB API

#ifndef DO_NOT_DOCUMENT  // tell Doxygen not to document this header

namespace TagLib
{
  namespace ASF
  {
    namespace
    {

      inline unsigned short readWORD(File *file, bool *ok = 0)
      {
        const ByteVector v = file->readBlock(2);
        if(v.size() != 2) {
          if(ok) *ok = false;
          return 0;
        }
        if(ok) *ok = true;
        return v.toUShort(false);
      }

      inline unsigned int readDWORD(File *file, bool *ok = 0)
      {
        const ByteVector v = file->readBlock(4);
        if(v.size() != 4) {
          if(ok) *ok = false;
          return 0;
        }
        if(ok) *ok = true;
        return v.toUInt(false);
      }

      inline long long readQWORD(File *file, bool *ok = 0)
      {
        const ByteVector v = file->readBlock(8);
        if(v.size() != 8) {
          if(ok) *ok = false;
          return 0;
        }
        if(ok) *ok = true;
        return v.toLongLong(false);
      }

      inline String readString(File *file, int length)
      {
        ByteVector data = file->readBlock(length);
        unsigned int size = data.size();
        while (size >= 2) {
          if(data[size - 1] != '\0' || data[size - 2] != '\0') {
            break;
          }
          size -= 2;
        }
        if(size != data.size()) {
          data.resize(size);
        }
        return String(data, String::UTF16LE);
      }

      inline ByteVector renderString(const String &str, bool includeLength = false)
      {
        ByteVector data = str.data(String::UTF16LE) + ByteVector::fromShort(0, false);
        if(includeLength) {
          data = ByteVector::fromShort(data.size(), false) + data;
        }
        return data;
      }

    }  // namespace
  }  // namespace ASF
}  // namespace TagLib

#endif

#endif
