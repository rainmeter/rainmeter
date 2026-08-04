// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_ENCODER_HPP
#define JSONCONS_JSON_ENCODER_HPP

#include <array> // std::array
#include <cstddef>
#include <cstdint>
#include <cmath> // std::isfinite, std::isnan
#include <limits> // std::numeric_limits
#include <memory>
#include <string>
#include <utility> // std::move
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/sink.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/unicode_traits.hpp>
#include <jsoncons/json_encoders.hpp>

namespace jsoncons { 

    template <typename CharT,typename Sink=jsoncons::stream_sink<CharT>,typename Allocator=std::allocator<char>>
    class basic_json_encoder final : public basic_json_visitor<CharT>
    {
        static const jsoncons::basic_string_view<CharT> null_literal()
        {
            static const jsoncons::basic_string_view<CharT> k = JSONCONS_STRING_VIEW_CONSTANT(CharT, "null");
            return k;
        }
        static const jsoncons::basic_string_view<CharT> true_literal()
        {
            static const jsoncons::basic_string_view<CharT> k = JSONCONS_STRING_VIEW_CONSTANT(CharT, "true");
            return k;
        }
        static const jsoncons::basic_string_view<CharT> false_literal()
        {
            static const jsoncons::basic_string_view<CharT> k = JSONCONS_STRING_VIEW_CONSTANT(CharT, "false");
            return k;
        }

        static const std::array<CharT,1> colon;
        static const std::array<CharT,2> colon_space; 
        static const std::array<CharT,2> space_colon; 
        static const std::array<CharT,3> space_colon_space; 
        static const std::array<CharT,1> comma;
        static const std::array<CharT,2> comma_space; 
        static const std::array<CharT,2> space_comma; 
        static const std::array<CharT,3> space_comma_space; 
        static const std::array<CharT,1> left_brace; 
        static const std::array<CharT,1> right_brace; 
        static const std::array<CharT,2> left_brace_space;
        static const std::array<CharT,2> space_right_brace; 
        static const std::array<CharT,1> left_bracket; 
        static const std::array<CharT,1> right_bracket; 
        static const std::array<CharT,2> left_bracket_space;
        static const std::array<CharT,2> space_right_bracket; 
    public:
        using allocator_type = Allocator;
        using char_type = CharT;
        using typename basic_json_visitor<CharT>::string_view_type;
        using sink_type = Sink;
        using string_type = typename basic_json_encode_options<CharT>::string_type;

    private:
        enum class container_type {object, array};

        class encoding_context
        {
            container_type type_;
            line_split_kind split_kind_;
            bool indent_before_;
            bool new_line_after_;
            std::size_t begin_pos_{0};
            std::size_t data_pos_{0};
            std::size_t count_{0};
        public:
            encoding_context(container_type type, line_split_kind split_lines, bool indent_once,
                             std::size_t begin_pos, std::size_t data_pos) noexcept
               : type_(type), split_kind_(split_lines), indent_before_(indent_once), new_line_after_(false),
                 begin_pos_(begin_pos), data_pos_(data_pos)
            {
            }

            encoding_context(const encoding_context&) = default;
            
            ~encoding_context() = default;
            
            encoding_context& operator=(const encoding_context&) = default;

            void set_position(std::size_t pos)
            {
                data_pos_ = pos;
            }

            std::size_t begin_pos() const
            {
                return begin_pos_;
            }

            std::size_t data_pos() const
            {
                return data_pos_;
            }

            std::size_t count() const
            {
                return count_;
            }

            void increment_count()
            {
                ++count_;
            }

            bool new_line_after() const
            {
                return new_line_after_;
            }

            void new_line_after(bool value) 
            {
                new_line_after_ = value;
            }

            bool is_object() const
            {
                return type_ == container_type::object;
            }

            bool is_array() const
            {
                return type_ == container_type::array;
            }

            line_split_kind split_kind() const
            {
                return split_kind_;
            }

            bool is_multi_line() const
            {
                return split_kind_ == line_split_kind::multi_line;
            }

            bool is_indent_once() const
            {
                return count_ == 0 ? indent_before_ : false;
            }

        };
        using encoding_context_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<encoding_context>;

        Sink sink_;
        basic_json_encode_options<CharT> options_;
        char_type indent_char_{' '};
        jsoncons::write_double fp_;

        std::vector<encoding_context,encoding_context_allocator_type> stack_;
        int indent_amount_{0};
        std::size_t column_{0};
        jsoncons::basic_string_view<CharT> colon_str_;
        jsoncons::basic_string_view<CharT> comma_str_;
        jsoncons::basic_string_view<CharT> open_brace_str_;
        jsoncons::basic_string_view<CharT> close_brace_str_;
        jsoncons::basic_string_view<CharT> open_bracket_str_;
        jsoncons::basic_string_view<CharT> close_bracket_str_;
        int nesting_depth_{0};
    public:

        // Noncopyable and nonmoveable
        basic_json_encoder(const basic_json_encoder&) = delete;
        basic_json_encoder(basic_json_encoder&&) = delete;

        basic_json_encoder(Sink&& sink, 
                           const Allocator& alloc = Allocator())
            : basic_json_encoder(std::forward<Sink>(sink), basic_json_encode_options<CharT>(), alloc)
        {
        }

        basic_json_encoder(Sink&& sink, 
                           const basic_json_encode_options<CharT>& options, 
                           const Allocator& alloc = Allocator())
           : sink_(std::forward<Sink>(sink)), 
             options_(options),
             indent_char_(options.indent_char()),
             fp_(options.float_format(), options.precision()),
             stack_(alloc)
        {
            switch (options.spaces_around_colon())
            {
                case spaces_option::space_after:
                    colon_str_ = jsoncons::basic_string_view<CharT>(colon_space.data(), colon_space.size());
                    break;
                case spaces_option::space_before:
                    colon_str_ = jsoncons::basic_string_view<CharT>(space_colon.data(), space_colon.size());
                    break;
                case spaces_option::space_before_and_after:
                    colon_str_ = jsoncons::basic_string_view<CharT>(space_colon_space.data(), space_colon_space.size());
                    break;
                default:
                    colon_str_ = jsoncons::basic_string_view<CharT>(colon.data(), colon.size());
                    break;
            }
            switch (options.spaces_around_comma())
            {
                case spaces_option::space_after:
                    comma_str_ = jsoncons::basic_string_view<CharT>(comma_space.data(), colon_space.size());
                    break;
                case spaces_option::space_before:
                    comma_str_ = jsoncons::basic_string_view<CharT>(space_comma.data(), space_comma.size());
                    break;
                case spaces_option::space_before_and_after:
                    comma_str_ = jsoncons::basic_string_view<CharT>(space_comma_space.data(), space_comma_space.size());
                    break;
                default:
                    comma_str_ = jsoncons::basic_string_view<CharT>(comma.data(), comma.size());
                    break;
            }
            if (options.pad_inside_object_braces())
            {
                open_brace_str_ = jsoncons::basic_string_view<CharT>(left_brace_space.data(), left_brace_space.size());
                close_brace_str_ = jsoncons::basic_string_view<CharT>(space_right_brace.data(), space_right_brace.size());
            }
            else
            {
                open_brace_str_ = jsoncons::basic_string_view<CharT>(left_brace.data(), left_brace.size());
                close_brace_str_ = jsoncons::basic_string_view<CharT>(right_brace.data(), right_brace.size());
            }
            if (options.pad_inside_array_brackets())
            {
                open_bracket_str_ = jsoncons::basic_string_view<CharT>(left_bracket_space.data(), left_bracket_space.size());
                close_bracket_str_ = jsoncons::basic_string_view<CharT>(space_right_bracket.data(), space_right_bracket.size());
            }
            else
            {
                open_bracket_str_ = jsoncons::basic_string_view<CharT>(left_bracket.data(), left_bracket.size());
                close_bracket_str_ = jsoncons::basic_string_view<CharT>(right_bracket.data(), right_bracket.size());
            }
        }

        ~basic_json_encoder() noexcept
        {
            JSONCONS_TRY
            {
                sink_.flush();
            }
            JSONCONS_CATCH(...)
            {
            }
        }

        basic_json_encoder& operator=(const basic_json_encoder&) = delete;
        basic_json_encoder& operator=(basic_json_encoder&&) = delete;

        void reset()
        {
            stack_.clear();
            indent_amount_ = 0;
            column_ = 0;
            nesting_depth_ = 0;
        }

        void reset(Sink&& sink)
        {
            sink_ = std::move(sink);
            reset();
        }

    private:
        // Implementing methods
        void visit_flush() final
        {
            sink_.flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag, const ser_context&, std::error_code& ec) final
        {
            if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_nesting_depth()))
            {
                ec = json_errc::max_nesting_depth_exceeded;
                JSONCONS_VISITOR_RETURN;
            } 
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.append(comma_str_.data(),comma_str_.length());
                column_ += comma_str_.length();
            }

            if (!stack_.empty()) // object or array
            {
                if (stack_.back().is_object())
                {
                    line_split_kind split_kind = static_cast<uint8_t>(options_.object_object_line_splits()) >= static_cast<uint8_t>(stack_.back().split_kind()) ? 
                        options_.object_object_line_splits() : stack_.back().split_kind();
                    switch (split_kind)
                    {
                        case line_split_kind::same_line:
                        case line_split_kind::new_line:
                            if (column_ >= options_.line_length_limit())
                            {
                                break_line();
                            }
                            break;
                        default: // multi_line
                            break;
                    }
                    stack_.emplace_back(container_type::object,split_kind, false,
                                        column_, column_+open_brace_str_.length());
                }
                else // array
                {
                    line_split_kind split_kind = static_cast<uint8_t>(options_.array_object_line_splits()) >= static_cast<uint8_t>(stack_.back().split_kind()) ? 
                        options_.array_object_line_splits() : stack_.back().split_kind();
                    switch (split_kind)
                    {
                        case line_split_kind::same_line:
                            if (column_ >= options_.line_length_limit())
                            {
                                //stack_.back().new_line_after(true);
                                new_line();
                            }
                            else
                            {
                                stack_.back().new_line_after(true);
                                new_line();
                            }
                            break;
                        case line_split_kind::new_line:
                            stack_.back().new_line_after(true);
                            new_line();
                            break;
                        default: // multi_line
                            stack_.back().new_line_after(true);
                            new_line();
                            break;
                    }
                    stack_.emplace_back(container_type::object,split_kind, false,
                                        column_, column_+open_brace_str_.length());
                }
            }
            else 
            {
                stack_.emplace_back(container_type::object, options_.root_line_splits(), false,
                                    column_, column_+open_brace_str_.length());
            }
            indent();
            
            sink_.append(open_brace_str_.data(), open_brace_str_.length());
            column_ += open_brace_str_.length();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) final
        {
            JSONCONS_ASSERT(!stack_.empty());
            --nesting_depth_;

            unindent();
            if (stack_.back().new_line_after())
            {
                new_line();
            }
            stack_.pop_back();
            sink_.append(close_brace_str_.data(), close_brace_str_.length());
            column_ += close_brace_str_.length();

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag, const ser_context&, std::error_code& ec) final
        {
            if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_nesting_depth()))
            {
                ec = json_errc::max_nesting_depth_exceeded;
                JSONCONS_VISITOR_RETURN;
            } 
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.append(comma_str_.data(),comma_str_.length());
                column_ += comma_str_.length();
            }
            if (!stack_.empty())
            {
                if (stack_.back().is_object())
                {
                    line_split_kind split_kind = static_cast<uint8_t>(options_.object_array_line_splits()) >= static_cast<uint8_t>(stack_.back().split_kind()) ? 
                        options_.object_array_line_splits() : 
                        stack_.back().split_kind();
                    switch (split_kind)
                    {
                        case line_split_kind::same_line:
                            stack_.emplace_back(container_type::array,split_kind,false,
                                                column_, column_ + open_bracket_str_.length());
                            break;
                        case line_split_kind::new_line:
                        {
                            stack_.emplace_back(container_type::array,split_kind,true,
                                                column_, column_+open_bracket_str_.length());
                            break;
                        }
                        default: // multi_line
                            stack_.emplace_back(container_type::array,split_kind,true,
                                                column_, column_+open_bracket_str_.length());
                            break;
                    }
                }
                else // array
                {
                    line_split_kind split_kind = static_cast<uint8_t>(options_.array_array_line_splits()) >= static_cast<uint8_t>(stack_.back().split_kind()) ? 
                        options_.array_array_line_splits() : stack_.back().split_kind();
                    switch (split_kind)
                    {
                        case line_split_kind::same_line:
                            if (stack_.back().is_multi_line())
                            {
                                stack_.back().new_line_after(true);
                                new_line();
                            }
                            stack_.emplace_back(container_type::array,split_kind, false,
                                                column_, column_+open_bracket_str_.length());
                            break;
                        case line_split_kind::new_line:
                            stack_.back().new_line_after(true);
                            new_line();
                            stack_.emplace_back(container_type::array,split_kind, true,
                                                column_, column_+open_bracket_str_.length());
                            break;
                        default: // multi_line
                            stack_.back().new_line_after(true);
                            new_line();
                            stack_.emplace_back(container_type::array,split_kind, false,
                                                column_, column_+open_bracket_str_.length());
                            break;
                    }
                }
            }
            else 
            {
                stack_.emplace_back(container_type::array, options_.root_line_splits(), false,
                                    column_, column_+open_bracket_str_.length());
            }
            indent();
            sink_.append(open_bracket_str_.data(), open_bracket_str_.length());
            column_ += open_bracket_str_.length();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) final
        {
            JSONCONS_ASSERT(!stack_.empty());
            --nesting_depth_;

            unindent();
            if (stack_.back().new_line_after())
            {
                new_line();
            }
            stack_.pop_back();
            sink_.append(close_bracket_str_.data(), close_bracket_str_.length());
            column_ += close_bracket_str_.length();
            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name, const ser_context&, std::error_code&) final
        {
            JSONCONS_ASSERT(!stack_.empty());
            if (stack_.back().count() > 0)
            {
                sink_.append(comma_str_.data(),comma_str_.length());
                column_ += comma_str_.length();
            }

            if (stack_.back().is_multi_line())
            {
                stack_.back().new_line_after(true);
                new_line();
            }
            else if (stack_.back().count() > 0 && column_ >= options_.line_length_limit())
            {
                //stack_.back().new_line_after(true);
                new_line(stack_.back().data_pos());
            }

            if (stack_.back().count() == 0)
            {
                stack_.back().set_position(column_);
            }
            sink_.push_back('\"');
            std::size_t length = jsoncons::detail::escape_string(name.data(), name.length(),options_.escape_all_non_ascii(),options_.escape_solidus(),sink_);
            sink_.push_back('\"');
            sink_.append(colon_str_.data(),colon_str_.length());
            column_ += (length+2+colon_str_.length());
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag, const ser_context&, std::error_code&) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }

            sink_.append(null_literal().data(), null_literal().size());
            column_ += null_literal().size();

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& sv, semantic_tag tag, const ser_context& context, std::error_code& ec) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }
            
            write_string(sv, tag, context, ec);

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        void write_string(const string_view_type& sv, semantic_tag tag, const ser_context&, std::error_code&) 
        {
            if (JSONCONS_LIKELY(tag == semantic_tag::noesc && !options_.escape_all_non_ascii() && !options_.escape_solidus()))
            {
                //std::cout << "noesc\n";
                sink_.push_back('\"');
                sink_.append(sv.data(), sv.length());
                sink_.push_back('\"');
                column_ += (sv.length()+2);
            }
            else if (tag == semantic_tag::bigint)
            {
                write_bignum_value(sv);
            }
            else if (tag == semantic_tag::bigdec && options_.bignum_format() == bignum_format_kind::raw)
            {
                write_bignum_value(sv);
            }
            else
            {
                //if (tag != semantic_tag::bigdec)
                //    std::cout << "esc\n";
                sink_.push_back('\"');
                std::size_t length = jsoncons::detail::escape_string(sv.data(), sv.length(),options_.escape_all_non_ascii(),options_.escape_solidus(),sink_);
                sink_.push_back('\"');
                column_ += (length+2);
            }           
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& b, 
                                  semantic_tag tag,
                                  const ser_context&,
                                  std::error_code&) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }

            byte_string_chars_format encoding_hint;
            switch (tag)
            {
                case semantic_tag::base16:
                    encoding_hint = byte_string_chars_format::base16;
                    break;
                case semantic_tag::base64:
                    encoding_hint = byte_string_chars_format::base64;
                    break;
                case semantic_tag::base64url:
                    encoding_hint = byte_string_chars_format::base64url;
                    break;
                default:
                    encoding_hint = byte_string_chars_format::none;
                    break;
            }

            byte_string_chars_format format = jsoncons::detail::resolve_byte_string_chars_format(options_.byte_string_format(), 
                                                                                                 encoding_hint, 
                                                                                                 byte_string_chars_format::base64url);
            switch (format)
            {
                case byte_string_chars_format::base16:
                {
                    sink_.push_back('\"');
                    std::size_t length = bytes_to_base16(b.begin(),b.end(),sink_);
                    sink_.push_back('\"');
                    column_ += (length + 2);
                    break;
                }
                case byte_string_chars_format::base64:
                {
                    sink_.push_back('\"');
                    std::size_t length = bytes_to_base64(b.begin(), b.end(), sink_);
                    sink_.push_back('\"');
                    column_ += (length + 2);
                    break;
                }
                case byte_string_chars_format::base64url:
                {
                    sink_.push_back('\"');
                    std::size_t length = bytes_to_base64url(b.begin(),b.end(),sink_);
                    sink_.push_back('\"');
                    column_ += (length + 2);
                    break;
                }
                default:
                {
                    JSONCONS_UNREACHABLE();
                }
            }

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
                             semantic_tag,
                             const ser_context& context,
                             std::error_code& ec) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }

            if (!std::isfinite(value))
            {
                if ((std::isnan)(value))
                {
                    if (options_.enable_nan_to_num())
                    {
                        sink_.append(options_.nan_to_num().data(), options_.nan_to_num().length());
                        column_ += options_.nan_to_num().length();
                    }
                    else if (options_.enable_nan_to_str())
                    {
                        write_string(options_.nan_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                        column_ += null_literal().size();
                    }
                }
                else if (value == std::numeric_limits<double>::infinity())
                {
                    if (options_.enable_inf_to_num())
                    {
                        sink_.append(options_.inf_to_num().data(), options_.inf_to_num().length());
                        column_ += options_.inf_to_num().length();
                    }
                    else if (options_.enable_inf_to_str())
                    {
                        write_string(options_.inf_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                        column_ += null_literal().size();
                    }
                }
                else
                {
                    if (options_.enable_neginf_to_num())
                    {
                        sink_.append(options_.neginf_to_num().data(), options_.neginf_to_num().length());
                        column_ += options_.neginf_to_num().length();
                    }
                    else if (options_.enable_neginf_to_str())
                    {
                        write_string(options_.neginf_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                        column_ += null_literal().size();
                    }
                }
            }
            else
            {
                std::size_t length = fp_(value, sink_);
                column_ += length;
            }

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
                            semantic_tag,
                            const ser_context&,
                            std::error_code&) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }
            std::size_t length = jsoncons::from_integer(value, sink_);
            column_ += length;
            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
                             semantic_tag, 
                             const ser_context&,
                             std::error_code&) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }
            std::size_t length = jsoncons::from_integer(value, sink_);
            column_ += length;
            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag, const ser_context&, std::error_code&) final
        {
            if (!stack_.empty()) 
            {
                if (stack_.back().is_array())
                {
                    begin_scalar_value();
                }
                if (!stack_.back().is_multi_line() && column_ >= options_.line_length_limit())
                {
                    break_line();
                }
            }

            if (value)
            {
                sink_.append(true_literal().data(), true_literal().size());
                column_ += true_literal().size();
            }
            else
            {
                sink_.append(false_literal().data(), false_literal().size());
                column_ += false_literal().size();
            }

            end_value();
            JSONCONS_VISITOR_RETURN;
        }

        void begin_scalar_value()
        {
            if (!stack_.empty())
            {
                if (stack_.back().count() > 0)
                {
                    sink_.append(comma_str_.data(),comma_str_.length());
                    column_ += comma_str_.length();
                }
                if (stack_.back().is_multi_line() || stack_.back().is_indent_once())
                {
                    stack_.back().new_line_after(true);
                    new_line();
                }
            }
        }

        void write_bignum_value(const string_view_type& sv)
        {
            switch (options_.bignum_format())
            {
                case bignum_format_kind::raw:
                {
                    sink_.append(sv.data(),sv.size());
                    column_ += sv.size();
                    break;
                }
                case bignum_format_kind::base64:
                {
                    bigint n(sv.data(), sv.length());
                    bool is_neg = n < 0;
                    if (is_neg)
                    {
                        n = - n -1;
                    }
                    int signum;
                    std::vector<uint8_t> v;
                    n.write_bytes_be(signum, v);

                    sink_.push_back('\"');
                    if (is_neg)
                    {
                        sink_.push_back('~');
                        ++column_;
                    }
                    std::size_t length = bytes_to_base64(v.begin(), v.end(), sink_);
                    sink_.push_back('\"');
                    column_ += (length+2);
                    break;
                }
                case bignum_format_kind::base64url:
                {
                    bigint n(sv.data(), sv.length());
                    bool is_neg = n < 0;
                    if (is_neg)
                    {
                        n = - n -1;
                    }
                    int signum;
                    std::vector<uint8_t> v;
                    n.write_bytes_be(signum, v);

                    sink_.push_back('\"');
                    if (is_neg)
                    {
                        sink_.push_back('~');
                        ++column_;
                    }
                    std::size_t length = bytes_to_base64url(v.begin(), v.end(), sink_);
                    sink_.push_back('\"');
                    column_ += (length+2);
                    break;
                }
                default:
                {
                    sink_.push_back('\"');
                    sink_.append(sv.data(),sv.size());
                    sink_.push_back('\"');
                    column_ += (sv.size() + 2);
                    break;
                }
            }
        }

        void end_value()
        {
            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
        }

        void indent()
        {
            indent_amount_ += static_cast<uint8_t>(options_.indent_size());
        }

        void unindent()
        {
            indent_amount_ -= static_cast<uint8_t>(options_.indent_size());
        }

        void new_line()
        {
            sink_.append(options_.new_line_chars().data(),options_.new_line_chars().length());
            for (int i = 0; i < indent_amount_; ++i)
            {
                sink_.push_back(indent_char_);
            }
            column_ = indent_amount_;
        }

        void new_line(std::size_t len)
        {
            sink_.append(options_.new_line_chars().data(),options_.new_line_chars().length());
            for (std::size_t i = 0; i < len; ++i)
            {
                sink_.push_back(' ');
            }
            column_ = len;
        }

        void break_line()
        {
            stack_.back().new_line_after(true);
            new_line();
        }
    };

    template <typename CharT, typename Sink, typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT, Sink, Allocator>::colon = {':'};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::colon_space = {':', ' '};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::space_colon = {' ', ':'};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,3> basic_json_encoder<CharT,Sink,Allocator>::space_colon_space = {' ', ':', ' '};

    template <typename CharT, typename Sink, typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT, Sink, Allocator>::comma = {','};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::comma_space = {',', ' '};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::space_comma = {' ', ','};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,3> basic_json_encoder<CharT,Sink,Allocator>::space_comma_space = {' ', ',', ' '};

    template <typename CharT, typename Sink, typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT, Sink, Allocator>::left_brace = {'{'};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT,Sink,Allocator>::right_brace = {'}'};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::left_brace_space = {'{', ' '};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::space_right_brace = {' ', '}'};

    template <typename CharT, typename Sink, typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT, Sink, Allocator>::left_bracket = {'['};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,1> basic_json_encoder<CharT,Sink,Allocator>::right_bracket = {']'};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::left_bracket_space = {'[', ' '};
    template <typename CharT,typename Sink,typename Allocator>
    const std::array<CharT,2> basic_json_encoder<CharT,Sink,Allocator>::space_right_bracket = {' ', ']'};

    template <typename CharT,typename Sink=jsoncons::stream_sink<CharT>,typename Allocator=std::allocator<char>>
    class basic_compact_json_encoder final : public basic_json_visitor<CharT>
    {
        static const std::array<CharT, 4>& null_literal()
        {
            static constexpr std::array<CharT,4> k{{'n','u','l','l'}};
            return k;
        }
        static const std::array<CharT, 4>& true_literal()
        {
            static constexpr std::array<CharT,4> k{{'t','r','u','e'}};
            return k;
        }
        static const std::array<CharT, 5>& false_literal()
        {
            static constexpr std::array<CharT,5> k{{'f','a','l','s','e'}};
            return k;
        }
    public:
        using allocator_type = Allocator;
        using char_type = CharT;
        using typename basic_json_visitor<CharT>::string_view_type;
        using sink_type = Sink;
        using string_type = typename basic_json_encode_options<CharT>::string_type;

    private:
        enum class container_type {object, array};

        class encoding_context
        {
            container_type type_;
            std::size_t count_{0};
        public:
            encoding_context(container_type type) noexcept
               : type_(type)
            {
            }

            std::size_t count() const
            {
                return count_;
            }

            void increment_count()
            {
                ++count_;
            }

            bool is_array() const
            {
                return type_ == container_type::array;
            }
        };
        using encoding_context_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<encoding_context>;

        Sink sink_;
        basic_json_encode_options<CharT> options_;
        jsoncons::write_double fp_;
        std::vector<encoding_context,encoding_context_allocator_type> stack_;
        int nesting_depth_;
    public:

        // Noncopyable and nonmoveable
        basic_compact_json_encoder(const basic_compact_json_encoder&) = delete;
        basic_compact_json_encoder(basic_compact_json_encoder&&) = delete;

        basic_compact_json_encoder(Sink&& sink, 
            const Allocator& alloc = Allocator())
            : basic_compact_json_encoder(std::forward<Sink>(sink), basic_json_encode_options<CharT>(), alloc)
        {
        }

        basic_compact_json_encoder(Sink&& sink, 
            const basic_json_encode_options<CharT>& options, 
            const Allocator& alloc = Allocator())
           : sink_(std::forward<Sink>(sink)),
             options_(options),
             fp_(options.float_format(), options.precision()),
             stack_(alloc),
             nesting_depth_(0)          
        {
        }

        ~basic_compact_json_encoder() noexcept
        {
            JSONCONS_TRY
            {
                sink_.flush();
            }
            JSONCONS_CATCH(...)
            {
            }
        }

        basic_compact_json_encoder& operator=(const basic_compact_json_encoder&) = delete;
        basic_compact_json_encoder& operator=(basic_compact_json_encoder&&) = delete;

        void reset()
        {
            stack_.clear();
            nesting_depth_ = 0;
        }

        void reset(Sink&& sink)
        {
            sink_ = std::move(sink);
            reset();
        }

    private:
        // Implementing methods
        void visit_flush() final
        {
            sink_.flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag, const ser_context&, std::error_code& ec) final
        {
            if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_nesting_depth()))
            {
                ec = json_errc::max_nesting_depth_exceeded;
                JSONCONS_VISITOR_RETURN;
            } 
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            stack_.emplace_back(container_type::object);
            sink_.push_back('{');
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) final
        {
            JSONCONS_ASSERT(!stack_.empty());
            --nesting_depth_;

            stack_.pop_back();
            sink_.push_back('}');

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }


        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag, const ser_context&, std::error_code& ec) final
        {
            if (JSONCONS_UNLIKELY(++nesting_depth_ > options_.max_nesting_depth()))
            {
                ec = json_errc::max_nesting_depth_exceeded;
                JSONCONS_VISITOR_RETURN;
            } 
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }
            stack_.emplace_back(container_type::array);
            sink_.push_back('[');
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) final
        {
            JSONCONS_ASSERT(!stack_.empty());
            --nesting_depth_;

            stack_.pop_back();
            sink_.push_back(']');
            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name, const ser_context&, std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            sink_.push_back('\"');
            jsoncons::detail::escape_string(name.data(), name.length(),options_.escape_all_non_ascii(),options_.escape_solidus(),sink_);
            sink_.push_back('\"');
            sink_.push_back(':');
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag, const ser_context&, std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            sink_.append(null_literal().data(), null_literal().size());

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        void write_bignum_value(const string_view_type& sv)
        {
            switch (options_.bignum_format())
            {
                case bignum_format_kind::raw:
                {
                    sink_.append(sv.data(),sv.size());
                    break;
                }
                case bignum_format_kind::base64:
                {
                    bigint n(sv.data(), sv.length());
                    bool is_neg = n < 0;
                    if (is_neg)
                    {
                        n = - n -1;
                    }
                    int signum;
                    std::vector<uint8_t> v;
                    n.write_bytes_be(signum, v);

                    sink_.push_back('\"');
                    if (is_neg)
                    {
                        sink_.push_back('~');
                    }
                    bytes_to_base64(v.begin(), v.end(), sink_);
                    sink_.push_back('\"');
                    break;
                }
                case bignum_format_kind::base64url:
                {
                    bigint n(sv.data(), sv.length());
                    bool is_neg = n < 0;
                    if (is_neg)
                    {
                        n = - n -1;
                    }
                    int signum;
                    std::vector<uint8_t> v;
                    n.write_bytes_be(signum, v);

                    sink_.push_back('\"');
                    if (is_neg)
                    {
                        sink_.push_back('~');
                    }
                    bytes_to_base64url(v.begin(), v.end(), sink_);
                    sink_.push_back('\"');
                    break;
                }
                default:
                {
                    sink_.push_back('\"');
                    sink_.append(sv.data(),sv.size());
                    sink_.push_back('\"');
                    break;
                }
            }
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& sv, semantic_tag tag, const ser_context& context, std::error_code& ec) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            write_string(sv, tag, context, ec);

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        void write_string(const string_view_type& sv, semantic_tag tag, const ser_context&, std::error_code&) 
        {
            if (JSONCONS_LIKELY(tag == semantic_tag::noesc && !options_.escape_all_non_ascii() && !options_.escape_solidus()))
            {
                //std::cout << "noesc\n";
                sink_.push_back('\"');
                sink_.append(sv.data(), sv.length());
                sink_.push_back('\"');
            }
            else if (tag == semantic_tag::bigint)
            {
                write_bignum_value(sv);
            }
            else if (tag == semantic_tag::bigdec && options_.bignum_format() == bignum_format_kind::raw)
            {
                write_bignum_value(sv);
            }
            else
            {
                //if (tag != semantic_tag::bigdec)
                //    std::cout << "esc\n";
                sink_.push_back('\"');
                jsoncons::detail::escape_string(sv.data(), sv.length(),options_.escape_all_non_ascii(),options_.escape_solidus(),sink_);
                sink_.push_back('\"');
            }           
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& b, 
            semantic_tag tag,
            const ser_context&,
            std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            byte_string_chars_format encoding_hint;
            switch (tag)
            {
                case semantic_tag::base16:
                    encoding_hint = byte_string_chars_format::base16;
                    break;
                case semantic_tag::base64:
                    encoding_hint = byte_string_chars_format::base64;
                    break;
                case semantic_tag::base64url:
                    encoding_hint = byte_string_chars_format::base64url;
                    break;
                default:
                    encoding_hint = byte_string_chars_format::none;
                    break;
            }

            byte_string_chars_format format = jsoncons::detail::resolve_byte_string_chars_format(options_.byte_string_format(), 
                                                                                       encoding_hint, 
                                                                                       byte_string_chars_format::base64url);
            switch (format)
            {
                case byte_string_chars_format::base16:
                {
                    sink_.push_back('\"');
                    bytes_to_base16(b.begin(),b.end(),sink_);
                    sink_.push_back('\"');
                    break;
                }
                case byte_string_chars_format::base64:
                {
                    sink_.push_back('\"');
                    bytes_to_base64(b.begin(), b.end(), sink_);
                    sink_.push_back('\"');
                    break;
                }
                case byte_string_chars_format::base64url:
                {
                    sink_.push_back('\"');
                    bytes_to_base64url(b.begin(),b.end(),sink_);
                    sink_.push_back('\"');
                    break;
                }
                default:
                {
                    JSONCONS_UNREACHABLE();
                }
            }

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
                             semantic_tag,
                             const ser_context& context,
                             std::error_code& ec) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            if (JSONCONS_UNLIKELY(!std::isfinite(value)))
            {
                if ((std::isnan)(value))
                {
                    if (options_.enable_nan_to_num())
                    {
                        sink_.append(options_.nan_to_num().data(), options_.nan_to_num().length());
                    }
                    else if (options_.enable_nan_to_str())
                    {
                        write_string(options_.nan_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                    }
                }
                else if (value == std::numeric_limits<double>::infinity())
                {
                    if (options_.enable_inf_to_num())
                    {
                        sink_.append(options_.inf_to_num().data(), options_.inf_to_num().length());
                    }
                    else if (options_.enable_inf_to_str())
                    {
                        write_string(options_.inf_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                    }
                }
                else 
                {
                    if (options_.enable_neginf_to_num())
                    {
                        sink_.append(options_.neginf_to_num().data(), options_.neginf_to_num().length());
                    }
                    else if (options_.enable_neginf_to_str())
                    {
                        write_string(options_.neginf_to_str(), semantic_tag::none, context, ec);
                    }
                    else
                    {
                        sink_.append(null_literal().data(), null_literal().size());
                    }
                }
            }
            else
            {
                fp_(value, sink_);
            }

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
                            semantic_tag,
                            const ser_context&,
                            std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }
            jsoncons::from_integer(value, sink_);
            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
                             semantic_tag, 
                             const ser_context&,
                             std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }
            jsoncons::from_integer(value, sink_);
            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag, const ser_context&, std::error_code&) final
        {
            if (!stack_.empty() && stack_.back().is_array() && stack_.back().count() > 0)
            {
                sink_.push_back(',');
            }

            if (value)
            {
                sink_.append(true_literal().data(), true_literal().size());
            }
            else
            {
                sink_.append(false_literal().data(), false_literal().size());
            }

            if (!stack_.empty())
            {
                stack_.back().increment_count();
            }
            JSONCONS_VISITOR_RETURN;
        }
    };

    using json_stream_encoder = basic_json_encoder<char,jsoncons::stream_sink<char>>;
    using wjson_stream_encoder = basic_json_encoder<wchar_t,jsoncons::stream_sink<wchar_t>>;
    using compact_json_stream_encoder = basic_compact_json_encoder<char,jsoncons::stream_sink<char>>;
    using compact_wjson_stream_encoder = basic_compact_json_encoder<wchar_t,jsoncons::stream_sink<wchar_t>>;

    using json_string_encoder = basic_json_encoder<char,jsoncons::string_sink<std::string>>;
    using wjson_string_encoder = basic_json_encoder<wchar_t,jsoncons::string_sink<std::wstring>>;
    using compact_json_string_encoder = basic_compact_json_encoder<char,jsoncons::string_sink<std::string>>;
    using compact_wjson_string_encoder = basic_compact_json_encoder<wchar_t,jsoncons::string_sink<std::wstring>>;

} // namespace jsoncons

#endif // JSONCONS_JSON_ENCODER_HPP
