// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_SEMANTIC_TYPE_HPP
#define JSONCONS_SEMANTIC_TYPE_HPP

#include <cstdint>
#include <ostream>

#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons {

enum class semantic_tag : uint8_t 
{
    none = 0,               // 00000000 
    noesc = 1,              // 00000001
    bigint = 2,             // 00000010
    bigdec = 3,             // 00000011
    datetime = 4,           // 00000111
    epoch_second = 5,       // 00001000
    epoch_milli = 6,        // 00001001
    epoch_nano = 7,         // 00001010 
    base16 = 8,             // 00000100
    base64 = 9,             // 00000101
    bigfloat = 10,          // 00001010
    float128 = 11,          // 00001011
    base64url = 12,         // 00001100
    undefined = 13,         
    uri = 14,                
    multi_dim_row_major = 15,
    multi_dim_column_major = 16,
    clamped = 17,
    ext = 18,
    id = 19,
    regex = 20,
    code = 21
};

inline bool is_number_tag(semantic_tag tag) noexcept
{
    constexpr uint8_t mask1{ uint8_t(semantic_tag::bigint) & uint8_t(semantic_tag::bigdec) 
        & uint8_t(semantic_tag::bigfloat) & uint8_t(semantic_tag::float128) };
    constexpr uint8_t mask2{ uint8_t(~uint8_t(semantic_tag::bigint) & ~uint8_t(semantic_tag::bigdec) 
        & ~uint8_t(semantic_tag::bigfloat) & ~uint8_t(semantic_tag::float128)) };

    return (uint8_t(tag) & mask1) == mask1 && (uint8_t(~(uint8_t)tag) & mask2) == mask2;
}

template <typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, semantic_tag tag)
{
    static constexpr const CharT* na_name = JSONCONS_CSTRING_CONSTANT(CharT, "n/a");
    static constexpr const CharT* noesc_name = JSONCONS_CSTRING_CONSTANT(CharT, "unescaped");
    static constexpr const CharT* undefined_name = JSONCONS_CSTRING_CONSTANT(CharT, "undefined");
    static constexpr const CharT* datetime_name = JSONCONS_CSTRING_CONSTANT(CharT, "datetime");
    static constexpr const CharT* epoch_second_name = JSONCONS_CSTRING_CONSTANT(CharT, "epoch-second");
    static constexpr const CharT* epoch_milli_name = JSONCONS_CSTRING_CONSTANT(CharT, "epoch-milli");
    static constexpr const CharT* epoch_nano_name = JSONCONS_CSTRING_CONSTANT(CharT, "epoch-nano");
    static constexpr const CharT* bigint_name = JSONCONS_CSTRING_CONSTANT(CharT, "bigint");
    static constexpr const CharT* bigdec_name = JSONCONS_CSTRING_CONSTANT(CharT, "bigdec");
    static constexpr const CharT* bigfloat_name = JSONCONS_CSTRING_CONSTANT(CharT, "bigfloat");
    static constexpr const CharT* base16_name = JSONCONS_CSTRING_CONSTANT(CharT, "base16");
    static constexpr const CharT* base64_name = JSONCONS_CSTRING_CONSTANT(CharT, "base64");
    static constexpr const CharT* base64url_name = JSONCONS_CSTRING_CONSTANT(CharT, "base64url");
    static constexpr const CharT* uri_name = JSONCONS_CSTRING_CONSTANT(CharT, "uri");
    static constexpr const CharT* clamped_name = JSONCONS_CSTRING_CONSTANT(CharT, "clamped");
    static constexpr const CharT* multi_dim_row_major_name = JSONCONS_CSTRING_CONSTANT(CharT, "multi-dim-row-major");
    static constexpr const CharT* multi_dim_column_major_name = JSONCONS_CSTRING_CONSTANT(CharT, "multi-dim-column-major");
    static constexpr const CharT* ext_name = JSONCONS_CSTRING_CONSTANT(CharT, "ext");
    static constexpr const CharT* id_name = JSONCONS_CSTRING_CONSTANT(CharT, "id");
    static constexpr const CharT*  float128_name = JSONCONS_CSTRING_CONSTANT(CharT, "float128");
    static constexpr const CharT*  regex_name = JSONCONS_CSTRING_CONSTANT(CharT, "regex");
    static constexpr const CharT*  code_name = JSONCONS_CSTRING_CONSTANT(CharT, "code");

    switch (tag)
    {
        case semantic_tag::none:
        {
            os << na_name;
            break;
        }
        case semantic_tag::noesc:
        {
            os << noesc_name;
            break;
        }
        case semantic_tag::undefined:
        {
            os << undefined_name;
            break;
        }
        case semantic_tag::datetime:
        {
            os << datetime_name;
            break;
        }
        case semantic_tag::epoch_second:
        {
            os << epoch_second_name;
            break;
        }
        case semantic_tag::epoch_milli:
        {
            os << epoch_milli_name;
            break;
        }
        case semantic_tag::epoch_nano:
        {
            os << epoch_nano_name;
            break;
        }
        case semantic_tag::bigint:
        {
            os << bigint_name;
            break;
        }
        case semantic_tag::bigdec:
        {
            os << bigdec_name;
            break;
        }
        case semantic_tag::bigfloat:
        {
            os << bigfloat_name;
            break;
        }
        case semantic_tag::float128:
        {
            os << float128_name;
            break;
        }
        case semantic_tag::base16:
        {
            os << base16_name;
            break;
        }
        case semantic_tag::base64:
        {
            os << base64_name;
            break;
        }
        case semantic_tag::base64url:
        {
            os << base64url_name;
            break;
        }
        case semantic_tag::uri:
        {
            os << uri_name;
            break;
        }
        case semantic_tag::clamped:
        {
            os << clamped_name;
            break;
        }
        case semantic_tag::multi_dim_row_major:
        {
            os << multi_dim_row_major_name;
            break;
        }
        case semantic_tag::multi_dim_column_major:
        {
            os << multi_dim_column_major_name;
            break;
        }
        case semantic_tag::ext:
        {
            os << ext_name;
            break;
        }
        case semantic_tag::id:
        {
            os << id_name;
            break;
        }
        case semantic_tag::regex:
        {
            os << regex_name;
            break;
        }
        case semantic_tag::code:
        {
            os << code_name;
            break;
        }
    }
    return os;
}

} // namespace jsoncons

#endif // JSONCONS_SEMANTIC_TYPE_HPP
