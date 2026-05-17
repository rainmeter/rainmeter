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

#include <algorithm>
#include <iostream>
#include <limits>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <tstring.h>
#include <tdebug.h>
#include <trefcounter.h>
#include <tutils.h>

#include "tbytevector.h"

// This is a bit ugly to keep writing over and over again.

// A rather obscure feature of the C++ spec that I hadn't thought of that makes
// working with C libs much more efficient.  There's more here:
//
// http://www.informit.com/isapi/product_id~{9C84DAB4-FE6E-49C5-BB0A-FB50331233EA}/content/index.asp

namespace TagLib {

template <class TIterator>
int findChar(
  const TIterator dataBegin, const TIterator dataEnd,
  char c, unsigned int offset, int byteAlign)
{
  const size_t dataSize = dataEnd - dataBegin;
  if(offset + 1 > dataSize)
    return -1;

  // n % 0 is invalid

  if(byteAlign == 0)
    return -1;

  for(TIterator it = dataBegin + offset; it < dataEnd; it += byteAlign) {
    if(*it == c)
      return static_cast<int>(it - dataBegin);
  }

  return -1;
}

template <class TIterator>
int findVector(
  const TIterator dataBegin, const TIterator dataEnd,
  const TIterator patternBegin, const TIterator patternEnd,
  unsigned int offset, int byteAlign)
{
  const size_t dataSize    = dataEnd    - dataBegin;
  const size_t patternSize = patternEnd - patternBegin;
  if(patternSize == 0 || offset + patternSize > dataSize)
    return -1;

  // Special case that pattern contains just single char.

  if(patternSize == 1)
    return findChar(dataBegin, dataEnd, *patternBegin, offset, byteAlign);

  // n % 0 is invalid

  if(byteAlign == 0)
    return -1;

  // We don't use sophisticated algorithms like Knuth-Morris-Pratt here.

  // In the current implementation of TagLib, data and patterns are too small
  // for such algorithms to work effectively.

  for(TIterator it = dataBegin + offset; it < dataEnd - patternSize + 1; it += byteAlign) {

    TIterator itData    = it;
    TIterator itPattern = patternBegin;

    while(*itData == *itPattern) {
      ++itData;
      ++itPattern;

      if(itPattern == patternEnd)
        return static_cast<int>(it - dataBegin);
    }
  }

  return -1;
}

template <class T>
T toNumber(const ByteVector &v, size_t offset, size_t length, bool mostSignificantByteFirst)
{
  if(offset >= v.size()) {
    debug("toNumber<T>() -- No data to convert. Returning 0.");
    return 0;
  }

  length = std::min(length, v.size() - offset);

  T sum = 0;
  for(size_t i = 0; i < length; i++) {
    const size_t shift = (mostSignificantByteFirst ? length - 1 - i : i) * 8;
    sum |= static_cast<T>(static_cast<unsigned char>(v[static_cast<int>(offset + i)])) << shift;
  }

  return sum;
}

template <class T>
T toNumber(const ByteVector &v, size_t offset, bool mostSignificantByteFirst)
{
  const bool isBigEndian = (Utils::systemByteOrder() == Utils::BigEndian);
  const bool swap = (mostSignificantByteFirst != isBigEndian);

  if(offset + sizeof(T) > v.size())
    return toNumber<T>(v, offset, v.size() - offset, mostSignificantByteFirst);

  // Uses memcpy instead of reinterpret_cast to avoid an alignment exception.
  T tmp;
  ::memcpy(&tmp, v.data() + offset, sizeof(T));

  if(swap)
    return Utils::byteSwap(tmp);
  return tmp;
}

template <class T>
ByteVector fromNumber(T value, bool mostSignificantByteFirst)
{
  const bool isBigEndian = (Utils::systemByteOrder() == Utils::BigEndian);
  const bool swap = (mostSignificantByteFirst != isBigEndian);

  if(swap)
    value = Utils::byteSwap(value);

  return ByteVector(reinterpret_cast<const char *>(&value), sizeof(T));
}

template <typename TFloat, typename TInt, Utils::ByteOrder ENDIAN>
TFloat toFloat(const ByteVector &v, size_t offset)
{
  if(offset > v.size() - sizeof(TInt)) {
    debug("toFloat() - offset is out of range. Returning 0.");
    return 0.0;
  }

  union {
    TInt   i;
    TFloat f;
  } tmp;
  ::memcpy(&tmp, v.data() + offset, sizeof(TInt));

  if(ENDIAN != Utils::systemByteOrder())
    tmp.i = Utils::byteSwap(tmp.i);

  return tmp.f;
}

template <typename TFloat, typename TInt, Utils::ByteOrder ENDIAN>
ByteVector fromFloat(TFloat value)
{
  union {
    TInt   i;
    TFloat f;
  } tmp;
  tmp.f = value;

  if(ENDIAN != Utils::systemByteOrder())
    tmp.i = Utils::byteSwap(tmp.i);

  return ByteVector(reinterpret_cast<char *>(&tmp), sizeof(TInt));
}

template <Utils::ByteOrder ENDIAN>
long double toFloat80(const ByteVector &v, size_t offset)
{
  using std::swap;

  if(offset > v.size() - 10) {
    debug("toFloat80() - offset is out of range. Returning 0.");
    return 0.0;
  }

  unsigned char bytes[10];
  ::memcpy(bytes, v.data() + offset, 10);

  if(ENDIAN == Utils::LittleEndian) {
    swap(bytes[0], bytes[9]);
    swap(bytes[1], bytes[8]);
    swap(bytes[2], bytes[7]);
    swap(bytes[3], bytes[6]);
    swap(bytes[4], bytes[5]);
  }

  // 1-bit sign
  const bool negative = ((bytes[0] & 0x80) != 0);

  // 15-bit exponent
  const int exponent = ((bytes[0] & 0x7F) << 8) | bytes[1];

  // 64-bit fraction. Leading 1 is explicit.
  const unsigned long long fraction
    = (static_cast<unsigned long long>(bytes[2]) << 56)
    | (static_cast<unsigned long long>(bytes[3]) << 48)
    | (static_cast<unsigned long long>(bytes[4]) << 40)
    | (static_cast<unsigned long long>(bytes[5]) << 32)
    | (static_cast<unsigned long long>(bytes[6]) << 24)
    | (static_cast<unsigned long long>(bytes[7]) << 16)
    | (static_cast<unsigned long long>(bytes[8]) <<  8)
    | (static_cast<unsigned long long>(bytes[9]));

  long double val;
  if(exponent == 0 && fraction == 0)
    val = 0;
  else {
    if(exponent == 0x7FFF) {
      debug("toFloat80() - can't handle the infinity or NaN. Returning 0.");
      return 0.0;
    }
    val = ::ldexp(static_cast<long double>(fraction), exponent - 16383 - 63);
  }

  if(negative)
    return -val;
  return val;
}

class ByteVector::ByteVectorPrivate
{
public:
  ByteVectorPrivate(unsigned int l, char c) :
    counter(new RefCounter()),
    data(new std::vector<char>(l, c)),
    offset(0),
    length(l) {}

  ByteVectorPrivate(const char *s, unsigned int l) :
    counter(new RefCounter()),
    data(new std::vector<char>(s, s + l)),
    offset(0),
    length(l) {}

  ByteVectorPrivate(const ByteVectorPrivate &d, unsigned int o, unsigned int l) :
    counter(d.counter),
    data(d.data),
    offset(d.offset + o),
    length(l)
  {
    counter->ref();
  }

  ~ByteVectorPrivate()
  {
    if(counter->deref()) {
      delete counter;
      delete data;
    }
  }

  RefCounter        *counter;
  std::vector<char> *data;
  unsigned int       offset;
  unsigned int       length;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

ByteVector ByteVector::null;

ByteVector ByteVector::fromCString(const char *s, unsigned int length)
{
  if(length == 0xffffffff)
    return ByteVector(s, static_cast<unsigned int>(::strlen(s)));
  return ByteVector(s, length);
}

ByteVector ByteVector::fromUInt(unsigned int value, bool mostSignificantByteFirst)
{
  return fromNumber<unsigned int>(value, mostSignificantByteFirst);
}

ByteVector ByteVector::fromShort(short value, bool mostSignificantByteFirst)
{
  return fromNumber<unsigned short>(value, mostSignificantByteFirst);
}

ByteVector ByteVector::fromLongLong(long long value, bool mostSignificantByteFirst)
{
  return fromNumber<unsigned long long>(value, mostSignificantByteFirst);
}

ByteVector ByteVector::fromFloat32LE(float value)
{
  return fromFloat<float, unsigned int, Utils::LittleEndian>(value);
}

ByteVector ByteVector::fromFloat32BE(float value)
{
  return fromFloat<float, unsigned int, Utils::BigEndian>(value);
}

ByteVector ByteVector::fromFloat64LE(double value)
{
  return fromFloat<double, unsigned long long, Utils::LittleEndian>(value);
}

ByteVector ByteVector::fromFloat64BE(double value)
{
  return fromFloat<double, unsigned long long, Utils::BigEndian>(value);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

ByteVector::ByteVector() :
  d(new ByteVectorPrivate(0, '\0'))
{
}

ByteVector::ByteVector(unsigned int size, char value) :
  d(new ByteVectorPrivate(size, value))
{
}

ByteVector::ByteVector(const ByteVector &v) :
  d(new ByteVectorPrivate(*v.d, 0, v.d->length))
{
}

ByteVector::ByteVector(const ByteVector &v, unsigned int offset, unsigned int length) :
  d(new ByteVectorPrivate(*v.d, offset, length))
{
}

ByteVector::ByteVector(char c) :
  d(new ByteVectorPrivate(1, c))
{
}

ByteVector::ByteVector(const char *data, unsigned int length) :
  d(new ByteVectorPrivate(data, length))
{
}

ByteVector::ByteVector(const char *data) :
  d(new ByteVectorPrivate(data, static_cast<unsigned int>(::strlen(data))))
{
}

ByteVector::~ByteVector()
{
  delete d;
}

ByteVector &ByteVector::setData(const char *s, unsigned int length)
{
  ByteVector(s, length).swap(*this);
  return *this;
}

ByteVector &ByteVector::setData(const char *data)
{
  ByteVector(data).swap(*this);
  return *this;
}

char *ByteVector::data()
{
  detach();
  return (size() > 0) ? (&(*d->data)[d->offset]) : 0;
}

const char *ByteVector::data() const
{
  return (size() > 0) ? (&(*d->data)[d->offset]) : 0;
}

ByteVector ByteVector::mid(unsigned int index, unsigned int length) const
{
  index  = std::min(index, size());
  length = std::min(length, size() - index);

  return ByteVector(*this, index, length);
}

char ByteVector::at(unsigned int index) const
{
  return (index < size()) ? (*d->data)[d->offset + index] : 0;
}

int ByteVector::find(const ByteVector &pattern, unsigned int offset, int byteAlign) const
{
  return findVector<ConstIterator>(
    begin(), end(), pattern.begin(), pattern.end(), offset, byteAlign);
}

int ByteVector::find(char c, unsigned int offset, int byteAlign) const
{
  return findChar<ConstIterator>(begin(), end(), c, offset, byteAlign);
}

int ByteVector::rfind(const ByteVector &pattern, unsigned int offset, int byteAlign) const
{
  if(offset > 0) {
    offset = size() - offset - pattern.size();
    if(offset >= size())
      offset = 0;
  }

  const int pos = findVector<ConstReverseIterator>(
    rbegin(), rend(), pattern.rbegin(), pattern.rend(), offset, byteAlign);

  if(pos == -1)
    return -1;
  return size() - pos - pattern.size();
}

bool ByteVector::containsAt(const ByteVector &pattern, unsigned int offset, unsigned int patternOffset, unsigned int patternLength) const
{
  if(pattern.size() < patternLength)
    patternLength = pattern.size();

  // do some sanity checking -- all of these things are needed for the search to be valid
  const unsigned int compareLength = patternLength - patternOffset;
  if(offset + compareLength > size() || patternOffset >= pattern.size() || patternLength == 0)
    return false;

  return (::memcmp(data() + offset, pattern.data() + patternOffset, compareLength) == 0);
}

bool ByteVector::startsWith(const ByteVector &pattern) const
{
  return containsAt(pattern, 0);
}

bool ByteVector::endsWith(const ByteVector &pattern) const
{
  return containsAt(pattern, size() - pattern.size());
}

ByteVector &ByteVector::replace(char oldByte, char newByte)
{
  detach();

  for(ByteVector::Iterator it = begin(); it != end(); ++it) {
    if(*it == oldByte)
      *it = newByte;
  }

  return *this;
}

ByteVector &ByteVector::replace(const ByteVector &pattern, const ByteVector &with)
{
  if(pattern.size() == 1 && with.size() == 1)
    return replace(pattern[0], with[0]);

  // Check if there is at least one occurrence of the pattern.

  int offset = find(pattern, 0);
  if(offset == -1)
    return *this;

  if(pattern.size() == with.size()) {

    // We think this case might be common enough to optimize it.

    detach();
    do
    {
      ::memcpy(data() + offset, with.data(), with.size());
      offset = find(pattern, offset + pattern.size());
    } while(offset != -1);
  }
  else {

    // Loop once to calculate the result size.

    unsigned int dstSize = size();
    do
    {
      dstSize += with.size() - pattern.size();
      offset = find(pattern, offset + pattern.size());
    } while(offset != -1);

    // Loop again to copy modified data to the new vector.

    ByteVector dst(dstSize);
    int dstOffset = 0;

    offset = 0;
    while(true) {
      const int next = find(pattern, offset);
      if(next == -1) {
        ::memcpy(dst.data() + dstOffset, data() + offset, size() - offset);
        break;
      }

      ::memcpy(dst.data() + dstOffset, data() + offset, next - offset);
      dstOffset += next - offset;

      ::memcpy(dst.data() + dstOffset, with.data(), with.size());
      dstOffset += with.size();

      offset = next + pattern.size();
    }

    swap(dst);
  }

  return *this;
}

int ByteVector::endsWithPartialMatch(const ByteVector &pattern) const
{
  if(pattern.size() > size())
    return -1;

  const int startIndex = size() - pattern.size();

  // try to match the last n-1 bytes from the vector (where n is the pattern
  // size) -- continue trying to match n-2, n-3...1 bytes

  for(unsigned int i = 1; i < pattern.size(); i++) {
    if(containsAt(pattern, startIndex + i, 0, pattern.size() - i))
      return startIndex + i;
  }

  return -1;
}

ByteVector &ByteVector::append(const ByteVector &v)
{
  if(v.isEmpty())
    return *this;

  detach();

  const unsigned int originalSize = size();
  const unsigned int appendSize = v.size();

  resize(originalSize + appendSize);
  ::memcpy(data() + originalSize, v.data(), appendSize);

  return *this;
}

ByteVector &ByteVector::append(char c)
{
  resize(size() + 1, c);
  return *this;
}

ByteVector &ByteVector::clear()
{
  ByteVector().swap(*this);
  return *this;
}

unsigned int ByteVector::size() const
{
  return d->length;
}

ByteVector &ByteVector::resize(unsigned int size, char padding)
{
  if(size != d->length) {
    detach();

    // Remove the excessive length of the internal buffer first to pad correctly.
    // This doesn't reallocate the buffer, since std::vector::resize() doesn't
    // reallocate the buffer when shrinking.

    d->data->resize(d->offset + d->length);
    d->data->resize(d->offset + size, padding);

    d->length = size;
  }

  return *this;
}

ByteVector::Iterator ByteVector::begin()
{
  detach();
  return d->data->begin() + d->offset;
}

ByteVector::ConstIterator ByteVector::begin() const
{
  return d->data->begin() + d->offset;
}

ByteVector::Iterator ByteVector::end()
{
  detach();
  return d->data->begin() + d->offset + d->length;
}

ByteVector::ConstIterator ByteVector::end() const
{
  return d->data->begin() + d->offset + d->length;
}

ByteVector::ReverseIterator ByteVector::rbegin()
{
  detach();
  return d->data->rbegin() + (d->data->size() - (d->offset + d->length));
}

ByteVector::ConstReverseIterator ByteVector::rbegin() const
{
  // Workaround for the Solaris Studio 12.4 compiler.
  // We need a const reference to the data vector so we can ensure the const version of rbegin() is called.
  const std::vector<char> &v = *d->data;
  return v.rbegin() + (v.size() - (d->offset + d->length));
}

ByteVector::ReverseIterator ByteVector::rend()
{
  detach();
  return d->data->rbegin() + (d->data->size() - d->offset);
}

ByteVector::ConstReverseIterator ByteVector::rend() const
{
  // Workaround for the Solaris Studio 12.4 compiler.
  // We need a const reference to the data vector so we can ensure the const version of rbegin() is called.
  const std::vector<char> &v = *d->data;
  return v.rbegin() + (v.size() - d->offset);
}

bool ByteVector::isNull() const
{
  return (d == null.d);
}

bool ByteVector::isEmpty() const
{
  return (d->length == 0);
}

unsigned int ByteVector::checksum() const
{
  static const unsigned int crcTable[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
  };

  unsigned int sum = 0;
  for(ByteVector::ConstIterator it = begin(); it != end(); ++it)
    sum = (sum << 8) ^ crcTable[((sum >> 24) & 0xff) ^ static_cast<unsigned char>(*it)];
  return sum;
}

unsigned int ByteVector::toUInt(bool mostSignificantByteFirst) const
{
  return toNumber<unsigned int>(*this, 0, mostSignificantByteFirst);
}

unsigned int ByteVector::toUInt(unsigned int offset, bool mostSignificantByteFirst) const
{
  return toNumber<unsigned int>(*this, offset, mostSignificantByteFirst);
}

unsigned int ByteVector::toUInt(unsigned int offset, unsigned int length, bool mostSignificantByteFirst) const
{
  return toNumber<unsigned int>(*this, offset, length, mostSignificantByteFirst);
}

short ByteVector::toShort(bool mostSignificantByteFirst) const
{
  return toNumber<unsigned short>(*this, 0, mostSignificantByteFirst);
}

short ByteVector::toShort(unsigned int offset, bool mostSignificantByteFirst) const
{
  return toNumber<unsigned short>(*this, offset, mostSignificantByteFirst);
}

unsigned short ByteVector::toUShort(bool mostSignificantByteFirst) const
{
  return toNumber<unsigned short>(*this, 0, mostSignificantByteFirst);
}

unsigned short ByteVector::toUShort(unsigned int offset, bool mostSignificantByteFirst) const
{
  return toNumber<unsigned short>(*this, offset, mostSignificantByteFirst);
}

long long ByteVector::toLongLong(bool mostSignificantByteFirst) const
{
  return toNumber<unsigned long long>(*this, 0, mostSignificantByteFirst);
}

long long ByteVector::toLongLong(unsigned int offset, bool mostSignificantByteFirst) const
{
  return toNumber<unsigned long long>(*this, offset, mostSignificantByteFirst);
}

float ByteVector::toFloat32LE(size_t offset) const
{
  return toFloat<float, unsigned int, Utils::LittleEndian>(*this, offset);
}

float ByteVector::toFloat32BE(size_t offset) const
{
  return toFloat<float, unsigned int, Utils::BigEndian>(*this, offset);
}

double ByteVector::toFloat64LE(size_t offset) const
{
  return toFloat<double, unsigned long long, Utils::LittleEndian>(*this, offset);
}

double ByteVector::toFloat64BE(size_t offset) const
{
  return toFloat<double, unsigned long long, Utils::BigEndian>(*this, offset);
}

long double ByteVector::toFloat80LE(size_t offset) const
{
  return toFloat80<Utils::LittleEndian>(*this, offset);
}

long double ByteVector::toFloat80BE(size_t offset) const
{
  return toFloat80<Utils::BigEndian>(*this, offset);
}

const char &ByteVector::operator[](int index) const
{
  return (*d->data)[d->offset + index];
}

char &ByteVector::operator[](int index)
{
  detach();
  return (*d->data)[d->offset + index];
}

bool ByteVector::operator==(const ByteVector &v) const
{
  if(size() != v.size())
    return false;

  return (::memcmp(data(), v.data(), size()) == 0);
}

bool ByteVector::operator!=(const ByteVector &v) const
{
  return !(*this == v);
}

bool ByteVector::operator==(const char *s) const
{
  if(size() != ::strlen(s))
    return false;

  return (::memcmp(data(), s, size()) == 0);
}

bool ByteVector::operator!=(const char *s) const
{
  return !(*this == s);
}

bool ByteVector::operator<(const ByteVector &v) const
{
  const int result = ::memcmp(data(), v.data(), std::min(size(), v.size()));
  if(result != 0)
    return result < 0;
  return size() < v.size();
}

bool ByteVector::operator>(const ByteVector &v) const
{
  return (v < *this);
}

ByteVector ByteVector::operator+(const ByteVector &v) const
{
  ByteVector sum(*this);
  sum.append(v);
  return sum;
}

ByteVector &ByteVector::operator=(const ByteVector &v)
{
  ByteVector(v).swap(*this);
  return *this;
}

ByteVector &ByteVector::operator=(char c)
{
  ByteVector(c).swap(*this);
  return *this;
}

ByteVector &ByteVector::operator=(const char *data)
{
  ByteVector(data).swap(*this);
  return *this;
}

void ByteVector::swap(ByteVector &v)
{
  using std::swap;

  swap(d, v.d);
}

ByteVector ByteVector::toHex() const
{
  static const char hexTable[17] = "0123456789abcdef";

  ByteVector encoded(size() * 2);
  char *p = encoded.data();

  for(unsigned int i = 0; i < size(); i++) {
    unsigned char c = data()[i];
    *p++ = hexTable[(c >> 4) & 0x0F];
    *p++ = hexTable[(c     ) & 0x0F];
  }

  return encoded;
}

ByteVector ByteVector::fromBase64(const ByteVector & input)
{
  static const unsigned char base64[256] = {
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x3e,0x80,0x80,0x80,0x3f,
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x80,0x80,0x80,0x80,0x80,
    0x80,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
  };

  unsigned int len = input.size();

  ByteVector output(len);

  const unsigned char * src = reinterpret_cast<const unsigned char*>(input.data());
  unsigned char *       dst = reinterpret_cast<unsigned char*>(output.data());

  while(4 <= len) {

    // Check invalid character
    if(base64[src[0]] == 0x80)
      break;

    // Check invalid character
    if(base64[src[1]] == 0x80)
      break;

    // Decode first byte
    *dst++ = ((base64[src[0]] << 2) & 0xfc) | ((base64[src[1]] >> 4) & 0x03);

    if(src[2] != '=') {

      // Check invalid character
      if(base64[src[2]] == 0x80)
        break;

      // Decode second byte
      *dst++ = ((base64[src[1]] & 0x0f) << 4) | ((base64[src[2]] >> 2) & 0x0f);

      if(src[3] != '=') {

        // Check invalid character
        if(base64[src[3]] == 0x80)
          break;

        // Decode third byte
        *dst++ = ((base64[src[2]] & 0x03) << 6) | (base64[src[3]] & 0x3f);
      }
      else {
        // assume end of data
        len -= 4;
        break;
      }
    }
    else {
      // assume end of data
      len -= 4;
      break;
    }
    src += 4;
    len -= 4;
  }

  // Only return output if we processed all bytes
  if(len == 0) {
    output.resize(static_cast<unsigned int>(dst - reinterpret_cast<unsigned char*>(output.data())));
    return output;
  }
  return ByteVector();
}

ByteVector ByteVector::toBase64() const
{
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if(!isEmpty()) {
    unsigned int len = size();
    ByteVector output(4 * ((len - 1) / 3 + 1)); // note roundup

    const char * src = data();
    char * dst = output.data();
    while(3 <= len) {
      *dst++ = alphabet[(src[0] >> 2) & 0x3f];
      *dst++ = alphabet[((src[0] & 0x03) << 4) | ((src[1] >> 4) & 0x0f)];
      *dst++ = alphabet[((src[1] & 0x0f) << 2) | ((src[2] >> 6) & 0x03)];
      *dst++ = alphabet[src[2] & 0x3f];
      src += 3;
      len -= 3;
    }
    if(len) {
      *dst++ = alphabet[(src[0] >> 2) & 0x3f];
      if(len>1) {
        *dst++ = alphabet[((src[0] & 0x03) << 4) | ((src[1] >> 4) & 0x0f)];
        *dst++ = alphabet[((src[1] & 0x0f) << 2)];
      }
      else {
        *dst++ = alphabet[(src[0] & 0x03) << 4];
        *dst++ = '=';
      }
    *dst++ = '=';
    }
    return output;
  }
  return ByteVector();
}


////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void ByteVector::detach()
{
  if(d->counter->count() > 1) {
    if(!isEmpty())
      ByteVector(&d->data->front() + d->offset, d->length).swap(*this);
    else
      ByteVector().swap(*this);
  }
}
}  // namespace TagLib

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

std::ostream &operator<<(std::ostream &s, const TagLib::ByteVector &v)
{
  for(unsigned int i = 0; i < v.size(); i++)
    s << v[i];
  return s;
}
