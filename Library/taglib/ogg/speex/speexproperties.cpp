/***************************************************************************
    copyright            : (C) 2006 by Lukáš Lalinský
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

#include "speexproperties.h"
#include "speexfile.h"

using namespace TagLib;
using namespace TagLib::Ogg;

class Speex::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
    length(0),
    bitrate(0),
    bitrateNominal(0),
    sampleRate(0),
    channels(0),
    speexVersion(0),
    vbr(false),
    mode(0) {}

  int length;
  int bitrate;
  int bitrateNominal;
  int sampleRate;
  int channels;
  int speexVersion;
  bool vbr;
  int mode;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Speex::Properties::Properties(File *file, ReadStyle style) :
  AudioProperties(style),
  d(new PropertiesPrivate())
{
  read(file);
}

Speex::Properties::~Properties()
{
  delete d;
}

int Speex::Properties::length() const
{
  return lengthInSeconds();
}

int Speex::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int Speex::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int Speex::Properties::bitrate() const
{
  return d->bitrate;
}

int Speex::Properties::bitrateNominal() const
{
  return d->bitrateNominal;
}

int Speex::Properties::sampleRate() const
{
  return d->sampleRate;
}

int Speex::Properties::channels() const
{
  return d->channels;
}

int Speex::Properties::speexVersion() const
{
  return d->speexVersion;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Speex::Properties::read(File *file)
{
  // Get the identification header from the Ogg implementation.

  const ByteVector data = file->packet(0);
  if(data.size() < 64) {
    debug("Speex::Properties::read() -- data is too short.");
    return;
  }

  unsigned int pos = 28;

  // speex_version_id;       /**< Version for Speex (for checking compatibility) */
  d->speexVersion = data.toUInt(pos, false);
  pos += 4;

  // header_size;            /**< Total size of the header ( sizeof(SpeexHeader) ) */
  pos += 4;

  // rate;                   /**< Sampling rate used */
  d->sampleRate = data.toUInt(pos, false);
  pos += 4;

  // mode;                   /**< Mode used (0 for narrowband, 1 for wideband) */
  d->mode = data.toUInt(pos, false);
  pos += 4;

  // mode_bitstream_version; /**< Version ID of the bit-stream */
  pos += 4;

  // nb_channels;            /**< Number of channels encoded */
  d->channels = data.toUInt(pos, false);
  pos += 4;

  // bitrate;                /**< Bit-rate used */
  d->bitrateNominal = data.toUInt(pos, false);
  pos += 4;

  // frame_size;             /**< Size of frames */
  // unsigned int frameSize = data.mid(pos, 4).toUInt(false);
  pos += 4;

  // vbr;                    /**< 1 for a VBR encoding, 0 otherwise */
  d->vbr = data.toUInt(pos, false) == 1;
  pos += 4;

  // frames_per_packet;      /**< Number of frames stored per Ogg packet */
  // unsigned int framesPerPacket = data.mid(pos, 4).toUInt(false);

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
        // Ignore the two header packets, see "Ogg file format" in
        // https://www.speex.org/docs/manual/speex-manual/node8.html
        for (unsigned int i = 0; i < 2; ++i) {
          fileLengthWithoutOverhead -= file->packet(i).size();
        }
        d->length  = static_cast<int>(length + 0.5);
        d->bitrate = static_cast<int>(fileLengthWithoutOverhead * 8.0 / length + 0.5);
      }
    }
    else {
      debug("Speex::Properties::read() -- Either the PCM values for the start or "
            "end of this file was incorrect or the sample rate is zero.");
    }
  }
  else
    debug("Speex::Properties::read() -- Could not find valid first and last Ogg pages.");

  // Alternative to the actual average bitrate.

  if(d->bitrate == 0 && d->bitrateNominal > 0)
    d->bitrate = static_cast<int>(d->bitrateNominal / 1000.0 + 0.5);
}
