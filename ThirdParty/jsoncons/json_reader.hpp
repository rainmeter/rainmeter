// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_READER_HPP
#define JSONCONS_JSON_READER_HPP

#include <cstddef>
#include <functional>
#include <ios>
#include <memory> // std::allocator
#include <string>
#include <system_error>
#include <utility> // std::move

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/source_adaptor.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

namespace jsoncons {

    // utf8_other_json_input_adapter

    template <typename CharT>
    class json_utf8_to_other_visitor_adaptor : public json_visitor
    {
    public:
        using json_visitor::string_view_type;
    private:
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT>& other_visitor_;
        //std::function<bool(json_errc,const ser_context&)> err_handler_;

        // noncopyable and nonmoveable
        json_utf8_to_other_visitor_adaptor(const json_utf8_to_other_visitor_adaptor<CharT>&) = delete;
        json_utf8_to_other_visitor_adaptor<CharT>& operator=(const json_utf8_to_other_visitor_adaptor<CharT>&) = delete;

    public:
        json_utf8_to_other_visitor_adaptor()
            : other_visitor_(default_visitor_)
        {
        }

        json_utf8_to_other_visitor_adaptor(basic_json_visitor<CharT>& other_visitor/*,
                                              std::function<bool(json_errc,const ser_context&)> err_handler*/)
            : other_visitor_(other_visitor)/*,
              err_handler_(err_handler)*/
        {
        }

    private:

        void visit_flush() override
        {
            other_visitor_.flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.begin_object(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.end_object(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.begin_array(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name, const ser_context& context, std::error_code& ec) override
        {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                name.data(), name.size(), target, 
                unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc())
            {
                JSONCONS_THROW(ser_error(result.ec,context.line(),context.column()));
            }
            other_visitor_.key(target, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                value.data(), value.size(), target, 
                unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc())
            {
                ec = result.ec;
                JSONCONS_VISITOR_RETURN;
            }
            other_visitor_.string_value(target, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec) override
        {
            other_visitor_.int64_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec) override
        {
            other_visitor_.uint64_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) override
        {
            other_visitor_.half_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) override
        {
            other_visitor_.double_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.bool_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            other_visitor_.null_value(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }
    };

    template <typename CharT,typename Source=jsoncons::stream_source<CharT>,typename TempAlloc =std::allocator<char>>
    class basic_json_reader 
    {
    public:
        using char_type = CharT;
        using source_type = Source;
        using string_view_type = jsoncons::basic_string_view<CharT>;
    private:
        using char_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<CharT>;

        static constexpr size_t default_max_buffer_size = 16384;

        json_source_adaptor<Source> source_;
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT>& visitor_;
        basic_json_parser<CharT,TempAlloc> parser_;

        // Noncopyable and nonmoveable
        basic_json_reader(const basic_json_reader&) = delete;
        basic_json_reader& operator=(const basic_json_reader&) = delete;

    public:

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
            const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>{}, 
            const TempAlloc& temp_alloc = TempAlloc())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                options,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
            basic_json_visitor<CharT>& visitor, 
            const TempAlloc& temp_alloc = TempAlloc())
            : basic_json_reader(std::forward<Sourceable>(source),
                                visitor,
                                basic_json_decode_options<CharT>(),
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
            basic_json_visitor<CharT>& visitor,
            const basic_json_decode_options<CharT>& options, 
            const TempAlloc& temp_alloc = TempAlloc())
        : source_(std::forward<Sourceable>(source)),
          visitor_(visitor),
          parser_(options, temp_alloc)
        {
        }

#if !defined(JSONCONS_NO_DEPRECATED)
        template <typename Sourceable>
        basic_json_reader(Sourceable&& source, 
            const basic_json_decode_options<CharT>& options,
            std::function<bool(json_errc,const ser_context&)> err_handler, 
            const TempAlloc& temp_alloc = TempAlloc())
            : basic_json_reader(std::forward<Sourceable>(source),
                                default_visitor_,
                                options,
                                err_handler,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source,
                          basic_json_visitor<CharT>& visitor,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAlloc& temp_alloc = TempAlloc())
            : basic_json_reader(std::forward<Sourceable>(source),
                                visitor,
                                basic_json_decode_options<CharT>(),
                                err_handler,
                                temp_alloc)
        {
        }

        template <typename Sourceable>
        basic_json_reader(Sourceable&& source,
                          basic_json_visitor<CharT>& visitor, 
                          const basic_json_decode_options<CharT>& options,
                          std::function<bool(json_errc,const ser_context&)> err_handler, 
                          const TempAlloc& temp_alloc = TempAlloc())
           : source_(std::forward<Sourceable>(source)),
             visitor_(visitor),
             parser_(options,err_handler,temp_alloc)
        {
        }
#endif
        void read_next()
        {
            std::error_code ec;
            read_next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec,parser_.line(),parser_.column()));
            }
        }

        void read_next(std::error_code& ec)
        {
            if (source_.is_error())
            {
                ec = json_errc::source_error;
                return;
            }        
            parser_.reset();
            while (!parser_.stopped())
            {
                if (parser_.source_exhausted())
                {
                    auto s = source_.read_buffer(ec);
                    if (JSONCONS_UNLIKELY(ec)) return;
                    if (s.size() > 0)
                    {
                        parser_.update(s.data(),s.size());
                    }
                }
                bool eof = parser_.source_exhausted();
                parser_.parse_some(visitor_, ec);
                if (JSONCONS_UNLIKELY(ec)) return;
                if (eof)
                {
                    if (parser_.enter())
                    {
                        break;
                    }
                    else if (!parser_.accept())
                    {
                        ec = json_errc::unexpected_eof;
                        return;
                    }
                }
            }
            
            parser_.skip_whitespace();
            while (!source_.eof())
            {
                parser_.skip_whitespace();
                if (parser_.source_exhausted())
                {
                    auto s = source_.read_buffer(ec);
                    if (JSONCONS_UNLIKELY(ec)) return;
                    if (s.size() > 0)
                    {
                        parser_.update(s.data(),s.size());
                    }
                }
                else
                {
                    break;
                }
            }
        }

        void check_done()
        {
            std::error_code ec;
            check_done(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec,parser_.line(),parser_.column()));
            }
        }

        std::size_t line() const
        {
            return parser_.line();
        }

        std::size_t column() const
        {
            return parser_.column();
        }

        void check_done(std::error_code& ec)
        {
            if (source_.is_error())
            {
                ec = json_errc::source_error;
                return;
            }   
            if (source_.eof())
            {
                parser_.check_done(ec);
                if (JSONCONS_UNLIKELY(ec)) return;
            }
            else
            {
                do
                {
                    if (parser_.source_exhausted())
                    {
                        auto s = source_.read_buffer(ec);
                        if (JSONCONS_UNLIKELY(ec)) return;
                        if (s.size() > 0)
                        {
                            parser_.update(s.data(),s.size());
                        }
                    }
                    if (!parser_.source_exhausted())
                    {
                        parser_.check_done(ec);
                        if (JSONCONS_UNLIKELY(ec)) return;
                    }
                }
                while (!eof());
            }
        }

        bool eof() const
        {
            return parser_.source_exhausted() && source_.eof();
        }

        void read()
        {
            read_next();
            check_done();
        }

        void read(std::error_code& ec)
        {
            read_next(ec);
            if (!ec)
            {
                check_done(ec);
            }
        }
    };

    using json_string_reader = basic_json_reader<char,string_source<char>>;
    using wjson_string_reader = basic_json_reader<wchar_t,string_source<wchar_t>>;
    using json_stream_reader = basic_json_reader<char,stream_source<char>>;
    using wjson_stream_reader = basic_json_reader<wchar_t,stream_source<wchar_t>>;

} // namespace jsoncons

#endif // JSONCONS_JSON_READER_HPP

