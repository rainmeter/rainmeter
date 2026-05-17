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

#include <tstring.h>
#include <tdebug.h>

#include <oggpageheader.h>

#include "vorbisproperties.h"
#include "vorbisfile.h"

using namespace TagLib;

class Vorbis::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
    length(0),
    bitrate(0),
    sampleRate(0),
    channels(0),
    vorbisVersion(0),
    bitrateMaximum(0),
    bitrateNominal(0),
    bitrateMinimum(0) {}

  int length;
  int bitrate;
  int sampleRate;
  int channels;
  int vorbisVersion;
  int bitrateMaximum;
  int bitrateNominal;
  int bitrateMinimum;
};

namespace TagLib {
  /*!
   * Vorbis headers can be found with one type ID byte and the string "vorbis" in
   * an Ogg stream.  0x01 indicates the setup header.
   */
  static const char vorbisSetupHeaderID[] = { 0x01, 'v', 'o', 'r', 'b', 'i', 's', 0 };
} // namespace TagLib

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Vorbis::Properties::Properties(File *file, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  read(file);
}

Vorbis::Properties::~Properties()
{
  delete d;
}

int Vorbis::Properties::length() const
{
  return lengthInSeconds();
}

int Vorbis::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int Vorbis::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int Vorbis::Properties::bitrate() const
{
  return d->bitrate;
}

int Vorbis::Properties::sampleRate() const
{
  return d->sampleRate;
}

int Vorbis::Properties::channels() const
{
  return d->channels;
}

int Vorbis::Properties::vorbisVersion() const
{
  return d->vorbisVersion;
}

int Vorbis::Properties::bitrateMaximum() const
{
  return d->bitrateMaximum;
}

int Vorbis::Properties::bitrateNominal() const
{
  return d->bitrateNominal;
}

int Vorbis::Properties::bitrateMinimum() const
{
  return d->bitrateMinimum;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Vorbis::Properties::read(File *file)
{
  // Get the identification header from the Ogg implementation.

  const ByteVector data = file->packet(0);
  if(data.size() < 28) {
    debug("Vorbis::Properties::read() -- data is too short.");
    return;
  }

  unsigned int pos = 0;

  if(data.mid(pos, 7) != vorbisSetupHeaderID) {
    debug("Vorbis::Properties::read() -- invalid Vorbis identification header");
    return;
  }

  pos += 7;

  d->vorbisVersion = data.toUInt(pos, false);
  pos += 4;

  d->channels = static_cast<unsigned char>(data[pos]);
  pos += 1;

  d->sampleRate = data.toUInt(pos, false);
  pos += 4;

  d->bitrateMaximum = data.toUInt(pos, false);
  pos += 4;

  d->bitrateNominal = data.toUInt(pos, false);
  pos += 4;

  d->bitrateMinimum = data.toUInt(pos, false);
  pos += 4;

  // Find the length of the file.  See http://wiki.xiph.org/VorbisStreamLength/
  // for my notes on the topic.

  const Ogg::PageHeader *first = file->firstPageHeader();
  const Ogg::PageHeader *last  = file->lastPageHeader();

  if(first && last) {
    const long long start = first->absoluteGranularPosition();
    const long long end   = last->absoluteGranularPosition();

    if(start >= 0 && end >= 0 && d->sampleRate > 0) {
      const long long frameCount = end - start;

      if(frameCount > 0) {
        const double length = frameCount * 1000.0 / d->sampleRate;
        long fileLengthWithoutOverhead = file->length();
        // Ignore the three initial header packets, see "1.3.1. Decode Setup" in
        // https://xiph.org/vorbis/doc/Vorbis_I_spec.html
        for (unsigned int i = 0; i < 3; ++i) {
          fileLengthWithoutOverhead -= file->packet(i).size();
        }
        d->length  = static_cast<int>(length + 0.5);
        d->bitrate = static_cast<int>(fileLengthWithoutOverhead * 8.0 / length + 0.5);
      }
    }
    else {
      debug("Vorbis::Properties::read() -- Either the PCM values for the start or "
            "end of this file was incorrect or the sample rate is zero.");
    }
  }
  else
    debug("Vorbis::Properties::read() -- Could not find valid first and last Ogg pages.");

  // Alternative to the actual average bitrate.

  if(d->bitrate == 0 && d->bitrateNominal > 0)
    d->bitrate = static_cast<int>(d->bitrateNominal / 1000.0 + 0.5);
}
