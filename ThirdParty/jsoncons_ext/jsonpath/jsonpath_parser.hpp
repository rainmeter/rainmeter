// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_JSONPATH_PARSER_HPP
#define JSONCONS_EXT_JSONPATH_JSONPATH_PARSER_HPP

#include <algorithm> // std::reverse
#include <cstddef>
#include <cstdint>
#include <regex>
#include <system_error>
#include <type_traits> // std::is_const
#include <utility> // std::move
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

#include <jsoncons_ext/jsonpath/token_evaluator.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/path_node.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_selector.hpp>

namespace jsoncons { 
namespace jsonpath {
namespace detail {
     
    enum class path_state 
    {
        start,
        root_or_current_node,
        expect_function_expr,
        relative_path,
        relative_location,
        parent_operator,
        ancestor_depth,
        filter_expression,
        expression_rhs,
        recursive_descent_or_expression_lhs,
        path_or_literal_or_function,
        json_text_or_function,
        json_text_or_function_name,
        json_text_string,
        json_value,
        json_text,
        identifier_or_function_expr,
        name_or_lbracket,
        unquoted_string,
        anything,
        number,
        function_expression,
        argument,
        zero_or_one_arguments,
        one_or_more_arguments,
        identifier,
        single_quoted_string,
        double_quoted_string,
        bracketed_unquoted_name_or_union,
        union_expression,
        identifier_or_union,
        bracket_specifier_or_union,
        bracketed_wildcard,
        index_or_slice,
        wildcard_or_union,
        union_element,
        index_or_slice_or_union,
        integer,
        digit,
        slice_expression_stop,
        slice_expression_step,
        comma_or_rbracket,
        expect_rparen,
        expect_rbracket,
        quoted_string_escape_char,
        escape_u1, 
        escape_u2, 
        escape_u3, 
        escape_u4, 
        escape_expect_surrogate_pair1, 
        escape_expect_surrogate_pair2, 
        escape_u5, 
        escape_u6, 
        escape_u7, 
        escape_u8,
        expression,
        comparator_expression,
        eq_or_regex,
        expect_regex,
        regex,
        regex_options,
        regex_pattern,
        cmp_lt_or_lte,
        cmp_gt_or_gte,
        cmp_ne,
        expect_or,
        expect_and
    };

    template <typename Json,
             class JsonReference>
    class jsonpath_evaluator : public ser_context
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using path_value_pair_type = path_value_pair<Json,JsonReference>;
        using value_type = Json;
        using reference = JsonReference;
        using pointer = typename path_value_pair_type::value_pointer;
        using token_type = token<Json,JsonReference>;
        using path_expression_type = path_expression<Json,JsonReference>;
        using token_evaluator_type = token_evaluator<Json,JsonReference>;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using selector_type = jsonpath_selector<Json,JsonReference>;

    private:

        allocator_type alloc_;
        std::size_t line_{1};
        std::size_t column_{1};
        const char_type* begin_input_{nullptr};
        const char_type* input_end_{nullptr};
        const char_type* p_{nullptr};

        using argument_type = std::vector<pointer>;
        std::vector<argument_type> function_stack_;
        std::vector<path_state> state_stack_;
        std::vector<token_type> output_stack_;
        std::vector<token_type> operator_stack_;

    public:
        jsonpath_evaluator(const allocator_type& alloc = allocator_type())
            : alloc_(alloc)
        {
        }

        jsonpath_evaluator(std::size_t line, std::size_t column, 
            const allocator_type& alloc = allocator_type())
            : alloc_(alloc), line_(line), column_(column)
        {
        }

        std::size_t line() const final
        {
            return line_;
        }

        std::size_t column() const final
        {
            return column_;
        }

        path_expression_type compile(static_resources<value_type>& resources, const string_view_type& path)
        {
            std::error_code ec;
            auto result = compile(resources, path, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(jsonpath_error(ec, line_, column_));
            }
            return result;
        }

        path_expression_type compile(static_resources<value_type>& resources, 
                                     const string_view_type& path, 
                                     std::error_code& ec)
        {
            std::size_t selector_id = 0;

            string_type buffer(alloc_);
            string_type buffer2(alloc_);
            uint32_t cp = 0;
            uint32_t cp2 = 0;

            begin_input_ = path.data();
            input_end_ = path.data() + path.length();
            p_ = begin_input_;

            slice slic;
            bool paths_required = false;
            int ancestor_depth = 0;

            state_stack_.emplace_back(path_state::start);
            while (p_ < input_end_ && !state_stack_.empty())
            {
                switch (state_stack_.back())
                {
                    case path_state::start: 
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '$':
                            case '@':
                            {
                                push_token(resources, token_type(resources.new_selector(current_node_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.emplace_back(path_state::relative_location);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                state_stack_.emplace_back(path_state::relative_location);
                                state_stack_.emplace_back(path_state::expect_function_expr);
                                state_stack_.emplace_back(path_state::unquoted_string);
                                break;
                            }
                        }
                        break;
                    }
                    case path_state::root_or_current_node: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '$':
                                push_token(resources, token_type(root_node_arg), ec);
                                push_token(resources, token_type(resources.new_selector(root_selector<Json,JsonReference>(selector_id++))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case '@':
                                push_token(resources, token_type(current_node_arg), ec); // ISSUE
                                push_token(resources, token_type(resources.new_selector(current_node_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::syntax_error;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::recursive_descent_or_expression_lhs:
                        switch (*p_)
                        {
                            case '.':
                                push_token(resources, token_type(resources.new_selector(recursive_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::name_or_lbracket;
                                break;
                            default:
                                state_stack_.back() = path_state::relative_path;
                                break;
                        }
                        break;
                    case path_state::name_or_lbracket: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '[': // [ can follow ..
                                state_stack_.back() = path_state::bracket_specifier_or_union;
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.clear();
                                state_stack_.back() = path_state::relative_path;
                                break;
                        }
                        break;
                    case path_state::json_text:
                    {
                        //std::cout << "literal: " << buffer << "\n";
                        push_token(resources, token_type(literal_arg, Json(buffer,semantic_tag::none,alloc_)), ec);
                        if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        buffer.clear();
                        state_stack_.pop_back(); // json_value
                        break;
                    }
                    case path_state::path_or_literal_or_function: 
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '$':
                            case '@':
                                state_stack_.back() = path_state::relative_location;
                                state_stack_.push_back(path_state::root_or_current_node);
                                break;
                            case '(':
                            {
                                ++p_;
                                ++column_;
                                push_token(resources, lparen_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::expect_rparen;
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                break;
                            }
                            case '\'':
                                state_stack_.back() = path_state::json_text;
                                state_stack_.emplace_back(path_state::single_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                state_stack_.back() = path_state::json_text;
                                state_stack_.emplace_back(path_state::double_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '!':
                            {
                                ++p_;
                                ++column_;
                                push_token(resources, token_type(resources.get_unary_not()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                break;
                            }
                            case '-':
                            {
                                ++p_;
                                ++column_;
                                push_token(resources, token_type(resources.get_unary_minus()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                break;
                            }
                            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                            {
                                state_stack_.back() = path_state::json_value;
                                state_stack_.emplace_back(path_state::number);
                                break;
                            }
                            default:
                            {
                                state_stack_.back() = path_state::json_text_or_function_name;
                                break;
                            }
                        }
                        break;
                    }
                    case path_state::json_text_or_function:
                    {
                        switch(*p_)
                        {
                            case '(':
                            {
                                auto f = resources.get_function(buffer, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                buffer.clear();
                                push_token(resources, current_node_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                push_token(resources, token_type(f), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::function_expression;
                                state_stack_.emplace_back(path_state::zero_or_one_arguments);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                json_decoder<Json> decoder(alloc_);
                                basic_json_parser<char_type> parser;
                                parser.update(buffer.data(),buffer.size());
                                parser.parse_some(decoder, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                parser.finish_parse(decoder, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                push_token(resources, token_type(literal_arg, decoder.get_result()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                break;
                            }
                        }
                        break;
                    }
                    case path_state::json_value:
                    {
                        json_decoder<Json> decoder(alloc_);
                        basic_json_parser<char_type> parser;
                        parser.update(buffer.data(),buffer.size());
                        parser.parse_some(decoder, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        parser.finish_parse(decoder, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        push_token(resources, token_type(literal_arg, decoder.get_result()), ec);
                        if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        buffer.clear();
                        state_stack_.pop_back();
                        break;
                    }
                    case path_state::json_text_or_function_name:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '{':
                            case '[':
                            {
                                json_decoder<Json> decoder(alloc_);
                                basic_json_parser<char_type> parser;
                                parser.update(p_,input_end_ - p_);
                                parser.parse_some(decoder, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                parser.finish_parse(decoder, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                push_token(resources, token_type(literal_arg, decoder.get_result()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                p_ = parser.current();
                                column_ = column_ + parser.column() - 1;
                                break;
                            }
                            case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                state_stack_.back() = path_state::json_text_or_function;
                                state_stack_.emplace_back(path_state::number);
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                state_stack_.back() = path_state::json_text_or_function;
                                state_stack_.emplace_back(path_state::json_text_string);
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                state_stack_.back() = path_state::json_text_or_function;
                                state_stack_.emplace_back(path_state::unquoted_string);
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        };
                        break;
                    case path_state::number: 
                        switch (*p_)
                        {
                            case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                            case 'e':case 'E':case '.':
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                state_stack_.pop_back(); // number
                                break;
                        };
                        break;
                    case path_state::json_text_string: 
                        switch (*p_)
                        {
                            case '\\':
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                if (p_ == input_end_)
                                {
                                    ec = jsonpath_errc::unexpected_eof;
                                    return path_expression_type(alloc_);
                                }
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                buffer.push_back(*p_);
                                state_stack_.pop_back(); 
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        };
                        break;
                    case path_state::relative_path: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '*':
                                push_token(resources, token_type(resources.new_selector(wildcard_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case '\'':
                                state_stack_.back() = path_state::identifier;
                                state_stack_.emplace_back(path_state::single_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                state_stack_.back() = path_state::identifier;
                                state_stack_.emplace_back(path_state::double_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '[':
                            case '.':
                                ec = jsonpath_errc::expected_relative_path;
                                return path_expression_type(alloc_);
                            default:
                                buffer.clear();
                                state_stack_.back() = path_state::identifier_or_function_expr;
                                state_stack_.emplace_back(path_state::unquoted_string);
                                break;
                        }
                        break;
                    case path_state::identifier_or_function_expr:
                    {
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '(':
                            {
                                auto f = resources.get_function(buffer, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                buffer.clear();
                                push_token(resources, current_node_arg, ec);
                                push_token(resources, token_type(f), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::function_expression;
                                state_stack_.emplace_back(path_state::zero_or_one_arguments);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back(); 
                                break;
                            }
                        }
                        break;
                    }
                    case path_state::expect_function_expr:
                    {
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '(':
                            {
                                auto f = resources.get_function(buffer, ec);
                                if (JSONCONS_UNLIKELY(ec))
                                {
                                    return path_expression_type(alloc_);
                                }
                                buffer.clear();
                                push_token(resources, current_node_arg, ec);
                                push_token(resources, token_type(f), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::function_expression;
                                state_stack_.emplace_back(path_state::zero_or_one_arguments);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                ec = jsonpath_errc::expected_root_or_function;
                                return path_expression_type(alloc_);
                            }
                        }
                        break;
                    }
                    case path_state::function_expression:
                    {
                        
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                                push_token(resources, token_type(current_node_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                push_token(resources, token_type(begin_expression_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.emplace_back(path_state::argument);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                            case ')':
                            {
                                push_token(resources, token_type(end_function_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); 
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                                ec = jsonpath_errc::syntax_error;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::zero_or_one_arguments:
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ')':
                                state_stack_.pop_back();
                                break;
                            default:
                                push_token(resources, token_type(begin_expression_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::one_or_more_arguments;
                                state_stack_.emplace_back(path_state::argument);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                break;
                        }
                        break;
                    }
                    case path_state::one_or_more_arguments:
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ')':
                                state_stack_.pop_back();
                                break;
                            case ',':
                                push_token(resources, token_type(begin_expression_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.emplace_back(path_state::argument);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                        }
                        break;
                    }
                    case path_state::argument:
                    {
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                            case ')':
                            {
                                push_token(resources, token_type(end_argument_expression_arg), ec);
                                push_token(resources, argument_arg, ec);
                                //push_token(resources, argument_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                break;
                            }
                            default:
                                ec = jsonpath_errc::expected_comma_or_rparen;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::unquoted_string: 
                        switch (*p_)
                        {
                            case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z':
                            case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z':
                            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                            case '_':
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                if (typename std::make_unsigned<char_type>::type(*p_) > 127)
                                {
                                    buffer.push_back(*p_);
                                    ++p_;
                                    ++column_;
                                }
                                else
                                {
                                    state_stack_.pop_back(); // unquoted_string
                                }
                                break;
                        };
                        break;                    
                    case path_state::relative_location: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '.':
                                state_stack_.emplace_back(path_state::recursive_descent_or_expression_lhs);
                                ++p_;
                                ++column_;
                                break;
                            case '[':
                                state_stack_.emplace_back(path_state::bracket_specifier_or_union);
                                ++p_;
                                ++column_;
                                break;
                            case '^':
                                ancestor_depth = 0;
                                state_stack_.emplace_back(path_state::parent_operator);
                                state_stack_.emplace_back(path_state::ancestor_depth);
                                break;
                            default:
                                state_stack_.pop_back();
                                break;
                        };
                        break;
                    case path_state::parent_operator: 
                    {
                        push_token(resources, token_type(resources.new_selector(parent_node_selector<Json,JsonReference>(ancestor_depth))), ec);
                        paths_required = true;
                        ancestor_depth = 0;
                        ++p_;
                        ++column_;
                        state_stack_.pop_back();
                        break;
                    }
                    case path_state::ancestor_depth: 
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '^':
                            {
                                ++ancestor_depth;
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                state_stack_.pop_back();
                                break;
                            }
                        }
                        break;
                    }
                    case path_state::expression_rhs: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '.':
                                state_stack_.emplace_back(path_state::recursive_descent_or_expression_lhs);
                                ++p_;
                                ++column_;
                                break;
                            case '[':
                                state_stack_.emplace_back(path_state::bracket_specifier_or_union);
                                ++p_;
                                ++column_;
                                break;
                            case ')':
                            {
                                state_stack_.pop_back();
                                break;
                            }
                            case '|':
                                ++p_;
                                ++column_;
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                state_stack_.emplace_back(path_state::expect_or);
                                break;
                            case '&':
                                ++p_;
                                ++column_;
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                state_stack_.emplace_back(path_state::expect_and);
                                break;
                            case '<':
                            case '>':
                            {
                                state_stack_.emplace_back(path_state::comparator_expression);
                                break;
                            }
                            case '=':
                            {
                                state_stack_.emplace_back(path_state::eq_or_regex);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '!':
                            {
                                ++p_;
                                ++column_;
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                state_stack_.emplace_back(path_state::cmp_ne);
                                break;
                            }
                            case '+':
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                push_token(resources, token_type(resources.get_plus_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                break;
                            case '-':
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                push_token(resources, token_type(resources.get_minus_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                break;
                            case '*':
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                push_token(resources, token_type(resources.get_mult_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                break;
                            case '/':
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                push_token(resources, token_type(resources.get_div_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                break;
                            case '%':
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                push_token(resources, token_type(resources.get_modulus_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                ++p_;
                                ++column_;
                                break;
                            case ']':
                            case ',':
                                state_stack_.pop_back();
                                break;
                            default:
                                ec = jsonpath_errc::expected_separator;
                                return path_expression_type(alloc_);
                        };
                        break;
                    case path_state::expect_or:
                    {
                        switch (*p_)
                        {
                            case '|':
                                push_token(resources, token_type(resources.get_or_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); 
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_or;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::expect_and:
                    {
                        switch(*p_)
                        {
                            case '&':
                                push_token(resources, token_type(resources.get_and_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); // expect_and
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_and;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::comparator_expression:
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '<':
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::path_or_literal_or_function;
                                state_stack_.emplace_back(path_state::cmp_lt_or_lte);
                                break;
                            case '>':
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::path_or_literal_or_function;
                                state_stack_.emplace_back(path_state::cmp_gt_or_gte);
                                break;
                            default:
                                if (state_stack_.size() > 1)
                                {
                                    state_stack_.pop_back();
                                }
                                else
                                {
                                    ec = jsonpath_errc::syntax_error;
                                    return path_expression_type(alloc_);
                                }
                                break;
                        }
                        break;
                    case path_state::eq_or_regex:
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '=':
                            {
                                push_token(resources, token_type(resources.get_eq_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::path_or_literal_or_function;
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '~':
                            {
                                ++p_;
                                ++column_;
                                state_stack_.emplace_back(path_state::expect_regex);
                                break;
                            }
                            default:
                                if (state_stack_.size() > 1)
                                {
                                    state_stack_.pop_back();
                                }
                                else
                                {
                                    ec = jsonpath_errc::syntax_error;
                                    return path_expression_type(alloc_);
                                }
                                break;
                        }
                        break;
                    case path_state::expect_regex: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '/':
                                state_stack_.back() = path_state::regex;
                                state_stack_.emplace_back(path_state::regex_options);
                                state_stack_.emplace_back(path_state::regex_pattern);
                                ++p_;
                                ++column_;
                                break;
                            default: 
                                ec = jsonpath_errc::expected_forward_slash;
                                return path_expression_type(alloc_);
                        };
                        break;
                    case path_state::regex: 
                    {
                        std::regex::flag_type options = std::regex_constants::ECMAScript; 
                        if (buffer2.find('i') != string_type::npos)
                        {
                            options |= std::regex_constants::icase;
                        }
                        std::basic_regex<char_type> pattern(buffer, options);
                        push_token(resources, resources.get_regex_operator(std::move(pattern)), ec);
                        if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        buffer.clear();
                        buffer2.clear();
                        state_stack_.pop_back();
                        break;
                    }
                    case path_state::regex_pattern: 
                    {
                        switch (*p_)
                        {                   
                            case '/':
                                {
                                    state_stack_.pop_back();
                                    ++p_;
                                    ++column_;
                                }
                                break;

                            default: 
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        }
                        break;
                    }
                    case path_state::regex_options: 
                    {
                        if (*p_ == 'i')
                        {           
                            buffer2.push_back(*p_);
                            ++p_;
                            ++column_;
                        }
                        else
                        {
                            state_stack_.pop_back();
                        }
                        break;
                    }
                    case path_state::cmp_lt_or_lte:
                    {
                        switch(*p_)
                        {
                            case '=':
                                push_token(resources, token_type(resources.get_lte_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            default:
                                push_token(resources, token_type(resources.get_lt_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                break;
                        }
                        break;
                    }
                    case path_state::cmp_gt_or_gte:
                    {
                        switch(*p_)
                        {
                            case '=':
                                push_token(resources, token_type(resources.get_gte_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); 
                                ++p_;
                                ++column_;
                                break;
                            default:
                                //std::cout << "Parse: gt_operator\n";
                                push_token(resources, token_type(resources.get_gt_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); 
                                break;
                        }
                        break;
                    }
                    case path_state::cmp_ne:
                    {
                        switch(*p_)
                        {
                            case '=':
                                push_token(resources, token_type(resources.get_ne_operator()), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back(); 
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_comparator;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::identifier:
                        push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                        if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        buffer.clear();
                        state_stack_.pop_back(); 
                        break;
                    case path_state::single_quoted_string:
                        switch (*p_)
                        {
                            case '\'':
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case '\\':
                                state_stack_.emplace_back(path_state::quoted_string_escape_char);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        };
                        break;
                    case path_state::double_quoted_string: 
                        switch (*p_)
                        {
                            case '\"':
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case '\\':
                                state_stack_.emplace_back(path_state::quoted_string_escape_char);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        };
                        break;
                    case path_state::comma_or_rbracket:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                                state_stack_.back() = path_state::bracket_specifier_or_union;
                                ++p_;
                                ++column_;
                                break;
                            case ']':
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_comma_or_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::expect_rbracket:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']':
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::expect_rparen:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ')':
                                ++p_;
                                ++column_;
                                push_token(resources, rparen_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::expression_rhs;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rparen;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::bracket_specifier_or_union:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '(':
                            {
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(begin_expression_arg), ec);
                                push_token(resources, lparen_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::expression);
                                state_stack_.emplace_back(path_state::expect_rparen);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '?':
                            {
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(begin_filter_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::filter_expression);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '*':
                                state_stack_.back() = path_state::wildcard_or_union;
                                ++p_;
                                ++column_;
                                break;
                            case '\'':
                                state_stack_.back() = path_state::identifier_or_union;
                                state_stack_.push_back(path_state::single_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                state_stack_.back() = path_state::identifier_or_union;
                                state_stack_.push_back(path_state::double_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case ':': // slice_expression
                                state_stack_.back() = path_state::index_or_slice_or_union;
                                break;
                            case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                state_stack_.back() = path_state::index_or_slice_or_union;
                                state_stack_.emplace_back(path_state::integer);
                                break;
                            case '$':
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, root_node_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::relative_location);                                
                                ++p_;
                                ++column_;
                                break;
                            case '@':
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(current_node_arg), ec); // ISSUE
                                push_token(resources, token_type(resources.new_selector(current_node_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::relative_location);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_bracket_specifier_or_union;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::union_element:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ':': // slice_expression
                                state_stack_.back() = path_state::index_or_slice;
                                break;
                            case '-':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                state_stack_.back() = path_state::index_or_slice;
                                state_stack_.emplace_back(path_state::integer);
                                break;
                            case '(':
                            {
                                push_token(resources, token_type(begin_expression_arg), ec);
                                push_token(resources, lparen_arg, ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::expression;
                                state_stack_.emplace_back(path_state::expect_rparen);
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '?':
                            {
                                push_token(resources, token_type(begin_filter_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::filter_expression;
                                state_stack_.emplace_back(path_state::expression_rhs);
                                state_stack_.emplace_back(path_state::path_or_literal_or_function);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case '*':
                                push_token(resources, token_type(resources.new_selector(wildcard_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::relative_location;
                                ++p_;
                                ++column_;
                                break;
                            case '$':
                                push_token(resources, token_type(root_node_arg), ec);
                                push_token(resources, token_type(resources.new_selector(root_selector<Json,JsonReference>(selector_id++))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::relative_location;
                                ++p_;
                                ++column_;
                                break;
                            case '@':
                                push_token(resources, token_type(current_node_arg), ec); // ISSUE
                                push_token(resources, token_type(resources.new_selector(current_node_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::relative_location;
                                ++p_;
                                ++column_;
                                break;
                            case '\'':
                                state_stack_.back() = path_state::identifier;
                                state_stack_.push_back(path_state::single_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            case '\"':
                                state_stack_.back() = path_state::identifier;
                                state_stack_.push_back(path_state::double_quoted_string);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_bracket_specifier_or_union;
                                return path_expression_type(alloc_);
                        }
                        break;

                    case path_state::integer:
                        switch(*p_)
                        {
                            case '-':
                                buffer.push_back(*p_);
                                state_stack_.back() = path_state::digit;
                                ++p_;
                                ++column_;
                                break;
                            default:
                                state_stack_.back() = path_state::digit;
                                break;
                        }
                        break;
                    case path_state::digit:
                        switch(*p_)
                        {
                            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                state_stack_.pop_back(); // digit
                                break;
                        }
                        break;
                    case path_state::index_or_slice_or_union:
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']':
                            {
                                if (buffer.empty())
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }
                                int64_t n{0};
                                auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                if (!r)
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }
                                push_token(resources, token_type(resources.new_selector(index_selector<Json,JsonReference>(n))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back(); // index_or_slice_or_union
                                ++p_;
                                ++column_;
                                break;
                            }
                            case ',':
                            {
                                push_token(resources, token_type(begin_union_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                if (buffer.empty())
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }

                                int64_t n{0};
                                auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                if (!r)
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }
                                push_token(resources, token_type(resources.new_selector(index_selector<Json,JsonReference>(n))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}

                                buffer.clear();
                                
                                push_token(resources, token_type(separator_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::union_element);
                                ++p_;
                                ++column_;
                                break;
                            }
                            case ':':
                            {
                                if (!buffer.empty())
                                {
                                    int64_t n{0};
                                    auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                    if (!r)
                                    {
                                        ec = jsonpath_errc::invalid_number;
                                        return path_expression_type(alloc_);
                                    }
                                    slic.start_ = n;
                                    buffer.clear();
                                }
                                push_token(resources, token_type(begin_union_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::slice_expression_stop);
                                state_stack_.emplace_back(path_state::integer);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::slice_expression_stop:
                    {
                        if (!buffer.empty())
                        {
                            int64_t n{0};
                            auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                            if (!r)
                            {
                                ec = jsonpath_errc::invalid_number;
                                return path_expression_type(alloc_);
                            }
                            slic.stop_ = jsoncons::optional<int64_t>(n);
                            buffer.clear();
                        }
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']':
                            case ',':
                                push_token(resources, token_type(resources.new_selector(slice_selector<Json,JsonReference>(slic))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                slic = slice{};
                                state_stack_.pop_back(); // bracket_specifier2
                                break;
                            case ':':
                                state_stack_.back() = path_state::slice_expression_step;
                                state_stack_.emplace_back(path_state::integer);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::slice_expression_step:
                    {
                        if (!buffer.empty())
                        {
                            int64_t n{0};
                            auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                            if (!r)
                            {
                                ec = jsonpath_errc::invalid_number;
                                return path_expression_type(alloc_);
                            }
                            if (n == 0)
                            {
                                ec = jsonpath_errc::step_cannot_be_zero;
                                return path_expression_type(alloc_);
                            }
                            slic.step_ = n;
                            buffer.clear();
                        }
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']':
                            case ',':
                                push_token(resources, token_type(resources.new_selector(slice_selector<Json,JsonReference>(slic))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                slic = slice{};
                                state_stack_.pop_back(); // slice_expression_step
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }

                    case path_state::bracketed_unquoted_name_or_union:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']': 
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case '.':
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::relative_path);                                
                                ++p_;
                                ++column_;
                                break;
                            case '[':
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::relative_path);                                
                                ++p_;
                                ++column_;
                                break;
                            case ',': 
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                push_token(resources, token_type(separator_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::relative_path);                                
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        }
                        break;
                    case path_state::union_expression:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '.':
                                state_stack_.emplace_back(path_state::relative_path);
                                ++p_;
                                ++column_;
                                break;
                            case '[':
                                state_stack_.emplace_back(path_state::bracket_specifier_or_union);
                                ++p_;
                                ++column_;
                                break;
                            case ',': 
                                push_token(resources, token_type(separator_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.emplace_back(path_state::union_element);
                                ++p_;
                                ++column_;
                                break;
                            case ']': 
                                push_token(resources, token_type(end_union_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::identifier_or_union:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']': 
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case ',': 
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                                push_token(resources, token_type(separator_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::union_element);                                
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::bracketed_wildcard:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '[':
                            case ']':
                            case ',':
                            case '.':
                                push_token(resources, token_type(resources.new_selector(wildcard_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::index_or_slice:
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                            case ']':
                            {
                                if (buffer.empty())
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }
                                
                                int64_t n{0};
                                auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                if (!r)
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_expression_type(alloc_);
                                }
                                push_token(resources, token_type(resources.new_selector(index_selector<Json,JsonReference>(n))), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}

                                buffer.clear();
                                
                                state_stack_.pop_back(); // bracket_specifier
                                break;
                            }
                            case ':':
                            {
                                if (!buffer.empty())
                                {
                                    int64_t n{0};
                                    auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                    if (!r)
                                    {
                                        ec = jsonpath_errc::invalid_number;
                                        return path_expression_type(alloc_);
                                    }
                                    slic.start_ = n;
                                    buffer.clear();
                                }
                                state_stack_.back() = path_state::slice_expression_stop;
                                state_stack_.emplace_back(path_state::integer);
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::wildcard_or_union:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']': 
                                push_token(resources, token_type(resources.new_selector(wildcard_selector<Json,JsonReference>())), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.pop_back();
                                ++p_;
                                ++column_;
                                break;
                            case ',': 
                                push_token(resources, token_type(begin_union_arg), ec);
                                push_token(resources, token_type(resources.new_selector(wildcard_selector<Json,JsonReference>())), ec);
                                push_token(resources, token_type(separator_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                buffer.clear();
                                state_stack_.back() = path_state::union_expression; // union
                                state_stack_.emplace_back(path_state::union_element);                                
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::quoted_string_escape_char:
                        switch (*p_)
                        {
                            case '\"':
                                buffer.push_back('\"');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case '\'':
                                buffer.push_back('\'');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case '\\': 
                                buffer.push_back('\\');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case '/':
                                buffer.push_back('/');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 'b':
                                buffer.push_back('\b');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 'f':
                                buffer.push_back('\f');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 'n':
                                buffer.push_back('\n');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 'r':
                                buffer.push_back('\r');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 't':
                                buffer.push_back('\t');
                                ++p_;
                                ++column_;
                                state_stack_.pop_back();
                                break;
                            case 'u':
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::escape_u1;
                                break;
                            default:
                                ec = jsonpath_errc::illegal_escaped_character;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::escape_u1:
                        cp = append_to_codepoint(0, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u2;
                        break;
                    case path_state::escape_u2:
                        cp = append_to_codepoint(cp, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u3;
                        break;
                    case path_state::escape_u3:
                        cp = append_to_codepoint(cp, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u4;
                        break;
                    case path_state::escape_u4:
                        cp = append_to_codepoint(cp, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        if (unicode_traits::is_high_surrogate(cp))
                        {
                            ++p_;
                            ++column_;
                            state_stack_.back() = path_state::escape_expect_surrogate_pair1;
                        }
                        else
                        {
                            unicode_traits::convert(&cp, 1, buffer);
                            ++p_;
                            ++column_;
                            state_stack_.pop_back();
                        }
                        break;
                    case path_state::escape_expect_surrogate_pair1:
                        switch (*p_)
                        {
                            case '\\': 
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::escape_expect_surrogate_pair2;
                                break;
                            default:
                                ec = jsonpath_errc::invalid_codepoint;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::escape_expect_surrogate_pair2:
                        switch (*p_)
                        {
                            case 'u': 
                                ++p_;
                                ++column_;
                                state_stack_.back() = path_state::escape_u5;
                                break;
                            default:
                                ec = jsonpath_errc::invalid_codepoint;
                                return path_expression_type(alloc_);
                        }
                        break;
                    case path_state::escape_u5:
                        cp2 = append_to_codepoint(0, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u6;
                        break;
                    case path_state::escape_u6:
                        cp2 = append_to_codepoint(cp2, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u7;
                        break;
                    case path_state::escape_u7:
                        cp2 = append_to_codepoint(cp2, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        ++p_;
                        ++column_;
                        state_stack_.back() = path_state::escape_u8;
                        break;
                    case path_state::escape_u8:
                    {
                        cp2 = append_to_codepoint(cp2, *p_, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return path_expression_type(alloc_);
                        }
                        uint32_t codepoint = 0x10000 + ((cp & 0x3FF) << 10) + (cp2 & 0x3FF);
                        unicode_traits::convert(&codepoint, 1, buffer);
                        state_stack_.pop_back();
                        ++p_;
                        ++column_;
                        break;
                    }
                    case path_state::filter_expression:
                    {
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                            case ']':
                            {
                                push_token(resources, token_type(end_filter_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                break;
                            }
                            default:
                                ec = jsonpath_errc::expected_comma_or_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    case path_state::expression:
                    {
                        switch(*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ',':
                            case ']':
                            {
                                push_token(resources, token_type(end_index_expression_arg), ec);
                                if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                                state_stack_.pop_back();
                                break;
                            }
                            default:
                                ec = jsonpath_errc::expected_comma_or_rbracket;
                                return path_expression_type(alloc_);
                        }
                        break;
                    }
                    default:
                        ++p_;
                        ++column_;
                        break;
                }
            }

            if (state_stack_.empty())
            {
                ec = jsonpath_errc::syntax_error;
                return path_expression_type(alloc_);
            }

            while (state_stack_.size() > 1)
            {
                switch (state_stack_.back())
                {
                    case path_state::name_or_lbracket: 
                        state_stack_.back() = path_state::relative_path;
                        break;
                    case path_state::relative_path: 
                        state_stack_.back() = path_state::identifier_or_function_expr;
                        state_stack_.emplace_back(path_state::unquoted_string);
                        break;
                    case path_state::identifier_or_function_expr:
                        if (!buffer.empty()) // Can't be quoted string
                        {
                            push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                            if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        }
                        state_stack_.pop_back(); 
                        break;
                    case path_state::unquoted_string: 
                        state_stack_.pop_back(); // unquoted_string
                        break;                    
                    case path_state::relative_location: 
                        state_stack_.pop_back();
                        break;
                    case path_state::identifier:
                        if (!buffer.empty()) // Can't be quoted string
                        {
                            push_token(resources, token_type(resources.new_selector(identifier_selector<Json,JsonReference>(buffer))), ec);
                            if (JSONCONS_UNLIKELY(ec)) {return path_expression_type(alloc_);}
                        }
                        state_stack_.pop_back(); 
                        break;
                    case path_state::parent_operator: 
                    {
                        push_token(resources, token_type(resources.new_selector(parent_node_selector<Json,JsonReference>(ancestor_depth))), ec);
                        if (JSONCONS_UNLIKELY(ec)) { return path_expression_type(alloc_); }
                        paths_required = true;
                        state_stack_.pop_back(); 
                        break;
                    }
                    case path_state::ancestor_depth: 
                        state_stack_.pop_back(); 
                        break;
                    default:
                        ec = jsonpath_errc::syntax_error;
                        return path_expression_type(alloc_);
                }
            }

            if (state_stack_.size() > 2)
            {
                ec = jsonpath_errc::unexpected_eof;
                return path_expression_type(alloc_);
            }

            //std::cout << "\nTokens\n\n";
            //for (const auto& tok : output_stack_)
            //{
            //    std::cout << tok.to_string(0) << "\n";
            //}
            //std::cout << "\n";

            if (output_stack_.empty() || !operator_stack_.empty())
            {
                ec = jsonpath_errc::unexpected_eof;
                return path_expression_type(alloc_);
            }

            return path_expression_type(output_stack_.back().selector_, paths_required, alloc_);
        }

        void advance_past_space_character()
        {
            switch (*p_)
            {
                case ' ':case '\t':
                    ++p_;
                    ++column_;
                    break;
                case '\r':
                    if (p_+1 < input_end_ && *(p_+1) == '\n')
                    {
                        ++p_;
                    }
                    ++line_;
                    column_ = 1;
                    ++p_;
                    break;
                case '\n':
                    ++line_;
                    column_ = 1;
                    ++p_;
                    break;
                default:
                    break;
            }
        }

        void unwind_rparen(std::error_code& ec)
        {
            auto it = operator_stack_.rbegin();
            while (it != operator_stack_.rend() && !(*it).is_lparen())
            {
                output_stack_.emplace_back(std::move(*it));
                ++it;
            }
            if (it == operator_stack_.rend())
            {
                ec = jsonpath_errc::unbalanced_parentheses;
                return;
            }
            ++it;
            operator_stack_.erase(it.base(),operator_stack_.end());
        }

        void push_token(jsoncons::jsonpath::detail::static_resources<value_type>& resources, token_type&& tok, std::error_code& ec)
        {
            //std::cout << tok.to_string(0) << "\n";
            switch (tok.token_kind())
            {
                case jsonpath_token_kind::begin_filter:
                    output_stack_.emplace_back(std::move(tok));
                    operator_stack_.emplace_back(token_type(lparen_arg));
                    break;
                case jsonpath_token_kind::end_filter:
                {
                    //std::cout << "push_token end_filter 1\n";
                    //for (const auto& tok2 : output_stack_)
                    //{
                    //    std::cout << tok2.to_string(0) << "\n";
                    //}
                    //std::cout << "\n\n";
                    unwind_rparen(ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return;
                    }
                    std::vector<token_type> toks;
                    auto it = output_stack_.rbegin();
                    while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::begin_filter)
                    {
                        toks.emplace_back(std::move(*it));
                        ++it;
                    }
                    if (it == output_stack_.rend())
                    {
                        ec = jsonpath_errc::unbalanced_parentheses;
                        return;
                    }
                    std::reverse(toks.begin(), toks.end());
                    ++it;
                    output_stack_.erase(it.base(),output_stack_.end());

                    if (!output_stack_.empty() && output_stack_.back().is_path())
                    {
                        output_stack_.back().selector_->append_selector(resources.new_selector(filter_selector<Json,JsonReference>(token_evaluator_type(std::move(toks)))));
                    }
                    else
                    {
                        output_stack_.emplace_back(token_type(resources.new_selector(filter_selector<Json,JsonReference>(token_evaluator_type(std::move(toks))))));
                    }
                    //std::cout << "push_token end_filter 2\n";
                    //for (const auto& tok2 : output_stack_)
                    //{
                    //    std::cout << tok2.to_string(0) << "\n";
                    //}
                    //std::cout << "\n\n";
                    break;
                }
                case jsonpath_token_kind::begin_expression:
                    //std::cout << "begin_expression\n";
                    output_stack_.emplace_back(std::move(tok));
                    operator_stack_.emplace_back(token_type(lparen_arg));
                    break;
                case jsonpath_token_kind::end_index_expression:
                {
                    //std::cout << "jsonpath_token_kind::end_index_expression\n";
                    //for (const auto& t : output_stack_)
                    //{
                    //    std::cout << t.to_string(0) << "\n";
                    //}
                    //std::cout << "/jsonpath_token_kind::end_index_expression\n";
                    unwind_rparen(ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return;
                    }
                    std::vector<token_type> toks;
                    auto it = output_stack_.rbegin();
                    while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::begin_expression)
                    {
                        toks.emplace_back(std::move(*it));
                        ++it;
                    }
                    if (it == output_stack_.rend())
                    {
                        ec = jsonpath_errc::unbalanced_parentheses;
                        return;
                    }
                    std::reverse(toks.begin(), toks.end());
                    ++it;
                    output_stack_.erase(it.base(),output_stack_.end());

                    if (!output_stack_.empty() && output_stack_.back().is_path())
                    {
                        output_stack_.back().selector_->append_selector(resources.new_selector(index_expression_selector<Json,JsonReference>(token_evaluator_type(std::move(toks)))));
                    }
                    else
                    {
                        output_stack_.emplace_back(token_type(resources.new_selector(index_expression_selector<Json,JsonReference>(token_evaluator_type(std::move(toks))))));
                    }
                    break;
                }
                case jsonpath_token_kind::end_argument_expression:
                {
                    //std::cout << "jsonpath_token_kind::end_index_expression\n";
                    //for (const auto& t : output_stack_)
                    //{
                    //    std::cout << t.to_string(0) << "\n";
                    //}
                    //std::cout << "/jsonpath_token_kind::end_index_expression\n";
                    unwind_rparen(ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return;
                    }
                    std::vector<token_type> toks;
                    auto it = output_stack_.rbegin();
                    while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::begin_expression)
                    {
                        toks.emplace_back(std::move(*it));
                        ++it;
                    }
                    if (it == output_stack_.rend())
                    {
                        ec = jsonpath_errc::unbalanced_parentheses;
                        return;
                    }
                    std::reverse(toks.begin(), toks.end());
                    ++it;
                    output_stack_.erase(it.base(),output_stack_.end());
                    output_stack_.emplace_back(token_type(jsoncons::make_unique<token_evaluator_type>(std::move(toks))));
                    break;
                }
                case jsonpath_token_kind::selector:
                {
                    if (!output_stack_.empty() && output_stack_.back().is_path())
                    {
                        output_stack_.back().selector_->append_selector(std::move(tok.selector_));
                    }
                    else
                    {
                        output_stack_.emplace_back(std::move(tok));
                    }
                    break;
                }
                case jsonpath_token_kind::separator:
                    output_stack_.emplace_back(std::move(tok));
                    break;
                case jsonpath_token_kind::begin_union:
                    output_stack_.emplace_back(std::move(tok));
                    break;
                case jsonpath_token_kind::end_union:
                {
                    std::vector<selector_type*> expressions;
                    auto it = output_stack_.rbegin();
                    while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::begin_union)
                    {
                        if ((*it).token_kind() == jsonpath_token_kind::selector)
                        {
                            expressions.emplace_back(std::move((*it).selector_));
                        }
                        do
                        {
                            ++it;
                        } 
                        while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::begin_union && (*it).token_kind() != jsonpath_token_kind::separator);
                        if ((*it).token_kind() == jsonpath_token_kind::separator)
                        {
                            ++it;
                        }
                    }
                    if (it == output_stack_.rend())
                    {
                        ec = jsonpath_errc::unbalanced_parentheses;
                        return;
                    }
                    std::reverse(expressions.begin(), expressions.end());
                    ++it;
                    output_stack_.erase(it.base(),output_stack_.end());

                    if (!output_stack_.empty() && output_stack_.back().is_path())
                    {
                        output_stack_.back().selector_->append_selector(resources.new_selector(union_selector<Json,JsonReference>(std::move(expressions))));
                    }
                    else
                    {
                        output_stack_.emplace_back(token_type(resources.new_selector(union_selector<Json,JsonReference>(std::move(expressions)))));
                    }
                    break;
                }
                case jsonpath_token_kind::lparen:
                    operator_stack_.emplace_back(std::move(tok));
                    break;
                case jsonpath_token_kind::rparen:
                {
                    unwind_rparen(ec);
                    break;
                }
                case jsonpath_token_kind::end_function:
                {
                    //std::cout << "jsonpath_token_kind::end_function\n";
                    unwind_rparen(ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return;
                    }
                    std::vector<token_type> toks;
                    auto it = output_stack_.rbegin();
                    std::size_t arg_count = 0;
                    while (it != output_stack_.rend() && (*it).token_kind() != jsonpath_token_kind::function)
                    {
                        if ((*it).token_kind() == jsonpath_token_kind::argument)
                        {
                            ++arg_count;
                        }
                        toks.emplace_back(std::move(*it));
                        ++it;
                    }
                    if (it == output_stack_.rend())
                    {
                        ec = jsonpath_errc::unbalanced_parentheses;
                        return;
                    }
                    std::reverse(toks.begin(), toks.end());
                    if ((*it).arity() && arg_count != *((*it).arity()))
                    {
                        ec = jsonpath_errc::invalid_arity;
                        return;
                    }
                    toks.push_back(std::move(*it));
                    ++it;
                    output_stack_.erase(it.base(),output_stack_.end());

                    if (!output_stack_.empty() && output_stack_.back().is_path())
                    {
                        output_stack_.back().selector_->append_selector(resources.new_selector(function_selector<Json,JsonReference>(token_evaluator_type(std::move(toks)))));
                    }
                    else
                    {
                        output_stack_.emplace_back(token_type(resources.new_selector(function_selector<Json,JsonReference>(std::move(toks)))));
                    }
                    break;
                }
                case jsonpath_token_kind::literal:
                    if (!output_stack_.empty() && (output_stack_.back().token_kind() == jsonpath_token_kind::current_node || output_stack_.back().token_kind() == jsonpath_token_kind::root_node))
                    {
                        output_stack_.back() = std::move(tok);
                    }
                    else
                    {
                        output_stack_.emplace_back(std::move(tok));
                    }
                    break;
                case jsonpath_token_kind::function:
                    output_stack_.emplace_back(std::move(tok));
                    operator_stack_.emplace_back(token_type(lparen_arg));
                    break;
                case jsonpath_token_kind::argument:
                    output_stack_.emplace_back(std::move(tok));
                    break;
                case jsonpath_token_kind::root_node:
                case jsonpath_token_kind::current_node:
                    output_stack_.emplace_back(std::move(tok));
                    break;
                case jsonpath_token_kind::unary_operator:
                case jsonpath_token_kind::binary_operator:
                {
                    if (operator_stack_.empty() || operator_stack_.back().is_lparen())
                    {
                        operator_stack_.emplace_back(std::move(tok));
                    }
                    else if (tok.precedence_level() < operator_stack_.back().precedence_level()
                             || (tok.precedence_level() == operator_stack_.back().precedence_level() && tok.is_right_associative()))
                    {
                        operator_stack_.emplace_back(std::move(tok));
                    }
                    else
                    {
                        auto it = operator_stack_.rbegin();
                        while (it != operator_stack_.rend() && (*it).is_operator()
                               && (tok.precedence_level() > (*it).precedence_level()
                             || (tok.precedence_level() == (*it).precedence_level() && tok.is_right_associative())))
                        {
                            output_stack_.emplace_back(std::move(*it));
                            ++it;
                        }

                        operator_stack_.erase(it.base(),operator_stack_.end());
                        operator_stack_.emplace_back(std::move(tok));
                    }
                    break;
                }
                default:
                    break;
            }
            //std::cout << "  " << "Output Stack\n";
            //for (auto&& t : output_stack_)
            //{
            //    std::cout << t.to_string(2) << "\n";
            //}
            //if (!operator_stack_.empty())
            //{
            //    std::cout << "  " << "Operator Stack\n";
            //    for (auto&& t : operator_stack_)
            //    {
            //        std::cout << t.to_string(2) << "\n";
            //    }
            //}
        }

        uint32_t append_to_codepoint(uint32_t cp, int c, std::error_code& ec)
        {
            cp *= 16;
            if (c >= '0'  &&  c <= '9')
            {
                cp += c - '0';
            }
            else if (c >= 'a'  &&  c <= 'f')
            {
                cp += c - 'a' + 10;
            }
            else if (c >= 'A'  &&  c <= 'F')
            {
                cp += c - 'A' + 10;
            }
            else
            {
                ec = jsonpath_errc::invalid_codepoint;
            }
            return cp;
        }
    };

    } // namespace detail

} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_JSONPATH_PARSER_HPP
