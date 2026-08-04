/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_ERROR_HPP
#define JSONCONS_JSON_ERROR_HPP

#include <string>
#include <system_error>
#include <type_traits>

#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons {

    enum class json_errc
    {
        success = 0,
        unexpected_eof = 1,
        source_error,
        syntax_error,
        extra_character,
        max_nesting_depth_exceeded,
        single_quote,
        illegal_character_in_string,
        extra_comma,
        expected_key,
        expected_value,
        invalid_value,
        expected_colon,
        illegal_control_character,
        illegal_escaped_character,
        expected_codepoint_surrogate_pair,
        invalid_hex_escape_sequence,
        invalid_unicode_escape_sequence,
        leading_zero,
        invalid_number,
        expected_comma_or_rbrace,
        expected_comma_or_rbracket,
        unexpected_rbracket,
        unexpected_rbrace,
        illegal_comment,
        expected_continuation_byte,
        over_long_utf8_sequence,
        illegal_codepoint,
        illegal_surrogate_value,
        unpaired_high_surrogate,
        illegal_unicode_character,
        unexpected_character
    };

    class json_error_category_impl
       : public std::error_category
    {
    public:
        const char* name() const noexcept final
        {
            return "jsoncons/json";
        }
        std::string message(int ev) const final
        {
            switch (static_cast<json_errc>(ev))
            {
                case json_errc::unexpected_eof:
                    return "Unexpected end of file";
                case json_errc::source_error:
                    return "Source error";
                case json_errc::syntax_error:
                    return "JSON syntax_error";
                case json_errc::extra_character:
                    return "Unexpected non-whitespace character after JSON text";
                case json_errc::max_nesting_depth_exceeded:
                    return "Data item nesting exceeds limit in options";
                case json_errc::single_quote:
                    return "JSON strings cannot be quoted with single quotes";
                case json_errc::illegal_character_in_string:
                    return "Illegal character in string";
                case json_errc::extra_comma:
                    return "Extra comma";
                case json_errc::expected_key:
                    return "Expected object member key";
                case json_errc::expected_value:
                    return "Expected value";
                case json_errc::invalid_value:
                    return "Invalid value";
                case json_errc::expected_colon:
                    return "Expected name separator ':'";
                case json_errc::illegal_control_character:
                    return "Illegal control character in string";
                case json_errc::illegal_escaped_character:
                    return "Illegal escaped character in string";
                case json_errc::expected_codepoint_surrogate_pair:
                    return "Invalid codepoint, expected another \\u token to begin the second half of a codepoint surrogate pair.";
                case json_errc::invalid_hex_escape_sequence:
                    return "Invalid codepoint, expected hexadecimal digit.";
                case json_errc::invalid_unicode_escape_sequence:
                    return "Invalid codepoint, expected four hexadecimal digits.";
                case json_errc::leading_zero:
                    return "A number cannot have a leading zero";
                case json_errc::invalid_number:
                    return "Invalid number";
                case json_errc::expected_comma_or_rbrace:
                    return "Expected comma or right brace '}'";
                case json_errc::expected_comma_or_rbracket:
                    return "Expected comma or right bracket ']'";
                case json_errc::unexpected_rbrace:
                    return "Unexpected right brace '}'";
                case json_errc::unexpected_rbracket:
                    return "Unexpected right bracket ']'";
                case json_errc::illegal_comment:
                    return "Illegal comment";
                case json_errc::expected_continuation_byte:
                    return "Expected continuation byte";
                case json_errc::over_long_utf8_sequence:
                    return "Over long UTF-8 sequence";
                case json_errc::illegal_codepoint:
                    return "Illegal codepoint (>= 0xd800 && <= 0xdfff)";
                case json_errc::illegal_surrogate_value:
                    return "UTF-16 surrogate values are illegal in UTF-32";
                case json_errc::unpaired_high_surrogate:
                    return "Expected low surrogate following the high surrogate";
                case json_errc::illegal_unicode_character:
                    return "Illegal unicode character";
                case json_errc::unexpected_character:
                    return "Unexpected character";
                default:
                    return "Unknown JSON parser error";
                }
        }
    };

    inline const std::error_category& json_error_category() noexcept
    {
      static json_error_category_impl instance;
      return instance;
    }

    inline std::error_code make_error_code(json_errc result) noexcept
    {
        return std::error_code(static_cast<int>(result),json_error_category());
    }

} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::json_errc> : public true_type
    {
    };

} // namespace std

#endif // JSONCONS_JSON_ERROR_HPP
