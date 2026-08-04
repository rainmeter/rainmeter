/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CONV_ERROR_HPP
#define JSONCONS_CONV_ERROR_HPP

#include <cstddef>
#include <string>
#include <system_error>
#include <type_traits>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_exception.hpp>

namespace jsoncons {

    class conv_error : public std::system_error, public virtual json_exception
    {
    public:
        conv_error(std::error_code ec)
            : std::system_error(ec)
        {
        }
        conv_error(std::error_code ec, const std::string& what_arg)
            : std::system_error(ec, what_arg)
        {
        }
        conv_error(std::error_code ec, const char* what_arg)
            : std::system_error(ec, what_arg)
        {
        }
        conv_error(const conv_error& other) = default;

        conv_error(conv_error&& other) = default;
        
        const char* what() const noexcept final
        {
            return std::system_error::what();
        }
    };

    enum class conv_errc
    {
        success = 0,
        conversion_failed,
        not_utf8,
        not_wide_char,
        not_vector,
        not_array,
        not_map,
        not_pair,
        not_string,
        not_string_view,
        not_byte_string,
        not_byte_string_view,
        not_integer,
        not_signed_integer,
        not_unsigned_integer,
        not_bigint,
        not_double,
        not_bool,
        not_variant,
        not_nullptr,
        not_jsoncons_null_type,
        not_bitset,
        not_base64,
        not_base64url,
        not_base16,
        not_epoch,
        missing_required_member
    };

} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::conv_errc> : public true_type
    {
    };

} // namespace std

namespace jsoncons {

namespace detail {
    class conv_error_category_impl
       : public std::error_category
    {
    public:
        const char* name() const noexcept override
        {
            return "jsoncons/convert";
        }
        std::string message(int ev) const override
        {
            switch (static_cast<conv_errc>(ev))
            {
                case conv_errc::conversion_failed:
                    return "Unable to convert into the provided type";
                case conv_errc::not_utf8:
                    return "Cannot convert string to UTF-8";
                case conv_errc::not_wide_char:
                    return "Cannot convert string to wide characters";
                case conv_errc::not_vector:
                    return "Cannot convert to vector";
                case conv_errc::not_array:
                    return "Cannot convert to std::array";
                case conv_errc::not_map:
                    return "Cannot convert to map";
                case conv_errc::not_pair:
                    return "Cannot convert to std::pair";
                case conv_errc::not_string:
                    return "Cannot convert to string";
                case conv_errc::not_string_view:
                    return "Cannot convert to string_view";
                case conv_errc::not_byte_string:
                    return "Cannot convert to byte_string";
                case conv_errc::not_byte_string_view:
                    return "Cannot convert to byte_string_view";
                case conv_errc::not_integer:
                    return "Cannot convert to integer";
                case conv_errc::not_signed_integer:
                    return "Cannot convert to signed integer";
                case conv_errc::not_unsigned_integer:
                    return "Cannot convert to unsigned integer";
                case conv_errc::not_bigint:
                    return "Cannot convert to bigint";
                case conv_errc::not_double:
                    return "Cannot convert to double";
                case conv_errc::not_bool:
                    return "Cannot convert to bool";
                case conv_errc::not_variant:
                    return "Cannot convert to std::variant";
                case conv_errc::not_nullptr:
                    return "Cannot convert to std::nullptr_t";
                case conv_errc::not_jsoncons_null_type:
                    return "Cannot convert to jsoncons::null_type";
                case conv_errc::not_bitset:
                    return "Cannot convert to std::bitset";
                case conv_errc::not_base64:
                    return "Input is not a base64 encoded string";
                case conv_errc::not_base64url:
                    return "Input is not a base64url encoded string";
                case conv_errc::not_base16:
                    return "Input is not a base16 encoded string";
                case conv_errc::not_epoch:
                    return "Cannot convert to epoch";
                case conv_errc::missing_required_member:
                    return "Missing required JSON object member";
                default:
                    return "Unknown conversion error";
            }
        }
    };
    
} // namespace detail

extern inline
const std::error_category& conv_error_category() noexcept
{
  static detail::conv_error_category_impl instance;
  return instance;
}

inline 
std::error_code make_error_code(conv_errc result) noexcept
{
    return std::error_code(static_cast<int>(result),conv_error_category());
}

} // namespace jsoncons

#endif // JSONCONS_CONV_ERROR_HPP
