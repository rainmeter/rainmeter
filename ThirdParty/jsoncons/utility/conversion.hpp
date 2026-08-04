// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_CONVERSION_HPP
#define JSONCONS_UTILITY_CONVERSION_HPP

#include <memory>
#include <string>
#include <system_error> // std::error_code

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/unicode_traits.hpp>
#include <jsoncons/utility/write_number.hpp> // from_integer

namespace jsoncons {

template <typename InputIt,typename Container>
typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value
    && ext_traits::is_string<Container>::value,size_t>::type
bytes_to_string(InputIt first, InputIt last, semantic_tag tag, Container& str)
{
    switch (tag)
    {
        case semantic_tag::base64:
            return bytes_to_base64(first, last, str);
        case semantic_tag::base16:
            return bytes_to_base16(first, last, str);
        default:
            return bytes_to_base64url(first, last, str);
    }
}

template <typename InputIt,typename Container>
typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value,to_bytes_result<InputIt>>::type
string_to_bytes(InputIt first, InputIt last, semantic_tag tag, Container& bytes)
{
    switch (tag)
    {
        case semantic_tag::base16:
            return base16_to_bytes(first, last, bytes);
        case semantic_tag::base64:
            return base64_to_bytes(first, last, bytes);
        case semantic_tag::base64url:
            return base64url_to_bytes(first, last, bytes);
        default:
            return to_bytes_result<InputIt>{first, conv_errc::conversion_failed};
    }
}

} // namespace jsoncons

#endif // JSONCONS_UTILITY_CONVERSION_HPP

