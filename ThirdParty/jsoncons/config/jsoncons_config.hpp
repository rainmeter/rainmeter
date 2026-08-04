// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CONFIG_JSONCONS_CONFIG_HPP
#define JSONCONS_CONFIG_JSONCONS_CONFIG_HPP

#include <cfloat> 
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <jsoncons/config/compiler_support.hpp>

namespace jsoncons {

    class assertion_error : public std::runtime_error
    {
    public:
        assertion_error(const std::string& s) noexcept
            : std::runtime_error(s)
        {
        }
        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }
    };

} // namespace jsoncons

#define JSONCONS_STR2(x)  #x
#define JSONCONS_STR(x)  JSONCONS_STR2(x)

#ifdef _DEBUG
#define JSONCONS_ASSERT(x) if (!(x)) { \
    JSONCONS_THROW(jsoncons::assertion_error("assertion '" #x "' failed at " __FILE__ ":" \
            JSONCONS_STR(__LINE__))); }
#else
#define JSONCONS_ASSERT(x) if (!(x)) { \
    JSONCONS_THROW(jsoncons::assertion_error("assertion '" #x "' failed at  <> :" \
            JSONCONS_STR( 0 ))); }
#endif // _DEBUG

#include <jsoncons/detail/utility.hpp>
namespace jsoncons {
using jsoncons::detail::in_place_t;
JSONCONS_INLINE_CONSTEXPR in_place_t in_place{};
} // namespace jsoncons

#if !defined(JSONCONS_HAS_STD_EXPECTED)
  #include <jsoncons/detail/expected.hpp>
  namespace jsoncons {
  using jsoncons::detail::expected;
  using jsoncons::detail::unexpect_t;
  using jsoncons::detail::unexpect;
  } // namespace jsoncons
#else
  #include <expected>
  namespace jsoncons {
  template <typename R,typename E>
  using expected = std::expected<R,E>;
  using unexpect_t = std::unexpect_t;
  JSONCONS_INLINE_CONSTEXPR unexpect_t unexpect{};
  } // namespace jsoncons
#endif
        
#include <jsoncons/detail/make_obj_using_allocator.hpp>
namespace jsoncons {
using jsoncons::detail::make_obj_using_allocator;
} // namespace jsoncons

#if !defined(JSONCONS_HAS_STD_STRING_VIEW)
#include <jsoncons/detail/string_view.hpp>
namespace jsoncons {
using jsoncons::detail::basic_string_view;
using string_view = jsoncons::detail::string_view;
using wstring_view = jsoncons::detail::wstring_view;
} // namespace jsoncons
#else 
#include <string_view>
namespace jsoncons {
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
}
#endif

#if !defined(JSONCONS_HAS_STD_SPAN)
#include <jsoncons/detail/span.hpp>
namespace jsoncons {
using jsoncons::detail::span;
}
#else 
#include <span>
namespace jsoncons {
using std::span;
}
#endif

#if defined(JSONCONS_HAS_STD_OPTIONAL)
    #include <optional>
    namespace jsoncons {
    using std::optional;
    }
#elif defined(JSONCONS_HAS_BOOST_OPTIONAL)
    #include <boost/optional.hpp>
    namespace jsoncons {
    using boost::optional;
    }
#else 
    #include <jsoncons/detail/optional.hpp>
    namespace jsoncons {
    using jsoncons::detail::optional;
}
#endif // !defined(JSONCONS_HAS_STD_OPTIONAL)

#if !defined(JSONCONS_HAS_STD_ENDIAN)
#include <jsoncons/detail/endian.hpp>
namespace jsoncons {
using jsoncons::detail::endian;
}
#else
#include <bit>
namespace jsoncons 
{
    using std::endian;
}
#endif

#if !defined(JSONCONS_HAS_STD_MAKE_UNIQUE)

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace jsoncons {

    template <typename T> 
    struct unique_if 
    {
        using value_is_not_array = std::unique_ptr<T>;
    };

    template <typename T> 
    struct unique_if<T[]> 
    {
        typedef std::unique_ptr<T[]> value_is_array_of_unknown_bound;
    };

    template <typename T, std::size_t N> 
    struct unique_if<T[N]> {
        using value_is_array_of_known_bound = void;
    };

    template <typename T,typename... Args>
    typename unique_if<T>::value_is_not_array
    make_unique(Args&&... args) 
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template <typename T>
    typename unique_if<T>::value_is_array_of_unknown_bound
    make_unique(std::size_t n) 
    {
        using U = typename std::remove_extent<T>::type;
        return std::unique_ptr<T>(new U[n]());
    }

    template <typename T,typename... Args>
    typename unique_if<T>::value_is_array_of_known_bound
    make_unique(Args&&...) = delete;
} // namespace jsoncons

#else

#include <memory>
namespace jsoncons 
{
    using std::make_unique;
}

#endif // !defined(JSONCONS_HAS_STD_MAKE_UNIQUE)

namespace jsoncons {

    template <typename CharT>
    constexpr const CharT* cstring_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    constexpr const char* cstring_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return c;
    }
    template<> inline
    constexpr const wchar_t* cstring_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return w;
    }

    template <typename CharT>
    std::basic_string<CharT> string_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    std::string string_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return std::string(c);
    }
    template<> inline
    std::wstring string_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return std::wstring(w);
    }

    template <typename CharT>
    jsoncons::basic_string_view<CharT> string_view_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    jsoncons::string_view string_view_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return jsoncons::string_view(c);
    }
    template<> inline
    jsoncons::wstring_view string_view_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return jsoncons::wstring_view(w);
    }

    // From boost 1_71
    template <typename T,typename U>
    T launder_cast(U* u)
    {
    #if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606
        return std::launder(reinterpret_cast<T>(u));
    #elif defined(__GNUC__) &&  (__GNUC__ * 100 + __GNUC_MINOR__) > 800
        return __builtin_launder(reinterpret_cast<T>(u));
    #else
        return reinterpret_cast<T>(u);
    #endif
    }

} // namespace jsoncons

// Preprocessor macros

#define JSONCONS_PP_EXPAND(X) X    
#define JSONCONS_PP_STRINGIFY(a) #a
#define JSONCONS_PP_QUOTE(Prefix, A) JSONCONS_PP_EXPAND(Prefix ## #A)
#define JSONCONS_PP_WIDEN(A) JSONCONS_PP_EXPAND(L ## A)

#define JSONCONS_CSTRING_CONSTANT(CharT, Str) cstring_constant_of_type<CharT>(Str, JSONCONS_PP_WIDEN(Str))
#define JSONCONS_STRING_CONSTANT(CharT, Str) string_constant_of_type<CharT>(Str, JSONCONS_PP_WIDEN(Str))
#define JSONCONS_STRING_VIEW_CONSTANT(CharT, Str) string_view_constant_of_type<CharT>(Str, JSONCONS_PP_WIDEN(Str))


#if defined(JSONCONS_VISITOR_VOID_RETURN) 
#define JSONCONS_VISITOR_RETURN_TYPE void 
#else 
#define JSONCONS_VISITOR_RETURN_TYPE bool  
#endif

#if defined(JSONCONS_VISITOR_VOID_RETURN) 
#define JSONCONS_VISITOR_RETURN return
#else 
#define JSONCONS_VISITOR_RETURN return true 
#endif

#endif // JSONCONS_CONFIG_JSONCONS_CONFIG_HPP


