// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_WRITE_NUMBER_HPP
#define JSONCONS_UTILITY_WRITE_NUMBER_HPP

#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits> // std::numeric_limits
#include <locale>
#include <stdexcept>
#include <stdio.h> // snprintf
#include <string>
#include <type_traits>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/detail/grisu3.hpp>
#include <jsoncons/utility/read_number.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons { 

inline
char to_hex_character(uint8_t c)
{
    return (char)((c < 10) ? ('0' + c) : ('A' - 10 + c));
}

// from_integer

template <typename Integer,typename Result>
typename std::enable_if<ext_traits::is_integer<Integer>::value,std::size_t>::type
from_integer(Integer value, Result& result)
{
    using char_type = typename Result::value_type;

    char_type buf[255];
    char_type *p = buf;
    const char_type* last = buf+255;

    bool is_negative = value < 0;

    if (value < 0)
    {
        do
        {
            *p++ = static_cast<char_type>(48 - (value % 10));
        }
        while ((value /= 10) && (p < last));
    }
    else
    {

        do
        {
            *p++ = static_cast<char_type>(48 + value % 10);
        }
        while ((value /= 10) && (p < last));
    }
    JSONCONS_ASSERT(p != last);

    std::size_t count = (p - buf);
    if (is_negative)
    {
        result.push_back('-');
        ++count;
    }
    while (--p >= buf)
    {
        result.push_back(*p);
    }

    return count;
}

// integer_to_hex

template <typename Integer,typename Result>
typename std::enable_if<ext_traits::is_integer<Integer>::value,std::size_t>::type
integer_to_hex(Integer value, Result& result)
{
    using char_type = typename Result::value_type;

    char_type buf[255];
    char_type *p = buf;
    const char_type* last = buf+255;

    bool is_negative = value < 0;

    if (value < 0)
    {
        do
        {
            *p++ = to_hex_character(0-(value % 16));
        }
        while ((value /= 16) && (p < last));
    }
    else
    {

        do
        {
            *p++ = to_hex_character(value % 16);
        }
        while ((value /= 16) && (p < last));
    }
    JSONCONS_ASSERT(p != last);

    std::size_t count = (p - buf);
    if (is_negative)
    {
        result.push_back('-');
        ++count;
    }
    while (--p >= buf)
    {
        result.push_back(*p);
    }

    return count;
}

// write_double

// fast exponent
template <typename Result>
void fill_exponent(int K, Result& result)
{
    if (K < 0)
    {
        result.push_back('-');
        K = -K;
    }
    else
    {
        result.push_back('+'); // compatibility with sprintf
    }

    if (K < 10)
    {
        result.push_back('0'); // compatibility with sprintf
        result.push_back((char)('0' + K));
    }
    else if (K < 100)
    {
        result.push_back((char)('0' + K / 10)); K %= 10;
        result.push_back((char)('0' + K));
    }
    else if (K < 1000)
    {
        result.push_back((char)('0' + K / 100)); K %= 100;
        result.push_back((char)('0' + K / 10)); K %= 10;
        result.push_back((char)('0' + K));
    }
    else
    {
        jsoncons::from_integer(K, result);
    }
}

template <typename Result>
void prettify_string(const char *buffer, std::size_t length, int k, int min_exp, int max_exp, Result& result)
{
    int nb_digits = (int)length;
    int offset;
    /* v = buffer * 10^k
       kk is such that 10^(kk-1) <= v < 10^kk
       this way kk gives the position of the decimal point.
    */
    int kk = nb_digits + k;

    if (nb_digits <= kk && kk <= max_exp)
    {
        /* the first digits are already in. Add some 0s and call it a day. */
        /* the max_exp is a personal choice. Only 16 digits could possibly be relevant.
         * Basically we want to print 12340000000 rather than 1234.0e7 or 1.234e10 */
        for (int i = 0; i < nb_digits; ++i)
        {
            result.push_back(buffer[i]);
        }
        for (int i = nb_digits; i < kk; ++i)
        {
            result.push_back('0');
        }
        result.push_back('.');
        result.push_back('0');
    } 
    else if (0 < kk && kk <= max_exp)
    {
        /* comma number. Just insert a '.' at the correct location. */
        for (int i = 0; i < kk; ++i)
        {
            result.push_back(buffer[i]);
        }
        result.push_back('.');
        for (int i = kk; i < nb_digits; ++i)
        {
            result.push_back(buffer[i]);
        }
    } 
    else if (min_exp < kk && kk <= 0)
    {
        offset = 2 - kk;

        result.push_back('0');
        result.push_back('.');
        for (int i = 2; i < offset; ++i) 
            result.push_back('0');
        for (int i = 0; i < nb_digits; ++i)
        {
            result.push_back(buffer[i]);
        }
    } 
    else if (nb_digits == 1)
    {
        result.push_back(buffer[0]);
        result.push_back('e');
        fill_exponent(kk - 1, result);
    } 
    else
    {
        result.push_back(buffer[0]);
        result.push_back('.');
        for (int i = 1; i < nb_digits; ++i)
        {
            result.push_back(buffer[i]);
        }
        result.push_back('e');
        fill_exponent(kk - 1, result);
    }
}

template <typename Result>
void dump_buffer(const char *buffer, std::size_t length, char decimal_point, Result& result)
{
    const char *sbeg = buffer;
    const char *send = sbeg + length;

    if (sbeg != send)
    {
        bool needs_dot = true;
        for (const char* q = sbeg; q < send; ++q)
        {
            switch (*q)
            {
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
                result.push_back(*q);
                break;
            case 'e':
            case 'E':
                result.push_back('e');
                needs_dot = false;
                break;
            default:
                if (*q == decimal_point)
                {
                    needs_dot = false;
                    result.push_back('.');
                }
                break;
            }
        }
        if (needs_dot)
        {
            result.push_back('.');
            result.push_back('0');
        }
    }
}

template <typename Result>
bool dtoa_scientific(double val, char decimal_point, Result& result)
{
    if (val == 0)
    {
        result.push_back('0');
        result.push_back('.');
        result.push_back('0');
        return true;
    }

    char buffer[100];
    int precision = std::numeric_limits<double>::digits10;
    int length = snprintf(buffer, sizeof(buffer), "%1.*e", precision, val);
    if (length < 0)
    {
        return false;
    }
    double x{0};
    auto res = decstr_to_double(buffer, length, x);
    if (res.ec == std::errc::invalid_argument)
    {
        return false;
    }
    if (x != val)
    {
        const int precision2 = std::numeric_limits<double>::max_digits10;
        length = snprintf(buffer, sizeof(buffer), "%1.*e", precision2, val);
        if (length < 0)
        {
            return false;
        }
    }
    dump_buffer(buffer, static_cast<std::size_t>(length), decimal_point, result);
    return true;
}

template <typename Result>
bool dtoa_general(double val, char decimal_point, Result& result, std::false_type)
{
    if (val == 0)
    {
        result.push_back('0');
        result.push_back('.');
        result.push_back('0');
        return true;
    }

    char buffer[100];
    int precision = std::numeric_limits<double>::digits10;
    int length = snprintf(buffer, sizeof(buffer), "%1.*g", precision, val);
    if (length < 0)
    {
        return false;
    }
    double x{0};
    auto res = decstr_to_double(buffer, length, x);
    if (res.ec == std::errc::invalid_argument)
    {
        return false;
    }
    if (x != val)
    {
        const int precision2 = std::numeric_limits<double>::max_digits10;
        length = snprintf(buffer, sizeof(buffer), "%1.*g", precision2, val);
        if (length < 0)
        {
            return false;
        }
    }
    dump_buffer(buffer, length, decimal_point, result);
    return true;
}

template <typename Result>
bool dtoa_general(double v, char decimal_point, Result& result, std::true_type)
{
    if (v == 0)
    {
        result.push_back('0');
        result.push_back('.');
        result.push_back('0');
        return true;
    }

    int length = 0;
    int k;

    char buffer[100];

    double u = std::signbit(v) ? -v : v;
    if (jsoncons::detail::grisu3(u, buffer, &length, &k))
    {
        if (std::signbit(v))
        {
            result.push_back('-');
        }
        // min exp: -4 is consistent with sprintf
        // max exp: std::numeric_limits<double>::max_digits10
        jsoncons::prettify_string(buffer, length, k, -4, std::numeric_limits<double>::max_digits10, result);
        return true;
    }
    else
    {
        return dtoa_general(v, decimal_point, result, std::false_type());
    }
}

template <typename Result>
bool dtoa_fixed(double val, char decimal_point, Result& result, std::false_type)
{
    if (val == 0)
    {
        result.push_back('0');
        result.push_back('.');
        result.push_back('0');
        return true;
    }

    char buffer[100];
    int precision = std::numeric_limits<double>::digits10;
    int length = snprintf(buffer, sizeof(buffer), "%1.*f", precision, val);
    if (length < 0)
    {
        return false;
    }
    double x{0};
    auto res = decstr_to_double(buffer, length, x);
    if (res.ec == std::errc::invalid_argument)
    {
        return false;
    }
    if (x != val)
    {
        const int precision2 = std::numeric_limits<double>::max_digits10;
        length = snprintf(buffer, sizeof(buffer), "%1.*f", precision2, val);
        if (length < 0)
        {
            return false;
        }
    }
    dump_buffer(buffer, length, decimal_point, result);
    return true;
}

template <typename Result>
bool dtoa_fixed(double v, char decimal_point, Result& result, std::true_type)
{
    if (v == 0)
    {
        result.push_back('0');
        result.push_back('.');
        result.push_back('0');
        return true;
    }

    int length = 0;
    int k;

    char buffer[100];

    double u = std::signbit(v) ? -v : v;
    if (jsoncons::detail::grisu3(u, buffer, &length, &k))
    {
        if (std::signbit(v))
        {
            result.push_back('-');
        }
        jsoncons::prettify_string(buffer, length, k, std::numeric_limits<int>::lowest(), (std::numeric_limits<int>::max)(), result);
        return true;
    }
    else
    {
        return dtoa_fixed(v, decimal_point, result, std::false_type());
    }
}

template <typename Result>
bool dtoa_fixed(double v, char decimal_point, Result& result)
{
    return dtoa_fixed(v, decimal_point, result, std::integral_constant<bool, std::numeric_limits<double>::is_iec559>());
}

template <typename Result>
bool dtoa_general(double v, char decimal_point, Result& result)
{
    return dtoa_general(v, decimal_point, result, std::integral_constant<bool, std::numeric_limits<double>::is_iec559>());
}

class write_double
{
private:
    float_chars_format float_format_;
    int precision_;
    char decimal_point_;
public:
    write_double(float_chars_format float_format, int precision)
       : float_format_(float_format), precision_(precision), decimal_point_('.')
    {
#if !defined(JSONCONS_NO_LOCALECONV)
        struct lconv *lc = localeconv();
        if (lc != nullptr && lc->decimal_point[0] != 0)
        {
            decimal_point_ = lc->decimal_point[0];
        }
#endif
    }
    write_double(const write_double&) = default;

    write_double& operator=(const write_double&) = default;

    template <typename Result>
    std::size_t operator()(double val, Result& result)
    {
        std::size_t count = 0;

        char number_buffer[200];
        int length = 0;

        switch (float_format_)
        {
        case float_chars_format::fixed:
            {
                if (precision_ > 0)
                {
                    length = snprintf(number_buffer, sizeof(number_buffer), "%1.*f", precision_, val);
                    if (length < 0)
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                    dump_buffer(number_buffer, length, decimal_point_, result);
                }
                else
                {
                    if (!dtoa_fixed(val, decimal_point_, result))
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                }
            }
            break;
        case float_chars_format::scientific:
            {
                if (precision_ > 0)
                {
                    length = snprintf(number_buffer, sizeof(number_buffer), "%1.*e", precision_, val);
                    if (length < 0)
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                    dump_buffer(number_buffer, length, decimal_point_, result);
                }
                else
                {
                    if (!dtoa_scientific(val, decimal_point_, result))
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                }
            }
            break;
        case float_chars_format::general:
            {
                if (precision_ > 0)
                {
                    length = snprintf(number_buffer, sizeof(number_buffer), "%1.*g", precision_, val);
                    if (length < 0)
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                    dump_buffer(number_buffer, length, decimal_point_, result);
                }
                else
                {
                    if (!dtoa_general(val, decimal_point_, result))
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                    }
                }             
                break;
            }
            default:
                JSONCONS_THROW(json_runtime_error<std::invalid_argument>("write_double failed."));
                break;
        }
        return count;
    }
};

} // namespace jsoncons

#endif // JSONCONS_UTILITY_WRITE_NUMBER_HPP
