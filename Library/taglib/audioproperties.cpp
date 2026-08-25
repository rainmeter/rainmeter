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

//#include "aiffproperties.h"
#include "apeproperties.h"
#include "asfproperties.h"
#include "flacproperties.h"
#include "mp4properties.h"
#include "mpcproperties.h"
#include "mpegproperties.h"
//#include "opusproperties.h"
//#include "speexproperties.h"
//#include "trueaudioproperties.h"
#include "vorbisproperties.h"
//#include "wavproperties.h"
//#include "wavpackproperties.h"
#include "audioproperties.h"

using namespace TagLib;

// This macro is a workaround for the fact that we can't add virtual functions.
// Should be true virtual functions in taglib2.

#define VIRTUAL_FUNCTION_WORKAROUND(function_name, default_value)               \
  if(dynamic_cast<const APE::Properties*>(this))                                \
    return dynamic_cast<const APE::Properties*>(this)->function_name();         \
  if(dynamic_cast<const ASF::Properties*>(this))                                \
    return dynamic_cast<const ASF::Properties*>(this)->function_name();         \
  if(dynamic_cast<const FLAC::Properties*>(this))                               \
    return dynamic_cast<const FLAC::Properties*>(this)->function_name();        \
  if(dynamic_cast<const MP4::Properties*>(this))                                \
    return dynamic_cast<const MP4::Properties*>(this)->function_name();         \
  if(dynamic_cast<const MPC::Properties*>(this))                                \
    return dynamic_cast<const MPC::Properties*>(this)->function_name();         \
  if(dynamic_cast<const MPEG::Properties*>(this))                               \
    return dynamic_cast<const MPEG::Properties*>(this)->function_name();        \
  if(dynamic_cast<const Vorbis::Properties*>(this))                             \
    return dynamic_cast<const Vorbis::Properties*>(this)->function_name();      \
  return (default_value);

class AudioProperties::AudioPropertiesPrivate
{

};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

AudioProperties::~AudioProperties()
{

}

int AudioProperties::lengthInSeconds() const
{
  VIRTUAL_FUNCTION_WORKAROUND(lengthInSeconds, 0)
}

int AudioProperties::lengthInMilliseconds() const
{
  VIRTUAL_FUNCTION_WORKAROUND(lengthInMilliseconds, 0)
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

AudioProperties::AudioProperties(ReadStyle) :
  d(0)
{

}
