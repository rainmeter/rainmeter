// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_EXCEPTION_HPP
#define JSONCONS_JSON_EXCEPTION_HPP

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string> // std::string
#include <system_error> // std::error_code

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/unicode_traits.hpp> // unicode_traits::convert

namespace jsoncons {

    // json_exception

    class json_exception
    {
    public:
        virtual ~json_exception() = default;
        virtual const char* what() const noexcept = 0;
    };

    // json_runtime_error

    template <typename Base,typename Enable = void>
    class json_runtime_error
    {
    };

    template <typename Base>
    class json_runtime_error<Base,
                             typename std::enable_if<std::is_convertible<Base*,std::exception*>::value &&
                                                     ext_traits::is_constructible_from_string<Base>::value>::type> 
        : public Base, public virtual json_exception
    {
    public:
        json_runtime_error(const std::string& s) noexcept
            : Base(s)
        {
        }
        ~json_runtime_error() noexcept
        {
        }
        const char* what() const noexcept override
        {
            return Base::what();
        }
    };

    class bad_cast : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class key_not_found : public std::out_of_range, public virtual json_exception
    {
        std::string name_;
        mutable std::string what_;
    public:
        template <typename CharT>
        explicit key_not_found(const CharT* key, std::size_t length) noexcept
            : std::out_of_range("Key not found")
        {
            JSONCONS_TRY
            {
                unicode_traits::convert(key, length, name_,
                                 unicode_traits::conv_flags::strict);
            }
            JSONCONS_CATCH(...)
            {
            }
        }

        virtual ~key_not_found() noexcept
        {
        }

        const char* what() const noexcept override
        {
            if (what_.empty())
            {
                JSONCONS_TRY
                {
                    what_.append(std::out_of_range::what());
                    what_.append(": '");
                    what_.append(name_);
                    what_.append("'");
                    return what_.c_str();
                }
                JSONCONS_CATCH(...)
                {
                    return std::out_of_range::what();
                }
            }
            else
            {
                return what_.c_str();
            }
        }
    };

    class not_an_object : public std::runtime_error, public virtual json_exception
    {
        std::string name_;
        mutable std::string what_;
    public:
        template <typename CharT>
        explicit not_an_object(const CharT* key, std::size_t length) noexcept
            : std::runtime_error("Attempting to access a member of a value that is not an object")
        {
            JSONCONS_TRY
            {
                unicode_traits::convert(key, length, name_,
                                 unicode_traits::conv_flags::strict);
            }
            JSONCONS_CATCH(...)
            {
            }
        }

        virtual ~not_an_object() noexcept
        {
        }
        const char* what() const noexcept override
        {
            if (what_.empty())
            {
                JSONCONS_TRY
                {
                    what_.append(std::runtime_error::what());
                    what_.append(": '");
                    what_.append(name_);
                    what_.append("'");
                    return what_.c_str();
                }
                JSONCONS_CATCH(...)
                {
                    return std::runtime_error::what();
                }
            }
            else
            {
                return what_.c_str();
            }
        }
    };

    class ser_error : public std::exception, public virtual json_exception
    {
        std::string err_;
        std::error_code ec_;
        std::size_t line_{0};
        std::size_t column_{0};
    public:
        ser_error(std::error_code ec)
            : ec_(ec)
        {
            err_ = to_what_arg(ec); 
        }
        ser_error(std::error_code ec, const std::string& what_arg)
            : ec_(ec)
        {
            err_ = to_what_arg(ec, what_arg.c_str()); 
        }
        ser_error(std::error_code ec, const char* what_arg)
            : ec_(ec)
        {
            err_ = to_what_arg(ec, what_arg); 
        }
        ser_error(std::error_code ec, std::size_t position)
            : ec_(ec), column_(position)
        {
            err_ = to_what_arg(ec, "", 0, position); 
        }
        ser_error(std::error_code ec, const std::string& what_arg, std::size_t position)
            : ec_(ec), column_(position)
        {
            err_ = to_what_arg(ec, what_arg.c_str(), 0, position); 
        }
        ser_error(std::error_code ec, const char* what_arg, std::size_t position)
            : ec_(ec), column_(position)
        {
            err_ = to_what_arg(ec, what_arg, 0, position); 
        }
        ser_error(std::error_code ec, std::size_t line, std::size_t column)
            : ec_(ec), line_(line), column_(column)
        {
            err_ = to_what_arg(ec, "", line, column); 
        }
        ser_error(std::error_code ec, const std::string& what_arg, std::size_t line, std::size_t column)
            : ec_(ec), line_(line), column_(column)
        {
            err_ = to_what_arg(ec, what_arg.c_str(), line, column); 
        }
        ser_error(std::error_code ec, const char* what_arg, std::size_t line, std::size_t column)
            : ec_(ec), line_(line), column_(column)
        {
            err_ = to_what_arg(ec, what_arg, line, column); 
        }
        ser_error(const ser_error& other) = default;

        ser_error& operator=(const ser_error& other) = default;

        const char* what() const noexcept final
        {
            return err_.c_str();
        }
        
        std::error_code code() const
        {
            return ec_;
        }

        std::size_t line() const noexcept
        {
            return line_;
        }

        std::size_t column() const noexcept
        {
            return column_;
        }
    private:
        static std::string to_what_arg(std::error_code ec, const char* s="", std::size_t line=0, std::size_t column=0)
        {
            std::string what_arg(s);
            if (!what_arg.empty())
            {
                what_arg.append(": ");
            }
            what_arg.append(ec.message());
            if (line != 0 && column != 0)
            {
                what_arg.append(" at line ");
                what_arg.append(std::to_string(line));
                what_arg.append(" and column ");
                what_arg.append(std::to_string(column));
            }
            else if (column != 0)
            {
                what_arg.append(" at position ");
                what_arg.append(std::to_string(column));
            }
            return what_arg; 
        }
    };

} // namespace jsoncons

#endif // JSONCONS_JSON_EXCEPTION_HPP
