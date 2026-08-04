// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_FWD_HPP
#define JSONCONS_JSON_FWD_HPP

#include <memory> // std::allocator

namespace jsoncons {

struct sorted_policy;
                        
template <typename CharT, 
          typename Policy = sorted_policy, 
          typename Allocator = std::allocator<CharT>>
class basic_json;

} // namespace jsoncons

#endif // JSONCONS_JSON_FWD_HPP
