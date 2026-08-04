// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_OPTIONS_HPP
#define JSONCONS_JSON_OPTIONS_HPP

#include <cstdint>
#include <cwchar>
#include <functional>
#include <string>
#include <system_error>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/ser_utils.hpp>

namespace jsoncons {

enum class float_chars_format : uint8_t {general,fixed,scientific,hex};

enum class indenting : uint8_t {no_indent = 0, indent = 1};

enum class line_split_kind  : uint8_t {multi_line=0, new_line=1, same_line=2};

enum class bignum_format_kind : uint8_t {raw, 
#if !defined(JSONCONS_NO_DEPRECATED)
    number=raw, // deprecated, use raw instead 
#endif    
    base10, 
    base64, 
    base64url};

#if !defined(JSONCONS_NO_DEPRECATED)
JSONCONS_DEPRECATED_MSG("Instead, use bignum_format_kind") typedef bignum_format_kind bigint_chars_format;
#endif

enum class byte_string_chars_format : uint8_t {none=0,base16,base64,base64url};

enum class spaces_option : uint8_t {no_spaces=0,space_after,space_before,space_before_and_after};


struct default_json_parsing
{
    bool operator()(json_errc ec, const ser_context&) noexcept 
    {
        return ec == json_errc::illegal_comment;
    }
};

#if !defined(JSONCONS_NO_DEPRECATED)

struct strict_json_parsing
{
    bool operator()(json_errc, const ser_context&) noexcept
    {
        return false;
    }
};

struct allow_trailing_commas
{
    bool operator()(const std::error_code& ec, const ser_context&) noexcept 
    {
        return ec == json_errc::illegal_comment || ec == jsoncons::json_errc::extra_comma;
    }
};

#endif

template <typename CharT>
class basic_json_options;

template <typename CharT>
class basic_json_options_common
{
    friend class basic_json_options<CharT>;
public:
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;
private:

    bool enable_nan_to_num_:1;
    bool enable_inf_to_num_:1;
    bool enable_neginf_to_num_:1;
    bool enable_nan_to_str_:1;
    bool enable_inf_to_str_:1;
    bool enable_neginf_to_str_:1;
    bool enable_str_to_nan_:1;
    bool enable_str_to_inf_:1;
    bool enable_str_to_neginf_:1;

    string_type nan_to_num_;
    string_type inf_to_num_;
    string_type neginf_to_num_;
    string_type nan_to_str_;
    string_type inf_to_str_;
    string_type neginf_to_str_;
    int max_nesting_depth_;

protected:
    basic_json_options_common()
       :
        enable_nan_to_num_(false),
        enable_inf_to_num_(false),
        enable_neginf_to_num_(false),
        enable_nan_to_str_(false),
        enable_inf_to_str_(false),
        enable_neginf_to_str_(false),
        enable_str_to_nan_(false),
        enable_str_to_inf_(false),
        enable_str_to_neginf_(false),
        max_nesting_depth_(1024)
    {}

    virtual ~basic_json_options_common() = default;

    basic_json_options_common(const basic_json_options_common&) = default;
    basic_json_options_common& operator=(const basic_json_options_common&) = default;
    basic_json_options_common(basic_json_options_common&&) = default;
    //basic_json_options_common& operator=(basic_json_options_common&&) = default;

public:

    bool enable_nan_to_num() const
    {
        return enable_nan_to_num_;
    }

    bool enable_inf_to_num() const
    {
        return enable_inf_to_num_;
    }

    bool enable_neginf_to_num() const
    {
        return enable_neginf_to_num_ || enable_inf_to_num_;
    }

    bool enable_nan_to_str() const
    {
        return enable_nan_to_str_;
    }

    bool enable_str_to_nan() const
    {
        return enable_str_to_nan_;
    }

    bool enable_inf_to_str() const
    {
        return enable_inf_to_str_;
    }

    bool enable_str_to_inf() const
    {
        return enable_str_to_inf_;
    }

    bool enable_neginf_to_str() const
    {
        return enable_neginf_to_str_ || enable_inf_to_str_;
    }

    bool enable_str_to_neginf() const
    {
        return enable_str_to_neginf_ || enable_str_to_inf_;
    }

    string_type nan_to_num() const
    {
        if (enable_nan_to_num_)
        {
            return nan_to_num_;
        }
        else
        {
            return nan_to_num_; // empty string
        }
    }

    string_type inf_to_num() const
    {
        if (enable_inf_to_num_)
        {
            return inf_to_num_;
        }
        else
        {
            return inf_to_num_; // empty string
        }
    }

    string_type neginf_to_num() const
    {
        if (enable_neginf_to_num_)
        {
            return neginf_to_num_;
        }
        else if (enable_inf_to_num_)
        {
            string_type s;
            s.push_back('-');
            s.append(inf_to_num_);
            return s;
        }
        else
        {
            return neginf_to_num_; // empty string
        }
    }

    string_type nan_to_str() const
    {
        return nan_to_str_;
    }

    string_type inf_to_str() const
    {
        return inf_to_str_;
    }

    string_type neginf_to_str() const
    {
        if (enable_neginf_to_str_)
        {
            return neginf_to_str_;
        }
        else if (enable_inf_to_str_)
        {
            string_type s;
            s.push_back('-');
            s.append(inf_to_str_);
            return s;
        }
        else
        {
            return neginf_to_str_; // empty string
        }
    }

    int max_nesting_depth() const 
    {
        return max_nesting_depth_;
    }
};

template <typename CharT>
class basic_json_decode_options : public virtual basic_json_options_common<CharT>
{
    friend class basic_json_options<CharT>;
    using super_type = basic_json_options_common<CharT>;
public:
    using typename super_type::char_type;
    using typename super_type::string_type;
private:
    bool lossless_number_{false};
    bool lossless_bignum_{true};
    bool allow_comments_{true};
    bool allow_trailing_comma_{false};
    std::function<bool(json_errc,const ser_context&)> err_handler_;
public:
    basic_json_decode_options()
        : err_handler_(default_json_parsing())
    {
    }

    basic_json_decode_options(const basic_json_decode_options&) = default;

    basic_json_decode_options(basic_json_decode_options&& other) noexcept
        : super_type(std::move(other)), 
          lossless_number_(other.lossless_number_), 
          lossless_bignum_(other.lossless_bignum_), 
          allow_comments_(other.allow_comments_), 
          allow_trailing_comma_(other.allow_trailing_comma_), 
          err_handler_(std::move(other.err_handler_))
    {
    }
protected:
    basic_json_decode_options& operator=(const basic_json_decode_options&) = default;
    basic_json_decode_options& operator=(basic_json_decode_options&&) = default;
public:
    bool lossless_number() const 
    {
        return lossless_number_;
    }
    bool lossless_bignum() const 
    {
        return lossless_bignum_;
    }

    bool allow_comments() const 
    {
        return allow_comments_;
    }

    bool allow_trailing_comma() const 
    {
        return allow_trailing_comma_;
    }

#if !defined(JSONCONS_NO_DEPRECATED)
    const std::function<bool(json_errc,const ser_context&)>& err_handler() const 
    {
        return err_handler_;
    }
#endif
};

template <typename CharT>
class basic_json_encode_options : public virtual basic_json_options_common<CharT>
{
    friend class basic_json_options<CharT>;
    using super_type = basic_json_options_common<CharT>;
public:
    using typename super_type::char_type;
    using typename super_type::string_type;

    static constexpr uint8_t indent_size_default = 4;
    static constexpr size_t line_length_limit_default = 120;
private:
    bool escape_all_non_ascii_:1;
    bool escape_solidus_:1;
    bool pad_inside_object_braces_:1;
    bool pad_inside_array_brackets_:1;
    float_chars_format float_format_;
    byte_string_chars_format byte_string_format_;
    bignum_format_kind bignum_format_;
    line_split_kind root_line_splits_;
    line_split_kind object_object_line_splits_;
    line_split_kind object_array_line_splits_;
    line_split_kind array_array_line_splits_;
    line_split_kind array_object_line_splits_;
    spaces_option spaces_around_colon_;
    spaces_option spaces_around_comma_;
    int8_t precision_{0};
    uint8_t indent_size_{indent_size_default};
    std::size_t line_length_limit_{line_length_limit_default};
    string_type new_line_chars_;
    char_type indent_char_;
public:
    basic_json_encode_options()
        : escape_all_non_ascii_(false),
          escape_solidus_(false),
          pad_inside_object_braces_(false),
          pad_inside_array_brackets_(false),
          float_format_(float_chars_format::general),
          byte_string_format_(byte_string_chars_format::none),
          bignum_format_(bignum_format_kind::raw),
          root_line_splits_(line_split_kind::multi_line),
          object_object_line_splits_(line_split_kind::multi_line),
          object_array_line_splits_(line_split_kind::multi_line),
          array_array_line_splits_(line_split_kind::multi_line),
          array_object_line_splits_(line_split_kind::multi_line),
          spaces_around_colon_(spaces_option::space_after),
          spaces_around_comma_(spaces_option::space_after),
          indent_char_(' ')
    {
        new_line_chars_.push_back('\n');
    }

    basic_json_encode_options(const basic_json_encode_options&) = default;

    basic_json_encode_options(basic_json_encode_options&& other) noexcept
        : super_type(std::move(other)),
          escape_all_non_ascii_(other.escape_all_non_ascii_),
          escape_solidus_(other.escape_solidus_),
          pad_inside_object_braces_(other.pad_inside_object_braces_),
          pad_inside_array_brackets_(other.pad_inside_array_brackets_),
          float_format_(other.float_format_),
          byte_string_format_(other.byte_string_format_),
          bignum_format_(other.bignum_format_),
          root_line_splits_(other.root_line_splits_),
          object_object_line_splits_(other.object_object_line_splits_),
          object_array_line_splits_(other.object_array_line_splits_),
          array_array_line_splits_(other.array_array_line_splits_),
          array_object_line_splits_(other.array_object_line_splits_),
          spaces_around_colon_(other.spaces_around_colon_),
          spaces_around_comma_(other.spaces_around_comma_),
          precision_(other.precision_),
          indent_size_(other.indent_size_),
          line_length_limit_(other.line_length_limit_),
          new_line_chars_(std::move(other.new_line_chars_)),
          indent_char_(other.indent_char_)
    {
    }
    
    ~basic_json_encode_options() = default;
protected:
    basic_json_encode_options& operator=(const basic_json_encode_options&) = default;
    basic_json_encode_options& operator=(basic_json_encode_options&&) = default;
public:
    byte_string_chars_format byte_string_format() const  {return byte_string_format_;}

#if !defined(JSONCONS_NO_DEPRECATED)
    JSONCONS_DEPRECATED_MSG("Instead, use bignum_format")
    bignum_format_kind bigint_format() const  {return bignum_format_;}
#endif    

    bignum_format_kind bignum_format() const  {return bignum_format_;}

#if !defined(JSONCONS_NO_DEPRECATED)
    line_split_kind line_splits() const  {return root_line_splits_;}
#endif    

    line_split_kind root_line_splits() const  {return root_line_splits_;}

    line_split_kind object_object_line_splits() const  {return object_object_line_splits_;}

    line_split_kind array_object_line_splits() const  {return array_object_line_splits_;}

    line_split_kind object_array_line_splits() const  {return object_array_line_splits_;}

    line_split_kind array_array_line_splits() const  {return array_array_line_splits_;}

    uint8_t indent_size() const 
    {
        return indent_size_;
    }

    spaces_option spaces_around_colon() const 
    {
        return spaces_around_colon_;
    }

    spaces_option spaces_around_comma() const 
    {
        return spaces_around_comma_;
    }

    char_type indent_char() const 
    {
        return indent_char_;
    }

    bool pad_inside_object_braces() const 
    {
        return pad_inside_object_braces_;
    }

    bool pad_inside_array_brackets() const 
    {
        return pad_inside_array_brackets_;
    }

    string_type new_line_chars() const 
    {
        return new_line_chars_;
    }

    std::size_t line_length_limit() const 
    {
        return line_length_limit_;
    }

    float_chars_format float_format() const 
    {
        return float_format_;
    }

    int8_t precision() const 
    {
        return precision_;
    }

    bool escape_all_non_ascii() const 
    {
        return escape_all_non_ascii_;
    }

    bool escape_solidus() const 
    {
        return escape_solidus_;
    }

};

template <typename CharT>
class basic_json_options final: public basic_json_decode_options<CharT>, 
                                public basic_json_encode_options<CharT>
{
public:
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;

    using basic_json_options_common<CharT>::max_nesting_depth;

    using basic_json_decode_options<CharT>::enable_str_to_nan;
    using basic_json_decode_options<CharT>::enable_str_to_inf;
    using basic_json_decode_options<CharT>::enable_str_to_neginf;
    using basic_json_decode_options<CharT>::nan_to_str;
    using basic_json_decode_options<CharT>::inf_to_str;
    using basic_json_decode_options<CharT>::neginf_to_str;
    using basic_json_decode_options<CharT>::nan_to_num;
    using basic_json_decode_options<CharT>::inf_to_num;
    using basic_json_decode_options<CharT>::neginf_to_num;

    using basic_json_decode_options<CharT>::lossless_number;
    using basic_json_decode_options<CharT>::lossless_bignum;
    using basic_json_decode_options<CharT>::allow_comments;
    using basic_json_decode_options<CharT>::allow_trailing_comma;
#if !defined(JSONCONS_NO_DEPRECATED)
    using basic_json_decode_options<CharT>::err_handler;
#endif
    using basic_json_encode_options<CharT>::byte_string_format;
    using basic_json_encode_options<CharT>::bignum_format;

#if !defined(JSONCONS_NO_DEPRECATED)
    using basic_json_encode_options<CharT>::line_splits;
#endif
    using basic_json_encode_options<CharT>::root_line_splits;
    using basic_json_encode_options<CharT>::object_object_line_splits;
    using basic_json_encode_options<CharT>::array_object_line_splits;
    using basic_json_encode_options<CharT>::object_array_line_splits;
    using basic_json_encode_options<CharT>::array_array_line_splits;
    using basic_json_encode_options<CharT>::indent_size;
    using basic_json_encode_options<CharT>::spaces_around_colon;
    using basic_json_encode_options<CharT>::spaces_around_comma;
    using basic_json_encode_options<CharT>::pad_inside_object_braces;
    using basic_json_encode_options<CharT>::pad_inside_array_brackets;
    using basic_json_encode_options<CharT>::new_line_chars;
    using basic_json_encode_options<CharT>::line_length_limit;
    using basic_json_encode_options<CharT>::float_format;
    using basic_json_encode_options<CharT>::precision;
    using basic_json_encode_options<CharT>::escape_all_non_ascii;
    using basic_json_encode_options<CharT>::escape_solidus;
public:

//  Constructors

    basic_json_options() = default;
    basic_json_options(const basic_json_options&) = default;
    basic_json_options(basic_json_options&&) = default;
    basic_json_options& operator=(const basic_json_options&) = default;
    basic_json_options& operator=(basic_json_options&&) = default;

    basic_json_options& nan_to_num(const string_type& value)
    {
        this->enable_nan_to_num_ = true;
        this->nan_to_str_.clear();
        this->nan_to_num_ = value;
        return *this;
    }

    basic_json_options& inf_to_num(const string_type& value)
    {
        this->enable_inf_to_num_ = true;
        this->inf_to_str_.clear();
        this->inf_to_num_ = value;
        return *this;
    }

    basic_json_options& neginf_to_num(const string_type& value)
    {
        this->enable_neginf_to_num_ = true;
        this->neginf_to_str_.clear();
        this->neginf_to_num_ = value;
        return *this;
    }

    basic_json_options& nan_to_str(const string_type& value, bool enable_inverse = true)
    {
        this->enable_nan_to_str_ = true;
        this->enable_str_to_nan_ = enable_inverse;
        this->nan_to_num_.clear();
        this->nan_to_str_ = value;
        return *this;
    }

    basic_json_options& inf_to_str(const string_type& value, bool enable_inverse = true)
    {
        this->enable_inf_to_str_ = true;
        this->enable_str_to_inf_ = enable_inverse;
        this->inf_to_num_.clear();
        this->inf_to_str_ = value;
        return *this;
    }

    basic_json_options& neginf_to_str(const string_type& value, bool enable_inverse = true)
    {
        this->enable_neginf_to_str_ = true;
        this->enable_str_to_neginf_ = enable_inverse;
        this->neginf_to_num_.clear();
        this->neginf_to_str_ = value;
        return *this;
    }

    basic_json_options&  byte_string_format(byte_string_chars_format value) {this->byte_string_format_ = value; return *this;}


#if !defined(JSONCONS_NO_DEPRECATED)
    JSONCONS_DEPRECATED_MSG("Instead, use bignum_format")
    basic_json_options& bigint_format(bignum_format_kind value) {this->bignum_format_ = value; return *this;}
#endif    

    basic_json_options& bignum_format(bignum_format_kind value) {this->bignum_format_ = value; return *this;}

#if !defined(JSONCONS_NO_DEPRECATED)
    basic_json_options& line_splits(line_split_kind value) {this->root_line_splits_ = value; return *this;}
#endif    

    basic_json_options& root_line_splits(line_split_kind value) {this->root_line_splits_ = value; return *this;}

    basic_json_options& object_object_line_splits(line_split_kind value) {this->object_object_line_splits_ = value; return *this;}

    basic_json_options& array_object_line_splits(line_split_kind value) {this->array_object_line_splits_ = value; return *this;}

    basic_json_options& object_array_line_splits(line_split_kind value) {this->object_array_line_splits_ = value; return *this;}

    basic_json_options& array_array_line_splits(line_split_kind value) {this->array_array_line_splits_ = value; return *this;}

    basic_json_options& indent_size(uint8_t value)
    {
        this->indent_size_ = value;
        return *this;
    }

    basic_json_options& spaces_around_colon(spaces_option value)
    {
        this->spaces_around_colon_ = value;
        return *this;
    }

    basic_json_options& spaces_around_comma(spaces_option value)
    {
        this->spaces_around_comma_ = value;
        return *this;
    }

    basic_json_options& indent_char(char_type value)
    {
        this->indent_char_ = value;
        return *this;
    }

    basic_json_options& pad_inside_object_braces(bool value)
    {
        this->pad_inside_object_braces_ = value;
        return *this;
    }

    basic_json_options& pad_inside_array_brackets(bool value)
    {
        this->pad_inside_array_brackets_ = value;
        return *this;
    }

    basic_json_options& new_line_chars(const string_type& value)
    {
        this->new_line_chars_ = value;
        return *this;
    }

    basic_json_options& lossless_number(bool value) 
    {
        this->lossless_number_ = value;
        return *this;
    }

    basic_json_options& lossless_bignum(bool value) 
    {
        this->lossless_bignum_ = value;
        return *this;
    }

    basic_json_options& allow_comments(bool value) 
    {
        this->allow_comments_ = value;
        return *this;
    }

    basic_json_options& allow_trailing_comma(bool value) 
    {
        this->allow_trailing_comma_ = value;
        return *this;
    }

#if !defined(JSONCONS_NO_DEPRECATED)
    basic_json_options& err_handler(const std::function<bool(json_errc,const ser_context&)>& value) 
    {
        this->err_handler_ = value;
        return *this;
    }
#endif
    basic_json_options& line_length_limit(std::size_t value)
    {
        this->line_length_limit_ = value;
        return *this;
    }

    basic_json_options& float_format(float_chars_format value)
    {
        this->float_format_ = value;
        return *this;
    }

    basic_json_options& precision(int8_t value)
    {
        this->precision_ = value;
        return *this;
    }

    basic_json_options& escape_all_non_ascii(bool value)
    {
        this->escape_all_non_ascii_ = value;
        return *this;
    }

    basic_json_options& escape_solidus(bool value)
    {
        this->escape_solidus_ = value;
        return *this;
    }

    basic_json_options& max_nesting_depth(int value)
    {
        this->max_nesting_depth_ = value;
        return *this;
    }

private:
    enum class input_state {initial,begin_quote,character,end_quote,escape,error};
    bool is_string(const string_type& s) const
    {
        input_state state = input_state::initial;
        for (char_type c : s)
        {
            switch (c)
            {
            case '\t': case ' ': case '\n': case'\r':
                break;
            case '\\':
                state = input_state::escape;
                break;
            case '\"':
                switch (state)
                {
                case input_state::initial:
                    state = input_state::begin_quote;
                    break;
                case input_state::begin_quote:
                case input_state::character:
                    state = input_state::end_quote;
                    break;
                case input_state::end_quote:
                    state = input_state::error;
                    break;
                case input_state::escape:
                    state = input_state::character;
                    break;
                default:
                    state = input_state::character;
                    break;
                }
                break;
            default:
                break;
            }

        }
        return state == input_state::end_quote;
    }
};

using json_options = basic_json_options<char>;
using wjson_options = basic_json_options<wchar_t>;

} // namespace jsoncons

#endif // JSONCONS_JSON_OPTIONS_HPP
