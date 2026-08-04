// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_PARSER_HPP
#define JSONCONS_JSON_PARSER_HPP

#include <cstddef>
#include <cstdint>
#include <functional> // std::function
#include <limits> // std::numeric_limits
#include <memory> // std::allocator
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/utility/read_number.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_filter.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

#define JSONCONS_ILLEGAL_CONTROL_CHARACTER \
        case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:case 0x08:case 0x0b: \
        case 0x0c:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:case 0x16: \
        case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f 

namespace jsoncons {

namespace detail {

}

enum class parse_state : uint8_t 
{
    root,
    start, 
    accept, 
    slash,  
    slash_slash, 
    slash_star, 
    slash_star_star,
    expect_comma_or_end,  
    object,
    expect_member_name_or_end, 
    expect_member_name, 
    expect_colon,
    expect_value_or_end,
    expect_value,
    array, 
    string,
    member_name,
    number,
    n,
    nu,
    nul,
    t,  
    tr,  
    tru,  
    f,  
    fa,  
    fal,  
    fals,  
    cr,
    done
};

enum class parse_string_state : uint8_t 
{
    text = 0,
    escape, 
    escape_u1, 
    escape_u2, 
    escape_u3, 
    escape_u4, 
    escape_expect_surrogate_pair1, 
    escape_expect_surrogate_pair2, 
    escape_u5, 
    escape_u6, 
    escape_u7, 
    escape_u8
};

enum class parse_number_state : uint8_t 
{
    minus, 
    zero,  
    integer,
    fraction1,
    fraction2,
    exp1,
    exp2,
    exp3
};

template <typename CharT,typename TempAlloc  = std::allocator<char>>
class basic_json_parser : public ser_context
{
public:
    using char_type = CharT;
    using string_view_type = typename basic_json_visitor<CharT>::string_view_type;
private:
    struct string_maps_to_double
    {
        string_view_type s;

        bool operator()(const std::pair<string_view_type,double>& val) const
        {
            return val.first == s;
        }
    };

    using temp_allocator_type = TempAlloc;
    using char_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<CharT>;
    using parse_state_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<parse_state>;

    static constexpr std::size_t initial_buffer_capacity = 256;
    static constexpr int default_initial_stack_capacity = 66;

    int max_nesting_depth_;
    bool allow_trailing_comma_;
    bool allow_comments_;    
    bool lossless_number_;    
    bool lossless_bignum_;    

    std::function<bool(json_errc,const ser_context&)> err_handler_;
    int level_{0};
    uint32_t cp_{0};
    uint32_t cp2_{0};
    std::size_t line_{1};
    std::size_t position_{0};
    std::size_t mark_position_{0};
    std::size_t begin_position_{0};
    const char_type* input_end_{nullptr};
    const char_type* input_ptr_{nullptr};
    parse_state state_{parse_state::start};
    parse_string_state string_state_{};
    parse_number_state number_state_{};
    bool more_{true};
    bool done_{false};
    bool cursor_mode_{false};
    int mark_level_{0};
    
    semantic_tag escape_tag_;
    std::basic_string<char_type,std::char_traits<char_type>,char_allocator_type> buffer_;

    std::vector<parse_state,parse_state_allocator_type> state_stack_;
    std::vector<std::pair<std::basic_string<char_type>,double>> string_double_map_;

    // Noncopyable and nonmoveable
    basic_json_parser(const basic_json_parser&) = delete;
    basic_json_parser& operator=(const basic_json_parser&) = delete;

public:

    basic_json_parser()
        : basic_json_parser(basic_json_decode_options<char_type>())
    {
    }

    explicit basic_json_parser(const TempAlloc& temp_alloc)
        : basic_json_parser(basic_json_decode_options<char_type>(), temp_alloc)
    {
    }

    basic_json_parser(const basic_json_decode_options<char_type>& options,
        const TempAlloc& temp_alloc = TempAlloc())
       : max_nesting_depth_(options.max_nesting_depth()),
         allow_trailing_comma_(options.allow_trailing_comma()),
         allow_comments_(options.allow_comments()),
         lossless_number_(options.lossless_number()),
         lossless_bignum_(options.lossless_bignum()),
#if !defined(JSONCONS_NO_DEPRECATED)
         err_handler_(options.err_handler()),
#else
         err_handler_(default_json_parsing()),
#endif
         buffer_(temp_alloc),
         state_stack_(temp_alloc)
    {
        buffer_.reserve(initial_buffer_capacity);

        std::size_t initial_stack_capacity = options.max_nesting_depth() <= (default_initial_stack_capacity-2) ? (options.max_nesting_depth()+2) : default_initial_stack_capacity;
        state_stack_.reserve(initial_stack_capacity );
        push_state(parse_state::root);

        if (options.enable_str_to_nan())
        {
            string_double_map_.emplace_back(options.nan_to_str(),std::nan(""));
        }
        if (options.enable_str_to_inf())
        {
            string_double_map_.emplace_back(options.inf_to_str(),std::numeric_limits<double>::infinity());
        }
        if (options.enable_str_to_neginf())
        {
            string_double_map_.emplace_back(options.neginf_to_str(),-std::numeric_limits<double>::infinity());
        }
    }
#if !defined(JSONCONS_NO_DEPRECATED)

    basic_json_parser(std::function<bool(json_errc,const ser_context&)> err_handler, 
        const TempAlloc& temp_alloc = TempAlloc())
        : basic_json_parser(basic_json_decode_options<char_type>(), err_handler, temp_alloc)
    {
    }
    basic_json_parser(const basic_json_decode_options<char_type>& options,
        std::function<bool(json_errc,const ser_context&)> err_handler, 
        const TempAlloc& temp_alloc = TempAlloc())
       : max_nesting_depth_(options.max_nesting_depth()),
         allow_trailing_comma_(options.allow_trailing_comma()),
         allow_comments_(options.allow_comments()),
         lossless_number_(options.lossless_number()),
         lossless_bignum_(options.lossless_bignum()),
         err_handler_(err_handler),
         buffer_(temp_alloc),
         state_stack_(temp_alloc)
    {
        buffer_.reserve(initial_buffer_capacity);

        std::size_t initial_stack_capacity = options.max_nesting_depth() <= (default_initial_stack_capacity-2) ? (options.max_nesting_depth()+2) : default_initial_stack_capacity;
        state_stack_.reserve(initial_stack_capacity );
        push_state(parse_state::root);

        if (options.enable_str_to_nan())
        {
            string_double_map_.emplace_back(options.nan_to_str(),std::nan(""));
        }
        if (options.enable_str_to_inf())
        {
            string_double_map_.emplace_back(options.inf_to_str(),std::numeric_limits<double>::infinity());
        }
        if (options.enable_str_to_neginf())
        {
            string_double_map_.emplace_back(options.neginf_to_str(),-std::numeric_limits<double>::infinity());
        }
    }
#endif
    
    void cursor_mode(bool value)
    {
        cursor_mode_ = value;
    }

    int level() const
    {
        return level_;
    }

    int mark_level() const 
    {
        return mark_level_;
    }

    void mark_level(int value)
    {
        mark_level_ = value;
    }

    bool source_exhausted() const
    {
        return input_ptr_ == input_end_;
    }

    const char_type* current() const
    {
        return input_ptr_;
    }

    ~basic_json_parser() noexcept
    {
    }

    parse_state parent() const
    {
        JSONCONS_ASSERT(state_stack_.size() >= 1);
        return state_stack_.back();
    }

    bool done() const
    {
        return done_;
    }

    bool enter() const
    {
        return state_ == parse_state::start;
    }

    bool accept() const
    {
        return state_ == parse_state::accept || done_;
    }

    bool stopped() const
    {
        return !more_;
    }

    parse_state state() const
    {
        return state_;
    }

    bool finished() const
    {
        return !more_ && state_ != parse_state::accept;
    }

    void skip_whitespace()
    {
        const char_type* local_input_end = input_end_;

        while (input_ptr_ != local_input_end) 
        {
            switch (state_)
            {
                case parse_state::cr:
                    ++line_;
                    //++position_;
                    switch (*input_ptr_)
                    {
                        case '\n':
                            ++input_ptr_;
                            ++position_;
                            mark_position_ = position_;
                            state_ = pop_state();
                            break;
                        default:
                            mark_position_ = position_;
                            state_ = pop_state();
                            break;
                    }
                    break;

                default:
                    switch (*input_ptr_)
                    {
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        default:
                            return;
                    }
                    break;
            }
        }
    }

    void begin_object(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (JSONCONS_UNLIKELY(++level_ > max_nesting_depth_))
        {
            more_ = err_handler_(json_errc::max_nesting_depth_exceeded, *this);
            if (!more_)
            {
                ec = json_errc::max_nesting_depth_exceeded;
                return;
            }
        } 

        push_state(parse_state::object);
        state_ = parse_state::expect_member_name_or_end;
        visitor.begin_object(semantic_tag::none, *this, ec);
        more_ = !cursor_mode_;
    }

    void end_object(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (JSONCONS_UNLIKELY(level_ < 1))
        {
            err_handler_(json_errc::unexpected_rbrace, *this);
            ec = json_errc::unexpected_rbrace;
            more_ = false;
            return;
        }
        state_ = pop_state();
        if (state_ == parse_state::object)
        {
            visitor.end_object(*this, ec);
        }
        else if (state_ == parse_state::array)
        {
            err_handler_(json_errc::expected_comma_or_rbracket, *this);
            ec = json_errc::expected_comma_or_rbracket;
            more_ = false;
            return;
        }
        else
        {
            err_handler_(json_errc::unexpected_rbrace, *this);
            ec = json_errc::unexpected_rbrace;
            more_ = false;
            return;
        }

        more_ = !cursor_mode_;
        if (level_ == mark_level_)
        {
            more_ = false;
        }
        --level_;
        if (level_ == 0)
        {
            state_ = parse_state::accept;
        }
        else
        {
            state_ = parse_state::expect_comma_or_end;
        }
    }

    void begin_array(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (++level_ > max_nesting_depth_)
        {
            more_ = err_handler_(json_errc::max_nesting_depth_exceeded, *this);
            if (!more_)
            {
                ec = json_errc::max_nesting_depth_exceeded;
                return;
            }
        }

        push_state(parse_state::array);
        state_ = parse_state::expect_value_or_end;
        visitor.begin_array(semantic_tag::none, *this, ec);

        more_ = !cursor_mode_;
    }

    void end_array(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (level_ < 1)
        {
            err_handler_(json_errc::unexpected_rbracket, *this);
            ec = json_errc::unexpected_rbracket;
            more_ = false;
            return;
        }
        state_ = pop_state();
        if (state_ == parse_state::array)
        {
            visitor.end_array(*this, ec);
        }
        else if (state_ == parse_state::object)
        {
            err_handler_(json_errc::expected_comma_or_rbrace, *this);
            ec = json_errc::expected_comma_or_rbrace;
            more_ = false;
            return;
        }
        else
        {
            err_handler_(json_errc::unexpected_rbracket, *this);
            ec = json_errc::unexpected_rbracket;
            more_ = false;
            return;
        }

        more_ = !cursor_mode_;
        if (level_ == mark_level_)
        {
            more_ = false;
        }
        --level_;

        if (level_ == 0)
        {
            state_ = parse_state::accept;
        }
        else
        {
            state_ = parse_state::expect_comma_or_end;
        }
    }

    void reinitialize()
    {
        reset();
        cp_ = 0;
        cp2_ = 0;
        begin_position_ = 0;
        input_end_ = nullptr;
        input_ptr_ = nullptr;
        buffer_.clear();
    }

    void reset()
    {
        state_stack_.clear();
        push_state(parse_state::root);
        state_ = parse_state::start;
        more_ = true;
        done_ = false;
        line_ = 1;
        position_ = 0;
        mark_position_ = 0;
        level_ = 0;
    }

    void restart()
    {
        more_ = true;
    }

    void check_done()
    {
        std::error_code ec;
        check_done(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(ser_error(ec,line_,column()));
        }
    }

    void check_done(std::error_code& ec)
    {
        for (; input_ptr_ != input_end_; ++input_ptr_)
        {
            char_type curr_char_ = *input_ptr_;
            switch (curr_char_)
            {
                case '\n':
                case '\r':
                case '\t':
                case ' ':
                    break;
                default:
                    more_ = err_handler_(json_errc::extra_character, *this);
                    if (!more_)
                    {
                        ec = json_errc::extra_character;
                        return;
                    }
                    break;
            }
        }
    }

    void update(const string_view_type sv)
    {
        update(sv.data(),sv.length());
    }

    void update(const char_type* data, std::size_t length)
    {
        input_end_ = data + length;
        input_ptr_ = data;
    }

    void parse_some(basic_json_visitor<char_type>& visitor)
    {
        std::error_code ec;
        parse_some(visitor, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(ser_error(ec,line_,column()));
        }
    }

    void parse_some(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        parse_some_(visitor, ec);
    }

    void finish_parse(basic_json_visitor<char_type>& visitor)
    {
        std::error_code ec;
        finish_parse(visitor, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(ser_error(ec,line_,column()));
        }
    }

    void finish_parse(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        while (!finished())
        {
            parse_some(visitor, ec);
        }
    }

    void parse_some_(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (state_ == parse_state::accept)
        {
            visitor.flush();
            done_ = true;
            state_ = parse_state::done;
            more_ = false;
            return;
        }
        const char_type* local_input_end = input_end_;

        if (input_ptr_ == local_input_end && more_)
        {
            switch (state_)
            {
                case parse_state::number:  
                    if (number_state_ == parse_number_state::zero || number_state_ == parse_number_state::integer)
                    {
                        end_integer_value(visitor, ec);
                        if (JSONCONS_UNLIKELY(ec)) return;
                    }
                    else if (number_state_ == parse_number_state::fraction2 || number_state_ == parse_number_state::exp3)
                    {
                        end_fraction_value(visitor, ec);
                        if (JSONCONS_UNLIKELY(ec)) return;
                    }
                    else
                    {
                        err_handler_(json_errc::unexpected_eof, *this);
                        ec = json_errc::unexpected_eof;
                        more_ = false;
                    }
                    break;
                case parse_state::accept:
                    visitor.flush();
                    done_ = true;
                    state_ = parse_state::done;
                    more_ = false;
                    break;
                case parse_state::start:
                    more_ = false;
                    ec = json_errc::unexpected_eof;
                    break;                
                case parse_state::done:
                    more_ = false;
                    break;
                case parse_state::cr:
                    state_ = pop_state();
                    break;
                default:
                    err_handler_(json_errc::unexpected_eof, *this);
                    ec = json_errc::unexpected_eof;
                    more_ = false;
                    return;
            }
        }

        while ((input_ptr_ < local_input_end) && more_)
        {
            switch (state_)
            {
                case parse_state::accept:
                    visitor.flush();
                    done_ = true;
                    state_ = parse_state::done;
                    more_ = false;
                    break;
                case parse_state::cr:
                    ++line_;
                    switch (*input_ptr_)
                    {
                        case '\n':
                            ++input_ptr_;
                            ++position_;
                            state_ = pop_state();
                            break;
                        default:
                            state_ = pop_state();
                            break;
                    }
                    mark_position_ = position_;
                    break;
                case parse_state::start: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/': 
                            ++input_ptr_;
                            ++position_;
                            push_state(state_);
                            state_ = parse_state::slash;
                            break;
                        case '{':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_object(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '[':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_array(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\"':
                            state_ = parse_state::string;
                            string_state_ = parse_string_state{};
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            escape_tag_ = semantic_tag::noesc;
                            buffer_.clear();
                            input_ptr_ = parse_string(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '-':
                            buffer_.clear();
                            buffer_.push_back('-');
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::minus;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '0': 
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::zero;
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::integer;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'n':
                            parse_null(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 't':
                            input_ptr_ = parse_true(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'f':
                            input_ptr_ = parse_false(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '}':
                            err_handler_(json_errc::unexpected_rbrace, *this);
                            ec = json_errc::unexpected_rbrace;
                            more_ = false;
                            return;
                        case ']':
                            err_handler_(json_errc::unexpected_rbracket, *this);
                            ec = json_errc::unexpected_rbracket;
                            more_ = false;
                            return;
                        default:
                            err_handler_(json_errc::syntax_error, *this);
                            ec = json_errc::syntax_error;
                            more_ = false;
                            return;
                    }
                    break;
                }
                case parse_state::expect_comma_or_end: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/':
                            ++input_ptr_;
                            ++position_;
                            push_state(state_); 
                            state_ = parse_state::slash;
                            break;
                        case '}':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            end_object(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case ']':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            end_array(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case ',':
                            begin_member_or_element(ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            if (parent() == parse_state::array)
                            {
                                more_ = err_handler_(json_errc::expected_comma_or_rbracket, *this);
                                if (!more_)
                                {
                                    ec = json_errc::expected_comma_or_rbracket;
                                    return;
                                }
                            }
                            else if (parent() == parse_state::object)
                            {
                                more_ = err_handler_(json_errc::expected_comma_or_rbrace, *this);
                                if (!more_)
                                {
                                    ec = json_errc::expected_comma_or_rbrace;
                                    return;
                                }
                            }
                            else
                            {
                                more_ = err_handler_(json_errc::unexpected_character, *this);
                                if (!more_)
                                {
                                    ec = json_errc::unexpected_character;
                                    return;
                                }
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::expect_member_name_or_end: 
                {
                    if (input_ptr_ >= local_input_end)
                    {
                        return;
                    }
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/':
                            ++input_ptr_;
                            ++position_;
                            push_state(state_); 
                            state_ = parse_state::slash;
                            break;
                        case '}':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            end_object(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\"':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            push_state(parse_state::member_name);
                            state_ = parse_state::string;
                            string_state_ = parse_string_state{};
                            escape_tag_ = semantic_tag::noesc;
                            buffer_.clear();
                            input_ptr_ = parse_string(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\'':
                            more_ = err_handler_(json_errc::single_quote, *this);
                            if (!more_)
                            {
                                ec = json_errc::single_quote;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            more_ = err_handler_(json_errc::expected_key, *this);
                            if (!more_)
                            {
                                ec = json_errc::expected_key;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::expect_member_name: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/': 
                            ++input_ptr_;
                            ++position_;
                            push_state(state_);
                            state_ = parse_state::slash;
                            break;
                        case '\"':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            push_state(parse_state::member_name);
                            state_ = parse_state::string;
                            string_state_ = parse_string_state{};
                            escape_tag_ = semantic_tag::noesc;
                            buffer_.clear();
                            input_ptr_ = parse_string(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '}':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            if (!allow_trailing_comma_)
                            {
                                more_ = err_handler_(json_errc::extra_comma, *this);
                                if (!more_)
                                {
                                    ec = json_errc::extra_comma;
                                    return;
                                }
                            }
                            end_object(visitor, ec);  // Recover
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\'':
                            more_ = err_handler_(json_errc::single_quote, *this);
                            if (!more_)
                            {
                                ec = json_errc::single_quote;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            more_ = err_handler_(json_errc::expected_key, *this);
                            if (!more_)
                            {
                                ec = json_errc::expected_key;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::expect_colon: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/': 
                            push_state(state_);
                            state_ = parse_state::slash;
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ':':
                            state_ = parse_state::expect_value;
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            more_ = err_handler_(json_errc::expected_colon, *this);
                            if (!more_)
                            {
                                ec = json_errc::expected_colon;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::expect_value: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/': 
                            push_state(state_);
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::slash;
                            break;
                        case '{':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_object(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '[':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_array(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\"':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::string;
                            string_state_ = parse_string_state{};
                            escape_tag_ = semantic_tag::noesc;
                            buffer_.clear();
                            input_ptr_ = parse_string(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '-':
                            buffer_.clear();
                            buffer_.push_back('-');
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::minus;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '0': 
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::zero;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::integer;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'n':
                            parse_null(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 't':
                            input_ptr_ = parse_true(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'f':
                            input_ptr_ = parse_false(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case ']':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            if (parent() == parse_state::array)
                            {
                                if (!allow_trailing_comma_)
                                {
                                    more_ = err_handler_(json_errc::extra_comma, *this);
                                    if (!more_)
                                    {
                                        ec = json_errc::extra_comma;
                                        return;
                                    }
                                }
                                end_array(visitor, ec);  // Recover
                                if (JSONCONS_UNLIKELY(ec)) return;
                            }
                            else
                            {
                                more_ = err_handler_(json_errc::expected_value, *this);
                                if (!more_)
                                {
                                    ec = json_errc::expected_value;
                                    return;
                                }
                            }
                            
                            break;
                        case '\'':
                            more_ = err_handler_(json_errc::single_quote, *this);
                            if (!more_)
                            {
                                ec = json_errc::single_quote;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            more_ = err_handler_(json_errc::expected_value, *this);
                            if (!more_)
                            {
                                ec = json_errc::expected_value;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::expect_value_or_end: 
                {
                    switch (*input_ptr_)
                    {
                        JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                            more_ = err_handler_(json_errc::illegal_control_character, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_control_character;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        case ' ':case '\t':case '\n':case '\r':
                            skip_space(&input_ptr_);
                            break;
                        case '/': 
                            ++input_ptr_;
                            ++position_;
                            push_state(state_);
                            state_ = parse_state::slash;
                            break;
                        case '{':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_object(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '[':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            begin_array(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case ']':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            end_array(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '\"':
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::string;
                            string_state_ = parse_string_state{};
                            escape_tag_ = semantic_tag::noesc;
                            buffer_.clear();
                            input_ptr_ = parse_string(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) return;
                            break;
                        case '-':
                            buffer_.clear();
                            buffer_.push_back('-');
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::minus;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '0': 
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::zero;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                            buffer_.clear();
                            buffer_.push_back(static_cast<char>(*input_ptr_));
                            begin_position_ = position_;
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::number;
                            number_state_ = parse_number_state::integer;
                            input_ptr_ = parse_number(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'n':
                            parse_null(visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 't':
                            input_ptr_ = parse_true(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case 'f':
                            input_ptr_ = parse_false(input_ptr_, visitor, ec);
                            if (JSONCONS_UNLIKELY(ec)) {return;}
                            break;
                        case '\'':
                            more_ = err_handler_(json_errc::single_quote, *this);
                            if (!more_)
                            {
                                ec = json_errc::single_quote;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        default:
                            more_ = err_handler_(json_errc::expected_value, *this);
                            if (!more_)
                            {
                                ec = json_errc::expected_value;
                                return;
                            }
                            ++input_ptr_;
                            ++position_;
                            break;
                        }
                    }
                    break;
                case parse_state::string: 
                    input_ptr_ = parse_string(input_ptr_, visitor, ec);
                    if (JSONCONS_UNLIKELY(ec)) return;
                    break;
                case parse_state::number:
                    input_ptr_ = parse_number(input_ptr_, visitor, ec);  
                    if (JSONCONS_UNLIKELY(ec)) return;
                    break;
                case parse_state::t: 
                    switch (*input_ptr_)
                    {
                        case 'r':
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::tr;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    break;
                case parse_state::tr: 
                    switch (*input_ptr_)
                    {
                        case 'u':
                            state_ = parse_state::tru;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                case parse_state::tru: 
                    switch (*input_ptr_)
                    {
                        case 'e':
                            ++input_ptr_;
                            ++position_;
                            visitor.bool_value(true,  semantic_tag::none, *this, ec);
                            if (level_ == 0)
                            {
                                state_ = parse_state::accept;
                            }
                            else
                            {
                                state_ = parse_state::expect_comma_or_end;
                            }
                            more_ = !cursor_mode_;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    break;
                case parse_state::f: 
                    switch (*input_ptr_)
                    {
                        case 'a':
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::fa;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    break;
                case parse_state::fa: 
                    switch (*input_ptr_)
                    {
                        case 'l':
                            state_ = parse_state::fal;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                case parse_state::fal: 
                    switch (*input_ptr_)
                    {
                        case 's':
                            state_ = parse_state::fals;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                case parse_state::fals: 
                    switch (*input_ptr_)
                    {
                        case 'e':
                            ++input_ptr_;
                            ++position_;
                            visitor.bool_value(false, semantic_tag::none, *this, ec);
                            if (level_ == 0)
                            {
                                state_ = parse_state::accept;
                            }
                            else
                            {
                                state_ = parse_state::expect_comma_or_end;
                            }
                            more_ = !cursor_mode_;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    break;
                case parse_state::n: 
                    switch (*input_ptr_)
                    {
                        case 'u':
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::nu;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    break;
                case parse_state::nu: 
                    switch (*input_ptr_)
                    {
                        case 'l':
                            state_ = parse_state::nul;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                case parse_state::nul: 
                    ++position_;
                    switch (*input_ptr_)
                    {
                        case 'l':
                            visitor.null_value(semantic_tag::none, *this, ec);
                            if (level_ == 0)
                            {
                                state_ = parse_state::accept;
                            }
                            else
                            {
                                state_ = parse_state::expect_comma_or_end;
                            }
                            more_ = !cursor_mode_;
                            break;
                        default:
                            err_handler_(json_errc::invalid_value, *this);
                            ec = json_errc::invalid_value;
                            more_ = false;
                            return;
                    }
                    ++input_ptr_;
                    break;
                case parse_state::slash: 
                {
                    switch (*input_ptr_)
                    {
                        case '*':
                            if (!allow_comments_)
                            {
                                ec = json_errc::illegal_comment;
                                return;
                            }
                            more_ = err_handler_(json_errc::illegal_comment, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_comment;
                                return;
                            }
                            state_ = parse_state::slash_star;
                            break;
                        case '/':
                            if (!allow_comments_)
                            {
                                ec = json_errc::illegal_comment;
                                return;
                            }
                            more_ = err_handler_(json_errc::illegal_comment, *this);
                            if (!more_)
                            {
                                ec = json_errc::illegal_comment;
                                return;
                            }
                            state_ = parse_state::slash_slash;
                            break;
                        default:    
                            more_ = err_handler_(json_errc::syntax_error, *this);
                            if (!more_)
                            {
                                ec = json_errc::syntax_error;
                                return;
                            }
                            break;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                }
                case parse_state::slash_star:  
                {
                    switch (*input_ptr_)
                    {
                        case '\r':
                            push_state(state_);
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::cr;
                            break;
                        case '\n':
                            ++input_ptr_;
                            ++line_;
                            ++position_;
                            mark_position_ = position_;
                            break;
                        case '*':
                            ++input_ptr_;
                            ++position_;
                            state_ = parse_state::slash_star_star;
                            break;
                        default:
                            ++input_ptr_;
                            ++position_;
                            break;
                    }
                    break;
                }
                case parse_state::slash_slash: 
                {
                    switch (*input_ptr_)
                    {
                    case '\r':
                    case '\n':
                        state_ = pop_state();
                        break;
                    default:
                        ++input_ptr_;
                        ++position_;
                    }
                    break;
                }
                case parse_state::slash_star_star: 
                {
                    switch (*input_ptr_)
                    {
                    case '/':
                        state_ = pop_state();
                        break;
                    default:    
                        state_ = parse_state::slash_star;
                        break;
                    }
                    ++input_ptr_;
                    ++position_;
                    break;
                }
                default:
                    JSONCONS_ASSERT(false);
                    break;
            }
        }
    }

    const char_type* parse_true(const char_type* cur, basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        begin_position_ = position_;
        if (JSONCONS_LIKELY(input_end_ - cur >= 4))
        {
            if (*(cur+1) == 'r' && *(cur+2) == 'u' && *(cur+3) == 'e')
            {
                cur += 4;
                position_ += 4;
                visitor.bool_value(true, semantic_tag::none, *this, ec);
                if (level_ == 0)
                {
                    state_ = parse_state::accept;
                }
                else
                {
                    state_ = parse_state::expect_comma_or_end;
                }
                more_ = !cursor_mode_;
            }
            else
            {
                err_handler_(json_errc::invalid_value, *this);
                ec = json_errc::invalid_value;
                more_ = false;
                return cur;
            }
        }
        else
        {
            ++cur;
            ++position_;
            state_ = parse_state::t;
        }
        return cur;
    }

    void parse_null(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        begin_position_ = position_;
        if (JSONCONS_LIKELY(input_end_ - input_ptr_ >= 4))
        {
            if (*(input_ptr_+1) == 'u' && *(input_ptr_+2) == 'l' && *(input_ptr_+3) == 'l')
            {
                input_ptr_ += 4;
                position_ += 4;
                visitor.null_value(semantic_tag::none, *this, ec);
                more_ = !cursor_mode_;
                if (level_ == 0)
                {
                    state_ = parse_state::accept;
                }
                else
                {
                    state_ = parse_state::expect_comma_or_end;
                }
            }
            else
            {
                err_handler_(json_errc::invalid_value, *this);
                ec = json_errc::invalid_value;
                more_ = false;
                return;
            }
        }
        else
        {
            ++input_ptr_;
            ++position_;
            state_ = parse_state::n;
        }
    }

    const char_type* parse_false(const char_type* cur, basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        begin_position_ = position_;
        if (JSONCONS_LIKELY(input_end_ - cur >= 5))
        {
            if (*(cur+1) == 'a' && *(cur+2) == 'l' && *(cur+3) == 's' && *(cur+4) == 'e')
            {
                cur += 5;
                position_ += 5;
                visitor.bool_value(false, semantic_tag::none, *this, ec);
                more_ = !cursor_mode_;
                if (level_ == 0)
                {
                    state_ = parse_state::accept;
                }
                else
                {
                    state_ = parse_state::expect_comma_or_end;
                }
            }
            else
            {
                err_handler_(json_errc::invalid_value, *this);
                ec = json_errc::invalid_value;
                more_ = false;
                return cur;
            }
        }
        else
        {
            ++cur;
            ++position_;
            state_ = parse_state::f;
        }
        return cur;
    }

    const char_type* parse_number(const char_type* hdr, basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        const char_type* cur = hdr;
        const char_type* local_input_end = input_end_;

        switch (number_state_)
        {
            case parse_number_state::minus:
                goto minus_sign;
            case parse_number_state::zero:
                goto zero;
            case parse_number_state::integer:
                goto integer;
            case parse_number_state::fraction1:
                goto fraction1;
            case parse_number_state::fraction2:
                goto fraction2;
            case parse_number_state::exp1:
                goto exp1;
            case parse_number_state::exp2:
                goto exp2;
            case parse_number_state::exp3:
                goto exp3;
            default:
                JSONCONS_UNREACHABLE();               
        }
minus_sign:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            number_state_ = parse_number_state::minus;
            buffer_.append(hdr, cur);
            position_ += (cur - hdr);
            return cur;
        }
        if (jsoncons::is_nonzero_digit(*cur))
        {
            ++cur;
            goto integer;
        }
        if (*cur == '0')
        {
            ++cur;
            goto zero;
        }
        err_handler_(json_errc::invalid_number, *this);
        ec = json_errc::invalid_number;
        more_ = false;
        position_ += (cur - hdr);
        return cur;
zero:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            number_state_ = parse_number_state::integer;
            buffer_.append(hdr, cur);
            position_ += (cur - hdr);
            return cur;
        }
        if (*cur == '.')
        {
            ++cur;
            goto fraction1;
        }
        if (jsoncons::is_exp(*cur))
        {
            ++cur;
            goto exp1;
        }
        if (jsoncons::is_digit(*cur))
        {
            err_handler_(json_errc::leading_zero, *this);
            ec = json_errc::leading_zero;
            more_ = false;
            number_state_ = parse_number_state::zero;

            position_ += (cur - hdr);
            return cur;
        }
        buffer_.append(hdr, cur);
        position_ += (cur - hdr);
        end_integer_value(visitor, ec);
        return cur;
integer:
        while (true)
        {
            if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
            {
                number_state_ = parse_number_state::integer;
                buffer_.append(hdr, cur);
                position_ += (cur - hdr);
                return cur;
            }
            if (JSONCONS_UNLIKELY(!jsoncons::is_digit(*cur)))
            {
                break;
            }
            ++cur;
        }
        if (*cur == '.')
        {
            ++cur;
            goto fraction1;
        }
        if (jsoncons::is_exp(*cur))
        {
            ++cur;
            goto exp1;
        }
        buffer_.append(hdr, cur);
        position_ += (cur - hdr);
        end_integer_value(visitor, ec);
        return cur;
fraction1:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            number_state_ = parse_number_state::fraction1;
            buffer_.append(hdr, cur);
            position_ += (cur - hdr);
            return cur;
        }
        if (jsoncons::is_digit(*cur))
        {
            ++cur;
            goto fraction2;
        }
        err_handler_(json_errc::invalid_number, *this);
        ec = json_errc::invalid_number;
        more_ = false;
        number_state_ = parse_number_state::fraction1;
        position_ += (cur - hdr);
        return cur;
fraction2:
        while (true)
        {
            if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
            {
                number_state_ = parse_number_state::fraction2;
                buffer_.append(hdr, cur);
                position_ += (cur - hdr);
                return cur;
            }
            if (JSONCONS_UNLIKELY(!jsoncons::is_digit(*cur)))
            {
                break;
            }
            ++cur;
        }
        if (jsoncons::is_exp(*cur))
        {
            ++cur;
            goto exp1;
        }
        buffer_.append(hdr, cur);
        position_ += (cur - hdr);
        end_fraction_value(visitor, ec);
        return cur;
exp1:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            number_state_ = parse_number_state::exp1;
            buffer_.append(hdr, cur);
            position_ += (cur - hdr);
            return cur;
        }
        if (*cur == '-')
        {
            ++cur;
            goto exp2;
        }
        if (jsoncons::is_digit(*cur))
        {
            ++cur;
            goto exp3;
        }
        if (*cur == '+')
        {
            ++cur;
            goto exp2;
        }
        err_handler_(json_errc::invalid_number, *this);
        ec = json_errc::invalid_number;
        more_ = false;
        position_ += (cur - hdr);
        return cur;
exp2:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            number_state_ = parse_number_state::exp2;
            buffer_.append(hdr, cur);
            position_ += (cur - hdr);
            return cur;
        }
        if (jsoncons::is_digit(*cur))
        {
            ++cur;
            goto exp3;
        }
        err_handler_(json_errc::invalid_number, *this);
        ec = json_errc::invalid_number;
        more_ = false;
        position_ += (cur - hdr);
        return cur;
        
exp3:
        while (true)
        {
            if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
            {
                number_state_ = parse_number_state::exp3;
                buffer_.append(hdr, cur);
                position_ += (cur - hdr);
                return cur;
            }
            if (JSONCONS_UNLIKELY(!jsoncons::is_digit(*cur)))
            {
                break;
            }
            ++cur;
        }
        buffer_.append(hdr, cur);
        position_ += (cur - hdr);
        end_fraction_value(visitor, ec);
        return cur;
    }

    const char_type* parse_string(const char_type* cur, basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        const char_type* local_input_end = input_end_;
        const char_type* sb = cur;

        switch (string_state_)
        {
            case parse_string_state::text:
                goto text;
            case parse_string_state::escape:
                goto escape;
            case parse_string_state::escape_u1:
                goto escape_u1;
            case parse_string_state::escape_u2:
                goto escape_u2;
            case parse_string_state::escape_u3:
                goto escape_u3;
            case parse_string_state::escape_u4:
                goto escape_u4;
            case parse_string_state::escape_expect_surrogate_pair1:
                goto escape_expect_surrogate_pair1;
            case parse_string_state::escape_expect_surrogate_pair2:
                goto escape_expect_surrogate_pair2;
            case parse_string_state::escape_u5:
                goto escape_u5;
            case parse_string_state::escape_u6:
                goto escape_u6;
            case parse_string_state::escape_u7:
                goto escape_u7;
            case parse_string_state::escape_u8:
                goto escape_u8;
            default:
                JSONCONS_UNREACHABLE();               
        }

text:
        while (cur < local_input_end)
        {
            switch (*cur)
            {
                JSONCONS_ILLEGAL_CONTROL_CHARACTER:
                {
                    position_ += (cur - sb + 1);
                    more_ = err_handler_(json_errc::illegal_control_character, *this);
                    if (!more_)
                    {
                        ec = json_errc::illegal_control_character;
                        string_state_ = parse_string_state{};
                        return cur;
                    }
                    // recovery - skip
                    buffer_.append(sb,cur-sb);
                    ++cur;
                    string_state_ = parse_string_state{};
                    return cur;
                }
                case '\n':
                case '\r':
                case '\t':
                {
                    position_ += (cur - sb + 1);
                    if (!err_handler_(json_errc::illegal_character_in_string, *this))
                    {
                        more_ = false;
                        ec = json_errc::illegal_character_in_string;
                        return cur;
                    }
                    // recovery - skip
                    buffer_.append(sb,cur-sb);
                    sb = cur + 1;
                    break;
                }
                case '\\': 
                {
                    buffer_.append(sb,cur-sb);
                    position_ += (cur - sb + 1);
                    ++cur;
                    escape_tag_ = semantic_tag{};
                    goto escape;
                }
                case '\"':
                {
                    position_ += (cur - sb + 1);
                    if (buffer_.empty())
                    {
                        end_string_value(sb,cur-sb, visitor, ec);
                        if (JSONCONS_UNLIKELY(ec)) {return cur;}
                    }
                    else
                    {
                        buffer_.append(sb,cur-sb);
                        end_string_value(buffer_.data(), buffer_.length(), visitor, ec);
                        if (JSONCONS_UNLIKELY(ec)) {return cur;}
                    }
                    ++cur;
                    return cur;
                }
            default:
                break;
            }
            ++cur;
        }

        // Buffer exhausted               
        {
            buffer_.append(sb,cur-sb);
            position_ += (cur - sb);
            string_state_ = parse_string_state{};
            return cur;
        }

escape:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape;
            return cur;
        }
        switch (*cur)
        {
        case '\"':
            buffer_.push_back('\"');
            sb = ++cur;
            ++position_;
            goto text;
        case '\\': 
            buffer_.push_back('\\');
            sb = ++cur;
            ++position_;
            goto text;
        case '/':
            buffer_.push_back('/');
            sb = ++cur;
            ++position_;
            goto text;
        case 'b':
            buffer_.push_back('\b');
            sb = ++cur;
            ++position_;
            goto text;
        case 'f':
            buffer_.push_back('\f');
            sb = ++cur;
            ++position_;
            goto text;
        case 'n':
            buffer_.push_back('\n');
            sb = ++cur;
            ++position_;
            goto text;
        case 'r':
            buffer_.push_back('\r');
            sb = ++cur;
            ++position_;
            goto text;
        case 't':
            buffer_.push_back('\t');
            sb = ++cur;
            ++position_;
            goto text;
        case 'u':
             cp_ = 0;
             ++cur;
             ++position_;
             goto escape_u1;
        default:    
            err_handler_(json_errc::illegal_escaped_character, *this);
            ec = json_errc::illegal_escaped_character;
            more_ = false;
            string_state_ = parse_string_state::escape;
            return cur;
        }

escape_u1:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u1;
            return cur;
        }
        {
            cp_ = append_to_codepoint(0, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u1;
                return cur;
            }
            ++cur;
            ++position_;
            goto escape_u2;
        }

escape_u2:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u2;
            return cur;
        }
        {
            cp_ = append_to_codepoint(cp_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u2;
                return cur;
            }
            ++cur;
            ++position_;
            goto escape_u3;
        }

escape_u3:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u3;
            return cur;
        }
        {
            cp_ = append_to_codepoint(cp_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u3;
                return cur;
            }
            ++cur;
            ++position_;
            goto escape_u4;
        }

escape_u4:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u4;
            return cur;
        }
        {
            cp_ = append_to_codepoint(cp_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u4;
                return cur;
            }
            if (unicode_traits::is_high_surrogate(cp_))
            {
                ++cur;
                ++position_;
                goto escape_expect_surrogate_pair1;
            }
            else
            {
                unicode_traits::convert(&cp_, 1, buffer_);
                sb = ++cur;
                ++position_;
                string_state_ = parse_string_state{};
                return cur;
            }
        }

escape_expect_surrogate_pair1:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_expect_surrogate_pair1;
            return cur;
        }
        {
            switch (*cur)
            {
            case '\\': 
                cp2_ = 0;
                ++cur;
                ++position_;
                goto escape_expect_surrogate_pair2;
            default:
                err_handler_(json_errc::expected_codepoint_surrogate_pair, *this);
                ec = json_errc::expected_codepoint_surrogate_pair;
                more_ = false;
                string_state_ = parse_string_state::escape_expect_surrogate_pair1;
                return cur;
            }
        }

escape_expect_surrogate_pair2:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_expect_surrogate_pair2;
            return cur;
        }
        {
            switch (*cur)
            {
            case 'u':
                ++cur;
                ++position_;
                goto escape_u5;
            default:
                err_handler_(json_errc::expected_codepoint_surrogate_pair, *this);
                ec = json_errc::expected_codepoint_surrogate_pair;
                more_ = false;
                string_state_ = parse_string_state::escape_expect_surrogate_pair2;
                return cur;
            }
        }

escape_u5:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u5;
            return cur;
        }
        {
            cp2_ = append_to_codepoint(0, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u5;
                return cur;
            }
        }
        ++cur;
        ++position_;
        goto escape_u6;

escape_u6:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u6;
            return cur;
        }
        {
            cp2_ = append_to_codepoint(cp2_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u6;
                return cur;
            }
            ++cur;
            ++position_;
            goto escape_u7;
        }

escape_u7:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u7;
            return cur;
        }
        {
            cp2_ = append_to_codepoint(cp2_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u7;
                return cur;
            }
            ++cur;
            ++position_;
            goto escape_u8;
        }

escape_u8:
        if (JSONCONS_UNLIKELY(cur >= local_input_end)) // Buffer exhausted               
        {
            string_state_ = parse_string_state::escape_u8;
            return cur;
        }
        {
            cp2_ = append_to_codepoint(cp2_, *cur, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                string_state_ = parse_string_state::escape_u8;
                return cur;
            }
            uint32_t cp = 0x10000 + ((cp_ & 0x3FF) << 10) + (cp2_ & 0x3FF);
            unicode_traits::convert(&cp, 1, buffer_);
            sb = ++cur;
            ++position_;
            goto text;
        }

        JSONCONS_UNREACHABLE();               
    }

    void translate_conv_errc(unicode_traits::conv_errc result, std::error_code& ec)
    {
        switch (result)
        {
        case unicode_traits::conv_errc():
            break;
        case unicode_traits::conv_errc::over_long_utf8_sequence:
            more_ = err_handler_(json_errc::over_long_utf8_sequence, *this);
            if (!more_)
            {
                ec = json_errc::over_long_utf8_sequence;
                return;
            }
            break;
        case unicode_traits::conv_errc::unpaired_high_surrogate:
            more_ = err_handler_(json_errc::unpaired_high_surrogate, *this);
            if (!more_)
            {
                ec = json_errc::unpaired_high_surrogate;
                return;
            }
            break;
        case unicode_traits::conv_errc::expected_continuation_byte:
            more_ = err_handler_(json_errc::expected_continuation_byte, *this);
            if (!more_)
            {
                ec = json_errc::expected_continuation_byte;
                return;
            }
            break;
        case unicode_traits::conv_errc::illegal_surrogate_value:
            more_ = err_handler_(json_errc::illegal_surrogate_value, *this);
            if (!more_)
            {
                ec = json_errc::illegal_surrogate_value;
                return;
            }
            break;
        default:
            more_ = err_handler_(json_errc::illegal_codepoint, *this);
            if (!more_)
            {
                ec = json_errc::illegal_codepoint;
                return;
            }
            break;
        }
    }

    std::size_t line() const override
    {
        return line_;
    }

    std::size_t column() const override
    {
        return (position_ - mark_position_) + 1;
    }

    std::size_t begin_position() const override
    {
        return begin_position_;
    }

    std::size_t position() const override
    {
        return begin_position_;
    }

    std::size_t end_position() const override
    {
        return position_;
    }

private:

    void skip_space(char_type const ** ptr)
    {
        const char_type* local_input_end = input_end_;
        const char_type* cur = *ptr;

        while (cur < local_input_end) 
        {
            if (*cur == ' ' || *cur == '\t')
            {
                ++cur;
                ++position_;
                continue;
            }
            if (*cur == '\n')
            {
                ++cur;
                ++line_;
                ++position_;
                mark_position_ = position_;
                continue;
            }
            if (*cur == '\r')
            {
                ++cur;
                ++position_;
                if (cur < local_input_end)
                {
                    ++line_;
                    if (*cur == '\n')
                    {
                        ++cur;
                        ++position_;
                    }
                    mark_position_ = position_;
                }
                else
                {
                    push_state(state_);
                    state_ = parse_state::cr;
                    *ptr = cur;
                    return; 
                }
                continue;
            }
            break;
        }
        *ptr = cur;
    }

    void end_integer_value(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (buffer_[0] == '-')
        {
            end_negative_value(visitor, ec);
        }
        else
        {
            end_positive_value(visitor, ec);
        }
    }

    void end_negative_value(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        int64_t val;
        auto result = jsoncons::dec_to_integer(buffer_.data(), buffer_.length(), val);
        if (result)
        {
            visitor.int64_value(val, semantic_tag::none, *this, ec);
        }
        else // Must be overflow
        {
            if (lossless_bignum_)
            {
                visitor.string_value(buffer_, semantic_tag::bigint, *this, ec);
            }
            else
            {
                double d{0};
                result = jsoncons::decstr_to_double(&buffer_[0], buffer_.length(), d);
                if (JSONCONS_LIKELY(result))
                {
                    visitor.double_value(d, semantic_tag::none, *this, ec);
                }
                else if (result.ec == std::errc::result_out_of_range)
                {
                    visitor.double_value(d, semantic_tag{}, *this, ec); // REVISIT
                }
                else
                {
                    ec = json_errc::invalid_number;
                    more_ = false;
                    return;
                }
            }

        }
        more_ = !cursor_mode_;
        after_value(ec);
    }

    void end_positive_value(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        uint64_t val;
        auto result = jsoncons::dec_to_integer(buffer_.data(), buffer_.length(), val);
        if (result)
        {
            visitor.uint64_value(val, semantic_tag::none, *this, ec);
        }
        else // Must be overflow
        {
            if (lossless_bignum_)
            {
                visitor.string_value(buffer_, semantic_tag::bigint, *this, ec);
            }
            else
            {
                double d{0};
                result = jsoncons::decstr_to_double(&buffer_[0], buffer_.length(), d);
                if (JSONCONS_LIKELY(result))
                {
                    visitor.double_value(d, semantic_tag::none, *this, ec);
                }
                else if (result.ec == std::errc::result_out_of_range)
                {
                    visitor.double_value(d, semantic_tag{}, *this, ec); // REVISIT
                }
                else
                {
                    ec = json_errc::invalid_number;
                    more_ = false;
                    return;
                }
            }
        }
        more_ = !cursor_mode_;
        after_value(ec);
    }

    void end_fraction_value(basic_json_visitor<char_type>& visitor, std::error_code& ec)
    {
        if (lossless_number_)
        {
            visitor.string_value(buffer_, semantic_tag::bigdec, *this, ec);
        }
        else
        {
            double d{0};
            auto result = jsoncons::decstr_to_double(&buffer_[0], buffer_.length(), d);
            if (JSONCONS_LIKELY(result))
            {
                visitor.double_value(d, semantic_tag::none, *this, ec);
            }
            else if (result.ec == std::errc::result_out_of_range)
            {
                if (lossless_bignum_)
                {
                    visitor.string_value(buffer_, semantic_tag::bigdec, *this, ec);
                }
                else
                {
                    visitor.double_value(d, semantic_tag{}, *this, ec); // REVISIT
                }
            }
            else
            {
                ec = json_errc::invalid_number;
                more_ = false;
                return;
            }
        }

        more_ = !cursor_mode_;
        after_value(ec);
    }

    void end_string_value(const char_type* s, std::size_t length, basic_json_visitor<char_type>& visitor, std::error_code& ec) 
    {
        string_view_type sv(s, length);
        auto result = unicode_traits::validate(s, length);
        if (result.ec != unicode_traits::conv_errc())
        {
            translate_conv_errc(result.ec,ec);
            position_ += (result.ptr - s);
            return;
        }
        switch (parent())
        {
            case parse_state::member_name:
                visitor.key(sv, *this, ec);
                more_ = !cursor_mode_;
                pop_state();
                state_ = parse_state::expect_colon;
                break;
            case parse_state::object:
            case parse_state::array:
            {
                auto it = std::find_if(string_double_map_.begin(), string_double_map_.end(), string_maps_to_double{ sv });
                if (it != string_double_map_.end())
                {
                    visitor.double_value((*it).second, semantic_tag::none, *this, ec);
                    more_ = !cursor_mode_;
                }
                else
                {
                    visitor.string_value(sv, escape_tag_, *this, ec);
                    more_ = !cursor_mode_;
                }
                state_ = parse_state::expect_comma_or_end;
                break;
            }
            case parse_state::root:
            {
                auto it = std::find_if(string_double_map_.begin(),string_double_map_.end(),string_maps_to_double{sv});
                if (it != string_double_map_.end())
                {
                    visitor.double_value((*it).second, semantic_tag::none, *this, ec);
                    more_ = !cursor_mode_;
                }
                else
                {
                    visitor.string_value(sv, escape_tag_, *this, ec);
                    more_ = !cursor_mode_;
                }
                state_ = parse_state::accept;
                break;
            }
            default:
                more_ = err_handler_(json_errc::syntax_error, *this);
                if (!more_)
                {
                    ec = json_errc::syntax_error;
                    return;
                }
                break;
        }
    }

    void begin_member_or_element(std::error_code& ec) 
    {
        switch (parent())
        {
            case parse_state::object:
                state_ = parse_state::expect_member_name;
                break;
            case parse_state::array:
                state_ = parse_state::expect_value;
                break;
            case parse_state::root:
                break;
            default:
                more_ = err_handler_(json_errc::syntax_error, *this);
                if (!more_)
                {
                    ec = json_errc::syntax_error;
                    return;
                }
                break;
        }
    }

    void after_value(std::error_code& ec) 
    {
        switch (parent())
        {
            case parse_state::array:
            case parse_state::object:
                state_ = parse_state::expect_comma_or_end;
                break;
            case parse_state::root:
                state_ = parse_state::accept;
                break;
            default:
                more_ = err_handler_(json_errc::syntax_error, *this);
                if (!more_)
                {
                    ec = json_errc::syntax_error;
                    return;
                }
                break;
        }
    }

    void push_state(parse_state state)
    {
        state_stack_.push_back(state);
        //std::cout << "max_nesting_depth: " << max_nesting_depth_ << ", capacity: " << state_stack_.capacity() << ", nesting_depth: " << level_ << ", stack size: " << state_stack_.size() << "\n";
    }

    parse_state pop_state()
    {
        JSONCONS_ASSERT(!state_stack_.empty())
        parse_state state = state_stack_.back();
        state_stack_.pop_back();
        return state;
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
            more_ = err_handler_(json_errc::invalid_unicode_escape_sequence, *this);
            if (!more_)
            {
                ec = json_errc::invalid_unicode_escape_sequence;
                return cp;
            }
        }
        return cp;
    }
};

using json_parser = basic_json_parser<char>;
using wjson_parser = basic_json_parser<wchar_t>;

} // namespace jsoncons

#endif // JSONCONS_JSON_PARSER_HPP

