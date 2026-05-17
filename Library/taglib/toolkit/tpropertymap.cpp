/***************************************************************************
    copyright           : (C) 2012 by Michael Helmling
    email               : helmling@mathematik.uni-kl.de
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


#include "tpropertymap.h"
using namespace TagLib;


PropertyMap::PropertyMap()
{
}

PropertyMap::PropertyMap(const PropertyMap &m) : SimplePropertyMap(m), unsupported(m.unsupported)
{
}

PropertyMap::PropertyMap(const SimplePropertyMap &m)
{
  for(SimplePropertyMap::ConstIterator it = m.begin(); it != m.end(); ++it){
    String key = it->first.upper();
    if(!key.isEmpty())
      insert(it->first, it->second);
    else
      unsupported.append(it->first);
  }
}

PropertyMap::~PropertyMap()
{
}

bool PropertyMap::insert(const String &key, const StringList &values)
{
  String realKey = key.upper();
  Iterator result = SimplePropertyMap::find(realKey);
  if(result == end())
    SimplePropertyMap::insert(realKey, values);
  else
    SimplePropertyMap::operator[](realKey).append(values);
  return true;
}

bool PropertyMap::replace(const String &key, const StringList &values)
{
  String realKey = key.upper();
  SimplePropertyMap::erase(realKey);
  SimplePropertyMap::insert(realKey, values);
  return true;
}

PropertyMap::Iterator PropertyMap::find(const String &key)
{
  return SimplePropertyMap::find(key.upper());
}

PropertyMap::ConstIterator PropertyMap::find(const String &key) const
{
  return SimplePropertyMap::find(key.upper());
}

bool PropertyMap::contains(const String &key) const
{
  return SimplePropertyMap::contains(key.upper());
}

bool PropertyMap::contains(const PropertyMap &other) const
{
  for(ConstIterator it = other.begin(); it != other.end(); ++it) {
    if(!SimplePropertyMap::contains(it->first))
      return false;
    if ((*this)[it->first] != it->second)
      return false;
  }
  return true;
}

PropertyMap &PropertyMap::erase(const String &key)
{
  SimplePropertyMap::erase(key.upper());
  return *this;
}

PropertyMap &PropertyMap::erase(const PropertyMap &other)
{
  for(ConstIterator it = other.begin(); it != other.end(); ++it)
    erase(it->first);
  return *this;
}

PropertyMap &PropertyMap::merge(const PropertyMap &other)
{
  for(PropertyMap::ConstIterator it = other.begin(); it != other.end(); ++it)
    insert(it->first, it->second);
  unsupported.append(other.unsupported);
  return *this;
}

StringList PropertyMap::value(const String &key,
                                    const StringList &defaultValue) const
{
  return SimplePropertyMap::value(key.upper(), defaultValue);
}

const StringList &PropertyMap::operator[](const String &key) const
{
  return SimplePropertyMap::operator[](key.upper());
}

StringList &PropertyMap::operator[](const String &key)
{
  return SimplePropertyMap::operator[](key.upper());
}

bool PropertyMap::operator==(const PropertyMap &other) const
{
  for(ConstIterator it = other.begin(); it != other.end(); ++it) {
    ConstIterator thisFind = find(it->first);
    if( thisFind == end() || (thisFind->second != it->second) )
      return false;
  }
  for(ConstIterator it = begin(); it != end(); ++it) {
    ConstIterator otherFind = other.find(it->first);
    if( otherFind == other.end() || (otherFind->second != it->second) )
      return false;
  }
  return unsupported == other.unsupported;
}

bool PropertyMap::operator!=(const PropertyMap &other) const
{
  return !(*this == other);
}

String PropertyMap::toString() const
{
  String ret;

  for(ConstIterator it = begin(); it != end(); ++it)
    ret += it->first+"="+it->second.toString(", ") + "\n";
  if(!unsupported.isEmpty())
    ret += "Unsupported Data: " + unsupported.toString(", ") + "\n";
  return ret;
}

void PropertyMap::removeEmpty()
{
  PropertyMap m;
  for(ConstIterator it = begin(); it != end(); ++it) {
    if(!it->second.isEmpty())
      m.insert(it->first, it->second);
  }
  *this = m;
}

StringList &PropertyMap::unsupportedData()
{
  return unsupported;
}

const StringList &PropertyMap::unsupportedData() const
{
  return unsupported;
}

#ifdef _MSC_VER
// When building with shared libraries and tests, MSVC will fail with
// "already defined in test_opus.obj" as soon as operator[] of
// Ogg::FieldListMap is used because this will instantiate the same template
// Map<String, StringList>. Therefore this template is instantiated here
// and declared extern in the headers using it.
template class TagLib::Map<TagLib::String, TagLib::StringList>;
#endif
