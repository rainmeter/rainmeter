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

#ifndef TAGLIB_ID3V2TAG_H
#define TAGLIB_ID3V2TAG_H

#include "tag.h"
#include "tbytevector.h"
#include "tstring.h"
#include "tlist.h"
#include "tmap.h"
#include "taglib_export.h"

#include "id3v2framefactory.h"

namespace TagLib {

  class File;

  //! An ID3v2 implementation

  /*!
   * This is a relatively complete and flexible framework for working with ID3v2
   * tags.
   *
   * \see ID3v2::Tag
   */

  namespace ID3v2 {

    class Header;
    class ExtendedHeader;
    class Footer;

    typedef List<Frame *> FrameList;
    typedef Map<ByteVector, FrameList> FrameListMap;

    //! An abstraction for the ISO-8859-1 string to data encoding in ID3v2 tags.

    /*!
     * ID3v2 tag can store strings in ISO-8859-1 (Latin1), and TagLib only 
     * supports genuine ISO-8859-1 by default.  However, in practice, non 
     * ISO-8859-1 encodings are often used instead of ISO-8859-1, such as 
     * Windows-1252 for western languages, Shift_JIS for Japanese and so on.
     *
     * Here is an option to read such tags by subclassing this class,
     * reimplementing parse() and setting your reimplementation as the default 
     * with ID3v2::Tag::setStringHandler().
     *
     * \note Writing non-ISO-8859-1 tags is not implemented intentionally.
     * Use UTF-16 or UTF-8 instead.
     *
     * \see ID3v2::Tag::setStringHandler()
     */
    class TAGLIB_EXPORT Latin1StringHandler
    {
    public:
      Latin1StringHandler();
      virtual ~Latin1StringHandler();

      /*!
       * Decode a string from \a data.  The default implementation assumes that
       * \a data is an ISO-8859-1 (Latin1) character array.
       */
      virtual String parse(const ByteVector &data) const;
    };

    //! The main class in the ID3v2 implementation

    /*!
     * This is the main class in the ID3v2 implementation.  It serves two
     * functions.  This first, as is obvious from the public API, is to provide a
     * container for the other ID3v2 related classes.  In addition, through the
     * read() and parse() protected methods, it provides the most basic level of
     * parsing.  In these methods the ID3v2 tag is extracted from the file and
     * split into data components.
     *
     * ID3v2 tags have several parts, TagLib attempts to provide an interface
     * for them all.  header(), footer() and extendedHeader() corespond to those
     * data structures in the ID3v2 standard and the APIs for the classes that
     * they return attempt to reflect this.
     *
     * Also ID3v2 tags are built up from a list of frames, which are in turn
     * have a header and a list of fields.  TagLib provides two ways of accessing
     * the list of frames that are in a given ID3v2 tag.  The first is simply
     * via the frameList() method.  This is just a list of pointers to the frames.
     * The second is a map from the frame type -- i.e. "COMM" for comments -- and
     * a list of frames of that type.  (In some cases ID3v2 allows for multiple
     * frames of the same type, hence this being a map to a list rather than just
     * a map to an individual frame.)
     *
     * More information on the structure of frames can be found in the ID3v2::Frame
     * class.
     *
     * read() and parse() pass binary data to the other ID3v2 class structures,
     * they do not handle parsing of flags or fields, for instace.  Those are
     * handled by similar functions within those classes.
     *
     * \note All pointers to data structures within the tag will become invalid
     * when the tag is destroyed.
     *
     * \warning Dealing with the nasty details of ID3v2 is not for the faint of
     * heart and should not be done without much meditation on the spec.  It's
     * rather long, but if you're planning on messing with this class and others
     * that deal with the details of ID3v2 (rather than the nice, safe, abstract
     * TagLib::Tag and friends), it's worth your time to familiarize yourself
     * with said spec (which is distrubuted with the TagLib sources).  TagLib
     * tries to do most of the work, but with a little luck, you can still
     * convince it to generate invalid ID3v2 tags.  The APIs for ID3v2 assume a
     * working knowledge of ID3v2 structure.  You're been warned.
     */

    class TAGLIB_EXPORT Tag : public TagLib::Tag
    {
    public:
      /*!
       * Constructs an empty ID3v2 tag.
       *
       * \note You must create at least one frame for this tag to be valid.
       */
      Tag();

      /*!
       * Constructs an ID3v2 tag read from \a file starting at \a tagOffset.
       * \a factory specifies which FrameFactory will be used for the
       * construction of new frames.
       *
       * \note You should be able to ignore the \a factory parameter in almost
       * all situations.  You would want to specify your own FrameFactory
       * subclass in the case that you are extending TagLib to support additional
       * frame types, which would be incorperated into your factory.
       *
       * \see FrameFactory
       */
      Tag(File *file, long tagOffset,
          const FrameFactory *factory = FrameFactory::instance());

      /*!
       * Destroys this Tag instance.
       */
      virtual ~Tag();

      // Reimplementations.

      virtual String title() const;
      virtual String artist() const;
      virtual String album() const;
      virtual String comment() const;
      virtual String genre() const;
      virtual uint year() const;
      virtual uint track() const;

      virtual void setTitle(const String &s);
      virtual void setArtist(const String &s);
      virtual void setAlbum(const String &s);
      virtual void setComment(const String &s);
      virtual void setGenre(const String &s);
      virtual void setYear(uint i);
      virtual void setTrack(uint i);

      virtual bool isEmpty() const;

      /*!
       * Returns a pointer to the tag's header.
       */
      Header *header() const;

      /*!
       * Returns a pointer to the tag's extended header or null if there is no
       * extended header.
       */
      ExtendedHeader *extendedHeader() const;

      /*!
       * Returns a pointer to the tag's footer or null if there is no footer.
       *
       * \deprecated I don't see any reason to keep this around since there's
       * nothing useful to be retrieved from the footer, but well, again, I'm
       * prone to change my mind, so this gets to stay around until near a
       * release.
       */
      Footer *footer() const;

      /*!
       * Returns a reference to the frame list map.  This is an FrameListMap of
       * all of the frames in the tag.
       *
       * This is the most convenient structure for accessing the tag's frames.
       * Many frame types allow multiple instances of the same frame type so this
       * is a map of lists.  In most cases however there will only be a single
       * frame of a certain type.
       *
       * Let's say for instance that you wanted to access the frame for total
       * beats per minute -- the TBPM frame.
       *
       * \code
       * TagLib::MPEG::File f("foo.mp3");
       *
       * // Check to make sure that it has an ID3v2 tag
       *
       * if(f.ID3v2Tag()) {
       *
       *   // Get the list of frames for a specific frame type
       *
       *   TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["TBPM"];
       *
       *   if(!l.isEmpty())
       *     std::cout << l.front()->toString() << std::endl;
       * }
       *
       * \endcode
       *
       * \warning You should not modify this data structure directly, instead
       * use addFrame() and removeFrame().
       *
       * \see frameList()
       */
      const FrameListMap &frameListMap() const;

      /*!
       * Returns a reference to the frame list.  This is an FrameList of all of
       * the frames in the tag in the order that they were parsed.
       *
       * This can be useful if for example you want iterate over the tag's frames
       * in the order that they occur in the tag.
       *
       * \warning You should not modify this data structure directly, instead
       * use addFrame() and removeFrame().
       */
      const FrameList &frameList() const;

      /*!
       * Returns the frame list for frames with the id \a frameID or an empty
       * list if there are no frames of that type.  This is just a convenience
       * and is equivalent to:
       *
       * \code
       * frameListMap()[frameID];
       * \endcode
       *
       * \see frameListMap()
       */
      const FrameList &frameList(const ByteVector &frameID) const;

      /*!
       * Add a frame to the tag.  At this point the tag takes ownership of
       * the frame and will handle freeing its memory.
       *
       * \note Using this method will invalidate any pointers on the list
       * returned by frameList()
       */
      void addFrame(Frame *frame);

      /*!
       * Remove a frame from the tag.  If \a del is true the frame's memory
       * will be freed; if it is false, it must be deleted by the user.
       *
       * \note Using this method will invalidate any pointers on the list
       * returned by frameList()
       */
      void removeFrame(Frame *frame, bool del = true);

      /*!
       * Remove all frames of type \a id from the tag and free their memory.
       *
       * \note Using this method will invalidate any pointers on the list
       * returned by frameList()
       */
      void removeFrames(const ByteVector &id);

      /*!
       * Implements the unified property interface -- export function.
       * This function does some work to translate the hard-specified ID3v2
       * frame types into a free-form string-to-stringlist PropertyMap:
       *  - if ID3v2 frame ID is known by Frame::frameIDToKey(), the returned
       *    key is used
       *  - if the frame ID is "TXXX" (user text frame), the description() is
       *    used as key
       *  - if the frame ID is "WXXX" (user url frame),
       *    - if the description is empty or "URL", the key "URL" is used
       *    - otherwise, the key "URL:<description>" is used;
       *  - if the frame ID is "COMM" (comments frame),
       *    - if the description is empty or "COMMENT", the key "COMMENT"
       *      is used
       *    - otherwise, the key "COMMENT:<description>" is used;
       *  - if the frame ID is "USLT" (unsynchronized lyrics),
       *    - if the description is empty or "LYRICS", the key "LYRICS" is used
       *    - otherwise, the key "LYRICS:<description>" is used;
       *  - if the frame ID is "TIPL" (involved peoples list), and if all the
       *    roles defined in the frame are known in TextIdentificationFrame::involvedPeopleMap(),
       *    then "<role>=<name>" will be contained in the returned obejct for each
       *  - if the frame ID is "TMCL" (musician credit list), then
       *    "PERFORMER:<instrument>=<name>" will be contained in the returned
       *    PropertyMap for each defined musician
       *  In any other case, the unsupportedData() of the returned object will contain
       *  the frame's ID and, in case of a frame ID which is allowed to appear more than
       *  once, the description, separated by a "/".
       *
       */
      PropertyMap properties() const;

      /*!
       * Removes unsupported frames given by \a properties. The elements of
       * \a properties must be taken from properties().unsupportedData(); they
       * are of one of the following forms:
       *  - a four-character frame ID, if the ID3 specification allows only one
       *    frame with that ID (thus, the frame is uniquely determined)
       *  - frameID + "/" + description(), when the ID is one of "TXXX", "WXXX",
       *    "COMM", or "USLT",
       *  - "UNKNOWN/" + frameID, for frames that could not be parsed by TagLib.
       *    In that case, *all* unknown frames with the given ID will be removed.
       */
      void removeUnsupportedProperties(const StringList &properties);

      /*!
       * Implements the unified property interface -- import function.
       * See the comments in properties().
       */
      PropertyMap setProperties(const PropertyMap &);

      /*!
       * Render the tag back to binary data, suitable to be written to disk.
       */
      ByteVector render() const;

      /*!
       * Render the tag back to binary data, suitable to be written to disk.
       *
       * The \a version parameter specifies the version of the rendered
       * ID3v2 tag. It can be either 4 or 3.
       */
      // BIC: combine with the above method
      ByteVector render(int version) const;
      
      /*!
       * Gets the current string handler that decides how the "Latin-1" data 
       * will be converted to and from binary data.
       *
       * \see Latin1StringHandler
       */
      static Latin1StringHandler const *latin1StringHandler();

      /*!
       * Sets the string handler that decides how the "Latin-1" data will be
       * converted to and from binary data.
       * If the parameter \a handler is null, the previous handler is
       * released and default ISO-8859-1 handler is restored.
       *
       * \note The caller is responsible for deleting the previous handler
       * as needed after it is released. 
       *
       * \see Latin1StringHandler
       */
      static void setLatin1StringHandler(const Latin1StringHandler *handler);

    protected:
      /*!
       * Reads data from the file specified in the constructor.  It does basic
       * parsing of the data in the largest chunks.  It partitions the tag into
       * the Header, the body of the tag  (which contains the ExtendedHeader and
       * frames) and Footer.
       */
      void read();

      /*!
       * This is called by read to parse the body of the tag.  It determines if an
       * extended header exists and adds frames to the FrameListMap.
       */
      void parse(const ByteVector &data);

      /*!
       * Sets the value of the text frame with the Frame ID \a id to \a value.
       * If the frame does not exist, it is created.
       */
      void setTextFrame(const ByteVector &id, const String &value);

      void downgradeFrames(FrameList *existingFrames, FrameList *newFrames) const;

    private:
      Tag(const Tag &);
      Tag &operator=(const Tag &);

      class TagPrivate;
      TagPrivate *d;
    };

  }
}

#endif
