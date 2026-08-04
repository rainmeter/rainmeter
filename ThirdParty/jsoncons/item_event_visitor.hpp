// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_ITEM_EVENT_VISITOR_HPP
#define JSONCONS_ITEM_EVENT_VISITOR_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/sink.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons { 

    template <typename CharT,typename Allocator = std::allocator<char>>
    class basic_item_event_visitor_to_json_visitor;

    template <typename CharT>
    class basic_item_event_visitor 
    {
        template <typename Ch,typename Allocator>
        friend class basic_item_event_visitor_to_json_visitor;
    public:
        using char_type = CharT;
        using char_traits_type = std::char_traits<char_type>;

        using string_view_type = jsoncons::basic_string_view<char_type,char_traits_type>;

        basic_item_event_visitor(basic_item_event_visitor&&) = default;

        basic_item_event_visitor& operator=(basic_item_event_visitor&&) = default;

        basic_item_event_visitor() = default;

        virtual ~basic_item_event_visitor() = default;

        void flush()
        {
            visit_flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_object(semantic_tag tag=semantic_tag::none,
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_begin_object(tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_object(std::size_t length, 
            semantic_tag tag=semantic_tag::none, 
            const ser_context& context = ser_context())
        {
            std::error_code ec;
            visit_begin_object(length, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_object(const ser_context& context = ser_context())
        {
            std::error_code ec;
            visit_end_object(context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_array(semantic_tag tag=semantic_tag::none,
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_begin_array(tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_array(std::size_t length, 
            semantic_tag tag=semantic_tag::none,
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_begin_array(length, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_array(const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_end_array(context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE key(const string_view_type& name, const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_string(name, semantic_tag::none, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE null_value(semantic_tag tag = semantic_tag::none,
            const ser_context& context=ser_context()) 
        {
            std::error_code ec;
            visit_null(tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE bool_value(bool value, 
            semantic_tag tag = semantic_tag::none,
            const ser_context& context=ser_context()) 
        {
            std::error_code ec;
            visit_bool(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE string_value(const string_view_type& value, 
            semantic_tag tag = semantic_tag::none, 
            const ser_context& context=ser_context()) 
        {
            std::error_code ec;
            visit_string(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        template <typename BytesViewLike>
        JSONCONS_VISITOR_RETURN_TYPE byte_string_value(const BytesViewLike& b, 
            semantic_tag tag=semantic_tag::none, 
            const ser_context& context=ser_context(),
            typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            std::error_code ec;
            visit_byte_string(byte_string_view(reinterpret_cast<const uint8_t*>(b.data()),b.size()), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        template <typename BytesViewLike>
        JSONCONS_VISITOR_RETURN_TYPE byte_string_value(const BytesViewLike& b, 
            uint64_t ext_tag, 
            const ser_context& context=ser_context(),
            typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            std::error_code ec;
            visit_byte_string(byte_string_view(reinterpret_cast<const uint8_t*>(b.data()),b.size()), ext_tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE uint64_value(uint64_t value, 
            semantic_tag tag = semantic_tag::none, 
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_uint64(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE int64_value(int64_t value, 
            semantic_tag tag = semantic_tag::none, 
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_int64(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE half_value(uint16_t value, 
            semantic_tag tag = semantic_tag::none, 
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_half(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE double_value(double value, 
            semantic_tag tag = semantic_tag::none, 
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_double(value, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_object(semantic_tag tag,
            const ser_context& context,
            std::error_code& ec)
        {
            visit_begin_object(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_object(std::size_t length, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec)
        {
            visit_begin_object(length, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_object(const ser_context& context, std::error_code& ec)
        {
            visit_end_object(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_array(semantic_tag tag, const ser_context& context, std::error_code& ec)
        {
            visit_begin_array(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_array(std::size_t length, semantic_tag tag, const ser_context& context, std::error_code& ec)
        {
            visit_begin_array(length, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_array(const ser_context& context, std::error_code& ec)
        {
            visit_end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE key(const string_view_type& name, const ser_context& context, std::error_code& ec)
        {
            visit_string(name, semantic_tag::none, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE null_value(semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) 
        {
            visit_null(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE bool_value(bool value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) 
        {
            visit_bool(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE string_value(const string_view_type& value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec) 
        {
            visit_string(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        template <typename Source>
        JSONCONS_VISITOR_RETURN_TYPE byte_string_value(const Source& b, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec,
            typename std::enable_if<ext_traits::is_bytes_view_like<Source>::value,int>::type = 0)
        {
            visit_byte_string(byte_string_view(reinterpret_cast<const uint8_t*>(b.data()),b.size()), tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        template <typename Source>
        JSONCONS_VISITOR_RETURN_TYPE byte_string_value(const Source& b, 
            uint64_t ext_tag, 
            const ser_context& context,
            std::error_code& ec,
            typename std::enable_if<ext_traits::is_bytes_view_like<Source>::value,int>::type = 0)
        {
            visit_byte_string(byte_string_view(reinterpret_cast<const uint8_t*>(b.data()),b.size()), ext_tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE uint64_value(uint64_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec)
        {
            visit_uint64(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE int64_value(int64_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec)
        {
            visit_int64(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE half_value(uint16_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec)
        {
            visit_half(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE double_value(double value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec)
        {
            visit_double(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        template <typename T>
        JSONCONS_VISITOR_RETURN_TYPE typed_array(const jsoncons::span<T>& data, 
            semantic_tag tag=semantic_tag::none,
            const ser_context& context=ser_context())
        {
            std::error_code ec;
            visit_typed_array(data, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        template <typename T>
        JSONCONS_VISITOR_RETURN_TYPE typed_array(const jsoncons::span<T>& data, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec)
        {
            visit_typed_array(data, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE typed_array(half_arg_t, const jsoncons::span<const uint16_t>& s,
            semantic_tag tag = semantic_tag::none,
            const ser_context& context = ser_context())
        {
            std::error_code ec;
            visit_typed_array(half_arg, s, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE typed_array(half_arg_t, const jsoncons::span<const uint16_t>& s,
                         semantic_tag tag,
                         const ser_context& context,
                         std::error_code& ec)
        {
            visit_typed_array(half_arg, s, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_multi_dim(const jsoncons::span<const size_t>& shape,
                             semantic_tag tag = semantic_tag::multi_dim_row_major,
                             const ser_context& context=ser_context()) 
        {
            std::error_code ec;
            visit_begin_multi_dim(shape, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE begin_multi_dim(const jsoncons::span<const size_t>& shape,
                             semantic_tag tag,
                             const ser_context& context,
                             std::error_code& ec) 
        {
            visit_begin_multi_dim(shape, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_multi_dim(const ser_context& context=ser_context()) 
        {
            std::error_code ec;
            visit_end_multi_dim(context, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, context.line(), context.column()));
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE end_multi_dim(const ser_context& context,
                           std::error_code& ec) 
        {
            visit_end_multi_dim(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

    private:

        virtual void visit_flush() = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t /*length*/, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec)
        {
            visit_begin_object(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t /*length*/, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec)
        {
            visit_begin_array(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code&) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& value, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& value, 
            uint64_t /*ext_tag*/, 
            const ser_context& context,
            std::error_code& ec) 
        {
            visit_byte_string(value, semantic_tag::none, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
            semantic_tag tag, 
            const ser_context& context,
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec)
        {
            visit_double(binary::decode_half(value),
                tag,
                context,
                ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) = 0;

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint8_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                uint64_value(*p, semantic_tag::none, context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint16_t>& s, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                uint64_value(*p, semantic_tag::none, context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint32_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) 
        {
            begin_array(s.size(), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                uint64_value(*p, semantic_tag::none, context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint64_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) 
        {
            begin_array(s.size(), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                uint64_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int8_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                int64_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int16_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                int64_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int32_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                int64_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int64_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                int64_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(half_arg_t, 
            const jsoncons::span<const uint16_t>& s, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                half_value(*p, semantic_tag::none, context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const float>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                double_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const double>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec)  
        {
            begin_array(s.size(), tag,context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            for (auto p = s.begin(); p != s.end(); ++p)
            {
                double_value(*p,semantic_tag::none,context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_begin_multi_dim(const jsoncons::span<const size_t>& shape,
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) 
        {
            visit_begin_array(2, tag, context, ec);
            if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            visit_begin_array(shape.size(), tag, context, ec);
            for (auto it = shape.begin(); it != shape.end(); ++it)
            {
                visit_uint64(*it, semantic_tag::none, context, ec);
                if (JSONCONS_UNLIKELY(ec)) {JSONCONS_VISITOR_RETURN;}
            }
            visit_end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        virtual JSONCONS_VISITOR_RETURN_TYPE visit_end_multi_dim(const ser_context& context,
                                      std::error_code& ec) 
        {
            visit_end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }
    };

 template <typename CharT,typename Allocator>
    class basic_item_event_visitor_to_json_visitor : public basic_item_event_visitor<CharT>
    {
    public:
        using typename basic_item_event_visitor<CharT>::char_type;
        using typename basic_item_event_visitor<CharT>::string_view_type;
    private:
        using char_allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<char_type>;

        using string_type = std::basic_string<char_type,std::char_traits<char_type>,char_allocator_type>;

        enum class container_t {root, array, object};
        enum class target_t {destination, buffer};

        struct level
        {
        private:
            target_t state_;
            container_t type_;
            int even_odd_;
            std::size_t count_{0};
        public:

            level(target_t state, container_t type) noexcept
                : state_(state), type_(type), even_odd_(type == container_t::object ? 0 : 1)
            {
            }

            void advance()
            {
                if (!is_key())
                {
                    ++count_;
                }
                if (is_object())
                {
                    even_odd_ = !even_odd_;
                }
            }

            bool is_key() const
            {
                return even_odd_ == 0;
            }

            bool is_object() const
            {
                return type_ == container_t::object;
            }

            target_t target() const
            {
                return state_;
            }

            std::size_t count() const
            {
                return count_;
            }
        };
        using level_allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<level>;

        basic_default_json_visitor<char_type> default_visitor_;
        basic_json_visitor<char_type>* destination_;
        string_type key_;
        string_type key_buffer_;
        std::vector<level,level_allocator_type> level_stack_;

        const std::basic_string<char> null_constant = {'n','u','l','l'};
        const std::basic_string<char> true_constant = { 't','r','u','e' };
        const std::basic_string<char> false_constant = { 'f', 'a', 'l', 's', 'e' };

        // noncopyable and nonmoveable
        basic_item_event_visitor_to_json_visitor(const basic_item_event_visitor_to_json_visitor&) = delete;
        basic_item_event_visitor_to_json_visitor& operator=(const basic_item_event_visitor_to_json_visitor&) = delete;
    public:
        explicit basic_item_event_visitor_to_json_visitor(const Allocator& alloc = Allocator())
            : default_visitor_(), destination_(std::addressof(default_visitor_)),
              key_(alloc), key_buffer_(alloc), level_stack_(alloc)
        {
            level_stack_.emplace_back(target_t::destination,container_t::root); // root
        }

        explicit basic_item_event_visitor_to_json_visitor(basic_json_visitor<char_type>& visitor, 
                                                     const Allocator& alloc = Allocator())
            : destination_(std::addressof(visitor)), 
              key_(alloc), key_buffer_(alloc), level_stack_(alloc)
        {
            level_stack_.emplace_back(target_t::destination,container_t::root); // root
        }

        void reset()
        {
            key_.clear();
            key_buffer_.clear();
            level_stack_.clear();
            level_stack_.emplace_back(target_t::destination,container_t::root); // root
        }

        basic_json_visitor<char_type>& destination()
        {
            return *destination_;
        }

        void destination(basic_json_visitor<char_type>& dest)
        {
            destination_ = std::addressof(dest);
        }

    private:
        void visit_flush() override
        {
            destination_->flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key())
            {
                if (level_stack_.back().target() == target_t::buffer && level_stack_.back().count() > 0)
                {
                    key_buffer_.push_back(',');
                }
                level_stack_.emplace_back(target_t::buffer, container_t::object);
                key_buffer_.push_back('{');
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        level_stack_.emplace_back(target_t::buffer, container_t::object);
                        key_buffer_.push_back('{');
                        break;
                    default:
                        level_stack_.emplace_back(target_t::destination, container_t::object);
                        destination_->begin_object(tag, context, ec);
                        break;
                }
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t length, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key())
            {
                if (level_stack_.back().target() == target_t::buffer && level_stack_.back().count() > 0)
                {
                    key_buffer_.push_back(',');
                }
                level_stack_.emplace_back(target_t::buffer, container_t::object);
                key_buffer_.push_back('{');
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        level_stack_.emplace_back(target_t::buffer, container_t::object);
                        key_buffer_.push_back('{');
                        break;
                    default:
                        level_stack_.emplace_back(target_t::destination, container_t::object);
                        destination_->begin_object(length, tag, context, ec);
                        break;
                }
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context& context, std::error_code& ec) override
        {
            switch (level_stack_.back().target())
            {
                case target_t::buffer:
                    key_buffer_.push_back('}');
                    JSONCONS_ASSERT(level_stack_.size() > 1);
                    level_stack_.pop_back();
                    
                    if (level_stack_.back().target() == target_t::destination)
                    {
                        destination_->key(key_buffer_,context, ec);
                        key_buffer_.clear();
                    }
                    else if (level_stack_.back().is_key())
                    {
                        key_buffer_.push_back(':');
                    }
                    level_stack_.back().advance();
                    break;
                default:
                    JSONCONS_ASSERT(level_stack_.size() > 1);
                    level_stack_.pop_back();
                    level_stack_.back().advance();
                    destination_->end_object(context, ec);
                    break;
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key())
            {
                if (level_stack_.back().target() == target_t::buffer && level_stack_.back().count() > 0)
                {
                    key_buffer_.push_back(',');
                }
                level_stack_.emplace_back(target_t::buffer, container_t::array);
                key_buffer_.push_back('[');
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        level_stack_.emplace_back(target_t::buffer, container_t::array);
                        key_buffer_.push_back('[');
                        break;
                    default:
                        level_stack_.emplace_back(target_t::destination, container_t::array);
                        destination_->begin_array(tag, context, ec);
                        break;
                }
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t length, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key())
            {
                if (level_stack_.back().target() == target_t::buffer && level_stack_.back().count() > 0)
                {
                    key_buffer_.push_back(',');
                }
                level_stack_.emplace_back(target_t::buffer, container_t::array);
                key_buffer_.push_back('[');
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        level_stack_.emplace_back(target_t::buffer, container_t::array);
                        key_buffer_.push_back('[');
                        break;
                    default:
                        level_stack_.emplace_back(target_t::destination, container_t::array);
                        destination_->begin_array(length, tag, context, ec);
                        break;
                }
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context& context, std::error_code& ec) override
        {
            switch (level_stack_.back().target())
            {
                case target_t::buffer:
                    key_buffer_.push_back(']');
                    JSONCONS_ASSERT(level_stack_.size() > 1);
                    level_stack_.pop_back();
                    if (level_stack_.back().target() == target_t::destination)
                    {
                        destination_->key(key_buffer_, context, ec);
                        key_buffer_.clear();
                    }
                    else if (level_stack_.back().is_key())
                    {
                        key_buffer_.push_back(':');
                    }
                    level_stack_.back().advance();
                    break;
                default:
                    JSONCONS_ASSERT(level_stack_.size() > 1);
                    level_stack_.pop_back();
                    level_stack_.back().advance();
                    destination_->end_array(context, ec);
                    break;
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& value,
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) override
        {
            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), value.begin(), value.end());
                        key_buffer_.push_back('\"');
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(value, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), value.begin(), value.end());
                        key_buffer_.push_back('\"');
                        break;
                    default:
                        destination_->string_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& value, 
            semantic_tag tag,
            const ser_context& context,
            std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                switch (tag)
                {
                    case semantic_tag::base64:
                        bytes_to_base64(value.begin(), value.end(), key_);
                        break;
                    case semantic_tag::base16:
                        bytes_to_base16(value.begin(), value.end(),key_);
                        break;
                    default:
                        bytes_to_base64url(value.begin(), value.end(),key_);
                        break;
                }
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back('\"');
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back('\"');
                        break;
                    default:
                        destination_->byte_string_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& value, 
            uint64_t ext_tag,
            const ser_context& context,
            std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                bytes_to_base64url(value.begin(), value.end(),key_);
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back('\"');
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.push_back('\"');
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back('\"');
                        break;
                    default:
                        destination_->byte_string_value(value, ext_tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                jsoncons::from_integer(value,key_);
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->uint64_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                jsoncons::from_integer(value,key_);
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->int64_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                jsoncons::string_sink<string_type> sink(key_);
                jsoncons::write_double f{float_chars_format::general,0};
                double x = binary::decode_half(value);
                f(x, sink);
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->half_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear();
                string_sink<string_type> sink(key_);
                jsoncons::write_double f{float_chars_format::general,0};
                f(value, sink);
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->double_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear(); 
                if (value)
                {
                    key_.insert(key_.begin(), true_constant.begin(), true_constant.end());
                }
                else
                {
                    key_.insert(key_.begin(), false_constant.begin(), false_constant.end());
                }
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->bool_value(value, tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            if (level_stack_.back().is_key() || level_stack_.back().target() == target_t::buffer)
            {
                key_.clear(); 
                key_.insert(key_.begin(), null_constant.begin(), null_constant.end());
            }

            if (level_stack_.back().is_key())
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        key_buffer_.push_back(':');
                        break;
                    default:
                        destination_->key(key_, context, ec);
                        break;
                }
            }
            else
            {
                switch (level_stack_.back().target())
                {
                    case target_t::buffer:
                        if (!level_stack_.back().is_object() && level_stack_.back().count() > 0)
                        {
                            key_buffer_.push_back(',');
                        }
                        key_buffer_.insert(key_buffer_.end(), key_.begin(), key_.end());
                        break;
                    default:
                        destination_->null_value(tag, context, ec);
                        break;
                }
            }

            level_stack_.back().advance();
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint8_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override 
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint16_t>& s, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint32_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override 
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint64_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override 
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int8_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int16_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int32_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int64_t>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(half_arg_t, 
            const jsoncons::span<const uint16_t>& s, 
            semantic_tag tag, 
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(half_arg,s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(half_arg, s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const float>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const double>& s, 
            semantic_tag tag,
            const ser_context& context, 
            std::error_code& ec) override  
        {
            bool is_key = level_stack_.back().is_key();
            level_stack_.back().advance();

            if (is_key || level_stack_.back().target() == target_t::buffer)
            {
                basic_item_event_visitor<CharT>::visit_typed_array(s,tag,context,ec);
            }
            else
            {
                destination_->typed_array(s, tag, context, ec);
            }
            JSONCONS_VISITOR_RETURN;
        }
    };

    template <typename CharT>
    class basic_default_item_event_visitor : public basic_item_event_visitor<CharT>
    {
    public:
        using typename basic_item_event_visitor<CharT>::string_view_type;

        basic_default_item_event_visitor() = default;
    private:
        void visit_flush() override
        {
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type&, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view&, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool, semantic_tag, const ser_context&, std::error_code&) override
        {
            JSONCONS_VISITOR_RETURN;
        }
    };

    // basic_json_visitor_to_item_event_visitor

    template <typename CharT>
    class basic_json_visitor_to_item_event_visitor : public basic_json_visitor<CharT>
    {
    public:
        using typename basic_json_visitor<CharT>::char_type;
        using typename basic_json_visitor<CharT>::string_view_type;
    private:
        basic_item_event_visitor<char_type>& destination_;

        // noncopyable and nonmoveable
        basic_json_visitor_to_item_event_visitor(const basic_json_visitor_to_item_event_visitor&) = delete;
        basic_json_visitor_to_item_event_visitor& operator=(const basic_json_visitor_to_item_event_visitor&) = delete;
    public:
        basic_json_visitor_to_item_event_visitor(basic_item_event_visitor<char_type>& visitor)
            : destination_(visitor)
        {
        }

        basic_item_event_visitor<char_type>& destination()
        {
            destination_;
        }

    private:
        void visit_flush() override
        {
            destination_.flush();
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.begin_object(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t length, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.begin_object(length, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context& context, std::error_code& ec) override
        {
            destination_.end_object(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.begin_array(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t length, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.begin_array(length, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context& context, std::error_code& ec) override
        {
            destination_.end_array(context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name,
                       const ser_context& context,
                       std::error_code& ec) override
        {
            destination_.visit_string(name, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& value,
                          semantic_tag tag,
                          const ser_context& context,
                          std::error_code& ec) override
        {
            destination_.string_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& b, 
                               semantic_tag tag,
                               const ser_context& context,
                               std::error_code& ec) override
        {
            destination_.byte_string_value(b, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.uint64_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.int64_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.half_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.double_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.bool_value(value, tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, const ser_context& context, std::error_code& ec) override
        {
            destination_.null_value(tag, context, ec);
            JSONCONS_VISITOR_RETURN;
        }
    };

    using item_event_visitor = basic_item_event_visitor<char>;
    using default_item_event_visitor = basic_default_item_event_visitor<char>;
    using item_event_visitor_to_visitor_adaptor = basic_item_event_visitor_to_json_visitor<char>;

} // namespace jsoncons

#endif // JSONCONS_ITEM_EVENT_VISITOR_HPP
