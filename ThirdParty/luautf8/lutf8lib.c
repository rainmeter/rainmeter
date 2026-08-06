/* vim: set ft=c nu et sw=2 fdc=2 fdm=syntax : */
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>

#include "unidata.h"

/* UTF-8 string operations */

#define LUTF8_VERSION "0.2.1"

#define UTF8_BUFFSZ 8
#define UTF8_MAX    0x7FFFFFFFu
#define UTF8_MAXCP  0x10FFFFu
#define iscontp(p)    ((*(p) & 0xC0) == 0x80)
#define CAST(tp,expr) ((tp)(expr))

#ifndef LUA_QL
# define LUA_QL(x) "'" x "'"
#endif

static int utf8_invalid (utfint ch)
{ return (ch > UTF8_MAXCP || (0xD800u <= ch && ch <= 0xDFFFu)); }

static size_t utf8_encode (char *buff, utfint x) {
  int n = 1;  /* number of bytes put in buffer (backwards) */
  assert(x <= UTF8_MAX);
  if (x < 0x80)  /* ascii? */
    buff[UTF8_BUFFSZ - 1] = x & 0x7F;
  else {  /* need continuation bytes */
    utfint mfb = 0x3f;  /* maximum that fits in first byte */
    do {  /* add continuation bytes */
      buff[UTF8_BUFFSZ - (n++)] = 0x80 | (x & 0x3f);
      x >>= 6;  /* remove added bits */
      mfb >>= 1;  /* now there is one less bit available in first byte */
    } while (x > mfb);  /* still needs continuation byte? */
    buff[UTF8_BUFFSZ - n] = ((~mfb << 1) | x) & 0xFF;  /* add first byte */
  }
  return n;
}

static const char *utf8_decode (const char *s, utfint *val, int strict) {
  static const utfint limits[] =
  {~0u, 0x80u, 0x800u, 0x10000u, 0x200000u, 0x4000000u};
  unsigned int c = (unsigned char)s[0];
  utfint res = 0;  /* final result */
  if (c < 0x80)  /* ascii? */
    res = c;
  else {
    int count = 0;  /* to count number of continuation bytes */
    for (; c & 0x40; c <<= 1) {  /* while it needs continuation bytes... */
      unsigned int cc = (unsigned char)s[++count];  /* read next byte */
      if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
        return NULL;  /* invalid byte sequence */
      res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
    }
    res |= ((utfint)(c & 0x7F) << (count * 5));  /* add first byte */
    if (count > 5 || res > UTF8_MAX || res < limits[count])
      return NULL;  /* invalid byte sequence */
    s += count;  /* skip continuation bytes read */
  }
  if (strict) {
    /* check for invalid code points; too large or surrogates */
    if (res > UTF8_MAXCP || (0xD800u <= res && res <= 0xDFFFu))
      return NULL;
  }
  if (val) *val = res;
  return s + 1;  /* +1 to include first byte */
}

static const char *utf8_prev (const char *s, const char *e) {
  while (s < e && iscontp(e - 1)) --e;
  return s < e ? e - 1 : s;
}

static const char *utf8_next (const char *s, const char *e) {
  while (s < e && iscontp(s + 1)) ++s;
  return s < e ? s + 1 : e;
}

static size_t utf8_length (const char *s, const char *e) {
  size_t i;
  for (i = 0; s < e; ++i)
    s = utf8_next(s, e);
  return i;
}

static const char *utf8_offset (const char *s, const char *e, lua_Integer offset, lua_Integer idx) {
  const char *p = s + offset - 1;
  if (idx >= 0) {
    while (p < e && idx > 0)
      p = utf8_next(p, e), --idx;
    return idx == 0 ? p : NULL;
  } else {
    while (s < p && idx < 0)
      p = utf8_prev(s, p), ++idx;
    return idx == 0 ? p : NULL;
  }
}

static const char *utf8_relat (const char *s, const char *e, int idx) {
  return idx >= 0 ?
    utf8_offset(s, e, 1, idx - 1) :
    utf8_offset(s, e, e-s+1, idx);
}

static int utf8_range(const char *s, const char *e, lua_Integer *i, lua_Integer *j) {
  const char *ps = utf8_relat(s, e, CAST(int, *i));
  const char *pe = utf8_relat(s, e, CAST(int, *j));
  *i = (ps ? ps : (*i > 0 ? e : s)) - s;
  *j = (pe ? utf8_next(pe, e) : (*j > 0 ? e : s)) - s;
  return *i < *j;
}

/* Indexed by top nibble of first byte in code unit */
static uint8_t utf8_code_unit_len[] = {
  1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 2, 2, 3, 4
};

/* Return pointer to first invalid UTF-8 sequence in 's', or NULL if valid */
static const char *utf8_invalid_offset(const char *s, const char *e) {
  while (s < e) {
    uint8_t c = *s;
    if (c >= 0x80) {
      /* c < 0xC0 means a continuation byte, but we are not in the middle of a multi-byte code unit
       * c >= 0xC0 && c < 0xC2 means an overlong 2-byte code unit
       * c >= 0xF8 means a 5-byte or 6-byte code unit, which is illegal, or else illegal byte 0xFE/0xFF
       * c >= 0xF5 && c < 0xF8 means a 4-byte code unit encoding invalid codepoint > U+10FFFF */
      if (c < 0xC2 || c >= 0xF5)
        return s;
      uint8_t needed_bytes = utf8_code_unit_len[c >> 4];
      if (e - s < needed_bytes)
        return s; /* String is truncated */
      uint8_t c2 = *(s+1);
      if ((c2 & 0xC0) != 0x80)
        return s; /* 2nd byte of code unit is not a continuation byte */
      if (needed_bytes >= 3) {
        uint8_t c3 = *(s+2);
        if ((c3 & 0xC0) != 0x80)
          return s; /* 3rd byte of code unit is not a continuation byte */
        if (needed_bytes == 3) {
          if (c == 0xE0 && c2 < 0xA0)
            return s; /* Overlong 3-byte code unit */
          if (c == 0xED && c2 >= 0xA0)
            return s; /* Reserved codepoint from U+D800-U+DFFF */
        } else {
          uint8_t c4 = *(s+3);
          if ((c4 & 0xC0) != 0x80)
            return s; /* 4th byte of code unit is not a continuation byte */
          if (c == 0xF0 && c2 < 0x90)
            return s; /* Overlong 4-byte code unit */
          if (c == 0xF4 && c2 >= 0x90)
            return s; /* Illegal codepoint > U+10FFFF */
        }
      }
      s += needed_bytes;
    } else {
      s++;
    }
  }
  return NULL;
}

/* Unicode character categories */

#define table_size(t) (sizeof(t)/sizeof((t)[0]))

#define utf8_categories(X) \
  X('a', alpha) \
  X('c', cntrl) \
  X('d', digit) \
  X('l', lower) \
  X('p', punct) \
  X('s', space) \
  X('t', compose) \
  X('u', upper) \
  X('x', xdigit)

#define utf8_converters(X) \
  X(lower) \
  X(upper) \
  X(title) \
  X(fold)

static int find_in_range (range_table *t, size_t size, utfint ch) {
  size_t begin, end;

  begin = 0;
  end = size;

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    if (t[mid].last < ch)
      begin = mid + 1;
    else if (t[mid].first > ch)
      end = mid;
    else
      return (ch - t[mid].first) % t[mid].step == 0;
  }

  return 0;
}

static int convert_char (conv_table *t, size_t size, utfint ch) {
  size_t begin, end;

  begin = 0;
  end = size;

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    if (t[mid].last < ch)
      begin = mid + 1;
    else if (t[mid].first > ch)
      end = mid;
    else if ((ch - t[mid].first) % t[mid].step == 0)
      return ch + t[mid].offset;
    else
      return ch;
  }

  return ch;
}

/* Normalization */

static int lookup_canon_cls (utfint ch) {
  /* The first codepoint with canonicalization class != 0 is U+0300 COMBINING GRAVE ACCENT */
  if (ch < 0x300) {
    return 0;
  }
  size_t begin = 0, end = table_size(nfc_combining_table);

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    if (nfc_combining_table[mid].last < ch)
      begin = mid + 1;
    else if (nfc_combining_table[mid].first > ch)
      end = mid;
    else
      return nfc_combining_table[mid].canon_cls;
  }

  return 0;
}

static nfc_table *nfc_quickcheck (utfint ch) {
  /* The first character which needs to be checked for possible NFC violations
   * is U+0300 COMBINING GRAVE ACCENT */
  size_t begin = 0, end;
  if (ch < 0x300) return NULL;
  end = table_size(nfc_quickcheck_table);

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    utfint found = nfc_quickcheck_table[mid].cp;
    if (found < ch)
      begin = mid + 1;
    else if (found > ch)
      end = mid;
    else
      return &nfc_quickcheck_table[mid];
  }

  return NULL;
}

static int nfc_combine (utfint cp1, utfint cp2, utfint *dest) {
  size_t begin = 0, end = table_size(nfc_composite_table);
  unsigned int hash = (cp1 * 213) + cp2;

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    utfint val = nfc_composite_table[mid].hash;
    if (val < hash) {
      begin = mid + 1;
    } else if (val > hash) {
      end = mid;
    } else if (nfc_composite_table[mid].cp1 == cp1 && nfc_composite_table[mid].cp2 == cp2) {
      if (dest)
        *dest = nfc_composite_table[mid].dest;
      return 1;
    } else {
      return 0;
    }
  }

  return 0;
}

static decompose_table *nfc_decompose (utfint ch) {
  size_t begin = 0, end = table_size(nfc_decompose_table);

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    utfint found = nfc_decompose_table[mid].cp;
    if (found < ch)
      begin = mid + 1;
    else if (found > ch)
      end = mid;
    else
      return &nfc_decompose_table[mid];
  }

  return NULL;
}

static int nfc_check (utfint ch, nfc_table *entry, utfint starter, unsigned int canon_cls, unsigned int prev_canon_cls) {
  int reason = entry->reason;

  if (reason == REASON_MUST_CONVERT_1 || reason == REASON_MUST_CONVERT_2) {
    /* This codepoint has a different, canonical form, so this string is not NFC */
    return 0;
  } else if (reason == REASON_STARTER_CAN_COMBINE) {
    /* It is possible that this 'starter' codepoint should have been combined with the
     * preceding 'starter' codepoint; if so, this string is not NFC */
    if (!prev_canon_cls && nfc_combine(starter, ch, NULL)) {
      /* These codepoints should have been combined */
      return 0;
    }
  } else if (reason == REASON_COMBINING_MARK) {
    /* Combining mark; check if it should have been combined with preceding starter codepoint */
    if (canon_cls <= prev_canon_cls) {
      return 1;
    }
    if (nfc_combine(starter, ch, NULL)) {
      /* Yes, they should have been combined. This string is not NFC */
      return 0;
    }
    /* Could it be that preceding 'starter' codepoint is already combined, but with a
     * combining mark which is out of order with this one? */
    decompose_table *decomp = nfc_decompose(starter);
    if (decomp) {
      if (decomp->canon_cls2 > canon_cls && nfc_combine(decomp->to1, ch, NULL)) {
        return 0;
      } else {
        decompose_table *decomp2 = nfc_decompose(decomp->to1);
        if (decomp2 && decomp2->canon_cls2 > canon_cls && nfc_combine(decomp2->to1, ch, NULL)) {
          return 0;
        }
      }
    }
  } else if (reason == REASON_JAMO_VOWEL) {
    if (!prev_canon_cls && starter >= 0x1100 && starter <= 0x1112) {
      /* Preceding codepoint was a leading jamo; they should have been combined */
      return 0;
    }
  } else if (reason == REASON_JAMO_TRAILING) {
    if (!prev_canon_cls && starter >= 0xAC00 && starter <= 0xD7A3) {
      /* Preceding codepoint was a precomposed Hangul syllable; check if it had no trailing jamo */
      if ((starter - 0xAC00) % 28 == 0) {
        /* It didn't have a trailing jamo, so this trailing jamo should have been combined */
        return 0;
      }
    }
  }

  return 1;
}

static void merge_combining_marks (uint32_t *src1, uint32_t *src2, uint32_t *dest, size_t size1, size_t size2) {
  while (size1 && size2) {
    if ((*src1 & 0xFF) > (*src2 & 0xFF)) {
      *dest++ = *src2++;
      size2--;
    } else {
      *dest++ = *src1++;
      size1--;
    }
  }
  while (size1) {
    *dest++ = *src1++;
    size1--;
  }
  while (size2) {
    *dest++ = *src2++;
    size2--;
  }
}

static void stable_sort_combining_marks (uint32_t *vector, uint32_t *scratch, size_t size) {
  /* We need to use a stable sort for sorting combining marks which are in the wrong order
   * when doing NFC normalization; bottom-up merge sort is fast and stable */
  size_t limit = size - 1;
  for (unsigned int i = 0; i < limit; i += 2) {
    if ((vector[i] & 0xFF) > (vector[i+1] & 0xFF)) {
      uint32_t temp = vector[i];
      vector[i] = vector[i+1];
      vector[i+1] = temp;
    }
  }
  if (size <= 2)
    return;

  uint32_t *src = vector, *dest = scratch;
  unsigned int runsize = 2; /* Every consecutive slice of this size is sorted */
  while (runsize < size) {
    unsigned int blocksize = runsize * 2; /* We will now sort slices of this size */
    limit = size & ~(blocksize - 1);
    for (unsigned int i = 0; i < limit; i += blocksize)
      merge_combining_marks(&src[i], &src[i+runsize], &dest[i], runsize, runsize);
    if (size - limit > runsize) {
      merge_combining_marks(&src[limit], &src[limit+runsize], &dest[limit], runsize, size - limit - runsize);
    } else {
      memcpy(&dest[limit], &src[limit], (size - limit) * sizeof(uint32_t));
    }
    /* After each series of (progressively larger) merges, we swap src & dest to
     * avoid memcpy'ing the partially sorted results from dest back into src */
    uint32_t *temp = src; src = dest; dest = temp;
    runsize = blocksize;
  }

  if (dest == vector) {
    /* Since src & dest are swapped on each iteration of the above loop,
     * this actually means the last buffer which was written into
     * was 'scratch' */
    memcpy(vector, scratch, size * sizeof(uint32_t));
  }
}

/* Shuffle item `i` up or down to get it into the right position */
static void stable_insert_combining_mark (uint32_t *vector, size_t vec_size, unsigned int i)
{
  unsigned int item = vector[i];
  unsigned int canon_cls = item & 0xFF;
  if (i > 0) {
    if (canon_cls < (vector[i-1] & 0xFF)) {
      do {
        vector[i] = vector[i-1];
        i--;
      } while (i > 0 && canon_cls < (vector[i-1] & 0xFF));
      vector[i] = item;
      return;
    }
  }
  if (i < vec_size-1) {
    if (canon_cls > (vector[i+1] & 0xFF)) {
      do {
        vector[i] = vector[i+1];
        i++;
      } while (i < vec_size-1 && canon_cls > (vector[i+1] & 0xFF));
      vector[i] = item;
      return;
    }
  }
}

static void add_utf8char (luaL_Buffer *b, utfint ch);

static inline void grow_vector_if_needed (uint32_t **vector, uint32_t *onstack, size_t *size, size_t needed)
{
  size_t current_size = *size;
  if (needed >= current_size) {
    size_t new_size = current_size * 2; /* `needed` is never bigger than `current_size * 2` */
    uint32_t *new_vector = malloc(new_size * sizeof(uint32_t));
    memcpy(new_vector, *vector, current_size * sizeof(uint32_t));
    *size = new_size;
    if (*vector != onstack) free(*vector);
    *vector = new_vector;
  }
}

static void string_to_nfc (lua_State *L, luaL_Buffer *buff, const char *s, const char *e)
{
  /* Converting a string to Normal Form C involves:
   * 1) Ensuring that codepoints with "built-in" accents are used whenever possible
   *    rather than separate codepoints for a base character and combining mark
   * 2) Where combining marks must be used, putting them into canonical order
   * 3) Converting some deprecated codepoints to the recommended variant
   * 4) Ensuring that Korean Hangul are represented as precomposed syllable
   *    codepoints whenever possible, rather than sequences of Jamo codepoints
   *
   * (Combining marks are accents which appear on top of or below the preceding
   * character. Starter codepoints are the base characters which combining marks can
   * 'combine' with. Almost all codepoints are starters, including all the Latin alphabet.
   * Every Unicode codepoint has a numeric 'canonicalization class'; starters have class = 0.
   * Combining marks must be sorted in order of their canonicalization class. Since the
   * canonicalization class numbers are not unique, the sort must be stable.)
   *
   * When converting to NFC, the largest scope which we need to work on at once
   * consists of a 'starter' codepoint and either 1 or more ensuing combining marks,
   * OR else a directly following starter codepoint.
   *
   * As we walk through the string, whenever we pass by a complete sequence of starter +
   * combining marks or starter + starter, we process that sequence to see if it is NFC or not.
   * If it is, we memcpy the bytes verbatim into the output buffer. If it is not, then we
   * convert the codepoints to NFC and then emit those codepoints as UTF-8 bytes. */

  utfint starter = (utfint)-1, ch; /* 'starter' is last starter codepoint seen */
  const char *to_copy = s; /* pointer to next bytes we might need to memcpy into output buffer */
  unsigned int prev_canon_cls = 0;
  int fixedup = 0; /* has the sequence currently under consideration been modified to make it NFC? */

  /* Temporary storage for a sequence of consecutive combining marks
   * In the vast majority of cases, this small on-stack array will provide enough
   * space; if not, we will switch to a malloc'd buffer */
  uint32_t onstack[8];
  size_t vec_size = 0, vec_max = sizeof(onstack)/sizeof(uint32_t);
  uint32_t *vector = onstack;
  nfc_table *entry = NULL;

  while (s < e) {
    const char *new_s = utf8_decode(s, &ch, 1);
    if (new_s == NULL) {
      if (vector != onstack) free(vector);
      luaL_error(L, "string is not valid UTF-8");
    }
    unsigned int canon_cls = lookup_canon_cls(ch);

    if (!canon_cls) {
      /* This is a starter codepoint */
      entry = nfc_quickcheck(ch);

      /* But in rare cases, a deprecated 'starter' codepoint may convert
       * to combining marks instead!
       * Why, oh why, did the Unicode Consortium do this?? */
      if (entry && entry->reason == REASON_MUST_CONVERT_2) {
        utfint conv1 = entry->data1;
        unsigned int canon_cls1 = lookup_canon_cls(conv1);
        if (canon_cls1) {
          utfint conv2 = entry->data2;
          unsigned int canon_cls2 = lookup_canon_cls(conv2);
          grow_vector_if_needed(&vector, onstack, &vec_max, vec_size + 2);
          vector[vec_size++] = (conv1 << 8) | (canon_cls1 & 0xFF);
          vector[vec_size++] = (conv2 << 8) | (canon_cls2 & 0xFF);
          s = new_s;
          prev_canon_cls = canon_cls2;
          fixedup = 1;
          continue;
        }
      }

      /* Handle preceding starter and optional sequence of combining marks which may have followed it */
      if (prev_canon_cls) {
        /* Before this starter, there was a sequence of combining marks.
         * Check those over and emit output to 'buff' */
process_combining_marks:

        /* Check if accumulated combining marks were in correct order */
        for (unsigned int i = 1; i < vec_size; i++) {
          if ((vector[i-1] & 0xFF) > (vector[i] & 0xFF)) {
            /* Order is incorrect, we need to sort */
            uint32_t *scratch = malloc(vec_size * sizeof(uint32_t));
            stable_sort_combining_marks(vector, scratch, vec_size);
            free(scratch);
            fixedup = 1;
            break;
          }
        }

        /* Check if any of those combining marks are in violation of NFC */
        unsigned int i = 0;
        while (i < vec_size) {
          utfint combine_mark = vector[i] >> 8;
          nfc_table *mark_entry = nfc_quickcheck(combine_mark);
          if (mark_entry) {
            if (mark_entry->reason == REASON_MUST_CONVERT_1) {
              /* This combining mark must be converted to a different one */
              vector[i] = (mark_entry->data1 << 8) | mark_entry->data2;
              fixedup = 1;
              continue;
            } else if (mark_entry->reason == REASON_MUST_CONVERT_2) {
              /* This combining mark must be converted to two others */
              grow_vector_if_needed(&vector, onstack, &vec_max, vec_size + 1);
              memmove(&vector[i+2], &vector[i+1], sizeof(uint32_t) * (vec_size - i - 1));
              vector[i] = (mark_entry->data1 << 8) | lookup_canon_cls(mark_entry->data1);
              vector[i+1] = (mark_entry->data2 << 8) | lookup_canon_cls(mark_entry->data2);
              vec_size++;
              fixedup = 1;
              continue;
            } else if (mark_entry->reason == REASON_COMBINING_MARK) {
              unsigned int mark_canon_cls = vector[i] & 0xFF;
              if (i == 0 || mark_canon_cls > (vector[i-1] & 0xFF)) {
                if (nfc_combine(starter, combine_mark, &starter)) {
                  /* This combining mark must be combined with preceding starter */
                  vec_size--;
                  memmove(&vector[i], &vector[i+1], sizeof(uint32_t) * (vec_size - i)); /* Remove element i */
                  fixedup = 1;
                  continue;
                }

                decompose_table *decomp = nfc_decompose(starter);
                if (decomp) {
                  if (decomp->canon_cls2 > mark_canon_cls && nfc_combine(decomp->to1, combine_mark, &starter)) {
                    /* The preceding starter already included an accent, but when represented as a combining
                     * mark, that accent has a HIGHER canonicalization class than this one
                     * Further, this one is able to combine with the same base character
                     * In other words, the base character was wrongly combined with a "lower-priority"
                     * combining mark; fix that up */
                    unsigned int class2 = lookup_canon_cls(decomp->to2);
                    memmove(&vector[1], &vector[0], sizeof(uint32_t) * i);
                    vector[0] = (decomp->to2 << 8) | class2;
                    stable_insert_combining_mark(vector, vec_size, 0);
                    fixedup = 1;
                    continue;
                  } else {
                    decompose_table *decomp2 = nfc_decompose(decomp->to1);
                    if (decomp2 && decomp2->canon_cls2 > mark_canon_cls && nfc_combine(decomp2->to1, combine_mark, &starter)) {
                      grow_vector_if_needed(&vector, onstack, &vec_max, vec_size + 1);
                      memmove(&vector[i+2], &vector[i+1], sizeof(uint32_t) * (vec_size - i - 1));
                      memmove(&vector[2], &vector[0], sizeof(uint32_t) * i);
                      vector[0] = (decomp2->to2 << 8) | lookup_canon_cls(decomp2->to2);
                      vector[1] = (decomp->to2 << 8) | lookup_canon_cls(decomp->to2);
                      vec_size++;
                      stable_insert_combining_mark(vector, vec_size, 1);
                      stable_insert_combining_mark(vector, vec_size, 0);
                      fixedup = 1;
                      continue;
                    }
                  }
                }
              }
            }
          }
          i++;
        }

        if (fixedup) {
          /* The preceding starter/combining mark sequence was bad; convert fixed-up codepoints
           * to UTF-8 bytes */
          if (starter != (utfint)-1)
            add_utf8char(buff, starter);
          for (unsigned int i = 0; i < vec_size; i++)
            add_utf8char(buff, vector[i] >> 8);
        } else {
          /* The preceding starter/combining mark sequence was good; copy raw bytes to output */
          luaL_addlstring(buff, to_copy, s - to_copy);
        }
        if (s >= e) {
          /* We jumped in to the middle of the main loop to finish processing trailing
           * combining marks... we are actually done now */
          if (vector != onstack)
            free(vector);
          return;
        }
        vec_size = 0; /* Clear vector of combining marks in readiness for next such sequence */
        fixedup = 0;
      } else if (starter != (utfint)-1) {
        /* This starter was preceded immediately by another starter
         * Check if this one should combine with it */
        fixedup = 0;
        if (entry) {
          if (entry->reason == REASON_STARTER_CAN_COMBINE && nfc_combine(starter, ch, &ch)) {
            fixedup = 1;
          } else if (entry->reason == REASON_JAMO_VOWEL && starter >= 0x1100 && starter <= 0x1112) {
            ch = 0xAC00 + ((starter - 0x1100) * 588) + ((ch - 0x1161) * 28);
            fixedup = 1;
          } else if (entry->reason == REASON_JAMO_TRAILING) {
            if (starter >= 0xAC00 && starter <= 0xD7A3 && (starter - 0xAC00) % 28 == 0) {
              ch = starter + ch - 0x11A7;
              fixedup = 1;
            }
          }
        }
        if (!fixedup)
          add_utf8char(buff, starter); /* Emit previous starter to output */
      }
      starter = ch;
      to_copy = s;

      /* We are finished processing the preceding starter and optional sequence of combining marks
       * Now check if this (possibly deprecated) starter needs to be converted to a canonical variant */
      if (entry) {
        if (entry->reason == REASON_MUST_CONVERT_1) {
          starter = entry->data1;
          fixedup = 1;
        } else if (entry->reason == REASON_MUST_CONVERT_2) {
          utfint conv1 = entry->data1;
          utfint conv2 = entry->data2;
          /* It's possible that 'ch' might convert to two other codepoints,
           * where the 2nd one is a combining mark */
          unsigned int canon_cls2 = lookup_canon_cls(conv2);
          if (canon_cls2) {
            /* It's possible that the 1st resulting codepoint may need to be
             * split again into more codepoints */
            nfc_table *conv_entry = nfc_quickcheck(conv1);
            if (conv_entry && conv_entry->reason == REASON_MUST_CONVERT_2) {
              utfint conv3 = conv2;
              unsigned int canon_cls3 = canon_cls2;
              conv1 = conv_entry->data1;
              conv2 = conv_entry->data2;
              canon_cls2 = lookup_canon_cls(conv2);
              if (canon_cls2) {
                starter = conv1;
                vector[0] = (conv2 << 8) | canon_cls2;
                vector[1] = (conv3 << 8) | canon_cls3;
                vec_size = 2;
              } else {
                add_utf8char(buff, conv1);
                starter = conv2;
                vector[0] = (conv3 << 8) | canon_cls3;
                vec_size = 1;
              }
              canon_cls = canon_cls3;
            } else {
              starter = conv1;
              vector[0] = (conv2 << 8) | canon_cls2;
              vec_size = 1;
              canon_cls = canon_cls2;
            }
          } else {
            add_utf8char(buff, conv1);
            starter = conv2;
          }
          fixedup = 1;
        }
      }
    } else {
      /* Accumulate combining marks in vector */
      grow_vector_if_needed(&vector, onstack, &vec_max, vec_size + 1);
      vector[vec_size++] = (ch << 8) | (canon_cls & 0xFF);
    }

    s = new_s;
    prev_canon_cls = canon_cls;
  }

  if (vec_size)
    goto process_combining_marks; /* Finish processing trailing combining marks */
  if (starter != (utfint)-1)
    add_utf8char(buff, starter);

  if (vector != onstack) free(vector);
}

/* Grapheme cluster support */

static int hangul_type (utfint ch) {
  /* The first Hangul codepoint is U+1100 */
  if (ch < 0x1100) {
    return 0;
  }
  size_t begin = 0, end = table_size(hangul_table);

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    if (hangul_table[mid].last < ch)
      begin = mid + 1;
    else if (hangul_table[mid].first > ch)
      end = mid;
    else
      return hangul_table[mid].type;
  }

  return 0;
}

static int indic_conjunct_type (utfint ch) {
  /* The first Indic conjunct codepoint is U+0300 */
  if (ch < 0x300) {
    return 0;
  }
  size_t begin = 0, end = table_size(indic_table);

  while (begin < end) {
    size_t mid = (begin + end) / 2;
    if (indic_table[mid].last < ch)
      begin = mid + 1;
    else if (indic_table[mid].first > ch)
      end = mid;
    else
      return indic_table[mid].type;
  }

  return 0;
}

#define define_category(cls, name) static int utf8_is##name (utfint ch)\
{ return find_in_range(name##_table, table_size(name##_table), ch); }
#define define_converter(name) static utfint utf8_to##name (utfint ch) \
{ return convert_char(to##name##_table, table_size(to##name##_table), ch); }
utf8_categories(define_category)
utf8_converters(define_converter)
#undef define_category
#undef define_converter

static int utf8_isgraph (utfint ch) {
  if (find_in_range(space_table, table_size(space_table), ch))
    return 0;
  if (find_in_range(graph_table, table_size(graph_table), ch))
    return 1;
  if (find_in_range(compose_table, table_size(compose_table), ch))
    return 1;
  return 0;
}

static int utf8_isalnum (utfint ch) {
  if (find_in_range(alpha_table, table_size(alpha_table), ch))
    return 1;
  if (find_in_range(alnum_extend_table, table_size(alnum_extend_table), ch))
    return 1;
  return 0;
}

static int utf8_width (utfint ch, int ambiwidth, int default_width) {
  if (find_in_range(unprintable_table, table_size(unprintable_table), ch))
    return default_width;
  if (find_in_range(compose_table, table_size(compose_table), ch))
    return default_width;
  if (find_in_range(doublewidth_table, table_size(doublewidth_table), ch))
    return 2;
  if (find_in_range(ambiwidth_table, table_size(ambiwidth_table), ch))
    return ambiwidth;
  return 1;
}

/* string module compatible interface */

static int typeerror (lua_State *L, int idx, const char *tname)
{ return luaL_error(L, "%s expected, got %s", tname, luaL_typename(L, idx)); }

static const char *check_utf8 (lua_State *L, int idx, const char **end) {
  size_t len;
  const char *s = luaL_checklstring(L, idx, &len);
  if (end) *end = s+len;
  return s;
}

static const char *to_utf8 (lua_State *L, int idx, const char **end) {
  size_t len;
  const char *s = lua_tolstring(L, idx, &len);
  if (end) *end = s+len;
  return s;
}

static const char *utf8_safe_decode (lua_State *L, const char *p, utfint *pval) {
  p = utf8_decode(p, pval, 0);
  if (p == NULL) luaL_error(L, "invalid UTF-8 code");
  return p;
}

static void add_utf8char (luaL_Buffer *b, utfint ch) {
  char buff[UTF8_BUFFSZ];
  size_t n = utf8_encode(buff, ch);
  luaL_addlstring(b, buff+UTF8_BUFFSZ-n, n);
}

static lua_Integer byte_relat (lua_Integer pos, size_t len) {
  if (pos >= 0) return pos;
  else if (0u - (size_t)pos > len) return 0;
  else return (lua_Integer)len + pos + 1;
}

static void check_byte_range(lua_State *L, size_t len, lua_Integer *posi, lua_Integer *posj) {
  luaL_argcheck(L, 1 <= *posi && --*posi <= (lua_Integer)len, 2,
      "initial position out of bounds");
  luaL_argcheck(L, --*posj < (lua_Integer)len, 3,
      "final position out of bounds");
}

static int Lutf8_len (lua_State *L) {
  size_t len, n;
  const char *s = luaL_checklstring(L, 1, &len), *p, *e;
  lua_Integer posi = byte_relat(luaL_optinteger(L, 2, 1), len);
  lua_Integer posj = byte_relat(luaL_optinteger(L, 3, len), len);
  int lax = lua_toboolean(L, 4);
  check_byte_range(L, len, &posi, &posj);
  for (n = 0, p=s+posi, e=s+posj+1; p < e; ++n) {
    if (lax)
      p = utf8_next(p, e);
    else {
      utfint ch;
      const char *np = utf8_decode(p, &ch, !lax);
      if (np == NULL || utf8_invalid(ch)) {
        lua_pushnil(L);
        lua_pushinteger(L, p - s + 1);
        return 2;
      }
      p = np;
    }
  }
  lua_pushinteger(L, n);
  return 1;
}

static int Lutf8_sub (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  lua_Integer posi = luaL_checkinteger(L, 2);
  lua_Integer posj = luaL_optinteger(L, 3, -1);
  if (utf8_range(s, e, &posi, &posj))
    lua_pushlstring(L, s+posi, posj-posi);
  else
    lua_pushliteral(L, "");
  return 1;
}

static int Lutf8_reverse (lua_State *L) {
  luaL_Buffer b;
  const char *prev, *pprev, *ends, *e, *s = check_utf8(L, 1, &e);
  (void) ends;
  int lax = lua_toboolean(L, 2);
  luaL_buffinit(L, &b);
  if (lax) {
    for (prev = e; s < prev; e = prev) {
      prev = utf8_prev(s, prev);
      luaL_addlstring(&b, prev, e-prev);
    }
  } else {
    for (prev = e; s < prev; prev = pprev) {
      utfint code = 0;
      ends = utf8_safe_decode(L, pprev = utf8_prev(s, prev), &code);
      assert(ends == prev);
      if (utf8_invalid(code))
        return luaL_error(L, "invalid UTF-8 code");
      if (!utf8_iscompose(code)) {
        luaL_addlstring(&b, pprev, e-pprev);
        e = pprev;
      }
    }
  }
  luaL_pushresult(&b);
  return 1;
}

static int Lutf8_byte (lua_State *L) {
  size_t n = 0;
  const char *e, *s = check_utf8(L, 1, &e);
  lua_Integer posi = luaL_optinteger(L, 2, 1);
  lua_Integer posj = luaL_optinteger(L, 3, posi);
  if (utf8_range(s, e, &posi, &posj)) {
    for (e = s + posj, s = s + posi; s < e; ++n) {
      utfint ch = 0;
      s = utf8_safe_decode(L, s, &ch);
      lua_pushinteger(L, ch);
    }
  }
  return CAST(int, n);
}

static int Lutf8_codepoint (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  size_t len = e-s;
  lua_Integer posi = byte_relat(luaL_optinteger(L, 2, 1), len);
  lua_Integer posj = byte_relat(luaL_optinteger(L, 3, posi), len);
  int lax = lua_toboolean(L, 4);
  int n;
  const char *se;
  luaL_argcheck(L, posi >= 1, 2, "out of bounds");
  luaL_argcheck(L, posj <= (lua_Integer)len, 3, "out of bounds");
  if (posi > posj) return 0;  /* empty interval; return no values */
  if (posj - posi >= INT_MAX)  /* (lua_Integer -> int) overflow? */
    return luaL_error(L, "string slice too long");
  n = (int)(posj -  posi + 1);
  luaL_checkstack(L, n, "string slice too long");
  n = 0;  /* count the number of returns */
  se = s + posj;  /* string end */
  for (n = 0, s += posi - 1; s < se;) {
    utfint code = 0;
    s = utf8_safe_decode(L, s, &code);
    if (!lax && utf8_invalid(code))
      return luaL_error(L, "invalid UTF-8 code");
    lua_pushinteger(L, code);
    n++;
  }
  return n;
}

static int Lutf8_char (lua_State *L) {
  int i, n = lua_gettop(L); /* number of arguments */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (i = 1; i <= n; ++i) {
    utfint code = (utfint)luaL_checkinteger(L, i);
    luaL_argcheck(L, code <= UTF8_MAXCP, i, "value out of range");
    add_utf8char(&b, CAST(utfint, code));
  }
  luaL_pushresult(&b);
  return 1;
}

#define bind_converter(name)                                   \
static int Lutf8_##name (lua_State *L) {                        \
  int t = lua_type(L, 1);                                      \
  if (t == LUA_TNUMBER)                                        \
    lua_pushinteger(L, utf8_to##name(CAST(utfint, lua_tointeger(L, 1))));    \
  else if (t == LUA_TSTRING) {                                 \
    luaL_Buffer b;                                             \
    const char *e, *s = to_utf8(L, 1, &e);                     \
    luaL_buffinit(L, &b);                                      \
    while (s < e) {                                            \
      utfint ch = 0;                                             \
      s = utf8_safe_decode(L, s, &ch);                         \
      add_utf8char(&b, utf8_to##name(ch));                     \
    }                                                          \
    luaL_pushresult(&b);                                       \
  }                                                            \
  else return typeerror(L, 1, "number/string");                \
  return 1;                                                    \
}
utf8_converters(bind_converter)
#undef bind_converter


/* unicode extra interface */

static const char *parse_escape (lua_State *L, const char *s, const char *e, int hex, utfint *pch) {
  utfint code = 0;
  int in_bracket = 0;
  if (*s == '{') ++s, in_bracket = 1;
  for (; s < e; ++s) {
    utfint ch = (unsigned char)*s;
    if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (hex && ch >= 'A' && ch <= 'F') ch = 10 + (ch - 'A');
    else if (hex && ch >= 'a' && ch <= 'f') ch = 10 + (ch - 'a');
    else if (!in_bracket) break;
    else if (ch == '}')   { ++s; break; }
    else luaL_error(L, "invalid escape '%c'", ch);
    code *= hex ? 16 : 10;
    code += ch;
  }
  *pch = code;
  return s;
}

static int Lutf8_escape (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  while (s < e) {
    utfint ch = 0;
    s = utf8_safe_decode(L, s, &ch);
    if (ch == '%') {
      int hex = 0;
      switch (*s) {
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
      case '8': case '9': case '{':
        break;
      case 'x': case 'X': hex = 1; /* FALLTHROUGH */
      case 'u': case 'U': if (s+1 < e) { ++s; break; } /* FALLTHROUGH */
      default:
        s = utf8_safe_decode(L, s, &ch);
        goto next;
      }
      s = parse_escape(L, s, e, hex, &ch);
    }
next:
    add_utf8char(&b, ch);
  }
  luaL_pushresult(&b);
  return 1;
}

static int Lutf8_insert (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  size_t sublen;
  const char *subs;
  luaL_Buffer b;
  int nargs = 2;
  const char *first = e;
  if (lua_type(L, 2) == LUA_TNUMBER) {
    int idx = (int)lua_tointeger(L, 2);
    if (idx != 0) first = utf8_relat(s, e, idx);
    luaL_argcheck(L, first, 2, "invalid index");
    ++nargs;
  }
  subs = luaL_checklstring(L, nargs, &sublen);
  luaL_buffinit(L, &b);
  luaL_addlstring(&b, s, first-s);
  luaL_addlstring(&b, subs, sublen);
  luaL_addlstring(&b, first, e-first);
  luaL_pushresult(&b);
  return 1;
}

static int Lutf8_remove (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  lua_Integer posi = luaL_optinteger(L, 2, -1);
  lua_Integer posj = luaL_optinteger(L, 3, -1);
  if (!utf8_range(s, e, &posi, &posj))
    lua_settop(L, 1);
  else {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaL_addlstring(&b, s, posi);
    luaL_addlstring(&b, s+posj, e-s-posj);
    luaL_pushresult(&b);
  }
  return 1;
}

static int push_offset (lua_State *L, const char *s, const char *e, lua_Integer offset, lua_Integer idx) {
  utfint ch = 0;
  const char *p;
  if (idx != 0)
    p = utf8_offset(s, e, offset, idx);
  else if (p = s+offset-1, iscontp(p))
    p = utf8_prev(s, p);
  if (p == NULL || p == e) return 0;
  utf8_decode(p, &ch, 0);
  lua_pushinteger(L, p-s+1);
  lua_pushinteger(L, ch);
  return 2;
}

static int Lutf8_charpos (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  lua_Integer offset = 1;
  if (lua_isnoneornil(L, 3)) {
      lua_Integer idx = luaL_optinteger(L, 2, 0);
      if (idx > 0) --idx;
      else if (idx < 0) offset = e-s+1;
      return push_offset(L, s, e, offset, idx);
  }
  offset = byte_relat(luaL_optinteger(L, 2, 1), e-s);
  if (offset < 1) offset = 1;
  return push_offset(L, s, e, offset, luaL_checkinteger(L, 3));
}

static int Lutf8_offset (lua_State *L) {
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  lua_Integer n  = luaL_checkinteger(L, 2);
  lua_Integer posi = (n >= 0) ? 1 : len + 1;
  posi = byte_relat(luaL_optinteger(L, 3, posi), len);
  luaL_argcheck(L, 1 <= posi && --posi <= (lua_Integer)len, 3,
                   "position out of range");
  if (n == 0) {
    /* find beginning of current byte sequence */
    while (posi > 0 && iscontp(s + posi)) posi--;
  } else {
    if (iscontp(s + posi))
      return luaL_error(L, "initial position is a continuation byte");
    if (n < 0) {
       while (n < 0 && posi > 0) {  /* move back */
         do {  /* find beginning of previous character */
           posi--;
         } while (posi > 0 && iscontp(s + posi));
         n++;
       }
     } else {
       n--;  /* do not move for 1st character */
       while (n > 0 && posi < (lua_Integer)len) {
         do {  /* find beginning of next character */
           posi++;
         } while (iscontp(s + posi));  /* (cannot pass final '\0') */
         n--;
       }
     }
  }
  if (n != 0) return lua_pushnil(L), 1;
  lua_pushinteger(L, posi + 1);
  if ((s[posi] & 0x80) != 0) {
    do {
      posi++;
    } while (iscontp(s + posi + 1));
  }
  /* else one-byte character: final position is the initial one */
  lua_pushinteger(L, posi + 1);  /* 'posi' now is the final position */
  return 2;
}

static int Lutf8_next (lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  lua_Integer offset = byte_relat(luaL_optinteger(L, 2, 1), e-s);
  lua_Integer idx = luaL_optinteger(L, 3, !lua_isnoneornil(L, 2));
  return push_offset(L, s, e, offset, idx);
}

static int iter_aux (lua_State *L, int strict) {
  const char *e, *s = check_utf8(L, 1, &e);
  int n = CAST(int, lua_tointeger(L, 2));
  const char *p = n <= 0 ? s : utf8_next(s+n-1, e);
  if (p < e) {
    utfint code = 0;
    utf8_safe_decode(L, p, &code);
    if (strict && utf8_invalid(code))
      return luaL_error(L, "invalid UTF-8 code");
    lua_pushinteger(L, p-s+1);
    lua_pushinteger(L, code);
    return 2;
  }
  return 0;  /* no more codepoints */
}

static int iter_auxstrict (lua_State *L) { return iter_aux(L, 1); }
static int iter_auxlax (lua_State *L) { return iter_aux(L, 0); }

static int Lutf8_codes (lua_State *L) {
  int lax = lua_toboolean(L, 2);
  luaL_checkstring(L, 1);
  lua_pushcfunction(L, lax ? iter_auxlax : iter_auxstrict);
  lua_pushvalue(L, 1);
  lua_pushinteger(L, 0);
  return 3;
}

static int width_opt (lua_State *L, int idx, int *pdefault) {
  int ambiwidth = CAST(int, luaL_optinteger(L, idx, 1));
  if (pdefault != NULL) *pdefault = CAST(int, luaL_optinteger(L, idx+1, 0));
  return ambiwidth;
}

static int Lutf8_width (lua_State *L) {
  int t = lua_type(L, 1);
  int width = 0, ambiwidth, default_width;
  if (t != LUA_TNUMBER && t != LUA_TSTRING)
    return typeerror(L, 1, "number/string");
  if (t == LUA_TSTRING) {
    size_t len;
    const char *e, *s = luaL_checklstring(L, 1, &len);
    lua_Integer posi = byte_relat(luaL_optinteger(L, 2, 1), len);
    lua_Integer posj = byte_relat(luaL_optinteger(L, 3, len), len);
    ambiwidth = width_opt(L, 4, &default_width);
    luaL_argcheck(L, 1 <= posi && --posi <= (lua_Integer)len, 2,
        "initial position out of bounds");
    luaL_argcheck(L, --posj < (lua_Integer)len, 3,
        "final position out of bounds");
    e = s + posj + 1, s += posi;
    while (s < e) {
      utfint ch = 0;
      s = utf8_safe_decode(L, s, &ch);
      width += utf8_width(ch, ambiwidth, default_width);
    }
    return lua_pushinteger(L, (lua_Integer)width), 1;
  }
  ambiwidth = width_opt(L, 2, &default_width);
  return lua_pushinteger(L, utf8_width(CAST(utfint, lua_tointeger(L, 1)),
        ambiwidth, default_width)), 1;
}

static int Lutf8_widthindex (lua_State *L) {
  size_t len;
  const char *e, *s = luaL_checklstring(L, 1, &len);
  int chwidth, width = CAST(int, luaL_checkinteger(L, 2));
  lua_Integer posi = byte_relat(luaL_optinteger(L, 3, 1), len);
  lua_Integer posj = byte_relat(luaL_optinteger(L, 4, len), len), idx;
  int default_width, ambiwidth = width_opt(L, 5, &default_width);
  check_byte_range(L, len, &posi, &posj);
  for (idx = 0, e = s+posj+1, s += posi; s < e; ++idx, width -= chwidth) {
    utfint ch = 0;
    s = utf8_safe_decode(L, s, &ch);
    chwidth = utf8_width(ch, ambiwidth, default_width);
    if (width <= chwidth) {
      lua_pushinteger(L, idx + 1);
      lua_pushinteger(L, width);
      lua_pushinteger(L, chwidth);
      return 3;
    }
  }
  return lua_pushinteger(L, idx), 1;
}

static int Lutf8_widthlimit(lua_State *L) {
  size_t len;
  const char *s, *e, *n, *h = luaL_checklstring(L, 1, &len);
  lua_Integer width = luaL_checkinteger(L, 2);
  lua_Integer posi = byte_relat(luaL_optinteger(L, 3, 1), len);
  lua_Integer posj = byte_relat(luaL_optinteger(L, 4, len), len);
  int chwidth, default_width, ambiwidth = width_opt(L, 5, &default_width);
  utfint ch;
  check_byte_range(L, len, &posi, &posj);
  s = h+posi, e = h+posj+1;
  if (width >= 0) {
    for (; s < e && width != 0; s = n, width -= chwidth) {
      n = utf8_safe_decode(L, s, &ch);
      chwidth = utf8_width(ch, ambiwidth, default_width);
      if (width < chwidth) break;
    }
    lua_pushinteger(L, s - h);
  } else {
    for (; s < e && width != 0; e = n, width += chwidth) {
      utf8_safe_decode(L, n = utf8_prev(s, e), &ch);
      chwidth = utf8_width(ch, ambiwidth, default_width);
      if (-width < chwidth) break;
    }
    lua_pushinteger(L, e - h + 1);
  }
  lua_pushinteger(L, width);
  return 2;
}

static int Lutf8_ncasecmp (lua_State *L) {
  const char *e1, *s1 = check_utf8(L, 1, &e1);
  const char *e2, *s2 = check_utf8(L, 2, &e2);
  while (s1 < e1 || s2 < e2) {
    utfint ch1 = 0, ch2 = 0;
    if (s1 == e1)
      ch2 = 1;
    else if (s2 == e2)
      ch1 = 1;
    else {
      s1 = utf8_safe_decode(L, s1, &ch1);
      s2 = utf8_safe_decode(L, s2, &ch2);
      ch1 = utf8_tofold(ch1);
      ch2 = utf8_tofold(ch2);
    }
    if (ch1 != ch2) {
      lua_pushinteger(L, ch1 > ch2 ? 1 : -1);
      return 1;
    }
  }
  lua_pushinteger(L, 0);
  return 1;
}


/* utf8 pattern matching implement */

#ifndef   LUA_MAXCAPTURES
# define  LUA_MAXCAPTURES  32
#endif /* LUA_MAXCAPTURES */

#define CAP_UNFINISHED (-1)
#define CAP_POSITION   (-2)


typedef struct MatchState {
  int matchdepth;  /* control for recursive depth (to avoid C stack overflow) */
  const char *src_init;  /* init of source string */
  const char *src_end;  /* end ('\0') of source string */
  const char *p_end;  /* end ('\0') of pattern */
  lua_State *L;
  int level;  /* total number of captures (finished or unfinished) */
  struct {
    const char *init;
    ptrdiff_t len;
  } capture[LUA_MAXCAPTURES];
} MatchState;

/* recursive function */
static const char *match (MatchState *ms, const char *s, const char *p);

/* maximum recursion depth for 'match' */
#if !defined(MAXCCALLS)
#define MAXCCALLS       200
#endif

#define L_ESC           '%'
#define SPECIALS        "^$*+?.([%-"

static int check_capture (MatchState *ms, int l) {
  l -= '1';
  if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
    return luaL_error(ms->L, "invalid capture index %%%d", l + 1);
  return l;
}

static int capture_to_close (MatchState *ms) {
  int level = ms->level;
  while (--level >= 0)
    if (ms->capture[level].len == CAP_UNFINISHED) return level;
  return luaL_error(ms->L, "invalid pattern capture");
}

static const char *classend (MatchState *ms, const char *p) {
  utfint ch = 0;
  p = utf8_safe_decode(ms->L, p, &ch);
  switch (ch) {
    case L_ESC: {
      if (p == ms->p_end)
        luaL_error(ms->L, "malformed pattern (ends with " LUA_QL("%%") ")");
      return utf8_next(p, ms->p_end);
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (p == ms->p_end)
          luaL_error(ms->L, "malformed pattern (missing " LUA_QL("]") ")");
        if (*(p++) == L_ESC && p < ms->p_end)
          p++;  /* skip escapes (e.g. `%]') */
      } while (*p != ']');
      return p+1;
    }
    default: {
      return p;
    }
  }
}

static int match_class (utfint c, utfint cl) {
  int res;
  switch (utf8_tolower(cl)) {
#define X(cls, name) case cls: res = utf8_is##name(c); break;
    utf8_categories(X)
#undef  X
    case 'g' : res = utf8_isgraph(c); break;
    case 'w' : res = utf8_isalnum(c); break;
    case 'z' : res = (c == 0); break;  /* deprecated option */
    default: return (cl == c);
  }
  return (utf8_islower(cl) ? res : !res);
}

static int matchbracketclass (MatchState *ms, utfint c, const char *p, const char *ec) {
  int sig = 1;
  assert(*p == '[');
  if (*++p == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (p < ec) {
    utfint ch = 0;
    p = utf8_safe_decode(ms->L, p, &ch);
    if (ch == L_ESC) {
      p = utf8_safe_decode(ms->L, p, &ch);
      if (match_class(c, ch))
        return sig;
    } else {
      utfint next = 0;
      const char *np = utf8_safe_decode(ms->L, p, &next);
      if (next == '-' && np < ec) {
        p = utf8_safe_decode(ms->L, np, &next);
        if (ch <= c && c <= next)
          return sig;
      }
      else if (ch == c) return sig;
    }
  }
  return !sig;
}

static int singlematch (MatchState *ms, const char *s, const char *p, const char *ep) {
  if (s >= ms->src_end)
    return 0;
  else {
    utfint ch=0, pch=0;
    utf8_safe_decode(ms->L, s, &ch);
    p = utf8_safe_decode(ms->L, p, &pch);
    switch (pch) {
      case '.': return 1;  /* matches any char */
      case L_ESC: utf8_safe_decode(ms->L, p, &pch);
                  return match_class(ch, pch);
      case '[': return matchbracketclass(ms, ch, p-1, ep-1);
      default:  return pch == ch;
    }
  }
}

static const char *matchbalance (MatchState *ms, const char *s, const char **p) {
  utfint ch=0, begin=0, end=0;
  *p = utf8_safe_decode(ms->L, *p, &begin);
  if (*p >= ms->p_end)
    luaL_error(ms->L, "malformed pattern "
                      "(missing arguments to " LUA_QL("%%b") ")");
  *p = utf8_safe_decode(ms->L, *p, &end);
  s = utf8_safe_decode(ms->L, s, &ch);
  if (ch != begin) return NULL;
  else {
    int cont = 1;
    while (s < ms->src_end) {
      s = utf8_safe_decode(ms->L, s, &ch);
      if (ch == end) {
        if (--cont == 0) return s;
      }
      else if (ch == begin) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}

static const char *max_expand (MatchState *ms, const char *s, const char *p, const char *ep) {
  const char *m = s; /* matched end of single match p */
  while (singlematch(ms, m, p, ep))
    m = utf8_next(m, ms->src_end);
  /* keeps trying to match with the maximum repetitions */
  while (s <= m) {
    const char *res = match(ms, m, ep+1);
    if (res) return res;
    /* else didn't match; reduce 1 repetition to try again */
    if (s == m) break;
    m = utf8_prev(s, m);
  }
  return NULL;
}

static const char *min_expand (MatchState *ms, const char *s, const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (singlematch(ms, s, p, ep))
      s = utf8_next(s, ms->src_end);  /* try with one more repetition */
    else return NULL;
  }
}

static const char *start_capture (MatchState *ms, const char *s, const char *p, int what) {
  const char *res;
  int level = ms->level;
  if (level >= LUA_MAXCAPTURES) luaL_error(ms->L, "too many captures");
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level+1;
  if ((res=match(ms, s, p)) == NULL)  /* match failed? */
    ms->level--;  /* undo capture */
  return res;
}

static const char *end_capture (MatchState *ms, const char *s, const char *p) {
  int l = capture_to_close(ms);
  const char *res;
  ms->capture[l].len = s - ms->capture[l].init;  /* close capture */
  if ((res = match(ms, s, p)) == NULL)  /* match failed? */
    ms->capture[l].len = CAP_UNFINISHED;  /* undo capture */
  return res;
}

static const char *match_capture (MatchState *ms, const char *s, int l) {
  size_t len;
  l = check_capture(ms, l);
  len = ms->capture[l].len;
  if ((size_t)(ms->src_end-s) >= len &&
      memcmp(ms->capture[l].init, s, len) == 0)
    return s+len;
  else return NULL;
}

static const char *match (MatchState *ms, const char *s, const char *p) {
  if (ms->matchdepth-- == 0)
    luaL_error(ms->L, "pattern too complex");
  init: /* using goto's to optimize tail recursion */
  if (p != ms->p_end) {  /* end of pattern? */
    utfint ch = 0;
    utf8_safe_decode(ms->L, p, &ch);
    switch (ch) {
      case '(': {  /* start capture */
        if (*(p + 1) == ')')  /* position capture? */
          s = start_capture(ms, s, p + 2, CAP_POSITION);
        else
          s = start_capture(ms, s, p + 1, CAP_UNFINISHED);
        break;
      }
      case ')': {  /* end capture */
        s = end_capture(ms, s, p + 1);
        break;
      }
      case '$': {
        if ((p + 1) != ms->p_end)  /* is the `$' the last char in pattern? */
          goto dflt;  /* no; go to default */
        s = (s == ms->src_end) ? s : NULL;  /* check end of string */
        break;
      }
      case L_ESC: {  /* escaped sequence not in the format class[*+?-]? */
        const char *prev_p = p;
        p = utf8_safe_decode(ms->L, p+1, &ch);
        switch (ch) {
          case 'b': {  /* balanced string? */
            s = matchbalance(ms, s, &p);
            if (s != NULL)
              goto init;  /* return match(ms, s, p + 4); */
            /* else fail (s == NULL) */
            break;
          }
          case 'f': {  /* frontier? */
            const char *ep; utfint previous = 0, current = 0;
            if (*p != '[')
              luaL_error(ms->L, "missing " LUA_QL("[") " after "
                                 LUA_QL("%%f") " in pattern");
            ep = classend(ms, p);  /* points to what is next */
            if (s != ms->src_init)
              utf8_decode(utf8_prev(ms->src_init, s), &previous, 0);
            if (s != ms->src_end)
              utf8_decode(s, &current, 0);
            if (!matchbracketclass(ms, previous, p, ep - 1) &&
                 matchbracketclass(ms, current, p, ep - 1)) {
              p = ep; goto init;  /* return match(ms, s, ep); */
            }
            s = NULL;  /* match failed */
            break;
          }
          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
          case '8': case '9': {  /* capture results (%0-%9)? */
            s = match_capture(ms, s, ch);
            if (s != NULL) goto init;  /* return match(ms, s, p + 2) */
            break;
          }
          default: p = prev_p; goto dflt;
        }
        break;
      }
      default: dflt: {  /* pattern class plus optional suffix */
        const char *ep = classend(ms, p);  /* points to optional suffix */
        /* does not match at least once? */
        if (!singlematch(ms, s, p, ep)) {
          if (*ep == '*' || *ep == '?' || *ep == '-') {  /* accept empty? */
            p = ep + 1; goto init;  /* return match(ms, s, ep + 1); */
          } else  /* '+' or no suffix */
            s = NULL;  /* fail */
        } else {  /* matched once */
          const char *next_s = utf8_next(s, ms->src_end);
          switch (*ep) {  /* handle optional suffix */
            case '?': {  /* optional */
              const char *res;
              const char *next_ep = utf8_next(ep, ms->p_end);
              if ((res = match(ms, next_s, next_ep)) != NULL)
                s = res;
              else {
                p = next_ep; goto init;  /* else return match(ms, s, ep + 1); */
              }
              break;
            }
            case '+':  /* 1 or more repetitions */
              s = next_s;  /* 1 match already done */
              /* FALLTHROUGH */
            case '*':  /* 0 or more repetitions */
              s = max_expand(ms, s, p, ep);
              break;
            case '-':  /* 0 or more repetitions (minimum) */
              s = min_expand(ms, s, p, ep);
              break;
            default:  /* no suffix */
              s = next_s; p = ep; goto init;  /* return match(ms, s + 1, ep); */
          }
        }
        break;
      }
    }
  }
  ms->matchdepth++;
  return s;
}

static const char *lmemfind (const char *s1, size_t l1, const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}

static int get_index (const char *p, const char *s, const char *e) {
    int idx;
    for (idx = 0; s < e && s < p; ++idx)
        s = utf8_next(s, e);
    return s == p ? idx : idx - 1;
}

static void push_onecapture (MatchState *ms, int i, const char *s, const char *e) {
  if (i >= ms->level) {
    if (i == 0)  /* ms->level == 0, too */
      lua_pushlstring(ms->L, s, e - s);  /* add whole match */
    else
      luaL_error(ms->L, "invalid capture index");
  } else {
    ptrdiff_t l = ms->capture[i].len;
    if (l == CAP_UNFINISHED) luaL_error(ms->L, "unfinished capture");
    if (l == CAP_POSITION) {
      int idx = get_index(ms->capture[i].init, ms->src_init, ms->src_end);
      lua_pushinteger(ms->L, idx+1);
    } else
      lua_pushlstring(ms->L, ms->capture[i].init, l);
  }
}

static int push_captures (MatchState *ms, const char *s, const char *e) {
  int i;
  int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
  luaL_checkstack(ms->L, nlevels, "too many captures");
  for (i = 0; i < nlevels; i++)
    push_onecapture(ms, i, s, e);
  return nlevels;  /* number of strings pushed */
}

/* check whether pattern has no special characters */
static int nospecials (const char *p, const char * ep) {
  while (p < ep) {
    if (strpbrk(p, SPECIALS))
      return 0;  /* pattern has a special character */
    p += strlen(p) + 1;  /* may have more after \0 */
  }
  return 1;  /* no special chars found */
}


/* utf8 pattern matching interface */

static int find_aux (lua_State *L, int find) {
  const char *es, *s = check_utf8(L, 1, &es);
  const char *ep, *p = check_utf8(L, 2, &ep);
  lua_Integer idx = luaL_optinteger(L, 3, 1);
  const char *init;
  if (!idx) idx = 1;
  init = utf8_relat(s, es, CAST(int, idx));
  if (init == NULL) {
    if (idx > 0) {
      lua_pushnil(L);  /* cannot find anything */
      return 1;
    }
    init = s;
  }
  /* explicit request or no special characters? */
  if (find && (lua_toboolean(L, 4) || nospecials(p, ep))) {
    /* do a plain search */
    const char *s2 = lmemfind(init, es-init, p, ep-p);
    if (s2) {
      const char *e2 = s2 + (ep - p);
      if (iscontp(e2)) e2 = utf8_next(e2, es);
      lua_pushinteger(L, idx = get_index(s2, s, es) + 1);
      lua_pushinteger(L, idx + get_index(e2, s2, es) - 1);
      return 2;
    }
  } else {
    MatchState ms;
    int anchor = (*p == '^');
    if (anchor) p++;  /* skip anchor character */
    if (idx < 0) idx += utf8_length(s, es)+1; /* TODO not very good */
    ms.L = L;
    ms.matchdepth = MAXCCALLS;
    ms.src_init = s;
    ms.src_end = es;
    ms.p_end = ep;
    do {
      const char *res;
      ms.level = 0;
      assert(ms.matchdepth == MAXCCALLS);
      if ((res=match(&ms, init, p)) != NULL) {
        if (find) {
          lua_pushinteger(L, idx);  /* start */
          lua_pushinteger(L, idx + utf8_length(init, res) - 1);   /* end */
          return push_captures(&ms, NULL, 0) + 2;
        } else
          return push_captures(&ms, init, res);
      }
      if (init == es) break;
      idx += 1;
      init = utf8_next(init, es);
    } while (init <= es && !anchor);
  }
  lua_pushnil(L);  /* not found */
  return 1;
}

static int Lutf8_find (lua_State *L) { return find_aux(L, 1); }
static int Lutf8_match (lua_State *L) { return find_aux(L, 0); }

static int gmatch_aux (lua_State *L) {
  MatchState ms;
  const char *es, *s = check_utf8(L, lua_upvalueindex(1), &es);
  const char *ep, *p = check_utf8(L, lua_upvalueindex(2), &ep);
  const char *src;
  ms.L = L;
  ms.matchdepth = MAXCCALLS;
  ms.src_init = s;
  ms.src_end = es;
  ms.p_end = ep;
  for (src = s + (size_t)lua_tointeger(L, lua_upvalueindex(3));
       src <= ms.src_end;
       src = utf8_next(src, ms.src_end)) {
    const char *e;
    ms.level = 0;
    assert(ms.matchdepth == MAXCCALLS);
    if ((e = match(&ms, src, p)) != NULL) {
      lua_Integer newstart = e-s;
      if (e == src) newstart++;  /* empty match? go at least one position */
      lua_pushinteger(L, newstart);
      lua_replace(L, lua_upvalueindex(3));
      return push_captures(&ms, src, e);
    }
    if (src == ms.src_end) break;
  }
  return 0;  /* not found */
}

static int Lutf8_gmatch (lua_State *L) {
  luaL_checkstring(L, 1);
  luaL_checkstring(L, 2);
  lua_settop(L, 2);
  lua_pushinteger(L, 0);
  lua_pushcclosure(L, gmatch_aux, 3);
  return 1;
}

static void add_s (MatchState *ms, luaL_Buffer *b, const char *s, const char *e) {
  const char *new_end, *news = to_utf8(ms->L, 3, &new_end);
  while (news < new_end) {
    utfint ch = 0;
    news = utf8_safe_decode(ms->L, news, &ch);
    if (ch != L_ESC)
      add_utf8char(b, ch);
    else {
      news = utf8_safe_decode(ms->L, news, &ch); /* skip ESC */
      if (!utf8_isdigit(ch)) {
        if (ch != L_ESC)
          luaL_error(ms->L, "invalid use of " LUA_QL("%c")
              " in replacement string", L_ESC);
        add_utf8char(b, ch);
      } else if (ch == '0')
        luaL_addlstring(b, s, e-s);
      else {
        push_onecapture(ms, ch-'1', s, e);
        luaL_addvalue(b);  /* add capture to accumulated result */
      }
    }
  }
}

static void add_value (MatchState *ms, luaL_Buffer *b, const char *s, const char *e, int tr) {
  lua_State *L = ms->L;
  switch (tr) {
    case LUA_TFUNCTION: {
      int n;
      lua_pushvalue(L, 3);
      n = push_captures(ms, s, e);
      lua_call(L, n, 1);
      break;
    }
    case LUA_TTABLE: {
      push_onecapture(ms, 0, s, e);
      lua_gettable(L, 3);
      break;
    }
    default: {  /* LUA_TNUMBER or LUA_TSTRING */
      add_s(ms, b, s, e);
      return;
    }
  }
  if (!lua_toboolean(L, -1)) {  /* nil or false? */
    lua_pop(L, 1);
    lua_pushlstring(L, s, e - s);  /* keep original text */
  } else if (!lua_isstring(L, -1))
    luaL_error(L, "invalid replacement value (a %s)", luaL_typename(L, -1));
  luaL_addvalue(b);  /* add result to accumulator */
}

static int Lutf8_gsub (lua_State *L) {
  const char *es, *s = check_utf8(L, 1, &es);
  const char *ep, *p = check_utf8(L, 2, &ep);
  int tr = lua_type(L, 3);
  lua_Integer max_s = luaL_optinteger(L, 4, (es-s)+1);
  int anchor = (*p == '^');
  lua_Integer n = 0;
  MatchState ms;
  luaL_Buffer b;
  luaL_argcheck(L, tr == LUA_TNUMBER || tr == LUA_TSTRING ||
                   tr == LUA_TFUNCTION || tr == LUA_TTABLE, 3,
                      "string/function/table expected");
  luaL_buffinit(L, &b);
  if (anchor) p++;  /* skip anchor character */
  ms.L = L;
  ms.matchdepth = MAXCCALLS;
  ms.src_init = s;
  ms.src_end = es;
  ms.p_end = ep;
  while (n < max_s) {
    const char *e;
    ms.level = 0;
    assert(ms.matchdepth == MAXCCALLS);
    e = match(&ms, s, p);
    if (e) {
      n++;
      add_value(&ms, &b, s, e, tr);
    }
    if (e && e > s) /* non empty match? */
      s = e;  /* skip it */
    else if (s < es) {
      utfint ch = 0;
      s = utf8_safe_decode(L, s, &ch);
      add_utf8char(&b, ch);
    } else break;
    if (anchor) break;
  }
  luaL_addlstring(&b, s, es-s);
  luaL_pushresult(&b);
  lua_pushinteger(L, n);  /* number of substitutions */
  return 2;
}

static int Lutf8_isvalid(lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  const char *invalid = utf8_invalid_offset(s, e);
  lua_pushboolean(L, invalid == NULL);
  return 1;
}

static int Lutf8_invalidoffset(lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  const char *orig_s = s;
  int offset = luaL_optinteger(L, 2, 0);
  if (offset > 1) {
    offset--;
    s += offset;
    if (s >= e) {
      lua_pushnil(L);
      return 1;
    }
  } else if (offset < 0 && s - e < offset) {
    s = e + offset;
  }
  const char *invalid = utf8_invalid_offset(s, e);
  if (invalid == NULL) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, invalid - orig_s + 1);
  }
  return 1;
}

static int Lutf8_clean(lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);

  /* Default replacement string is REPLACEMENT CHARACTER U+FFFD */
  size_t repl_len;
  const char *r = luaL_optlstring(L, 2, "\xEF\xBF\xBD", &repl_len);

  if (lua_gettop(L) > 1) {
    /* Check if replacement string is valid UTF-8 or not */
    if (utf8_invalid_offset(r, r + repl_len) != NULL) {
      lua_pushstring(L, "replacement string must be valid UTF-8");
      lua_error(L);
    }
  }

  const char *invalid = utf8_invalid_offset(s, e);
  if (invalid == NULL) {
    lua_settop(L, 1); /* Return input string without modification */
    lua_pushboolean(L, 1); /* String was clean already */
    return 2;
  }

  luaL_Buffer buff;
  luaL_buffinit(L, &buff);

  while (1) {
    /* Invariant: 's' points to first GOOD byte not in output buffer,
     * 'invalid' points to first BAD byte after that */
    luaL_addlstring(&buff, s, invalid - s);
    luaL_addlstring(&buff, r, repl_len);
    /* We do not replace every bad byte with the replacement character,
     * but rather a contiguous sequence of bad bytes
     * Restore the invariant by stepping forward until we find at least
     * one good byte */
    s = invalid;
    while (s == invalid) {
      s++;
      invalid = utf8_invalid_offset(s, e);
    }
    if (invalid == NULL) {
      luaL_addlstring(&buff, s, e - s);
      luaL_pushresult(&buff);
      lua_pushboolean(L, 0); /* String was not clean */
      return 2;
    }
  }
}

static int Lutf8_isnfc(lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e);
  utfint starter = 0, ch;
  unsigned int prev_canon_cls = 0;

  while (s < e) {
    s = utf8_decode(s, &ch, 1);
    luaL_argcheck(L, (s != NULL), 1, "string is not valid UTF-8");
    if (ch < 0x300) {
      starter = ch; /* Fast path */
      prev_canon_cls = 0;
      continue;
    }

    unsigned int canon_cls = lookup_canon_cls(ch);
    if (canon_cls && canon_cls < prev_canon_cls) {
      /* Combining marks are out of order; this string is not NFC */
      lua_pushboolean(L, 0); /* Return false */
      return 1;
    }

    nfc_table *entry = nfc_quickcheck(ch);
    if (entry && !nfc_check(ch, entry, starter, canon_cls, prev_canon_cls)) {
      lua_pushboolean(L, 0); /* Return false */
      return 1;
    }

    prev_canon_cls = canon_cls;
    if (!canon_cls)
      starter = ch;
  }

  lua_pushboolean(L, 1); /* Return true */
  return 1;
}

static int Lutf8_normalize_nfc(lua_State *L) {
  const char *e, *s = check_utf8(L, 1, &e), *p = s, *starter_p = s;
  utfint starter = 0, ch;
  unsigned int prev_canon_cls = 0;

  /* First scan to see if we can find any problems... if not, we may just return the
   * input string unchanged */
  while (p < e) {
    const char *new_p = utf8_decode(p, &ch, 1);
    luaL_argcheck(L, (new_p != NULL), 1, "string is not valid UTF-8");

    unsigned int canon_cls = lookup_canon_cls(ch);
    if (canon_cls && canon_cls < prev_canon_cls) {
      goto build_string; /* Combining marks are out of order; this string is not NFC */
    }

    nfc_table *entry = nfc_quickcheck(ch);
    if (entry && !nfc_check(ch, entry, starter, canon_cls, prev_canon_cls)) {
      goto build_string;
    }

    prev_canon_cls = canon_cls;
    if (!canon_cls) {
      starter = ch;
      starter_p = p;
    }
    p = new_p;
  }

  lua_settop(L, 1); /* Return input string without modification */
  lua_pushboolean(L, 1); /* String was in normal form already, so 2nd return value is 'true' */
  return 2;

build_string: ;
  /* We will need to build a new string, this one is not NFC */
  luaL_Buffer buff;
  luaL_buffinit(L, &buff);
  luaL_addlstring(&buff, s, starter_p - s);

  string_to_nfc(L, &buff, starter_p, e);

  luaL_pushresult(&buff);
  lua_pushboolean(L, 0);
  return 2;
}

static int iterate_grapheme_indices(lua_State *L) {
  const char *s = luaL_checkstring(L, lua_upvalueindex(1));
  lua_Integer pos = luaL_checkinteger(L, lua_upvalueindex(2));
  lua_Integer end = luaL_checkinteger(L, lua_upvalueindex(3));

  if (pos > end) {
    lua_pushnil(L);
    return 1;
  }
  const char *e = s + end;

  utfint ch, next_ch;
  const char *p = utf8_safe_decode(L, s + pos - 1, &ch);

  while (1) {
    const char *next_p = utf8_safe_decode(L, p, &next_ch);
    int bind = 0;

    if (ch == '\r') {
      if (next_ch == '\n') {
        /* CR binds to following LF */
        bind = 1;
      } else {
        break;
      }
    } else if (ch == '\n' || next_ch == '\r' || next_ch == '\n') {
      /* CR/LF do not bind to any other codepoint or in any other way */
      break;
    } else if (find_in_range(cntrl_table, table_size(cntrl_table), ch) && !find_in_range(prepend_table, table_size(prepend_table), ch) && ch != 0x200D) {
      /* Control characters do not bind to anything */
      break;
    } else if (next_ch == 0x200D) {
      /* U+200D is ZERO WIDTH JOINER, it always binds to preceding char */
      if (next_p < e && find_in_range(pictographic_table, table_size(pictographic_table), ch)) {
        /* After an Extended_Pictographic codepoint and ZWJ, we bind to a following Extended_Pictographic */
        utfint nextnext_ch;
        const char *probe_ep = utf8_safe_decode(L, next_p, &nextnext_ch);
        if (find_in_range(pictographic_table, table_size(pictographic_table), nextnext_ch)) {
          p = probe_ep;
          ch = nextnext_ch;
          continue;
        }
      }
      bind = 1;
    } else if (find_in_range(cntrl_table, table_size(cntrl_table), next_ch) && !find_in_range(prepend_table, table_size(prepend_table), next_ch)) {
      /* Control characters do not bind to anything */
      break;
    } else {
      if (indic_conjunct_type(ch) == INDIC_CONSONANT) {
        utfint probed_ch = next_ch;
        const char *probe = next_p;
        int indic_type = indic_conjunct_type(probed_ch);
        int saw_linker = 0;
        while (indic_type) {
          /* Consume any number of Extend or Linker codepoints, followed by a single Consonant
           * The sequence must contain at least one Linker, however! */
          if (indic_type == INDIC_LINKER) {
            saw_linker = 1;
          } else if (indic_type == INDIC_CONSONANT) {
            if (!saw_linker)
              break;
            p = probe;
            ch = probed_ch;
            goto next_iteration;
          }
          if (probe >= e)
            break;
          probe = utf8_safe_decode(L, probe, &probed_ch);
          indic_type = indic_conjunct_type(probed_ch);
        }
      }

      if (find_in_range(compose_table, table_size(compose_table), next_ch) || (next_ch >= 0x1F3FB && next_ch <= 0x1F3FF)) {
        /* The 2nd codepoint has property Grapheme_Extend, or is an Emoji_Modifier codepoint */
        if (next_p < e && find_in_range(pictographic_table, table_size(pictographic_table), ch)) {
          /* Consume any number of 'extend' codepoints, one ZWJ, and following Extended_Pictographic codepoint */
          utfint probed_ch;
          const char *probe = next_p;
          while (probe < e) {
            probe = utf8_safe_decode(L, probe, &probed_ch);
            if (probed_ch == 0x200D) {
              if (probe < e) {
                probe = utf8_safe_decode(L, probe, &probed_ch);
                if (find_in_range(pictographic_table, table_size(pictographic_table), probed_ch)) {
                  next_p = probe;
                  next_ch = probed_ch;
                }
              }
              break;
            } else if (find_in_range(compose_table, table_size(compose_table), probed_ch) || (probed_ch >= 0x1F3FB && probed_ch <= 0x1F3FF)) {
              next_p = probe;
              next_ch = probed_ch;
            } else {
              break;
            }
          }
        }
        bind = 1;
      } else if (find_in_range(spacing_mark_table, table_size(spacing_mark_table), next_ch)) {
        /* The 2nd codepoint is in general category Spacing_Mark */
        bind = 1;
      } else if (find_in_range(prepend_table, table_size(prepend_table), ch)) {
        /* The 1st codepoint has property Prepend_Concatenation_Mark, or is a type of
         * Indic Syllable which binds to the following codepoint */
        bind = 1;
      } else if (ch >= 0x1F1E6 && ch <= 0x1F1FF && next_ch >= 0x1F1E6 && next_ch <= 0x1F1FF) {
        /* Regional Indicator (flag) emoji bind together; but only in twos */
        p = next_p;
        ch = 0xFFFE; /* Set 'ch' to bogus value so we will not re-enter this branch on next iteration */
        continue;
      } else {
        /* Korean Hangul codepoints have their own special rules about when they
         * are considered a single grapheme cluster */
        int hangul1 = hangul_type(ch);
        if (hangul1) {
          int hangul2 = hangul_type(next_ch);
          if (hangul2) {
            if (hangul1 == HANGUL_L) {
              bind = (hangul2 != HANGUL_T);
            } else if (hangul1 == HANGUL_LV || hangul1 == HANGUL_V) {
              bind = (hangul2 == HANGUL_V || hangul2 == HANGUL_T);
            } else if (hangul1 == HANGUL_LVT || hangul1 == HANGUL_T) {
              bind = (hangul2 == HANGUL_T);
            }
          }
        }
      }
    }

    if (!bind)
      break;
    p = next_p;
    ch = next_ch;
next_iteration: ;
  }

  lua_pushinteger(L, (p - s) + 1);
  lua_replace(L, lua_upvalueindex(2));

  lua_pushinteger(L, pos);
  lua_pushinteger(L, p - s);
  return 2;
}

static int Lutf8_grapheme_indices(lua_State *L) {
  size_t len;
  lua_Integer start, end;
  luaL_checklstring(L, 1, &len);
  start = byte_relat(luaL_optinteger(L, 2, 1), len);
  end = byte_relat(luaL_optinteger(L, 3, len), len);
  luaL_argcheck(L, start >= 1, 2, "out of range");
  luaL_argcheck(L, end <= (lua_Integer)len, 3, "out of range");

  lua_settop(L, 1);
  lua_pushinteger(L, start);
  lua_pushinteger(L, end);
  lua_pushcclosure(L, iterate_grapheme_indices, 3);
  return 1;
}

/* lua module import interface */

#if LUA_VERSION_NUM >= 502
static const char UTF8PATT[] = "[\0-\x7F\xC2-\xF4][\x80-\xBF]*";
#else
static const char UTF8PATT[] = "[%z\1-\x7F\xC2-\xF4][\x80-\xBF]*";
#endif

LUALIB_API int luaopen_utf8 (lua_State *L) {
  luaL_Reg libs[] = {
#define ENTRY(name) { #name, Lutf8_##name }
    ENTRY(offset),
    ENTRY(codes),
    ENTRY(codepoint),

    ENTRY(find),
    ENTRY(gmatch),
    ENTRY(gsub),
    ENTRY(match),
    ENTRY(len),
    ENTRY(sub),
    ENTRY(reverse),
    ENTRY(lower),
    ENTRY(upper),
    ENTRY(byte),
    ENTRY(char),

    ENTRY(title),
    ENTRY(fold),
    ENTRY(escape),
    ENTRY(insert),
    ENTRY(remove),
    ENTRY(charpos),
    ENTRY(next),
    ENTRY(ncasecmp),

    ENTRY(width),
    ENTRY(widthindex),
    ENTRY(widthlimit),

    ENTRY(isvalid),
    ENTRY(invalidoffset),
    ENTRY(clean),
    ENTRY(isnfc),
    ENTRY(normalize_nfc),
    ENTRY(grapheme_indices),
#undef  ENTRY
    { NULL, NULL }
  };

#if LUA_VERSION_NUM >= 502
  luaL_newlib(L, libs);
#else
  luaL_register(L, "utf8", libs);
#endif

  lua_pushlstring(L, UTF8PATT, sizeof(UTF8PATT)-1);
  lua_setfield(L, -2, "charpattern");

  lua_pushliteral(L, LUTF8_VERSION);
  lua_setfield(L, -2, "version");

  return 1;
}

/* win32cc: flags+='-Wall -Wextra -s -O2 -mdll -DLUA_BUILD_AS_DLL'
 * win32cc: libs+='-llua54.dll' output='lua-utf8.dll'
 * win32cc: run='lua.exe test.lua'
 * maccc: run='lua -- test_compat.lua'
 * maccc: flags+='-g --coverage -bundle -undefined dynamic_lookup' output='lua-utf8.so' */

