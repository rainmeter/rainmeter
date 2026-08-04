// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_ENCODERS_HPP 
#define JSONCONS_JSON_ENCODERS_HPP 

#include <ostream>

#include <jsoncons/json_options.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/utility/write_number.hpp>

namespace jsoncons {
namespace detail {

inline
bool is_control_character(uint32_t c)
{
    return c <= 0x1F || c == 0x7f;
}

inline
bool is_non_ascii_codepoint(uint32_t cp)
{
    return cp >= 0x80;
}
template <typename CharT,typename Sink>
std::size_t escape_string(const CharT* s, std::size_t length,
    bool escape_all_non_ascii, bool escape_solidus,
    Sink& sink)
{
    std::size_t count = 0;
    const CharT* begin = s;
    const CharT* end = s + length;
    for (const CharT* it = begin; it != end; ++it)
    {
        CharT c = *it;
        switch (c)
        {
            case '\\':
                sink.push_back('\\');
                sink.push_back('\\');
                count += 2;
                break;
            case '"':
                sink.push_back('\\');
                sink.push_back('\"');
                count += 2;
                break;
            case '\b':
                sink.push_back('\\');
                sink.push_back('b');
                count += 2;
                break;
            case '\f':
                sink.push_back('\\');
                sink.push_back('f');
                count += 2;
                break;
            case '\n':
                sink.push_back('\\');
                sink.push_back('n');
                count += 2;
                break;
            case '\r':
                sink.push_back('\\');
                sink.push_back('r');
                count += 2;
                break;
            case '\t':
                sink.push_back('\\');
                sink.push_back('t');
                count += 2;
                break;
            default:
                if (escape_solidus && c == '/')
                {
                    sink.push_back('\\');
                    sink.push_back('/');
                    count += 2;
                }
                else if (is_control_character(c) || escape_all_non_ascii)
                {
                    // convert to codepoint
                    uint32_t cp;
                    auto r = unicode_traits::to_codepoint(it, end, cp, unicode_traits::conv_flags::strict);
                    if (r.ec != unicode_traits::conv_errc())
                    {
                        JSONCONS_THROW(ser_error(json_errc::illegal_codepoint));
                    }
                    it = r.ptr - 1;
                    if (is_non_ascii_codepoint(cp) || is_control_character(c))
                    {
                        if (cp > 0xFFFF)
                        {
                            cp -= 0x10000;
                            uint32_t first = (cp >> 10) + 0xD800;
                            uint32_t second = ((cp & 0x03FF) + 0xDC00);

                            sink.push_back('\\');
                            sink.push_back('u');
                            sink.push_back(jsoncons::to_hex_character(first >> 12 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(first >> 8 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(first >> 4 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(first & 0x000F));
                            sink.push_back('\\');
                            sink.push_back('u');
                            sink.push_back(jsoncons::to_hex_character(second >> 12 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(second >> 8 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(second >> 4 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(second & 0x000F));
                            count += 12;
                        }
                        else
                        {
                            sink.push_back('\\');
                            sink.push_back('u');
                            sink.push_back(jsoncons::to_hex_character(cp >> 12 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(cp >> 8 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(cp >> 4 & 0x000F));
                            sink.push_back(jsoncons::to_hex_character(cp & 0x000F));
                            count += 6;
                        }
                    }
                    else
                    {
                        sink.push_back(c);
                        ++count;
                    }
                }
                else
                {
                    sink.push_back(c);
                    ++count;
                }
                break;
        }
    }
    return count;
}

inline
byte_string_chars_format resolve_byte_string_chars_format(byte_string_chars_format format1,
    byte_string_chars_format format2,
    byte_string_chars_format default_format = byte_string_chars_format::base64url)
{
    byte_string_chars_format sink;
    switch (format1)
    {
        case byte_string_chars_format::base16:
        case byte_string_chars_format::base64:
        case byte_string_chars_format::base64url:
            sink = format1;
            break;
        default:
            switch (format2)
            {
                case byte_string_chars_format::base64url:
                case byte_string_chars_format::base64:
                case byte_string_chars_format::base16:
                    sink = format2;
                    break;
                default: // base64url
                {
                    sink = default_format;
                    break;
                }
            }
            break;
    }
    return sink;
}

} // namespace detail
} // namespace jsoncons

#endif // JSONCONS_JSON_ENCODERS_HPP 

