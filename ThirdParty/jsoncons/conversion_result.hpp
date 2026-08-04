/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons2 for latest version

#ifndef JSONCONS_CONVERSION_RESULT_HPP    
#define JSONCONS_CONVERSION_RESULT_HPP    

#include <ostream>
#include <system_error>
#include <type_traits>

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/conv_error.hpp>

namespace jsoncons {

class conversion_error
{
    std::error_code ec_;
    std::string message_arg_;
public:
    conversion_error(std::error_code ec)
        : ec_(ec)
    {
    }
    conversion_error(std::error_code ec, const jsoncons::string_view& message_arg)
        : ec_(ec), message_arg_(message_arg)
    {
    }

    conversion_error(const conversion_error& other) = default;

    conversion_error(conversion_error&& other) = default;

    conversion_error& operator=(const conversion_error& other) = default;

    conversion_error& operator=(conversion_error&& other) = default;
    
    std::error_code code() const
    {
        return ec_;
    }
    
    const std::string& message_arg() const 
    {
        return message_arg_;
    }

    std::string message() const
    {
        std::string str{message_arg_};
        if (!str.empty())
        {
            str.append(": ");
        }
        str.append(ec_.message());
        return str;
    }
};

template <typename T>
using conversion_result = expected<T,conversion_error>;

} // namespace jsoncons

#endif // JSONCONS_CONVERSION_RESULT_HPP
