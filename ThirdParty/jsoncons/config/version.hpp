// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CONFIG_VERSION_HPP
#define JSONCONS_CONFIG_VERSION_HPP
 
#include <ostream>
    
#define JSONCONS_VERSION_MAJOR 1
#define JSONCONS_VERSION_MINOR 7
#define JSONCONS_VERSION_PATCH 0

#define JSONCONS_VERSION_CONCAT_EX(major, minor, patch) \
    # major ## "." ## # minor ## "." ## # patch

#define JSONCONS_VERSION_CONCAT(major, minor, patch) \
    JSONCONS_VERSION_CONCAT_EX(major, minor, patch)

#define JSONCONS_VERSION_STRING                                 \
    JSONCONS_VERSION_CONCAT(JSONCONS_VERSION_MAJOR, JSONCONS_VERSION_MINOR, JSONCONS_VERSION_PATCH)

namespace jsoncons {

struct versioning_info
{
    unsigned int const major;
    unsigned int const minor;
    unsigned int const patch;

    friend std::ostream& operator<<(std::ostream& os, const versioning_info& ver)
    {
        os << ver.major << '.'
           << ver.minor << '.'
           << ver.patch;
        return os;
    } 
}; 

constexpr versioning_info version()
{
    return versioning_info{JSONCONS_VERSION_MAJOR, JSONCONS_VERSION_MINOR, JSONCONS_VERSION_PATCH};
}

} // namespace jsoncons

#endif // JSONCONS_CONFIG_VERSION_HPP
