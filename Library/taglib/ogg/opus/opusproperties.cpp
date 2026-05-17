/***************************************************************************
    copyright            : (C) 2012 by Lukáš Lalinský
    email                : lalinsky@gmail.com

    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
                           (original Vorbis implementation)
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

#include <oggpageheader.h>

#include "opusproperties.h"
#include "opusfile.h"

using namespace TagLib;
using namespace TagLib::Ogg;

class Opus::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
    length(0),
    bitrate(0),
    inputSampleRate(0),
    channels(0),
    opusVersion(0) {}

  int length;
  int bitrate;
  int inputSampleRate;
  int channels;
  int opusVersion;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Opus::Properties::Properties(File *file, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  read(file);
}

Opus::Properties::~Properties()
{
  delete d;
}

int Opus::Properties::length() const
{
  return lengthInSeconds();
}

int Ogg::Opus::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int Ogg::Opus::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int Opus::Properties::bitrate() const
{
  return d->bitrate;
}

int Opus::Properties::sampleRate() const
{
  // Opus can decode any stream at a sample rate of 8, 12, 16, 24, or 48 kHz,
  // so there is no single sample rate. Let's assume it's the highest
  // possible.
  return 48000;
}

int Opus::Properties::channels() const
{
  return d->channels;
}

int Opus::Properties::inputSampleRate() const
{
  return d->inputSampleRate;
}

int Opus::Properties::opusVersion() const
{
  return d->opusVersion;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Opus::Properties::read(File *file)
{
  // Get the identification header from the Ogg implementation.

  // http://tools.ietf.org/html/draft-terriberry-oggopus-01#section-5.1

  const ByteVector data = file->packet(0);

  // *Magic Signature*
  unsigned int pos = 8;

  // *Version* (8 bits, unsigned)
  d->opusVersion = static_cast<unsigned char>(data.at(pos));
  pos += 1;

  // *Output Channel Count* 'C' (8 bits, unsigned)
  d->channels = static_cast<unsigned char>(data.at(pos));
  pos += 1;

  // *Pre-skip* (16 bits, unsigned, little endian)
  const unsigned short preSkip = data.toUShort(pos, false);
  pos += 2;

  // *Input Sample Rate* (32 bits, unsigned, little endian)
  d->inputSampleRate = data.toUInt(pos, false);
  pos += 4;

  // *Output Gain* (16 bits, signed, little endian)
  pos += 2;

  // *Channel Mapping Family* (8 bits, unsigned)
  pos += 1;

  const Ogg::PageHeader *first = file->firstPageHeader();
  const Ogg::PageHeader *last  = file->lastPageHeader();

  if(first && last) {
    const long long start = first->absoluteGranularPosition();
    const long long end   = last->absoluteGranularPosition();

    if(start >= 0 && end >= 0) {
      const long long frameCount = (end - start - preSkip);

      if(frameCount > 0) {
        const double length = frameCount * 1000.0 / 48000.0;
        long fileLengthWithoutOverhead = file->length();
        // Ignore the two mandatory header packets, see "3. Packet Organization"
        // in https://tools.ietf.org/html/rfc7845.html
        for (unsigned int i = 0; i < 2; ++i) {
          fileLengthWithoutOverhead -= file->packet(i).size();
        }
        d->length  = static_cast<int>(length + 0.5);
        d->bitrate = static_cast<int>(fileLengthWithoutOverhead * 8.0 / length + 0.5);
      }
    }
    else {
      debug("Opus::Properties::read() -- The PCM values for the start or "
            "end of this file was incorrect.");
    }
  }
  else
    debug("Opus::Properties::read() -- Could not find valid first and last Ogg pages.");
}
