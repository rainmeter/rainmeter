// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CONFIG_COMPILER_SUPPORT_HPP
#define JSONCONS_CONFIG_COMPILER_SUPPORT_HPP

#include <cmath>
#include <cstdint>
#include <cstring> // std::memcpy
#include <limits> // std::numeric_limits

#if (!defined(JSONCONS_NO_EXCEPTIONS))
// Check if exceptions are disabled.
#  if defined( __cpp_exceptions) && __cpp_exceptions == 0
#   define JSONCONS_NO_EXCEPTIONS 1
#  endif
#endif

#if !defined(JSONCONS_NO_EXCEPTIONS)
#if defined(__GNUC__) && !defined(__EXCEPTIONS)
# define JSONCONS_NO_EXCEPTIONS 1
#elif defined(_MSC_VER)
#if defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS == 0
# define JSONCONS_NO_EXCEPTIONS 1
#elif !defined(_CPPUNWIND)
# define JSONCONS_NO_EXCEPTIONS 1
#endif
#endif
#endif

#if !defined(JSONCONS_NO_EXCEPTIONS)
    #define JSONCONS_THROW(exception) throw exception
    #define JSONCONS_RETHROW throw
    #define JSONCONS_TRY try
    #define JSONCONS_CATCH(exception) catch(exception)
#else
    #define JSONCONS_THROW(exception) std::terminate()
    #define JSONCONS_RETHROW std::terminate()
    #define JSONCONS_TRY if (true)
    #define JSONCONS_CATCH(exception) if (false)
#endif

#if defined(__GNUC__)
#   if defined(__GNUC_PATCHLEVEL__)
#       define JSONCONS_GCC_AVAILABLE(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) \
            >= ((major) * 10000 + (minor) * 100 + (patch)))
#   else
#       define JSONCONS_GCC_AVAILABLE(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) \
            >= ((major) * 10000 + (minor) * 100 + (patch)))
#   endif
#   else
#       define JSONCONS_GCC_AVAILABLE(major, minor, patch) 0
#endif

#if defined(__clang__)
#   define JSONCONS_CLANG_AVAILABLE(major, minor, patch) \
            ((__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) \
            >= ((major) * 10000 + (minor) * 100 + (patch)))
#   else
#       define JSONCONS_CLANG_AVAILABLE(major, minor, patch) 0
#endif

// Uncomment the following line to suppress deprecated names (recommended for new code)
//#define JSONCONS_NO_DEPRECATED

// The definitions below follow the definitions in compiler_support_p.h, https://github.com/01org/tinycbor
// MIT license

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54577

#if defined(__GNUC__) || defined(__clang__)
#define JSONCONS_LIKELY(x) __builtin_expect(!!(x), 1)
#define JSONCONS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define JSONCONS_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define JSONCONS_LIKELY(x) x
#define JSONCONS_UNLIKELY(x) x
#define JSONCONS_UNREACHABLE() __assume(0)
#else
#define JSONCONS_LIKELY(x) x
#define JSONCONS_UNLIKELY(x) x
#define JSONCONS_UNREACHABLE() do {} while (0)
#endif

// Deprecated symbols markup
#if (defined(__cplusplus) && __cplusplus >= 201402L)
#define JSONCONS_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(__GNUC__) && defined(__has_extension)
#if __has_extension(attribute_deprecated_with_message)
#define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#endif
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(_MSC_VER)
#if (_MSC_VER) >= 1920
#define JSONCONS_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#else
#define JSONCONS_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#endif
#endif

// Following boost/atomic/detail/config.hpp
#if !defined(JSONCONS_DEPRECATED_MSG) && (\
    (defined(__GNUC__) && ((__GNUC__ + 0) * 100 + (__GNUC_MINOR__ + 0)) >= 405) ||\
    (defined(__SUNPRO_CC) && (__SUNPRO_CC + 0) >= 0x5130))
    #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(__clang__) && defined(__has_extension)
    #if __has_extension(attribute_deprecated_with_message)
        #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
    #else
        #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated))
    #endif
#endif

#if !defined(JSONCONS_DEPRECATED_MSG)
#define JSONCONS_DEPRECATED_MSG(msg)
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#if __ANDROID_API__ >= 21
#define JSONCONS_HAS_STRTOLD_L
#else
#define JSONCONS_NO_LOCALECONV
#endif
#endif

#if defined(_MSC_VER)
#define JSONCONS_HAS_MSC_STRTOD_L
#define JSONCONS_HAS_FOPEN_S
#endif

#ifndef JSONCONS_HAS_CP14
   #if defined(_MSVC_LANG) 
       #if _MSVC_LANG >= 201402L
           #define JSONCONS_HAS_CP14 
       #endif
   #elif __cplusplus >= 201402L
        #define JSONCONS_HAS_CP14 
   #endif
#endif

#if !defined(JSONCONS_HAS_STD_FROM_CHARS)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 11)
#    if (__cplusplus >= 201703)
#     if !defined(__MINGW32__)
#      define JSONCONS_HAS_STD_FROM_CHARS 1
#     endif // !defined(__MINGW32__)
#    endif // (__cplusplus >= 201703)
#   endif // (__GNUC__ >= 11)
#  endif // defined(__GNUC__)
#  if defined(_MSC_VER)
#   if (_MSC_VER >= 1924 && _MSVC_LANG >= 201703)
#    define JSONCONS_HAS_STD_FROM_CHARS 1
#   endif // (_MSC_VER >= 1924 && MSVC_LANG >= 201703)
#  endif // defined(_MSC_VER)
#endif

#if defined(JSONCONS_HAS_STD_FROM_CHARS) && JSONCONS_HAS_STD_FROM_CHARS
#include <charconv>
#endif

#if !defined(JSONCONS_HAS_2017)
#  if defined(__clang__)
#   if (__cplusplus >= 201703)
#     define JSONCONS_HAS_2017 1
#   endif // (__cplusplus >= 201703)
#  endif // defined(__clang__)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 7)
#    if (__cplusplus >= 201703)
#     define JSONCONS_HAS_2017 1
#    endif // (__cplusplus >= 201703)
#   endif // (__GNUC__ >= 7)
#  endif // defined(__GNUC__)
#  if defined(_MSC_VER)
#   if (_MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#    define JSONCONS_HAS_2017 1
#   endif // (_MSC_VER >= 1910 && MSVC_LANG >= 201703)
#  endif // defined(_MSC_VER)
#endif

#if defined(JSONCONS_HAS_2017)
    #define JSONCONS_NODISCARD [[nodiscard]]
    #define JSONCONS_IF_CONSTEXPR if constexpr
    #define JSONCONS_INLINE_CONSTEXPR inline constexpr
    #define JSONCONS_ATTRIBUTE_NODISCARD [[nodiscard]]
#else
    #define JSONCONS_NODISCARD
    #define JSONCONS_IF_CONSTEXPR if 
    #define JSONCONS_INLINE_CONSTEXPR constexpr
    #define JSONCONS_ATTRIBUTE_NODISCARD 
#endif

#if !defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
#if defined(JSONCONS_HAS_2017)
#      if __has_include(<memory_resource>)
#        define JSONCONS_HAS_POLYMORPHIC_ALLOCATOR 1
#     endif // __has_include(<string_view>)
#endif
#endif

#if !defined(JSONCONS_HAS_STD_STRING_VIEW)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if __has_include(<string_view>)
#        define JSONCONS_HAS_STD_STRING_VIEW 1
#     endif // __has_include(<string_view>)
#   else
#      define JSONCONS_HAS_STD_STRING_VIEW 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_STRING_VIEW)

#if !defined(JSONCONS_HAS_STD_BYTE)
#  if (defined JSONCONS_HAS_2017)
#    define JSONCONS_HAS_STD_BYTE 1
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_BYTE)

#if !defined(JSONCONS_HAS_STD_OPTIONAL)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if __has_include(<optional>)
#        define JSONCONS_HAS_STD_OPTIONAL 1
#     endif // __has_include(<string_view>)
#   else
#      define JSONCONS_HAS_STD_OPTIONAL 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_OPTIONAL)

#if !defined(JSONCONS_HAS_STD_VARIANT)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if defined(__APPLE__)
#        if JSONCONS_CLANG_AVAILABLE(10,0,1)
#          define JSONCONS_HAS_STD_VARIANT 1
#        endif
#      elif __has_include(<variant>)
#        define JSONCONS_HAS_STD_VARIANT 1
#     endif // __has_include(<variant>)
#   else
#      define JSONCONS_HAS_STD_VARIANT 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_VARIANT)

#if !defined(JSONCONS_HAS_FILESYSTEM)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if __has_include(<filesystem>)
#        define JSONCONS_HAS_FILESYSTEM 1
#     endif // __has_include(<filesystem>)
#   else
#      define JSONCONS_HAS_FILESYSTEM 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_FILESYSTEM)

#if !defined(JSONCONS_HAS_STD_MAKE_UNIQUE)
   #if defined(__clang__) && defined(__cplusplus)
      #if defined(__APPLE__)
         #if __clang_major__ >= 6  && __cplusplus >= 201402L // Xcode 6
            #define JSONCONS_HAS_STD_MAKE_UNIQUE
         #endif
      #elif ((__clang_major__*100 +__clang_minor__) >= 340) && __cplusplus >= 201402L
         #define JSONCONS_HAS_STD_MAKE_UNIQUE
      #endif
   #elif defined(__GNUC__)
      #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 409 && __cplusplus > 201103L
         #define JSONCONS_HAS_STD_MAKE_UNIQUE
      #endif
   #elif defined(_MSC_VER)
      #if _MSC_VER >= 1800
         #define JSONCONS_HAS_STD_MAKE_UNIQUE
      #endif
   #endif
#endif // !defined(JSONCONS_HAS_STD_MAKE_UNIQUE)

#ifndef JSONCONS_HAS_CP14_CONSTEXPR
    #if defined(_MSC_VER)
        #if _MSC_VER >= 1910
            #define JSONCONS_HAS_CP14_CONSTEXPR
        #endif
   #elif defined(__GNUC__)
      #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 600 && __cplusplus >= 201402L
         #define JSONCONS_HAS_CP14_CONSTEXPR
      #endif
   #endif
#endif

#if defined(JSONCONS_HAS_CP14_CONSTEXPR)
#  define JSONCONS_CPP14_CONSTEXPR constexpr
#else
#  define JSONCONS_CPP14_CONSTEXPR
#endif

// Follows boost

// gcc and clang
#if !defined(__CUDA_ARCH__)

#if (defined(__clang__) || defined(__GNUC__)) && defined(__cplusplus)

#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER)
#  define JSONCONS_HAS_INT128
#endif

#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
#if defined(__clang__) 
#if (__clang_major__ >= 4) && defined(__has_include)
#if __has_include(<quadmath.h>)
#  define JSONCONS_HAS_FLOAT128
#endif
#endif
#endif
#endif 

#endif // (__clang__ || __GNUC__) && __cplusplus

#if defined(__GNUC__)
#if defined(_GLIBCXX_USE_FLOAT128)
# define JSONCONS_HAS_FLOAT128
#endif
#endif

#if defined(__clang__)
#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
#if (__clang_major__ >= 4) && defined(__has_include)
#if __has_include(<quadmath.h>)
#  define JSONCONS_HAS_FLOAT128
#endif
#endif
#endif
#endif

#endif // __CUDA_ARCH__

#ifndef JSONCONS_FORCE_INLINE
# ifdef _MSC_VER
#  define JSONCONS_FORCE_INLINE __forceinline
# elif defined(__GNUC__) || defined(__clang__)
#  define JSONCONS_FORCE_INLINE inline __attribute__((always_inline))
# else
#  define JSONCONS_FORCE_INLINE inline
# endif
#endif // JSONCONS_FORCE_INLINE
 

/** compiler builtin check (since gcc 10.0, clang 2.6, icc 2021) */
#ifndef JSONCONS_HAS_BUILTIN
#   ifdef __has_builtin
#       define JSONCONS_HAS_BUILTIN(x) __has_builtin(x)
#   else
#       define JSONCONS_HAS_BUILTIN(x) 0
#   endif
#endif

/** compiler attribute check (since gcc 5.0, clang 2.9, icc 17) */
#ifndef JSONCONS_HAS_ATTRIBUTE
#   ifdef __has_attribute
#       define JSONCONS_HAS_ATTRIBUTE(x) __has_attribute(x)
#   else
#       define JSONCONS_HAS_ATTRIBUTE(x) 0
#   endif
#endif

/** compiler feature check (since clang 2.6, icc 17) */
#ifndef JSONCONS_HAS_FEATURE
#   ifdef __has_feature
#       define JSONCONS_HAS_FEATURE(x) __has_feature(x)
#   else
#       define JSONCONS_HAS_FEATURE(x) 0
#   endif
#endif

/** include check (since gcc 5.0, clang 2.7, icc 16, msvc 2017 15.3) */
#ifndef JSONCONS_HAS_INCLUDE
#   ifdef __has_include
#       define JSONCONS_HAS_INCLUDE(x) __has_include(x)
#   else
#       define JSONCONS_HAS_INCLUDE(x) 0
#   endif
#endif

// Follows boost config/detail/suffix.hpp
#if defined(JSONCONS_HAS_INT128) && defined(__cplusplus)
namespace jsoncons{
#  ifdef __GNUC__
   __extension__ typedef __int128 int128_type;
   __extension__ typedef unsigned __int128 uint128_type;
#  else
   typedef __int128 int128_type;
   typedef unsigned __int128 uint128_type;
#  endif
}
#endif
#if defined(JSONCONS_HAS_FLOAT128) && defined(__cplusplus)
namespace jsoncons {
#  ifdef __GNUC__
   __extension__ typedef __float128 float128_type;
#  else
   typedef __float128 float128_type;
#  endif
}
#endif
    
#if defined(_MSC_VER) && _MSC_VER <= 1900
    #define JSONCONS_COPY(first,last,d_first) std::copy(first, last, stdext::make_checked_array_iterator(d_first, static_cast<std::size_t>(std::distance(first, last))))
#else 
    #define JSONCONS_COPY(first,last,d_first) std::copy(first, last, d_first)
#endif

#if defined(JSONCONS_HAS_CP14)
#define JSONCONS_CONSTEXPR constexpr
#else
#define JSONCONS_CONSTEXPR
#endif

#if !defined(JSONCONS_HAS_STD_REGEX)
#if defined(__clang__) 
#define JSONCONS_HAS_STD_REGEX 1
#elif (defined(__GNUC__) && (__GNUC__ == 4)) && (defined(__GNUC__) && __GNUC_MINOR__ < 9)
// GCC 4.8 has broken regex support: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53631
#else
#define JSONCONS_HAS_STD_REGEX 1
#endif
#endif

#if !defined(JSONCONS_HAS_STATEFUL_ALLOCATOR)
#if defined(__clang__) && !JSONCONS_CLANG_AVAILABLE(11,0,0)
#elif defined(__GNUC__) && !JSONCONS_GCC_AVAILABLE(10,0,0)
#else
#define JSONCONS_HAS_STATEFUL_ALLOCATOR 1
#endif
#endif

// The definitions below follow the definitions in compiler_support_p.h, https://github.com/01org/tinycbor
// MIT license

#ifdef __F16C__
#  include <immintrin.h>
#endif

#ifndef __has_builtin
#  define __has_builtin(x)  0
#endif

#if defined(__GNUC__)
#if (__GNUC__ * 100 + __GNUC_MINOR__ >= 403) || (__has_builtin(__builtin_bswap64) && __has_builtin(__builtin_bswap32))
#  define JSONCONS_BYTE_SWAP_64 __builtin_bswap64
#  define JSONCONS_BYTE_SWAP_32 __builtin_bswap32
#    ifdef __INTEL_COMPILER
#      define JSONCONS_BYTE_SWAP_16 _bswap16
#    elif (__GNUC__ * 100 + __GNUC_MINOR__ >= 608) || __has_builtin(__builtin_bswap16)
#      define JSONCONS_BYTE_SWAP_16    __builtin_bswap16
#  endif
#endif
#elif defined(__sun)
#  include <sys/byteorder.h>
#elif defined(_MSC_VER)
// MSVC, which implies sizeof(long) == 4 
#  define JSONCONS_BYTE_SWAP_64       _byteswap_uint64
#  define JSONCONS_BYTE_SWAP_32       _byteswap_ulong
#  define JSONCONS_BYTE_SWAP_16       _byteswap_ushort
#endif

namespace jsoncons { 
namespace binary { 

    static inline bool add_check_overflow(std::size_t v1, std::size_t v2, std::size_t *r)
    {
    #if ((defined(__GNUC__) && (__GNUC__ >= 5)) && !defined(__INTEL_COMPILER)) || __has_builtin(__builtin_add_overflow)
        return __builtin_add_overflow(v1, v2, r);
    #else
        // unsigned additions are well-defined 
        *r = v1 + v2;
        return v1 > v1 + v2;
    #endif
    }

    #if defined(__apple_build_version__) && ((__clang_major__ < 8) || ((__clang_major__ == 8) && (__clang_minor__ < 1)))
    #define APPLE_MISSING_INTRINSICS 1
    #endif

    inline 
    uint16_t encode_half(double val)
    {
    #if defined(__F16C__) && !defined(APPLE_MISSING_INTRINSICS)
        return _cvtss_sh((float)val, 3);
    #else
        uint64_t v;
        std::memcpy(&v, &val, sizeof(v));
        int64_t sign = static_cast<int64_t>(v >> 63 << 15);
        int64_t exp = (v >> 52) & 0x7ff;
        int64_t mant = v << 12 >> 12 >> (53-11);    /* keep only the 11 most significant bits of the mantissa */
        exp -= 1023;
        if (exp == 1024) {
            /* infinity or NaN */
            exp = 16;
            mant >>= 1;
        } else if (exp >= 16) {
            /* overflow, as largest number */
            exp = 15;
            mant = 1023;
        } else if (exp >= -14) {
            /* regular normal */
        } else if (exp >= -24) {
            /* subnormal */
            mant |= 1024;
            mant >>= -(exp + 14);
            exp = -15;
        } else {
            /* underflow, make zero */
            return 0;
        }

        /* safe cast here as bit operations above guarantee not to overflow */
        return static_cast<uint16_t>(sign | ((exp + 15) << 10) | mant);
    #endif
    }

    /* this function was copied & adapted from RFC 7049 Appendix D */
    inline 
    double decode_half(uint16_t half)
    {
    #if defined(__F16C__) && !defined(APPLE_MISSING_INTRINSICS)
        return _cvtsh_ss(half);
    #else
        int64_t exp = (half >> 10) & 0x1f;
        int64_t mant = half & 0x3ff;
        double val;
        if (exp == 0) 
        {
            val = ldexp(static_cast<double>(mant), -24);
        }
        else if (exp != 31) 
        {
            val = ldexp(static_cast<double>(mant) + 1024.0, static_cast<int>(exp - 25));
        } 
        else
        {
            val = mant == 0 ? std::numeric_limits<double>::infinity() : std::nan("");
        }
        return half & 0x8000 ? -val : val;
    #endif
    }

} // namespace binary
} // namespace jsoncons
// allow to disable exceptions

#if defined(JSONCONS_HAS_2017)
#  define JSONCONS_FALLTHROUGH [[fallthrough]]
#elif defined(__clang__)
#  define JSONCONS_FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__) && ((__GNUC__ >= 7))
#  define JSONCONS_FALLTHROUGH __attribute__((fallthrough))
#elif defined (__GNUC__)
#  define JSONCONS_FALLTHROUGH // FALLTHRU
#else
#  define JSONCONS_FALLTHROUGH
#endif

#endif // JSONCONS_CONFIG_COMPILER_SUPPORT_HPP
