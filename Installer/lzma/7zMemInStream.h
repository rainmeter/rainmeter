/* 7zMemInStream.h -- Memory input stream
** 2012 - Birunthan Mohanathas
**
** This file is public domain.
*/

#ifndef __7Z_MEMINSTREAM_H
#define __7Z_MEMINSTREAM_H

#include "Types.h"

typedef struct
{
  ILookInStream s;
  const Byte *begin;
  const Byte *pos;
  const Byte *end;
} CMemInStream;

void MemInStream_Init(CMemInStream *p, const void *begin, size_t length);

#endif
