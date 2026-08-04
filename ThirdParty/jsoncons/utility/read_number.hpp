// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_READ_NUMBER_HPP
#define JSONCONS_UTILITY_READ_NUMBER_HPP

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits> // std::enable_if
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons { 

// Inspired by yyjson https://github.com/ibireme/yyjson

// Digit: '0'.
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_ZERO       = 1 << 0;

// Digit: [1-9]. 
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_NONZERO    = 1 << 1;

// Plus sign (positive): '+'. 
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_POS        = 1 << 2;

// Minus sign (negative): '-'. 
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_NEG        = 1 << 3;

// Decimal point: '.' 
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_DOT        = 1 << 4;

// Exponent sign: 'e, 'E'. 
JSONCONS_INLINE_CONSTEXPR uint8_t DIGIT_TYPE_EXP        = 1 << 5;

// Digit type table (generate with misc/make_tables.c) 
JSONCONS_INLINE_CONSTEXPR uint8_t digi_table[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x10, 0x00,
    0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/** Match a character with specified type. */
constexpr bool is_type(uint8_t d, uint8_t type) {
    return (digi_table[d] & type) != 0;
}

// Match a sign: '+', '-' 
constexpr bool is_sign(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)(DIGIT_TYPE_POS | DIGIT_TYPE_NEG));
}

// Match a none zero digit: [1-9] 
constexpr bool is_nonzero_digit(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)DIGIT_TYPE_NONZERO);
}

// Match a digit: [0-9] 
constexpr bool is_digit(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)(DIGIT_TYPE_ZERO | DIGIT_TYPE_NONZERO));
}

// Match an exponent sign: 'e', 'E'. 
constexpr bool is_exp(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)DIGIT_TYPE_EXP);
}

// Match a floating point indicator: '.', 'e', 'E'. 
constexpr bool is_fp_indicator(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)(DIGIT_TYPE_DOT | DIGIT_TYPE_EXP));
}

// Match a digit or floating point indicator: [0-9], '.', 'e', 'E'. 
constexpr bool is_digit_or_fp(char d) {
    return is_type(static_cast<uint8_t>(d), (uint8_t)(DIGIT_TYPE_ZERO | DIGIT_TYPE_NONZERO |
                                       DIGIT_TYPE_DOT | DIGIT_TYPE_EXP));
}
constexpr bool is_sign(wchar_t d) {
    return d == '+' || d == '-';
}

// Match a none zero digit: [1-9] 
constexpr bool is_nonzero_digit(wchar_t d) {
    return d >= '1' && d <= '9';
}

// Match a digit: [0-9] 
constexpr bool is_digit(wchar_t d) {
    return d >= '0' && d <= '9';
}

// Match an exponent sign: 'e', 'E'. 
constexpr bool is_exp(wchar_t d) {
    return d == 'e' || d == 'E';
}

// Match a floating point indicator: '.', 'e', 'E'. 
constexpr bool is_fp(wchar_t d) {
    return d == '.' || d == 'e' || d == 'E';
}

// Match a digit or floating point indicator: [0-9], '.', 'e', 'E'. 
constexpr bool is_digit_or_fp(wchar_t d) {
    return is_digit(d) || is_fp(d);
}

template <typename CharT>
struct to_number_result
{
    const CharT* ptr;
    std::errc ec;
    constexpr to_number_result(const CharT* ptr_)
        : ptr(ptr_), ec(std::errc{})
    {
    }
    constexpr to_number_result(const CharT* ptr_, std::errc ec_)
        : ptr(ptr_), ec(ec_)
    {
    }

    to_number_result(const to_number_result&) = default;

    to_number_result& operator=(const to_number_result&) = default;

    constexpr explicit operator bool() const noexcept
    {
        return ec == std::errc{};
    }
    std::error_code error_code() const
    {
        return make_error_code(ec);
    }
};

enum class integer_chars_format : uint8_t {decimal=1,hex};
enum class integer_chars_state {initial,minus,integer,binary,octal,decimal,base16};

template <typename CharT>
bool is_base10(const CharT* s, std::size_t length)
{
    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    for (;s < end; ++s)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        state = integer_chars_state::decimal;
                        break;
                    case '-':
                        state = integer_chars_state::minus;
                        break;
                    default:
                        return false;
                }
                break;
            }
            case integer_chars_state::minus:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        state = integer_chars_state::decimal;
                        break;
                    default:
                        return false;
                }
                break;
            }
            case integer_chars_state::decimal:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        break;
                    default:
                        return false;
                }
                break;
            }
            default:
                break;
        }
    }
    return state == integer_chars_state::decimal ? true : false;
}

template <typename T,typename CharT>
bool is_base16(const CharT* s, std::size_t length)
{
    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    for (;s < end; ++s)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be base16
                    case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                        state = integer_chars_state::base16;
                        break;
                    default:
                        return false;
                }
                break;
            }
            case integer_chars_state::base16:
            {
                switch(*s)
                {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be base16
                    case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                        state = integer_chars_state::base16;
                        break;
                    default:
                        return false;
                }
                break;
            }
            default:
                break;
        }
    }
    return state == integer_chars_state::base16 ? true : false;
}
    
template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && !ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
dec_to_integer(const CharT* s, std::size_t length, T& value)
{
    if (JSONCONS_UNLIKELY(length == 0))
    {
        return to_number_result<CharT>(s, std::errc::invalid_argument);
    }

    static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
    static constexpr T max_value_div_10 = max_value / 10;

    T num = 0;
    const CharT* cur = s;
    const CharT* last = s + length;
    static constexpr std::size_t digits10 = static_cast<std::size_t>(ext_traits::integer_limits<T>::digits10);
    const std::size_t n = (std::min)(digits10, length);
    const CharT* stop = s + n;
     
    while (cur < stop)
    {
        uint8_t d;
        if (JSONCONS_LIKELY((d = static_cast<uint8_t>(*cur - '0')) <= 9) )
        {
            num = static_cast<T>(d) + num*10;
        }
        else
        {
            return to_number_result<CharT>(cur, std::errc::invalid_argument);
        }
        ++cur;
    }
    if (cur == last)
    {
        value = num;
        return to_number_result<CharT>(cur, std::errc{});
    }
    if (cur+1 != last)
    {
        return to_number_result<CharT>(cur, std::errc::result_out_of_range);
    }
    if (is_digit(*cur))
    {
        if (num > max_value_div_10)
        {
            return to_number_result<CharT>(cur, std::errc::result_out_of_range);
        }
        uint8_t d = static_cast<uint8_t>(*cur - '0');
        num = num*10;
        if (num > max_value - d)
        {
            return to_number_result<CharT>(s, std::errc::result_out_of_range);
        }
        num += d;
        ++cur;
        value = num;
        return to_number_result<CharT>(cur, std::errc{});
    }
    
    return to_number_result<CharT>(cur, std::errc::invalid_argument);
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
dec_to_integer(const CharT* s, std::size_t length, T& value)
{
    if (length == 0)
    {
        return to_number_result<CharT>(s, std::errc::invalid_argument);
    }

    bool sign = *s == '-';
    s += sign;
    length -= sign;

    using U = typename ext_traits::make_unsigned<T>::type;

    U num;
    auto ru = dec_to_integer(s, length, num);
    if (ru.ec != std::errc{})
    {
        return to_number_result<CharT>(ru.ptr, ru.ec);
    }
    if (sign)
    {
        if (num > static_cast<U>(-((ext_traits::integer_limits<T>::lowest)()+T(1))) + U(1))
        {
            return to_number_result<CharT>(ru.ptr, std::errc::result_out_of_range);
        }
        else
        {
            value = static_cast<T>(U(0) - num);
            return to_number_result<CharT>(ru.ptr, std::errc{});
        }
    }
    else
    {
        if (num > static_cast<U>((ext_traits::integer_limits<T>::max)()))
        {
            return to_number_result<CharT>(ru.ptr, std::errc::result_out_of_range);
        }
        else
        {
            value = static_cast<T>(num);
            return to_number_result<CharT>(ru.ptr, std::errc{});
        }
    }
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && !ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
to_integer(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    integer_chars_state state = integer_chars_state::initial;

    const CharT* end = s + length; 
    while (s < end)
    {
        switch(state)
        {
            case integer_chars_state::initial:
            {
                switch(*s)
                {
                    case '0':
                        state = integer_chars_state::integer; // Could be binary, octal, hex 
                        ++s;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': // Must be decimal
                        state = integer_chars_state::decimal;
                        break;
                    default:
                        return to_number_result<CharT>(s, std::errc::invalid_argument);
                }
                break;
            }
            case integer_chars_state::integer:
            {
                switch(*s)
                {
                    case 'b':case 'B':
                    {
                        state = integer_chars_state::binary;
                        ++s;
                        break;
                    }
                    case 'x':case 'X':
                    {
                        state = integer_chars_state::base16;
                        ++s;
                        break;
                    }
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    {
                        state = integer_chars_state::octal;
                        break;
                    }
                    default:
                        return to_number_result<CharT>(s, std::errc::invalid_argument);
                }
                break;
            }
            case integer_chars_state::binary:
            {
                static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_2 = max_value / 2;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_number_result<CharT>(s, std::errc::invalid_argument);
                    }
                    if (n > max_value_div_2)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n = n * 2;
                    if (n > max_value - x)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::octal:
            {
                static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_8 = max_value / 8;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_number_result<CharT>(s, std::errc::invalid_argument);
                    }
                    if (n > max_value_div_8)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n = n * 8;
                    if (n > max_value - x)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::decimal:
            {
                static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_10 = max_value / 10;
                for (; s < end; ++s)
                {
                    T x = 0;
                    switch(*s)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = static_cast<T>(*s) - static_cast<T>('0');
                            break;
                        default:
                            return to_number_result<CharT>(s, std::errc::invalid_argument);
                    }
                    if (n > max_value_div_10)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n = n * 10;
                    if (n > max_value - x)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n += x;
                }
                break;
            }
            case integer_chars_state::base16:
            {
                static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
                static constexpr T max_value_div_16 = max_value / 16;
                for (; s < end; ++s)
                {
                    CharT c = *s;
                    T x = 0;
                    switch (c)
                    {
                        case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            x = static_cast<T>(c - '0');
                            break;
                        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                            x = static_cast<T>(c - ('a' - 10));
                            break;
                        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                            x = static_cast<T>(c - ('A' - 10));
                            break;
                        default:
                            return to_number_result<CharT>(s, std::errc::invalid_argument);
                    }
                    if (n > max_value_div_16)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }
                    n = n * 16;
                    if (n > max_value - x)
                    {
                        return to_number_result<CharT>(s, std::errc::result_out_of_range);
                    }

                    n += x;
                }
                break;
            }
            default:
                JSONCONS_UNREACHABLE();
                break;
        }
    }
    return (state == integer_chars_state::initial) ? to_number_result<CharT>(s, std::errc::invalid_argument) : to_number_result<CharT>(s, std::errc{});
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
to_integer(const CharT* s, std::size_t length, T& n)
{
    n = 0;

    if (length == 0)
    {
        return to_number_result<CharT>(s, std::errc::invalid_argument);
    }

    bool is_negative = *s == '-' ? true : false;
    if (is_negative)
    {
        ++s;
        --length;
    }

    using U = typename ext_traits::make_unsigned<T>::type;

    U u;
    auto ru = to_integer(s, length, u);
    if (ru.ec != std::errc{})
    {
        return to_number_result<CharT>(ru.ptr, ru.ec);
    }
    if (is_negative)
    {
        if (u > static_cast<U>(-((ext_traits::integer_limits<T>::lowest)()+T(1))) + U(1))
        {
            return to_number_result<CharT>(ru.ptr, std::errc::result_out_of_range);
        }
        else
        {
            n = static_cast<T>(U(0) - u);
            return to_number_result<CharT>(ru.ptr, std::errc{});
        }
    }
    else
    {
        if (u > static_cast<U>((ext_traits::integer_limits<T>::max)()))
        {
            return to_number_result<CharT>(ru.ptr, std::errc::result_out_of_range);
        }
        else
        {
            n = static_cast<T>(u);
            return to_number_result<CharT>(ru.ptr, std::errc{});
        }
    }
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized,to_number_result<CharT>>::type
to_integer(const CharT* s, T& n)
{
    return to_integer<T,CharT>(s, std::char_traits<CharT>::length(s), n);
}

// hex_to_integer

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
hex_to_integer(const CharT* s, std::size_t length, T& n)
{
    static_assert(ext_traits::integer_limits<T>::is_specialized, "Integer type not specialized");
    JSONCONS_ASSERT(length > 0);

    n = 0;

    const CharT* end = s + length; 
    if (*s == '-')
    {
        static constexpr T min_value = (ext_traits::integer_limits<T>::lowest)();
        static constexpr T min_value_div_16 = min_value / 16;
        ++s;
        for (; s < end; ++s)
        {
            CharT c = *s;
            T x = 0;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    x = c - '0';
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    x = c - ('a' - 10);
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    x = c - ('A' - 10);
                    break;
                default:
                    return to_number_result<CharT>(s, std::errc::invalid_argument);
            }
            if (n < min_value_div_16)
            {
                return to_number_result<CharT>(s, std::errc::result_out_of_range);
            }
            n = n * 16;
            if (n < min_value + x)
            {
                return to_number_result<CharT>(s, std::errc::result_out_of_range);
            }
            n -= x;
        }
    }
    else
    {
        static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
        static constexpr T max_value_div_16 = max_value / 16;
        for (; s < end; ++s)
        {
            CharT c = *s;
            T x = 0;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    x = c - '0';
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    x = c - ('a' - 10);
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    x = c - ('A' - 10);
                    break;
                default:
                    return to_number_result<CharT>(s, std::errc::invalid_argument);
            }
            if (n > max_value_div_16)
            {
                return to_number_result<CharT>(s, std::errc::result_out_of_range);
            }
            n = n * 16;
            if (n > max_value - x)
            {
                return to_number_result<CharT>(s, std::errc::result_out_of_range);
            }

            n += x;
        }
    }

    return to_number_result<CharT>(s, std::errc{});
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::integer_limits<T>::is_specialized && !ext_traits::integer_limits<T>::is_signed,to_number_result<CharT>>::type
hex_to_integer(const CharT* s, std::size_t length, T& n)
{
    static_assert(ext_traits::integer_limits<T>::is_specialized, "Integer type not specialized");
    JSONCONS_ASSERT(length > 0);

    n = 0;
    const CharT* end = s + length; 

    static constexpr T max_value = (ext_traits::integer_limits<T>::max)();
    static constexpr T max_value_div_16 = max_value / 16;
    for (; s < end; ++s)
    {
        CharT c = *s;
        T x = *s;
        switch (c)
        {
            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                x = c - '0';
                break;
            case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                x = c - ('a' - 10);
                break;
            case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                x = c - ('A' - 10);
                break;
            default:
                return to_number_result<CharT>(s, std::errc::invalid_argument);
        }
        if (n > max_value_div_16)
        {
            return to_number_result<CharT>(s, std::errc::result_out_of_range);
        }
        n = n * 16;
        if (n > max_value - x)
        {
            return to_number_result<CharT>(s, std::errc::result_out_of_range);
        }

        n += x;
    }

    return to_number_result<CharT>(s, std::errc{});
}

// decstr_to_double

#if defined(JSONCONS_HAS_STD_FROM_CHARS) && JSONCONS_HAS_STD_FROM_CHARS

inline to_number_result<char> decstr_to_double(const char* s, std::size_t length, double& val) 
{
    const char* last = s+length;
    const auto res = std::from_chars(s, last, val);
    if (JSONCONS_LIKELY(res.ec == std::errc()))
    {
        return to_number_result<char>{res.ptr,res.ec};
    }
    if (JSONCONS_UNLIKELY(res.ptr != last))
    {
        return to_number_result<char>{res.ptr,std::errc::invalid_argument};
    }
    if (res.ec == std::errc::result_out_of_range)
    {
        bool negative = (s < last && *s == '-') ? true : false;
        val = negative ? -HUGE_VAL : HUGE_VAL;
    }

    return to_number_result<char>{res.ptr,res.ec};
}

inline to_number_result<wchar_t> decstr_to_double(const wchar_t* s, std::size_t length, double& val)
{
    std::string buf(length,'0');
    for (size_t i = 0; i < length; ++i)
    {
        buf[i] = static_cast<char>(s[i]);
    }
    
    const auto res = std::from_chars(buf.data(), buf.data()+length, val);
    if (JSONCONS_UNLIKELY(res.ptr != (buf.data()+length)))
    {
        return to_number_result<wchar_t>{s+(res.ptr-buf.data()),std::errc::invalid_argument};
    }
    return to_number_result<wchar_t>{s+length,res.ec};
}

#elif defined(JSONCONS_HAS_MSC_STRTOD_L)

inline to_number_result<char> decstr_to_double(const char* s, std::size_t length, double& val)
{
    static _locale_t locale = _create_locale(LC_NUMERIC, "C");

    const char* cur = s+length;
    char *end = nullptr;
    val = _strtod_l(s, &end, locale);
    if (JSONCONS_UNLIKELY(end != cur))
    {
        return to_number_result<char>{end,std::errc::invalid_argument};
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<char>{end, std::errc::result_out_of_range};
    }
    return to_number_result<char>{end};
}

inline to_number_result<wchar_t> decstr_to_double(const wchar_t* s, std::size_t length, double& val)
{
    static _locale_t locale = _create_locale(LC_NUMERIC, "C");

    const wchar_t* cur = s+length;
    wchar_t* end = nullptr;
    val = _wcstod_l(s, &end, locale);
    if (JSONCONS_UNLIKELY(end != cur))
    {
        return to_number_result<wchar_t>{end,std::errc::invalid_argument};
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<wchar_t>{end, std::errc::result_out_of_range};
    }
    return to_number_result<wchar_t>{end};
}


#elif defined(JSONCONS_HAS_STRTOLD_L)

inline to_number_result<char> decstr_to_double(const char* s, std::size_t length, double& val)
{
    locale_t locale = newlocale(LC_ALL_MASK, "C", (locale_t) 0);

    const char* cur = s+length;
    char *end = nullptr;
    val = strtod_l(s, &end, locale);
    if (JSONCONS_UNLIKELY(end != cur))
    {
        return to_number_result<char>{end,std::errc::invalid_argument};
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<char>{end, std::errc::result_out_of_range};
    }
    return to_number_result<char>{end};
}

inline to_number_result<wchar_t> decstr_to_double(const wchar_t* s, std::size_t length, double& val)
{
    locale_t locale = newlocale(LC_ALL_MASK, "C", (locale_t) 0);

    const wchar_t* cur = s+length;
    wchar_t *end = nullptr;
    val = wcstod_l(s, &end, locale);
    if (JSONCONS_UNLIKELY(end != cur))
    {
        return to_number_result<wchar_t>{end,std::errc::invalid_argument};
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<wchar_t>{end, std::errc::result_out_of_range};
    }
    return to_number_result<wchar_t>{end};
}

#else

inline to_number_result<char> decstr_to_double(const char* s, std::size_t length, double& val)
{
    const char* cur = s+length;
    char *end = nullptr;
    val = strtod(s, &end);
    const char* str_end = end;
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            std::string buf{s, length};
            char* dot_ptr = &buf[0] + (cur - end - 1);
            *dot_ptr = ',';
            end = nullptr;
            val = strtod(buf.c_str(), &end);
            str_end = s + (end - &buf[0]);
        }
        if (JSONCONS_UNLIKELY(str_end != cur))
        {
            return to_number_result<char>{str_end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<char>{str_end, std::errc::result_out_of_range};
    }
    return to_number_result<char>{str_end};
}

inline to_number_result<wchar_t> decstr_to_double(const wchar_t* s, std::size_t length, double& val)
{
    const wchar_t* cur = s+length;
    wchar_t *end = nullptr;
    val = wcstod(s, &end);
    const wchar_t* str_end = end;
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            std::wstring buf{s, length};
            wchar_t* dot_ptr = &buf[0] + (cur - end - 1);
            *dot_ptr = ',';
            end = nullptr;
            val = wcstod(buf.c_str(), &end);
            str_end = s + (end-&buf[0]);
        }
        if (JSONCONS_UNLIKELY(str_end != cur))
        {
            return to_number_result<wchar_t>{str_end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<wchar_t>{str_end, std::errc::result_out_of_range};
    }
    return to_number_result<wchar_t>{str_end};
}

inline to_number_result<char> decstr_to_double(char* s, std::size_t length)
{
    char* cur = s+length;
    char *end = nullptr;
    double val = strtod(s, &end);
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            char* dot_ptr = end;
            *end = ',';
            val = strtod(s, &end);
            *dot_ptr = '.';
        }
        if (JSONCONS_UNLIKELY(end != cur))
        {
            return to_number_result<char>{end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<char>{end, std::errc::result_out_of_range};
    }
    return to_number_result<char>{end};
}

inline to_number_result<wchar_t> decstr_to_double(wchar_t* s, std::size_t length)
{
    wchar_t* cur = s+length;
    wchar_t *end = nullptr;
    double val = wcstod(s, &end);
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            wchar_t* dot_ptr = end;
            *end = ',';
            val = wcstod(s, &end);
            *dot_ptr = '.';
        }
        if (JSONCONS_UNLIKELY(end != cur))
        {
            return to_number_result<wchar_t>{end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<wchar_t>{end, std::errc::result_out_of_range};
    }
    return to_number_result<wchar_t>{end};
}

#endif

inline to_number_result<char> hexstr_to_double(const char* s, std::size_t length, double& val)
{
    const char* cur = s+length;
    char *end = nullptr;
    val = strtod(s, &end);
    const char* str_end = end;
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            std::string buf{s, length};
            char* dot_ptr = &buf[0] + (cur - end - 1);
            *dot_ptr = ',';
            end = nullptr;
            val = strtod(buf.c_str(), &end);
            str_end = s + (end - &buf[0]);
        }
        if (JSONCONS_UNLIKELY(str_end != cur))
        {
            return to_number_result<char>{str_end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<char>{str_end, std::errc::result_out_of_range};
    }
    return to_number_result<char>{str_end};
}

inline to_number_result<wchar_t> hexstr_to_double(const wchar_t* s, std::size_t length, double& val)
{
    const wchar_t* cur = s+length;
    wchar_t *end = nullptr;
    val = wcstod(s, &end);
    const wchar_t* str_end = end;
    if (JSONCONS_UNLIKELY(end < cur))
    {
        if (*end == '.')
        {
            std::wstring buf{s, length};
            wchar_t* dot_ptr = &buf[0] + (cur - end - 1);
            *dot_ptr = ',';
            end = nullptr;
            val = wcstod(buf.c_str(), &end);
            str_end = s + (end-&buf[0]);
        }
        if (JSONCONS_UNLIKELY(str_end != cur))
        {
            return to_number_result<wchar_t>{str_end,std::errc::invalid_argument};
        }
    }
    if (JSONCONS_UNLIKELY(val <= -HUGE_VAL || val >= HUGE_VAL))
    {
        return to_number_result<wchar_t>{str_end, std::errc::result_out_of_range};
    }
    return to_number_result<wchar_t>{str_end};
}
        
} // namespace jsoncons

#endif // JSONCONS_UTILITY_READ_NUMBER_HPP
