/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_JSONPATH_ERROR_HPP
#define JSONCONS_EXT_JSONPATH_JSONPATH_ERROR_HPP

#include <cstddef>
#include <string>
#include <system_error>
#include <type_traits>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/json_exception.hpp>

namespace jsoncons { 
namespace jsonpath {

    enum class jsonpath_errc 
    {
        success = 0,
        expected_root_or_current_node,
        expected_lbracket_or_dot,
        expected_single_quote_or_digit,
        expected_root_or_function,
        expected_current_node,
        expected_rparen,
        expected_rbracket,
        expected_separator,
        expected_forward_slash,
        expected_slice_start,
        expected_slice_end,
        expected_slice_step,
        expected_bracket_specifier_or_union,
        unexpected_operator,
        invalid_function_name,
        invalid_argument,
        invalid_arity,
        function_name_not_found,
        parse_error_in_filter,
        argument_parse_error,
        unidentified_error,
        unexpected_eof,
        expected_colon_dot_left_bracket_comma_or_rbracket,
        invalid_argument_to_unflatten,
        invalid_flattened_key,
        step_cannot_be_zero,
        invalid_number,
        illegal_escaped_character,
        invalid_codepoint,
        unknown_function,
        invalid_type,
        unbalanced_parentheses,
        syntax_error,
        expected_comparator,
        expected_or,
        expected_and,
        expected_comma_or_rparen,
        expected_comma_or_rbracket,
        expected_relative_path
    };

    class jsonpath_error_category_impl
       : public std::error_category
    {
    public:
        const char* name() const noexcept override
        {
            return "jsoncons/jsonpath";
        }
        std::string message(int ev) const override
        {
            switch (static_cast<jsonpath_errc>(ev))
            {
                case jsonpath_errc::expected_root_or_current_node:
                    return "Expected '$' or '@'";
                case jsonpath_errc::expected_lbracket_or_dot:
                    return "Expected '[' or '.'";
                case jsonpath_errc::expected_single_quote_or_digit:
                    return "Expected '\'' or digit";
                case jsonpath_errc::expected_root_or_function:
                    return "Expected '$' or function expression";
                case jsonpath_errc::expected_current_node:
                    return "Expected @";
                case jsonpath_errc::expected_rbracket:
                    return "Expected ]";
                case jsonpath_errc::expected_rparen:
                    return "Expected )";
                case jsonpath_errc::expected_slice_start:
                    return "Expected slice start";
                case jsonpath_errc::expected_slice_end:
                    return "Expected slice end";
                case jsonpath_errc::expected_slice_step:
                    return "Expected slice step";
                case jsonpath_errc::expected_separator:
                    return "Expected dot or left bracket separator";
                case jsonpath_errc::expected_forward_slash:
                    return "Invalid path filter, expected '/'";
                case jsonpath_errc::expected_bracket_specifier_or_union:
                    return "Expected index, single or double quoted name, expression, filter, absolute ('$') path or relative ('@') path";
                case jsonpath_errc::invalid_function_name:
                    return "Invalid function name";
                case jsonpath_errc::invalid_argument:
                    return "Invalid argument type";
                case jsonpath_errc::invalid_arity:
                    return "Incorrect number of arguments";
                case jsonpath_errc::function_name_not_found:
                    return "Function name not found";
                case jsonpath_errc::parse_error_in_filter:
                    return "Could not parse JSON expression in a JSONPath filter";
                case jsonpath_errc::argument_parse_error:
                    return "Could not parse JSON expression passed to JSONPath function";
                case jsonpath_errc::unidentified_error:
                    return "Unidentified error";
                case jsonpath_errc::unexpected_eof:
                    return "Unexpected EOF while parsing jsonpath expression";
                case jsonpath_errc::expected_colon_dot_left_bracket_comma_or_rbracket:
                    return "Expected ':', '.', '[', ',', or ']'";
                case jsonpath_errc::invalid_argument_to_unflatten:
                    return "Argument to unflatten must be an object";
                case jsonpath_errc::invalid_flattened_key:
                    return "Flattened key is invalid";
                case jsonpath_errc::step_cannot_be_zero:
                    return "Slice step cannot be zero";
                case jsonpath_errc::invalid_number:
                    return "Invalid number";
                case jsonpath_errc::illegal_escaped_character:
                    return "Illegal escaped character";
                case jsonpath_errc::invalid_codepoint:
                    return "Invalid codepoint";
                case jsonpath_errc::unknown_function:
                    return "Unknown function";
                case jsonpath_errc::invalid_type:
                    return "Invalid type";
                case jsonpath_errc::unbalanced_parentheses:
                    return "Unbalanced parentheses";
                case jsonpath_errc::syntax_error:
                    return "Syntax error";
                case jsonpath_errc::expected_comparator:
                    return "Expected comparator";
                case jsonpath_errc::expected_or:
                    return "Expected operator '||'";
                case jsonpath_errc::expected_and:
                    return "Expected operator '&&'";
                case jsonpath_errc::expected_comma_or_rparen:
                    return "Expected comma or right parenthesis";
                case jsonpath_errc::expected_comma_or_rbracket:
                    return "Expected comma or right bracket";
                case jsonpath_errc::expected_relative_path:
                    return "Expected unquoted string, or single or double quoted string, or index or '*'";
                default:
                    return "Unknown jsonpath parser error";
            }
        }
    };

    inline
    const std::error_category& jsonpath_error_category() noexcept
    {
      static jsonpath_error_category_impl instance;
      return instance;
    }

    inline 
    std::error_code make_error_code(jsonpath_errc result) noexcept
    {
        return std::error_code(static_cast<int>(result),jsonpath_error_category());
    }

} // namespace jsonpath
} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::jsonpath::jsonpath_errc> : public true_type
    {
    };
} // namespace std

namespace jsoncons { 
namespace jsonpath {

    class jsonpath_error : public std::system_error, public virtual json_exception
    {
        std::size_t line_number_{0};
        std::size_t column_number_{0};
        mutable std::string what_;
    public:
        jsonpath_error(std::error_code ec)
            : std::system_error(ec)
        {
        }
        jsonpath_error(std::error_code ec, const std::string& what_arg)
            : std::system_error(ec, what_arg)
        {
        }
        jsonpath_error(std::error_code ec, std::size_t position)
            : std::system_error(ec), column_number_(position)
        {
        }
        jsonpath_error(std::error_code ec, std::size_t line, std::size_t column)
            : std::system_error(ec), line_number_(line), column_number_(column)
        {
        }
        jsonpath_error(const jsonpath_error& other) = default;

        jsonpath_error(jsonpath_error&& other) = default;
        
        ~jsonpath_error() override = default;

        const char* what() const noexcept override
        {
            if (what_.empty())
            {
                JSONCONS_TRY
                {
                    what_.append(std::system_error::what());
                    if (line_number_ != 0 && column_number_ != 0)
                    {
                        what_.append(" at line ");
                        what_.append(std::to_string(line_number_));
                        what_.append(" and column ");
                        what_.append(std::to_string(column_number_));
                    }
                    else if (column_number_ != 0)
                    {
                        what_.append(" at position ");
                        what_.append(std::to_string(column_number_));
                    }
                    return what_.c_str();
                }
                JSONCONS_CATCH(...)
                {
                    return std::system_error::what();
                }
            }
            else
            {
                return what_.c_str();
            }
        }

        std::size_t line() const noexcept
        {
            return line_number_;
        }

        std::size_t column() const noexcept
        {
            return column_number_;
        }
    };

} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_JSONPATH_ERROR_HPP
