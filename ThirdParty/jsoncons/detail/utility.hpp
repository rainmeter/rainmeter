/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons2 for latest version

#ifndef JSONCONS_DETAIL_UTILITY_HPP    
#define JSONCONS_DETAIL_UTILITY_HPP    

#include <utility>
#include <jsoncons/config/compiler_support.hpp>
#include <iostream>

namespace jsoncons {
namespace detail {
    
#if (defined(JSONCONS_HAS_2017))

    using in_place_t = std::in_place_t;

#else
    
    struct in_place_t
    {
        explicit in_place_t() = default; 
    };

#endif

JSONCONS_INLINE_CONSTEXPR in_place_t in_place{};

} // namespace detail
} // namespace jsoncons

#endif // JSONCONS_DETAIL_UTILITY_HPP
