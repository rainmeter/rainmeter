/* 7zMemInStream.c -- Memory input stream
** 2012 - Birunthan Mohanathas
**
** This file is public domain.
*/

#include "7zMemInStream.h"

static SRes MemInStream_Look(void *pp, const void **buf, size_t *size)
{
  CMemInStream *p = (CMemInStream *)pp;
  size_t remaining = p->end - p->pos;
  if (remaining == 0 && *size > 0)
  {
    // Restart stream.
    p->pos = 0;
    remaining = *size;
  }

  if (remaining < *size)
  {
    *size = remaining;
  }

  *buf = p->pos;
  return SZ_OK;
}

static SRes MemInStream_Skip(void *pp, size_t offset)
{
  CMemInStream *p = (CMemInStream *)pp;
  p->pos += offset;
  return SZ_OK;
}

static SRes MemInStream_Read(void *pp, void *buf, size_t *size)
{
  CMemInStream *p = (CMemInStream *)pp;
  size_t remaining = p->end - p->pos;
  if (remaining == 0)
  {
    // End of stream.
    *size = 0;
  }
  else
  {
    if (remaining > *size)
    {
      remaining = *size;
    }

    memcpy(buf, p->pos, remaining);
    p->pos += remaining;
    *size = remaining;
  }

  return SZ_OK;
}

static SRes MemInStream_Seek(void *pp, Int64 *pos, ESzSeek origin)
{
  CMemInStream *p = (CMemInStream *)pp;
  switch (origin)
  {
    case SZ_SEEK_SET: p->pos = p->begin + *pos; break;
    case SZ_SEEK_CUR: p->pos += *pos; break;
    case SZ_SEEK_END: p->pos = p->end - *pos; break;
    default: return 1;
  }

  *pos = p->pos - p->begin;
  return SZ_OK;
}

void MemInStream_Init(CMemInStream *p, const void *begin, size_t length)
{
  p->begin = p->pos = (Byte *)begin;
  p->end = p->begin + length;

  p->s.Look = MemInStream_Look;
  p->s.Skip = MemInStream_Skip;
  p->s.Read = MemInStream_Read;
  p->s.Seek = MemInStream_Seek;
}
