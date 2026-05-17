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

#include <tstring.h>
#include <tdebug.h>
#include <bitset>
#include <math.h>

#include "mpcproperties.h"
#include "mpcfile.h"

using namespace TagLib;

class MPC::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
    version(0),
    length(0),
    bitrate(0),
    sampleRate(0),
    channels(0),
    totalFrames(0),
    sampleFrames(0),
    trackGain(0),
    trackPeak(0),
    albumGain(0),
    albumPeak(0) {}

  int version;
  int length;
  int bitrate;
  int sampleRate;
  int channels;
  unsigned int totalFrames;
  unsigned int sampleFrames;
  int trackGain;
  int trackPeak;
  int albumGain;
  int albumPeak;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MPC::Properties::Properties(const ByteVector &data, long streamLength, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  readSV7(data, streamLength);
}

MPC::Properties::Properties(File *file, long streamLength, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  ByteVector magic = file->readBlock(4);
  if(magic == "MPCK") {
    // Musepack version 8
    readSV8(file, streamLength);
  }
  else {
    // Musepack version 7 or older, fixed size header
    readSV7(magic + file->readBlock(MPC::HeaderSize - 4), streamLength);
  }
}

MPC::Properties::~Properties()
{
  delete d;
}

int MPC::Properties::length() const
{
  return lengthInSeconds();
}

int MPC::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int MPC::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int MPC::Properties::bitrate() const
{
  return d->bitrate;
}

int MPC::Properties::sampleRate() const
{
  return d->sampleRate;
}

int MPC::Properties::channels() const
{
  return d->channels;
}

int MPC::Properties::mpcVersion() const
{
  return d->version;
}

unsigned int MPC::Properties::totalFrames() const
{
  return d->totalFrames;
}

unsigned int MPC::Properties::sampleFrames() const
{
  return d->sampleFrames;
}

int MPC::Properties::trackGain() const
{
  return d->trackGain;
}

int MPC::Properties::trackPeak() const
{
  return d->trackPeak;
}

int MPC::Properties::albumGain() const
{
  return d->albumGain;
}

int MPC::Properties::albumPeak() const
{
  return d->albumPeak;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

namespace
{
  unsigned long readSize(File *file, unsigned int &sizeLength, bool &eof)
  {
    sizeLength = 0;
    eof = false;

    unsigned char tmp;
    unsigned long size = 0;

    do {
      const ByteVector b = file->readBlock(1);
      if(b.isEmpty()) {
        eof = true;
        break;
      }

      tmp = b[0];
      size = (size << 7) | (tmp & 0x7F);
      sizeLength++;
    } while((tmp & 0x80));
    return size;
  }

  unsigned long readSize(const ByteVector &data, unsigned int &pos)
  {
    unsigned char tmp;
    unsigned long size = 0;

    do {
      tmp = data[pos++];
      size = (size << 7) | (tmp & 0x7F);
    } while((tmp & 0x80) && (pos < data.size()));
    return size;
  }

  // This array looks weird, but the same as original MusePack code found at:
  // https://www.musepack.net/index.php?pg=src
  const unsigned short sftable [8] = { 44100, 48000, 37800, 32000, 0, 0, 0, 0 };
}  // namespace

void MPC::Properties::readSV8(File *file, long streamLength)
{
  bool readSH = false, readRG = false;

  while(!readSH && !readRG) {
    const ByteVector packetType = file->readBlock(2);

    unsigned int packetSizeLength;
    bool eof;
    const unsigned long packetSize = readSize(file, packetSizeLength, eof);
    if(eof) {
      debug("MPC::Properties::readSV8() - Reached to EOF.");
      break;
    }

    const unsigned long dataSize = packetSize - 2 - packetSizeLength;

    const ByteVector data = file->readBlock(dataSize);
    if(data.size() != dataSize) {
      debug("MPC::Properties::readSV8() - dataSize doesn't match the actual data size.");
      break;
    }

    if(packetType == "SH") {
      // Stream Header
      // http://trac.musepack.net/wiki/SV8Specification#StreamHeaderPacket

      if(dataSize <= 5) {
        debug("MPC::Properties::readSV8() - \"SH\" packet is too short to parse.");
        break;
      }

      readSH = true;

      unsigned int pos = 4;
      d->version = data[pos];
      pos += 1;
      d->sampleFrames = readSize(data, pos);
      if(pos > dataSize - 3) {
        debug("MPC::Properties::readSV8() - \"SH\" packet is corrupt.");
        break;
      }

      const unsigned long begSilence = readSize(data, pos);
      if(pos > dataSize - 2) {
        debug("MPC::Properties::readSV8() - \"SH\" packet is corrupt.");
        break;
      }

      const unsigned short flags = data.toUShort(pos, true);
      pos += 2;

      d->sampleRate = sftable[(flags >> 13) & 0x07];
      d->channels   = ((flags >> 4) & 0x0F) + 1;

      const unsigned int frameCount = d->sampleFrames - begSilence;
      if(frameCount > 0 && d->sampleRate > 0) {
        const double length = frameCount * 1000.0 / d->sampleRate;
        d->length  = static_cast<int>(length + 0.5);
        d->bitrate = static_cast<int>(streamLength * 8.0 / length + 0.5);
      }
    }
    else if (packetType == "RG") {
      // Replay Gain
      // http://trac.musepack.net/wiki/SV8Specification#ReplaygainPacket

      if(dataSize <= 9) {
        debug("MPC::Properties::readSV8() - \"RG\" packet is too short to parse.");
        break;
      }

      readRG = true;

      const int replayGainVersion = data[0];
      if(replayGainVersion == 1) {
        d->trackGain = data.toShort(1, true);
        d->trackPeak = data.toShort(3, true);
        d->albumGain = data.toShort(5, true);
        d->albumPeak = data.toShort(7, true);
      }
    }

    else if(packetType == "SE") {
      break;
    }

    else {
      file->seek(dataSize, File::Current);
    }
  }
}

void MPC::Properties::readSV7(const ByteVector &data, long streamLength)
{
  if(data.startsWith("MP+")) {
    if(data.size() < 4)
      return;

    d->version = data[3] & 15;
    if(d->version < 7)
      return;

    d->totalFrames = data.toUInt(4, false);

    const unsigned int flags = data.toUInt(8, false);
    d->sampleRate = sftable[(flags >> 16) & 0x03];
    d->channels   = 2;

    const unsigned int gapless = data.toUInt(5, false);

    d->trackGain = data.toShort(14, false);
    d->trackPeak = data.toUShort(12, false);
    d->albumGain = data.toShort(18, false);
    d->albumPeak = data.toUShort(16, false);

    // convert gain info
    if(d->trackGain != 0) {
      int tmp = static_cast<int>((64.82 - static_cast<short>(d->trackGain) / 100.) * 256. + .5);
      if(tmp >= (1 << 16) || tmp < 0) tmp = 0;
      d->trackGain = tmp;
    }

    if(d->albumGain != 0) {
      int tmp = static_cast<int>((64.82 - d->albumGain / 100.) * 256. + .5);
      if(tmp >= (1 << 16) || tmp < 0) tmp = 0;
      d->albumGain = tmp;
    }

    if (d->trackPeak != 0)
      d->trackPeak = static_cast<int>(log10(static_cast<double>(d->trackPeak)) * 20 * 256 + .5);

    if (d->albumPeak != 0)
      d->albumPeak = static_cast<int>(log10(static_cast<double>(d->albumPeak)) * 20 * 256 + .5);

    bool trueGapless = (gapless >> 31) & 0x0001;
    if(trueGapless) {
      unsigned int lastFrameSamples = (gapless >> 20) & 0x07FF;
      d->sampleFrames = d->totalFrames * 1152 - lastFrameSamples;
    }
    else
      d->sampleFrames = d->totalFrames * 1152 - 576;
  }
  else {
    const unsigned int headerData = data.toUInt(0, false);

    d->bitrate    = (headerData >> 23) & 0x01ff;
    d->version    = (headerData >> 11) & 0x03ff;
    d->sampleRate = 44100;
    d->channels   = 2;

    if(d->version >= 5)
      d->totalFrames = data.toUInt(4, false);
    else
      d->totalFrames = data.toUShort(6, false);

    d->sampleFrames = d->totalFrames * 1152 - 576;
  }

  if(d->sampleFrames > 0 && d->sampleRate > 0) {
    const double length = d->sampleFrames * 1000.0 / d->sampleRate;
    d->length = static_cast<int>(length + 0.5);

    if(d->bitrate == 0)
      d->bitrate = static_cast<int>(streamLength * 8.0 / length + 0.5);
  }
}
