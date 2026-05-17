/***************************************************************************
    copyright            : (C) 2013 by Tsuda Kageyu
    email                : tsuda.kageyu@gmail.com
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "trefcounter.h"

#if defined(HAVE_GCC_ATOMIC)
# define ATOMIC_INT int
# define ATOMIC_INC(x) __sync_add_and_fetch(&(x), 1)
# define ATOMIC_DEC(x) __sync_sub_and_fetch(&(x), 1)
#elif defined(HAVE_WIN_ATOMIC)
# if !defined(NOMINMAX)
#   define NOMINMAX
# endif
# include <windows.h>
# define ATOMIC_INT long
# define ATOMIC_INC(x) InterlockedIncrement(&x)
# define ATOMIC_DEC(x) InterlockedDecrement(&x)
#elif defined(HAVE_MAC_ATOMIC)
# include <libkern/OSAtomic.h>
# define ATOMIC_INT int32_t
# define ATOMIC_INC(x) OSAtomicIncrement32Barrier(&x)
# define ATOMIC_DEC(x) OSAtomicDecrement32Barrier(&x)
#elif defined(HAVE_IA64_ATOMIC)
# include <ia64intrin.h>
# define ATOMIC_INT int
# define ATOMIC_INC(x) __sync_add_and_fetch(&x, 1)
# define ATOMIC_DEC(x) __sync_sub_and_fetch(&x, 1)
#else
# define ATOMIC_INT int
# define ATOMIC_INC(x) (++x)
# define ATOMIC_DEC(x) (--x)
#endif

namespace TagLib
{

  class RefCounter::RefCounterPrivate
  {
  public:
    RefCounterPrivate() :
      refCount(1) {}

    volatile ATOMIC_INT refCount;
  };

  RefCounter::RefCounter() :
    d(new RefCounterPrivate())
  {
  }

  RefCounter::~RefCounter()
  {
    delete d;
  }

  void RefCounter::ref()
  {
    ATOMIC_INC(d->refCount);
  }

  bool RefCounter::deref()
  {
    return (ATOMIC_DEC(d->refCount) == 0);
  }

  int RefCounter::count() const
  {
    return static_cast<int>(d->refCount);
  }
}  // namespace TagLib
