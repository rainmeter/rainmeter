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

#include <tdebug.h>
#include <tstring.h>

#include "mpegproperties.h"
#include "mpegfile.h"
#include "xingheader.h"
#include "apetag.h"
#include "apefooter.h"

using namespace TagLib;

class MPEG::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
    xingHeader(0),
    length(0),
    bitrate(0),
    sampleRate(0),
    channels(0),
    layer(0),
    version(Header::Version1),
    channelMode(Header::Stereo),
    protectionEnabled(false),
    isCopyrighted(false),
    isOriginal(false) {}

  ~PropertiesPrivate()
  {
    delete xingHeader;
  }

  XingHeader *xingHeader;
  int length;
  int bitrate;
  int sampleRate;
  int channels;
  int layer;
  Header::Version version;
  Header::ChannelMode channelMode;
  bool protectionEnabled;
  bool isCopyrighted;
  bool isOriginal;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

MPEG::Properties::Properties(File *file, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  read(file);
}

MPEG::Properties::~Properties()
{
  delete d;
}

int MPEG::Properties::length() const
{
  return lengthInSeconds();
}

int MPEG::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int MPEG::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int MPEG::Properties::bitrate() const
{
  return d->bitrate;
}

int MPEG::Properties::sampleRate() const
{
  return d->sampleRate;
}

int MPEG::Properties::channels() const
{
  return d->channels;
}

const MPEG::XingHeader *MPEG::Properties::xingHeader() const
{
  return d->xingHeader;
}

MPEG::Header::Version MPEG::Properties::version() const
{
  return d->version;
}

int MPEG::Properties::layer() const
{
  return d->layer;
}

bool MPEG::Properties::protectionEnabled() const
{
  return d->protectionEnabled;
}

MPEG::Header::ChannelMode MPEG::Properties::channelMode() const
{
  return d->channelMode;
}

bool MPEG::Properties::isCopyrighted() const
{
  return d->isCopyrighted;
}

bool MPEG::Properties::isOriginal() const
{
  return d->isOriginal;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void MPEG::Properties::read(File *file)
{
  // Only the first valid frame is required if we have a VBR header.

  const long firstFrameOffset = file->firstFrameOffset();
  if(firstFrameOffset < 0) {
    debug("MPEG::Properties::read() -- Could not find an MPEG frame in the stream.");
    return;
  }

  const Header firstHeader(file, firstFrameOffset, false);

  // Check for a VBR header that will help us in gathering information about a
  // VBR stream.

  file->seek(firstFrameOffset);
  d->xingHeader = new XingHeader(file->readBlock(firstHeader.frameLength()));
  if(!d->xingHeader->isValid()) {
    delete d->xingHeader;
    d->xingHeader = 0;
  }

  if(d->xingHeader && firstHeader.samplesPerFrame() > 0 && firstHeader.sampleRate() > 0) {

    // Read the length and the bitrate from the VBR header.

    const double timePerFrame = firstHeader.samplesPerFrame() * 1000.0 / firstHeader.sampleRate();
    const double length = timePerFrame * d->xingHeader->totalFrames();

    d->length  = static_cast<int>(length + 0.5);
    d->bitrate = static_cast<int>(d->xingHeader->totalSize() * 8.0 / length + 0.5);
  }
  else if(firstHeader.bitrate() > 0) {

    // Since there was no valid VBR header found, we hope that we're in a constant
    // bitrate file.

    // TODO: Make this more robust with audio property detection for VBR without a
    // Xing header.

    d->bitrate = firstHeader.bitrate();

    // Look for the last MPEG audio frame to calculate the stream length.

    const long lastFrameOffset = file->lastFrameOffset();
    if(lastFrameOffset < 0) {
      debug("MPEG::Properties::read() -- Could not find an MPEG frame in the stream.");
    }
    else
    {
      const Header lastHeader(file, lastFrameOffset, false);
      const long streamLength = lastFrameOffset - firstFrameOffset + lastHeader.frameLength();
      if (streamLength > 0)
        d->length = static_cast<int>(streamLength * 8.0 / d->bitrate + 0.5);
    }
  }

  d->sampleRate        = firstHeader.sampleRate();
  d->channels          = firstHeader.channelMode() == Header::SingleChannel ? 1 : 2;
  d->version           = firstHeader.version();
  d->layer             = firstHeader.layer();
  d->protectionEnabled = firstHeader.protectionEnabled();
  d->channelMode       = firstHeader.channelMode();
  d->isCopyrighted     = firstHeader.isCopyrighted();
  d->isOriginal        = firstHeader.isOriginal();
}
