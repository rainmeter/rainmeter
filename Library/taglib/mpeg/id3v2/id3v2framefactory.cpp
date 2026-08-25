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
#include <tzlib.h>

#include "id3v2framefactory.h"
#include "id3v2synchdata.h"
#include "id3v1genres.h"

#include "frames/attachedpictureframe.h"
#include "frames/commentsframe.h"
#include "frames/relativevolumeframe.h"
#include "frames/textidentificationframe.h"
#include "frames/uniquefileidentifierframe.h"
#include "frames/unknownframe.h"
#include "frames/generalencapsulatedobjectframe.h"
#include "frames/urllinkframe.h"
#include "frames/unsynchronizedlyricsframe.h"
#include "frames/popularimeterframe.h"
#include "frames/privateframe.h"
#include "frames/ownershipframe.h"
#include "frames/synchronizedlyricsframe.h"
#include "frames/eventtimingcodesframe.h"
#include "frames/chapterframe.h"
#include "frames/tableofcontentsframe.h"
#include "frames/podcastframe.h"

using namespace TagLib;
using namespace ID3v2;

namespace
{
  void updateGenre(TextIdentificationFrame *frame)
  {
    StringList fields = frame->fieldList();
    StringList newfields;

    for(StringList::ConstIterator it = fields.begin(); it != fields.end(); ++it) {
      String s = *it;
      int offset = 0;
      int end = 0;

      while(s.length() > offset && s[offset] == '(' &&
            (end = s.find(")", offset + 1)) > offset) {
        // "(12)Genre"
        const String genreCode = s.substr(offset + 1, end - 1);
        s = s.substr(end + 1);
        bool ok;
        int number = genreCode.toInt(&ok);
        if((ok && number >= 0 && number <= 255 &&
            !(ID3v1::genre(number) == s)) ||
           genreCode == "RX" || genreCode == "CR")
          newfields.append(genreCode);
      }
      if(!s.isEmpty())
        // "Genre" or "12"
        newfields.append(s);
    }

    if(newfields.isEmpty())
      fields.append(String());

    frame->setText(newfields);
  }
}  // namespace

class FrameFactory::FrameFactoryPrivate
{
public:
  FrameFactoryPrivate() :
    defaultEncoding(String::Latin1),
    useDefaultEncoding(false) {}

  String::Type defaultEncoding;
  bool useDefaultEncoding;

  template <class T> void setTextEncoding(T *frame)
  {
    if(useDefaultEncoding)
      frame->setTextEncoding(defaultEncoding);
  }
};

FrameFactory FrameFactory::factory;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FrameFactory *FrameFactory::instance()
{
  return &factory;
}

Frame *FrameFactory::createFrame(const ByteVector &data, bool synchSafeInts) const
{
  return createFrame(data, static_cast<unsigned int>(synchSafeInts ? 4 : 3));
}

Frame *FrameFactory::createFrame(const ByteVector &data, unsigned int version) const
{
  Header tagHeader;
  tagHeader.setMajorVersion(version);
  return createFrame(data, &tagHeader);
}

Frame *FrameFactory::createFrame(const ByteVector &origData, Header *tagHeader) const
{
    return createFrame(origData, const_cast<const Header *>(tagHeader));
}

Frame *FrameFactory::createFrame(const ByteVector &origData, const Header *tagHeader) const
{
  ByteVector data = origData;
  unsigned int version = tagHeader->majorVersion();
  Frame::Header *header = new Frame::Header(data, version);
  ByteVector frameID = header->frameID();

  // A quick sanity check -- make sure that the frameID is 4 uppercase Latin1
  // characters.  Also make sure that there is data in the frame.

  if(frameID.size() != (version < 3 ? 3 : 4) ||
     header->frameSize() <= static_cast<unsigned int>(header->dataLengthIndicator() ? 4 : 0) ||
     header->frameSize() > data.size())
  {
    delete header;
    return 0;
  }

#ifndef NO_ITUNES_HACKS
  if(version == 3 && frameID.size() == 4 && frameID[3] == '\0') {
    // iTunes v2.3 tags store v2.2 frames - convert now
    frameID = frameID.mid(0, 3);
    header->setFrameID(frameID);
    header->setVersion(2);
    updateFrame(header);
    header->setVersion(3);
  }
#endif

  for(ByteVector::ConstIterator it = frameID.begin(); it != frameID.end(); it++) {
    if( (*it < 'A' || *it > 'Z') && (*it < '0' || *it > '9') ) {
      delete header;
      return 0;
    }
  }

  if(version > 3 && (tagHeader->unsynchronisation() || header->unsynchronisation())) {
    // Data lengths are not part of the encoded data, but since they are synch-safe
    // integers they will be never actually encoded.
    ByteVector frameData = data.mid(Frame::Header::size(version), header->frameSize());
    frameData = SynchData::decode(frameData);
    data = data.mid(0, Frame::Header::size(version)) + frameData;
  }

  // TagLib doesn't mess with encrypted frames, so just treat them
  // as unknown frames.

  if(!zlib::isAvailable() && header->compression()) {
    debug("Compressed frames are currently not supported.");
    return new UnknownFrame(data, header);
  }

  if(header->encryption()) {
    debug("Encrypted frames are currently not supported.");
    return new UnknownFrame(data, header);
  }

  if(!updateFrame(header)) {
    header->setTagAlterPreservation(true);
    return new UnknownFrame(data, header);
  }

  // updateFrame() might have updated the frame ID.

  frameID = header->frameID();

  // This is where things get necissarily nasty.  Here we determine which
  // Frame subclass (or if none is found simply an Frame) based
  // on the frame ID.  Since there are a lot of possibilities, that means
  // a lot of if blocks.

  // Text Identification (frames 4.2)

  // Apple proprietary WFED (Podcast URL), MVNM (Movement Name), MVIN (Movement Number), GRP1 (Grouping) are in fact text frames.
  if(frameID.startsWith("T") || frameID == "WFED" || frameID == "MVNM" || frameID == "MVIN" || frameID == "GRP1") {

    TextIdentificationFrame *f = frameID != "TXXX"
      ? new TextIdentificationFrame(data, header)
      : new UserTextIdentificationFrame(data, header);

    d->setTextEncoding(f);

    if(frameID == "TCON")
      updateGenre(f);

    return f;
  }

  // Comments (frames 4.10)

  if(frameID == "COMM") {
    CommentsFrame *f = new CommentsFrame(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // Attached Picture (frames 4.14)

  if(frameID == "APIC") {
    AttachedPictureFrame *f = new AttachedPictureFrame(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // ID3v2.2 Attached Picture

  if(frameID == "PIC") {
    AttachedPictureFrame *f = new AttachedPictureFrameV22(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // Relative Volume Adjustment (frames 4.11)

  if(frameID == "RVA2")
    return new RelativeVolumeFrame(data, header);

  // Unique File Identifier (frames 4.1)

  if(frameID == "UFID")
    return new UniqueFileIdentifierFrame(data, header);

  // General Encapsulated Object (frames 4.15)

  if(frameID == "GEOB") {
    GeneralEncapsulatedObjectFrame *f = new GeneralEncapsulatedObjectFrame(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // URL link (frames 4.3)

  if(frameID.startsWith("W")) {
    if(frameID != "WXXX") {
      return new UrlLinkFrame(data, header);
    }
    UserUrlLinkFrame *f = new UserUrlLinkFrame(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // Unsynchronized lyric/text transcription (frames 4.8)

  if(frameID == "USLT") {
    UnsynchronizedLyricsFrame *f = new UnsynchronizedLyricsFrame(data, header);
    if(d->useDefaultEncoding)
      f->setTextEncoding(d->defaultEncoding);
    return f;
  }

  // Synchronized lyrics/text (frames 4.9)

  if(frameID == "SYLT") {
    SynchronizedLyricsFrame *f = new SynchronizedLyricsFrame(data, header);
    if(d->useDefaultEncoding)
      f->setTextEncoding(d->defaultEncoding);
    return f;
  }

  // Event timing codes (frames 4.5)

  if(frameID == "ETCO")
    return new EventTimingCodesFrame(data, header);

  // Popularimeter (frames 4.17)

  if(frameID == "POPM")
    return new PopularimeterFrame(data, header);

  // Private (frames 4.27)

  if(frameID == "PRIV")
    return new PrivateFrame(data, header);

  // Ownership (frames 4.22)

  if(frameID == "OWNE") {
    OwnershipFrame *f = new OwnershipFrame(data, header);
    d->setTextEncoding(f);
    return f;
  }

  // Chapter (ID3v2 chapters 1.0)

  if(frameID == "CHAP")
    return new ChapterFrame(tagHeader, data, header);

  // Table of contents (ID3v2 chapters 1.0)

  if(frameID == "CTOC")
    return new TableOfContentsFrame(tagHeader, data, header);

  // Apple proprietary PCST (Podcast)

  if(frameID == "PCST")
    return new PodcastFrame(data, header);

  return new UnknownFrame(data, header);
}

void FrameFactory::rebuildAggregateFrames(ID3v2::Tag *tag) const
{
  if(tag->header()->majorVersion() < 4 &&
     tag->frameList("TDRC").size() == 1 &&
     tag->frameList("TDAT").size() == 1)
  {
    TextIdentificationFrame *tdrc =
      dynamic_cast<TextIdentificationFrame *>(tag->frameList("TDRC").front());
    UnknownFrame *tdat = dynamic_cast<UnknownFrame *>(tag->frameList("TDAT").front());

    if(tdrc &&
       tdrc->fieldList().size() == 1 &&
       tdrc->fieldList().front().size() == 4 &&
       tdat &&
       tdat->data().size() >= 5)
    {
      String date(tdat->data().mid(1), static_cast<String::Type>(tdat->data()[0]));
      if(date.length() == 4) {
        tdrc->setText(tdrc->toString() + '-' + date.substr(2, 2) + '-' + date.substr(0, 2));
        if(tag->frameList("TIME").size() == 1) {
          UnknownFrame *timeframe = dynamic_cast<UnknownFrame *>(tag->frameList("TIME").front());
          if(timeframe && timeframe->data().size() >= 5) {
            String time(timeframe->data().mid(1), static_cast<String::Type>(timeframe->data()[0]));
            if(time.length() == 4) {
              tdrc->setText(tdrc->toString() + 'T' + time.substr(0, 2) + ':' + time.substr(2, 2));
            }
          }
        }
      }
    }
  }
}

String::Type FrameFactory::defaultTextEncoding() const
{
  return d->defaultEncoding;
}

void FrameFactory::setDefaultTextEncoding(String::Type encoding)
{
  d->useDefaultEncoding = true;
  d->defaultEncoding = encoding;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

FrameFactory::FrameFactory() :
  d(new FrameFactoryPrivate())
{
}

FrameFactory::~FrameFactory()
{
  delete d;
}

namespace
{
  // Frame conversion table ID3v2.2 -> 2.4
  const std::pair<const char *, const char *> frameConversion2[] = {
    std::make_pair("BUF", "RBUF"),
    std::make_pair("CNT", "PCNT"),
    std::make_pair("COM", "COMM"),
    std::make_pair("CRA", "AENC"),
    std::make_pair("ETC", "ETCO"),
    std::make_pair("GEO", "GEOB"),
    std::make_pair("IPL", "TIPL"),
    std::make_pair("MCI", "MCDI"),
    std::make_pair("MLL", "MLLT"),
    std::make_pair("POP", "POPM"),
    std::make_pair("REV", "RVRB"),
    std::make_pair("SLT", "SYLT"),
    std::make_pair("STC", "SYTC"),
    std::make_pair("TAL", "TALB"),
    std::make_pair("TBP", "TBPM"),
    std::make_pair("TCM", "TCOM"),
    std::make_pair("TCO", "TCON"),
    std::make_pair("TCP", "TCMP"),
    std::make_pair("TCR", "TCOP"),
    std::make_pair("TDY", "TDLY"),
    std::make_pair("TEN", "TENC"),
    std::make_pair("TFT", "TFLT"),
    std::make_pair("TKE", "TKEY"),
    std::make_pair("TLA", "TLAN"),
    std::make_pair("TLE", "TLEN"),
    std::make_pair("TMT", "TMED"),
    std::make_pair("TOA", "TOAL"),
    std::make_pair("TOF", "TOFN"),
    std::make_pair("TOL", "TOLY"),
    std::make_pair("TOR", "TDOR"),
    std::make_pair("TOT", "TOAL"),
    std::make_pair("TP1", "TPE1"),
    std::make_pair("TP2", "TPE2"),
    std::make_pair("TP3", "TPE3"),
    std::make_pair("TP4", "TPE4"),
    std::make_pair("TPA", "TPOS"),
    std::make_pair("TPB", "TPUB"),
    std::make_pair("TRC", "TSRC"),
    std::make_pair("TRD", "TDRC"),
    std::make_pair("TRK", "TRCK"),
    std::make_pair("TS2", "TSO2"),
    std::make_pair("TSA", "TSOA"),
    std::make_pair("TSC", "TSOC"),
    std::make_pair("TSP", "TSOP"),
    std::make_pair("TSS", "TSSE"),
    std::make_pair("TST", "TSOT"),
    std::make_pair("TT1", "TIT1"),
    std::make_pair("TT2", "TIT2"),
    std::make_pair("TT3", "TIT3"),
    std::make_pair("TXT", "TOLY"),
    std::make_pair("TXX", "TXXX"),
    std::make_pair("TYE", "TDRC"),
    std::make_pair("UFI", "UFID"),
    std::make_pair("ULT", "USLT"),
    std::make_pair("WAF", "WOAF"),
    std::make_pair("WAR", "WOAR"),
    std::make_pair("WAS", "WOAS"),
    std::make_pair("WCM", "WCOM"),
    std::make_pair("WCP", "WCOP"),
    std::make_pair("WPB", "WPUB"),
    std::make_pair("WXX", "WXXX"),

    // Apple iTunes nonstandard frames
    std::make_pair("PCS", "PCST"),
    std::make_pair("TCT", "TCAT"),
    std::make_pair("TDR", "TDRL"),
    std::make_pair("TDS", "TDES"),
    std::make_pair("TID", "TGID"),
    std::make_pair("WFD", "WFED"),
    std::make_pair("MVN", "MVNM"),
    std::make_pair("MVI", "MVIN"),
    std::make_pair("GP1", "GRP1"),
  };
  const size_t frameConversion2Size = sizeof(frameConversion2) / sizeof(frameConversion2[0]);

  // Frame conversion table ID3v2.3 -> 2.4
  const std::pair<const char *, const char *> frameConversion3[] = {
    std::make_pair("TORY", "TDOR"),
    std::make_pair("TYER", "TDRC"),
    std::make_pair("IPLS", "TIPL"),
  };
  const size_t frameConversion3Size = sizeof(frameConversion3) / sizeof(frameConversion3[0]);
}  // namespace

bool FrameFactory::updateFrame(Frame::Header *header) const
{
  const ByteVector frameID = header->frameID();

  switch(header->version()) {

  case 2: // ID3v2.2
  {
    if(frameID == "CRM" ||
       frameID == "EQU" ||
       frameID == "LNK" ||
       frameID == "RVA" ||
       frameID == "TIM" ||
       frameID == "TSI" ||
       frameID == "TDA")
    {
      debug("ID3v2.4 no longer supports the frame type " + String(frameID) +
            ".  It will be discarded from the tag.");
      return false;
    }

    // ID3v2.2 only used 3 bytes for the frame ID, so we need to convert all of
    // the frames to their 4 byte ID3v2.4 equivalent.

    for(size_t i = 0; i < frameConversion2Size; ++i) {
      if(frameID == frameConversion2[i].first) {
        header->setFrameID(frameConversion2[i].second);
        break;
      }
    }

    break;
  }

  case 3: // ID3v2.3
  {
    if(frameID == "EQUA" ||
       frameID == "RVAD" ||
       frameID == "TIME" ||
       frameID == "TRDA" ||
       frameID == "TSIZ" ||
       frameID == "TDAT")
    {
      debug("ID3v2.4 no longer supports the frame type " + String(frameID) +
            ".  It will be discarded from the tag.");
      return false;
    }

    for(size_t i = 0; i < frameConversion3Size; ++i) {
      if(frameID == frameConversion3[i].first) {
        header->setFrameID(frameConversion3[i].second);
        break;
      }
    }

    break;
  }

  default:

    // This should catch a typo that existed in TagLib up to and including
    // version 1.1 where TRDC was used for the year rather than TDRC.

    if(frameID == "TRDC")
      header->setFrameID("TDRC");

    break;
  }

  return true;
}
