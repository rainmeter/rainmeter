/***************************************************************************
    copyright            : (C) 2011 by Lukas Lalinsky
    email                : lalinsky@gmail.com
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

#ifdef _WIN32
# include <windows.h>
# include <tstring.h>
#endif

#include "tiostream.h"

using namespace TagLib;

#ifdef _WIN32

namespace
{
  std::wstring ansiToUnicode(const char *str)
  {
    const int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if(len == 0)
      return std::wstring();

    std::wstring wstr(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, &wstr[0], len);

    return wstr;
  }
}

// m_name is no longer used, but kept for backward compatibility.

FileName::FileName(const wchar_t *name) :
  m_name(),
  m_wname(name)
{
}

FileName::FileName(const char *name) :
  m_name(),
  m_wname(ansiToUnicode(name))
{
}

FileName::FileName(const FileName &name) :
  m_name(),
  m_wname(name.m_wname)
{
}

FileName::operator const wchar_t *() const
{
  return m_wname.c_str();
}

FileName::operator const char *() const
{
  return m_name.c_str();
}

const std::wstring &FileName::wstr() const
{
  return m_wname;
}

const std::string &FileName::str() const
{
  return m_name;
}

String FileName::toString() const
{
  return String(m_wname.c_str());
}

#endif  // _WIN32

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

IOStream::IOStream()
{
}

IOStream::~IOStream()
{
}

void IOStream::clear()
{
}

