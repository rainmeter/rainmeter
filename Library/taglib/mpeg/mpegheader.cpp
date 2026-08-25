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

#include <tbytevector.h>
#include <tstring.h>
#include <tfile.h>
#include <tdebug.h>
#include <trefcounter.h>

#include "mpegheader.h"
#include "mpegutils.h"

using namespace TagLib;

class MPEG::Header::HeaderPrivate : public RefCounter
{
public:
  HeaderPrivate() :
    isValid(false),
    version(Version1),
    layer(0),
    protectionEnabled(false),
    bitrate(0),
    sampleRate(0),
    isPadded(false),
    channelMode(Stereo),
    isCopyrighted(false),
    isOriginal(false),
    frameLength(0),
    samplesPerFrame(0) {}

  bool isValid;
  Version version;
  int layer;
  bool protectionEnabled;
  int bitrate;
  int sampleRate;
  bool isPadded;
  ChannelMode channelMode;
  bool isCopyrighted;
  bool isOriginal;
  int frameLength;
  int samplesPerFrame;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MPEG::Header::Header(const ByteVector &data) :
  d(new HeaderPrivate())
{
  debug("MPEG::Header::Header() - This constructor is no longer used.");
}

MPEG::Header::Header(File *file, long offset, bool checkLength) :
  d(new HeaderPrivate())
{
  parse(file, offset, checkLength);
}

MPEG::Header::Header(const Header &h) :
  d(h.d)
{
  d->ref();
}

MPEG::Header::~Header()
{
  if(d->deref())
    delete d;
}

bool MPEG::Header::isValid() const
{
  return d->isValid;
}

MPEG::Header::Version MPEG::Header::version() const
{
  return d->version;
}

int MPEG::Header::layer() const
{
  return d->layer;
}

bool MPEG::Header::protectionEnabled() const
{
  return d->protectionEnabled;
}

int MPEG::Header::bitrate() const
{
  return d->bitrate;
}

int MPEG::Header::sampleRate() const
{
  return d->sampleRate;
}

bool MPEG::Header::isPadded() const
{
  return d->isPadded;
}

MPEG::Header::ChannelMode MPEG::Header::channelMode() const
{
  return d->channelMode;
}

bool MPEG::Header::isCopyrighted() const
{
  return d->isCopyrighted;
}

bool MPEG::Header::isOriginal() const
{
  return d->isOriginal;
}

int MPEG::Header::frameLength() const
{
  return d->frameLength;
}

int MPEG::Header::samplesPerFrame() const
{
  return d->samplesPerFrame;
}

MPEG::Header &MPEG::Header::operator=(const Header &h)
{
  if(&h == this)
    return *this;

  if(d->deref())
    delete d;

  d = h.d;
  d->ref();
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void MPEG::Header::parse(File *file, long offset, bool checkLength)
{
  file->seek(offset);
  const ByteVector data = file->readBlock(4);

  if(data.size() < 4) {
    debug("MPEG::Header::parse() -- data is too short for an MPEG frame header.");
    return;
  }

  // Check for the MPEG synch bytes.

  if(!isFrameSync(data)) {
    debug("MPEG::Header::parse() -- MPEG header did not match MPEG synch.");
    return;
  }

  // Set the MPEG version

  const int versionBits = (static_cast<unsigned char>(data[1]) >> 3) & 0x03;

  if(versionBits == 0)
    d->version = Version2_5;
  else if(versionBits == 2)
    d->version = Version2;
  else if(versionBits == 3)
    d->version = Version1;
  else
    return;

  // Set the MPEG layer

  const int layerBits = (static_cast<unsigned char>(data[1]) >> 1) & 0x03;

  if(layerBits == 1)
    d->layer = 3;
  else if(layerBits == 2)
    d->layer = 2;
  else if(layerBits == 3)
    d->layer = 1;
  else
    return;

  d->protectionEnabled = (static_cast<unsigned char>(data[1] & 0x01) == 0);

  // Set the bitrate

  static const int bitrates[2][3][16] = {
    { // Version 1
      { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, // layer 1
      { 0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0 }, // layer 2
      { 0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0 }  // layer 3
    },
    { // Version 2 or 2.5
      { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }, // layer 1
      { 0, 8,  16, 24, 32, 40, 48, 56,  64,  80,  96,  112, 128, 144, 160, 0 }, // layer 2
      { 0, 8,  16, 24, 32, 40, 48, 56,  64,  80,  96,  112, 128, 144, 160, 0 }  // layer 3
    }
  };

  const int versionIndex = (d->version == Version1) ? 0 : 1;
  const int layerIndex   = (d->layer > 0) ? d->layer - 1 : 0;

  // The bitrate index is encoded as the first 4 bits of the 3rd byte,
  // i.e. 1111xxxx

  const int bitrateIndex = (static_cast<unsigned char>(data[2]) >> 4) & 0x0F;

  d->bitrate = bitrates[versionIndex][layerIndex][bitrateIndex];

  if(d->bitrate == 0)
    return;

  // Set the sample rate

  static const int sampleRates[3][4] = {
    { 44100, 48000, 32000, 0 }, // Version 1
    { 22050, 24000, 16000, 0 }, // Version 2
    { 11025, 12000, 8000,  0 }  // Version 2.5
  };

  // The sample rate index is encoded as two bits in the 3nd byte, i.e. xxxx11xx

  const int samplerateIndex = (static_cast<unsigned char>(data[2]) >> 2) & 0x03;

  d->sampleRate = sampleRates[d->version][samplerateIndex];

  if(d->sampleRate == 0) {
    return;
  }

  // The channel mode is encoded as a 2 bit value at the end of the 3nd byte,
  // i.e. xxxxxx11

  d->channelMode = static_cast<ChannelMode>((static_cast<unsigned char>(data[3]) >> 6) & 0x03);

  // TODO: Add mode extension for completeness

  d->isOriginal    = ((static_cast<unsigned char>(data[3]) & 0x04) != 0);
  d->isCopyrighted = ((static_cast<unsigned char>(data[3]) & 0x08) != 0);
  d->isPadded      = ((static_cast<unsigned char>(data[2]) & 0x02) != 0);

  // Samples per frame

  static const int samplesPerFrame[3][2] = {
    // MPEG1, 2/2.5
    {  384,   384 }, // Layer I
    { 1152,  1152 }, // Layer II
    { 1152,   576 }  // Layer III
  };

  d->samplesPerFrame = samplesPerFrame[layerIndex][versionIndex];

  // Calculate the frame length

  static const int paddingSize[3] = { 4, 1, 1 };

  d->frameLength = d->samplesPerFrame * d->bitrate * 125 / d->sampleRate;

  if(d->isPadded)
    d->frameLength += paddingSize[layerIndex];

  if(checkLength) {

    // Check if the frame length has been calculated correctly, or the next frame
    // header is right next to the end of this frame.

    // The MPEG versions, layers and sample rates of the two frames should be
    // consistent. Otherwise, we assume that either or both of the frames are
    // broken.

    file->seek(offset + d->frameLength);
    const ByteVector nextData = file->readBlock(4);

    if(nextData.size() < 4)
      return;

    const unsigned int HeaderMask = 0xfffe0c00;

    const unsigned int header     = data.toUInt(0, true)     & HeaderMask;
    const unsigned int nextHeader = nextData.toUInt(0, true) & HeaderMask;

    if(header != nextHeader)
      return;
  }

  // Now that we're done parsing, set this to be a valid frame.

  d->isValid = true;
}
