// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_STAJ_CURSOR_HPP
#define JSONCONS_STAJ_CURSOR_HPP

#include <array> // std::array
#include <cstddef>
#include <cstdint>
#include <functional> // std::function
#include <ios>
#include <memory> // std::allocator
#include <system_error>

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/allocator_set.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/sink.hpp>
#include <jsoncons/staj_event.hpp>
#include <jsoncons/typed_array_view.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/utility/conversion.hpp>

namespace jsoncons {

// basic_staj_visitor

enum class staj_cursor_state
{
    typed_array = 1,
    multi_dim,
    shape
};

template <typename CharT>
class basic_staj_visitor : public basic_json_visitor<CharT>
{
    using super_type = basic_json_visitor<CharT>;
public:
    using char_type = CharT;
    using typename super_type::string_view_type;
private:
    basic_staj_event<CharT> event_;

    staj_cursor_state state_;
    typed_array_view data_;
    jsoncons::span<const size_t> shape_;
    std::size_t index_{0};
public:
    basic_staj_visitor()
        : event_(staj_events::null_value),
          state_(), data_(), shape_()
    {
    }
    
    ~basic_staj_visitor() = default;

    void reset()
    {
        event_ = staj_events::null_value;
        state_ = {};
        data_ = {};
        shape_ = {};
        index_ = 0;
    }

    const basic_staj_event<CharT>& event() const
    {
        return event_;
    }

    bool in_available() const
    {
        return state_ != staj_cursor_state();
    }

    void send_available(std::error_code& ec)
    {
        switch (state_)
        {
            case staj_cursor_state::typed_array:
                advance_typed_array(ec);
                break;
            case staj_cursor_state::multi_dim:
            case staj_cursor_state::shape:
                advance_multi_dim(ec);
                break;
            default:
                break;
        }
    }

    bool is_typed_array() const
    {
        return data_.type() != typed_array_type();
    }

    staj_cursor_state state() const
    {
        return state_;
    }

    void advance_typed_array(std::error_code& ec)
    {
        if (is_typed_array())
        {
            if (index_ < data_.size())
            {
                switch (data_.type())
                {
                    case typed_array_type::uint8_value:
                    {
                        this->uint64_value(data_.data(uint8_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::uint16_value:
                    {
                        this->uint64_value(data_.data(uint16_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::uint32_value:
                    {
                        this->uint64_value(data_.data(uint32_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::uint64_value:
                    {
                        this->uint64_value(data_.data(uint64_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::int8_value:
                    {
                        this->int64_value(data_.data(int8_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::int16_value:
                    {
                        this->int64_value(data_.data(int16_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::int32_value:
                    {
                        this->int64_value(data_.data(int32_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::int64_value:
                    {
                        this->int64_value(data_.data(int64_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::half_value:
                    {
                        this->half_value(data_.data(half_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::float_value:
                    {
                        this->double_value(data_.data(float_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    case typed_array_type::double_value:
                    {
                        this->double_value(data_.data(double_array_arg)[index_], semantic_tag::none, ser_context(), ec);
                        break;
                    }
                    default:
                        break;
                }
                ++index_;
            }
            else
            {
                this->end_array();
                state_ = staj_cursor_state();
                data_ = typed_array_view();
                index_ = 0;
            }
        }
    }

    void advance_multi_dim(std::error_code& ec)
    {
        if (shape_.size() != 0)
        {
            if (state_ == staj_cursor_state::multi_dim)
            {
                this->begin_array(shape_.size(), semantic_tag::none, ser_context(), ec);
                state_ = staj_cursor_state::shape;
            }
            else if (index_ < shape_.size())
            {
                this->uint64_value(shape_[index_], semantic_tag::none, ser_context(), ec);
                ++index_;
            }
            else
            {
                state_ = staj_cursor_state();
                this->end_array(ser_context(), ec);
                shape_ = jsoncons::span<const size_t>();
                index_ = 0;
            }
        }
    }

    void dump(basic_json_visitor<CharT>& visitor, const ser_context& context, std::error_code& ec)
    {
        if (is_typed_array())
        {
            if (index_ != 0)
            {
                event().send_json_event(visitor, context, ec);
                const std::size_t len = data_.size();
                switch (data_.type())
                {
                    case typed_array_type::uint8_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.uint64_value(data_.data(uint8_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::uint16_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.uint64_value(data_.data(uint16_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::uint32_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.uint64_value(data_.data(uint32_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::uint64_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.uint64_value(data_.data(uint64_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::int8_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.int64_value(data_.data(int8_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::int16_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.int64_value(data_.data(int16_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::int32_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.int64_value(data_.data(int32_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::int64_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.int64_value(data_.data(int64_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::float_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.double_value(data_.data(float_array_arg)[i]);
                        }
                        break;
                    }
                    case typed_array_type::double_value:
                    {
                        for (auto i = index_; i < len; ++i) 
                        {
                            visitor.double_value(data_.data(double_array_arg)[i]);
                        }
                        break;
                    }
                    default:
                        break;
                }
                
                state_ = staj_cursor_state();
                data_ = typed_array_view();
                index_ = 0;
            }
            else
            {
                switch (data_.type())
                {
                    case typed_array_type::uint8_value:
                    {
                        visitor.typed_array(data_.data(uint8_array_arg));
                        break;
                    }
                    case typed_array_type::uint16_value:
                    {
                        visitor.typed_array(data_.data(uint16_array_arg));
                        break;
                    }
                    case typed_array_type::uint32_value:
                    {
                        visitor.typed_array(data_.data(uint32_array_arg));
                        break;
                    }
                    case typed_array_type::uint64_value:
                    {
                        visitor.typed_array(data_.data(uint64_array_arg));
                        break;
                    }
                    case typed_array_type::int8_value:
                    {
                        visitor.typed_array(data_.data(int8_array_arg));
                        break;
                    }
                    case typed_array_type::int16_value:
                    {
                        visitor.typed_array(data_.data(int16_array_arg));
                        break;
                    }
                    case typed_array_type::int32_value:
                    {
                        visitor.typed_array(data_.data(int32_array_arg));
                        break;
                    }
                    case typed_array_type::int64_value:
                    {
                        visitor.typed_array(data_.data(int64_array_arg));
                        break;
                    }
                    case typed_array_type::float_value:
                    {
                        visitor.typed_array(data_.data(float_array_arg));
                        break;
                    }
                    case typed_array_type::double_value:
                    {
                        visitor.typed_array(data_.data(double_array_arg));
                        break;
                    }
                    default:
                        break;
                }

                state_ = staj_cursor_state();
                data_ = typed_array_view();
            }
        }
        else
        {
            event().send_json_event(visitor, context, ec);
        }
    }

private:
    static constexpr bool accept(const basic_staj_event<CharT>&, const ser_context&) 
    {
        return true;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::begin_object, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t length, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::begin_object, length, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::end_object);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::begin_array, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t length, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::begin_array, length, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::end_array);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(name, staj_events::key);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(staj_events::null_value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& s, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(s, staj_events::string_value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& s, 
        semantic_tag tag,
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(s, staj_events::byte_string_value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& s, 
        uint64_t ext_tag,
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(s, staj_events::byte_string_value, ext_tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
        semantic_tag tag, 
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
        semantic_tag tag,
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, 
        semantic_tag tag,
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(half_arg, value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
        semantic_tag tag, 
        const ser_context&,
        std::error_code&) override
    {
        event_ = basic_staj_event<CharT>(value, tag);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint8_t>& v, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(v.data(), v.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint16_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint32_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint64_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int8_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int16_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int32_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const int64_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(half_arg_t, const jsoncons::span<const uint16_t>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const float>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const double>& data, 
        semantic_tag tag,
        const ser_context& context,
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::typed_array;
        data_ = typed_array_view(data.data(), data.size());
        index_ = 0;
        this->begin_array(tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }
/*
    JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const float128_type>&, 
        semantic_tag,
        const ser_context&,
        std::error_code&) override
    {
        JSONCONS_VISITOR_RETURN;
    }
*/
    JSONCONS_VISITOR_RETURN_TYPE visit_begin_multi_dim(const jsoncons::span<const size_t>& shape,
        semantic_tag tag,
        const ser_context& context, 
        std::error_code& ec) override
    {
        state_ = staj_cursor_state::multi_dim;
        shape_ = shape;
        this->begin_array(2, tag, context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_end_multi_dim(const ser_context& context,
        std::error_code& ec) override
    {
        this->end_array(context, ec);
        JSONCONS_VISITOR_RETURN;
    }

    void visit_flush() override
    {
    }
};


// basic_staj_cursor

template <typename CharT>
class basic_staj_cursor
{
public:
    virtual ~basic_staj_cursor() = default;

    virtual void array_expected(std::error_code& ec)
    {
        if (!(current().event_type() == staj_events::begin_array || current().event_type() == staj_events::byte_string_value))
        {
            ec = conv_errc::not_vector;
        }
    }

    virtual bool done() const = 0;

    virtual const basic_staj_event<CharT>& current() const = 0;

    virtual void read_to(basic_json_visitor<CharT>& visitor) = 0;

    virtual void read_to(basic_json_visitor<CharT>& visitor,
                         std::error_code& ec) = 0;

    virtual void next() = 0;

    virtual void next(std::error_code& ec) = 0;

    virtual const ser_context& context() const = 0;
    
    virtual std::size_t line() const = 0;

    virtual std::size_t column() const = 0;
    
};

template <typename CharT>
class basic_staj_filter_view : basic_staj_cursor<CharT>
{
    basic_staj_cursor<CharT>* cursor_;
    std::function<bool(const basic_staj_event<CharT>&, const ser_context&)> pred_;
public:
    basic_staj_filter_view(basic_staj_cursor<CharT>& cursor, 
                     std::function<bool(const basic_staj_event<CharT>&, const ser_context&)> pred)
        : cursor_(std::addressof(cursor)), pred_(pred)
    {
        while (!done() && !pred_(current(),context()))
        {
            cursor_->next();
        }
    }

    bool done() const override
    {
        return cursor_->done();
    }

    const basic_staj_event<CharT>& current() const override
    {
        return cursor_->current();
    }

    void read_to(basic_json_visitor<CharT>& visitor) override
    {
        cursor_->read_to(visitor);
    }

    void read_to(basic_json_visitor<CharT>& visitor,
                 std::error_code& ec) override
    {
        cursor_->read_to(visitor, ec);
    }

    void next() override
    {
        cursor_->next();
        while (!done() && !pred_(current(),context()))
        {
            cursor_->next();
        }
    }

    void next(std::error_code& ec) override
    {
        cursor_->next(ec);
        while (!done() && !pred_(current(),context()) && !ec)
        {
            cursor_->next(ec);
        }
    }

    const ser_context& context() const override
    {
        return cursor_->context();
    }

    std::size_t line() const override
    {
        return cursor_->line();
    }

    std::size_t column() const override
    {
        return cursor_->column();
    }

    friend
    basic_staj_filter_view<CharT> operator|(basic_staj_filter_view& cursor, 
                                      std::function<bool(const basic_staj_event<CharT>&, const ser_context&)> pred)
    {
        return basic_staj_filter_view<CharT>(cursor, pred);
    }
};

template <typename Json,typename Alloc,typename TempAlloc>
read_result<Json> to_json_single(const allocator_set<Alloc,TempAlloc>& aset, 
    basic_staj_cursor<typename Json::char_type>& cursor)
{
    using result_type = read_result<Json>;

    std::error_code ec;
    switch (cursor.current().event_type())
    {
        case staj_events::string_value:
        {
            auto sv = cursor.current().template get<jsoncons::basic_string_view<typename Json::char_type>>(ec);
            if (ec) return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
            return result_type{jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(),
                sv.data(), sv.length(), cursor.current().tag())};
        }
        case staj_events::byte_string_value:
        {
            auto j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), 
                byte_string_arg, cursor.current().template get<byte_string_view>(ec), cursor.current().tag());
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        case staj_events::null_value:
        {
            return result_type(Json{null_arg, semantic_tag::none});
        }
        case staj_events::bool_value:
        {
            auto j = Json{cursor.current().template get<bool>(ec), cursor.current().tag()};
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        case staj_events::int64_value:
        {
            auto j = Json{cursor.current().template get<std::int64_t>(ec), cursor.current().tag()};
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        case staj_events::uint64_value:
        {
            auto j = Json{cursor.current().template get<std::uint64_t>(ec), cursor.current().tag()};
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        case staj_events::half_value:
        {
            auto j = Json{half_arg, cursor.current().template get<std::uint16_t>(ec), cursor.current().tag()};
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        case staj_events::double_value:
        {
            auto j = Json{cursor.current().template get<double>(ec), cursor.current().tag()};
            return !ec ? result_type(std::move(j)) : result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        default:
            return result_type(jsoncons::unexpect, conv_errc::conversion_failed, cursor.line(), cursor.column());
    }
}

template <typename Json, typename Alloc, typename TempAlloc>
read_result<Json> to_json_container(const allocator_set<Alloc,TempAlloc>& aset, 
    basic_staj_cursor<typename Json::char_type>& cursor)
{
    using result_type = read_result<Json>;
    using json_ptr_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<Json*>;
    using char_type = typename Json::char_type;
    using char_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<char_type>;
    using key_type = std::basic_string<char_type,std::char_traits<char_type>, char_allocator_type>;

    std::error_code ec;

    auto cont = cursor.current().event_type() == staj_events::begin_object ? 
        jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none) : 
        jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg, semantic_tag::none);
    std::vector<Json*,json_ptr_allocator_type> stack(aset.get_temp_allocator());
    stack.push_back(std::addressof(cont));
    key_type key(aset.get_temp_allocator());
    if (cursor.current().event_type() == staj_events::begin_object)
    {
        goto begin_object;
    }
    goto begin_array;
    
begin_object:    
    cursor.next(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
    }
    while (!cursor.done() && !stack.empty())
    {
        switch (cursor.current().event_type())
        {
            case staj_events::begin_object:
            {
                auto result = stack.back()->try_emplace(key, json_object_arg);
                stack.push_back(std::addressof(result.first->value()));
                goto begin_object;
            }
            case staj_events::begin_array:
            {
                auto result = stack.back()->try_emplace(key, json_array_arg);
                stack.push_back(std::addressof(result.first->value()));
                goto begin_array;
            }
            case staj_events::key:
            {
                auto sv = cursor.current().template get<basic_string_view<char_type>>(ec);
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                key = key_type(sv.data(), sv.length(), aset.get_temp_allocator());
                break;
            }
            case staj_events::string_value:
                stack.back()->try_emplace(key, cursor.current().template get<jsoncons::basic_string_view<char_type>>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::byte_string_value:
                stack.back()->try_emplace(key, byte_string_arg, cursor.current().template get<byte_string_view>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::null_value:
                stack.back()->try_emplace(key, null_arg);
                break;
            case staj_events::bool_value:
                stack.back()->try_emplace(key, cursor.current().template get<bool>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::int64_value:
                stack.back()->try_emplace(key, cursor.current().template get<std::int64_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::uint64_value:
                stack.back()->try_emplace(key, cursor.current().template get<std::uint64_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::half_value:
                stack.back()->try_emplace(key, half_arg, cursor.current().template get<std::uint16_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::double_value:
                stack.back()->try_emplace(key, cursor.current().template get<double>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::end_object:
                stack.pop_back();
                if (stack.empty())
                {
                    return result_type(std::move(cont));
                }
                if (stack.back()->type() == json_type::object)
                {
                    goto begin_object;
                }
                goto begin_array;
                break;
            default:
                return result_type(jsoncons::unexpect, conv_errc::conversion_failed, cursor.line(), cursor.column());
        }
        cursor.next(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
    }
    return result_type(std::move(cont));

begin_array:    
    cursor.next(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
    }
    while (!cursor.done() && !stack.empty())
    {
        switch (cursor.current().event_type())
        {
            case staj_events::begin_object:
            {
                auto& result = stack.back()->emplace_back(json_object_arg);
                stack.push_back(std::addressof(result));
                goto begin_object;
            }
            case staj_events::begin_array:
            {
                auto& result = stack.back()->emplace_back(json_array_arg);
                stack.push_back(std::addressof(result));
                goto begin_array;
            }
            case staj_events::string_value:
                stack.back()->emplace_back(cursor.current().template get<jsoncons::basic_string_view<char_type>>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::byte_string_value:
                stack.back()->emplace_back(byte_string_arg, cursor.current().template get<byte_string_view>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::null_value:
                stack.back()->emplace_back(null_arg);
                break;
            case staj_events::bool_value:
                stack.back()->emplace_back(cursor.current().template get<bool>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::int64_value:
                stack.back()->emplace_back(cursor.current().template get<std::int64_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::uint64_value:
                stack.back()->emplace_back(cursor.current().template get<std::uint64_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::half_value:
                stack.back()->emplace_back(half_arg, cursor.current().template get<std::uint16_t>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::double_value:
                stack.back()->emplace_back(cursor.current().template get<double>(ec), cursor.current().tag());
                if (JSONCONS_UNLIKELY(ec))
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
                break;
            case staj_events::end_array:
                stack.pop_back();
                if (stack.empty())
                {
                    return cont;
                }
                if (stack.back()->type() == json_type::object)
                {
                    goto begin_object;
                }
                goto begin_array;
                break;
            default:
                return result_type(jsoncons::unexpect, conv_errc::conversion_failed, cursor.line(), cursor.column());
        }
        cursor.next(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
    }
    
    JSONCONS_UNREACHABLE();
}

template <typename Json, typename Alloc, typename TempAlloc>
read_result<Json> try_to_json(const allocator_set<Alloc,TempAlloc>& aset, 
    basic_staj_cursor<typename Json::char_type>& cursor)
{
    using result_type = read_result<Json>;

    if (JSONCONS_UNLIKELY(is_end_container(cursor.current().event_type())))
    {
        return result_type(jsoncons::unexpect, conv_errc::conversion_failed, cursor.line(), cursor.column());
    }
    if (!is_begin_container(cursor.current().event_type()))
    {
        return to_json_single<Json>(aset, cursor);
    }
    return to_json_container<Json>(aset, cursor);
}

template <typename Json>
read_result<Json> try_to_json(basic_staj_cursor<typename Json::char_type>& cursor)
{
    return try_to_json<Json>(allocator_set<typename Json::allocator_type, std::allocator<char>>(), cursor);
}

using staj_event = basic_staj_event<char>;
using wstaj_event = basic_staj_event<wchar_t>;

using staj_cursor = basic_staj_cursor<char>;
using wstaj_cursor = basic_staj_cursor<wchar_t>;

using staj_filter_view = basic_staj_filter_view<char>;
using wstaj_filter_view = basic_staj_filter_view<wchar_t>;

} // namespace jsoncons

#endif // JSONCONS_STAJ_CURSOR_HPP

