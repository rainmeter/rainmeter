/* Definitions for packing structures */
#ifndef __PACK_H__
#define __PACK_H__

#if defined(__BORLANDC__) && (__BORLANDC <= 0x520)
#  define PACK_START #pragma option -a1
#elif (defined(__GNUC__) && (__GNUC__ <= 2) && (__GNUC_MINOR__ < 95)) \
      || (defined(__WATCOMC__) && (__WATCOMC__ < 1100))
#   define PACK_START #pragma pack(1)
#else
#   define PACK_START #pragma pack(push,1)
#endif

#if defined(__BORLANDC__) && (__BORLANDC <= 0x520)
#   define PACK_STOP #pragma option -a.
#elif (defined(__GNUC__) && (__GNUC__ <= 2) && (__GNUC_MINOR__ < 95)) \
      || (defined(__WATCOMC__) && (__WATCOMC__ < 1100))
#   define PACK_STOP #pragma pack()
#else
#   define PACK_STOP #pragma pack(pop)
#endif

#endif /* __PACK_H__ */
