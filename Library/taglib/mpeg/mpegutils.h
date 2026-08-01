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

#ifndef TAGLIB_MPEGUTILS_H
#define TAGLIB_MPEGUTILS_H

// THIS FILE IS NOT A PART OF THE TAGLIB API

#ifndef DO_NOT_DOCUMENT  // tell Doxygen not to document this header

namespace TagLib
{
  namespace MPEG
  {
    namespace
    {

      /*!
       * MPEG frames can be recognized by the bit pattern 11111111 111, so the
       * first byte is easy to check for, however checking to see if the second byte
       * starts with \e 111 is a bit more tricky, hence these functions.
       *
       * \note This does not check the length of the vector, since this is an
       * internal utility function.
       */
      inline bool isFrameSync(const ByteVector &bytes, unsigned int offset = 0)
      {
        // 0xFF in the second byte is possible in theory, but it's very unlikely.

        const unsigned char b1 = bytes[offset + 0];
        const unsigned char b2 = bytes[offset + 1];
        return (b1 == 0xFF && b2 != 0xFF && (b2 & 0xE0) == 0xE0);
      }

    }  // namespace
  }  // namespace MPEG
}  // namespace TagLib

#endif

#endif
