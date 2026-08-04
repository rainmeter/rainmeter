// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_STAJ_EVENT_HPP
#define JSONCONS_STAJ_EVENT_HPP

#include <array> // std::array
#include <cstddef>
#include <cstdint>
#include <functional> // std::function
#include <ios>
#include <ostream> 
#include <memory> // std::allocator
#include <system_error>
#include <type_traits> // std::enable_if

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/item_event_visitor.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_parser.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/sink.hpp>
#include <jsoncons/typed_array_view.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons/utility/conversion.hpp>

namespace jsoncons {

enum class staj_events : uint64_t
{
    string_value      = 0b0000000000000001,
    byte_string_value = 0b0000000000000010,
    null_value        = 0b0000000000000100,
    bool_value        = 0b0000000000001000,
    int64_value       = 0b0000000000010000,
    uint64_value      = 0b0000000000100000,
    half_value        = 0b0000000001000000,
    double_value      = 0b0000000010000000,
    begin_object      = 0b0000000100000000,
    end_object        = 0b0000001000000000,   
    begin_array       = 0b0000010000000000,
    end_array         = 0b0000100000000000,
    key               = 0b0001000000000000
};

using staj_event_type = staj_events; // For backwards compatibility

JSONCONS_ATTRIBUTE_NODISCARD
constexpr staj_events
operator|(staj_events lhs, staj_events rhs) noexcept
{ return (staj_events)((uint64_t)lhs | (uint64_t)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr staj_events
operator&(staj_events lhs, staj_events rhs) noexcept
{ return (staj_events)((uint64_t)lhs & (uint64_t)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr staj_events
operator^(staj_events lhs, staj_events rhs) noexcept
{ return (staj_events)((uint64_t)lhs ^ (uint64_t)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr staj_events
operator~(staj_events types) noexcept
{ return (staj_events)~(uint64_t)types; }

constexpr staj_events&
operator|=(staj_events& lhs, staj_events rhs) noexcept
{ return lhs = lhs | rhs; }

constexpr staj_events&
operator&=(staj_events& lhs, staj_events rhs) noexcept
{ return lhs = lhs & rhs; }

constexpr staj_events&
operator^=(staj_events& lhs, staj_events rhs) noexcept
{ return lhs = lhs ^ rhs; }

inline bool is_begin_container(staj_events types) noexcept
{
    static const staj_events mask{ staj_events::begin_object | staj_events::begin_array };
    return (types & mask) != staj_events{};
}

inline bool is_end_container(staj_events types) noexcept
{
    static const staj_events mask{ staj_events::end_object | staj_events::end_array };
    return (types & mask) != staj_events{};
}

template <typename CharT>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, staj_events tag)
{
    static constexpr const CharT* begin_array_name = JSONCONS_CSTRING_CONSTANT(CharT, "begin_array");
    static constexpr const CharT* end_array_name = JSONCONS_CSTRING_CONSTANT(CharT, "end_array");
    static constexpr const CharT* begin_object_name = JSONCONS_CSTRING_CONSTANT(CharT, "begin_object");
    static constexpr const CharT* end_object_name = JSONCONS_CSTRING_CONSTANT(CharT, "end_object");
    static constexpr const CharT* key_name = JSONCONS_CSTRING_CONSTANT(CharT, "key");
    static constexpr const CharT* string_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "string_value");
    static constexpr const CharT* byte_string_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "byte_string_value");
    static constexpr const CharT* null_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "null_value");
    static constexpr const CharT* bool_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "bool_value");
    static constexpr const CharT* uint64_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "uint64_value");
    static constexpr const CharT* int64_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "int64_value");
    static constexpr const CharT* half_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "half_value");
    static constexpr const CharT* double_value_name = JSONCONS_CSTRING_CONSTANT(CharT, "double_value");

    switch (tag)
    {
        case staj_events::begin_array:
        {
            os << begin_array_name;
            break;
        }
        case staj_events::end_array:
        {
            os << end_array_name;
            break;
        }
        case staj_events::begin_object:
        {
            os << begin_object_name;
            break;
        }
        case staj_events::end_object:
        {
            os << end_object_name;
            break;
        }
        case staj_events::key:
        {
            os << key_name;
            break;
        }
        case staj_events::string_value:
        {
            os << string_value_name;
            break;
        }
        case staj_events::byte_string_value:
        {
            os << byte_string_value_name;
            break;
        }
        case staj_events::null_value:
        {
            os << null_value_name;
            break;
        }
        case staj_events::bool_value:
        {
            os << bool_value_name;
            break;
        }
        case staj_events::int64_value:
        {
            os << int64_value_name;
            break;
        }
        case staj_events::uint64_value:
        {
            os << uint64_value_name;
            break;
        }
        case staj_events::half_value:
        {
            os << half_value_name;
            break;
        }
        case staj_events::double_value:
        {
            os << double_value_name;
            break;
        }
    }
    return os;
}

template <typename CharT>
class basic_staj_event
{
    staj_events event_type_;
    semantic_tag tag_;
    uint64_t ext_tag_{0};
    union
    {
        bool bool_value_;
        int64_t int64_value_;
        uint64_t uint64_value_;
        uint16_t half_value_;
        double double_value_;
        const CharT* string_data_;
        const uint8_t* byte_string_data_;
    } value_;
    std::size_t length_{0};
public:
    using char_type = CharT;
    using string_view_type = jsoncons::basic_string_view<char_type>;

    basic_staj_event(staj_events event_type, semantic_tag tag = semantic_tag::none)
        : event_type_(event_type), tag_(tag), value_()
    {
    }

    basic_staj_event(staj_events event_type, std::size_t length, semantic_tag tag = semantic_tag::none)
        : event_type_(event_type), tag_(tag), value_(), length_(length)
    {
    }

    basic_staj_event(null_type, semantic_tag tag)
        : event_type_(staj_events::null_value), tag_(tag), value_()
    {
    }

    basic_staj_event(bool value, semantic_tag tag)
        : event_type_(staj_events::bool_value), tag_(tag)
    {
        value_.bool_value_ = value;
    }

    basic_staj_event(int64_t value, semantic_tag tag)
        : event_type_(staj_events::int64_value), tag_(tag)
    {
        value_.int64_value_ = value;
    }

    basic_staj_event(uint64_t value, semantic_tag tag)
        : event_type_(staj_events::uint64_value), tag_(tag)
    {
        value_.uint64_value_ = value;
    }

    basic_staj_event(half_arg_t, uint16_t value, semantic_tag tag)
        : event_type_(staj_events::half_value), tag_(tag)
    {
        value_.half_value_ = value;
    }

    basic_staj_event(double value, semantic_tag tag)
        : event_type_(staj_events::double_value), tag_(tag)
    {
        value_.double_value_ = value;
    }

    basic_staj_event(const string_view_type& s,
        staj_events event_type,
        semantic_tag tag = semantic_tag::none)
        : event_type_(event_type), tag_(tag), length_(s.length())
    {
        value_.string_data_ = s.data();
    }

    basic_staj_event(const byte_string_view& s,
        staj_events event_type,
        semantic_tag tag = semantic_tag::none)
        : event_type_(event_type), tag_(tag), length_(s.size())
    {
        value_.byte_string_data_ = s.data();
    }

    basic_staj_event(const byte_string_view& s,
        staj_events event_type,
        uint64_t ext_tag)
        : event_type_(event_type), tag_(semantic_tag::ext), ext_tag_(ext_tag), length_(s.size())
    {
        value_.byte_string_data_ = s.data();
    }
    
    ~basic_staj_event() = default;

    std::size_t size() const
    {
        return length_;
    }

    template <typename T>
    T get() const
    {
        std::error_code ec;
        T val = get<T>(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(ser_error(ec));
        }
        return val;
    }

    template <typename T>
    T get(std::error_code& ec) const
    {
        return get_<T>(std::allocator<char>{}, ec);
    }

    template <typename T,typename Allocator,typename CharT_ = CharT>
    typename std::enable_if<ext_traits::is_string<T>::value && std::is_same<typename T::value_type, CharT_>::value, T>::type
    get_(Allocator alloc,std::error_code& ec) const
    {
        constexpr const char_type* true_constant = JSONCONS_CSTRING_CONSTANT(char_type,"true"); 
        constexpr const char_type* false_constant = JSONCONS_CSTRING_CONSTANT(char_type,"false"); 
        constexpr const char_type* null_constant = JSONCONS_CSTRING_CONSTANT(char_type,"null"); 

        switch (event_type_)
        {
            case staj_events::key:
            case staj_events::string_value:
            {
                return jsoncons::make_obj_using_allocator<T>(alloc, value_.string_data_, length_);
            }
            case staj_events::byte_string_value:
            {
                auto s = jsoncons::make_obj_using_allocator<T>(alloc);
                bytes_to_string(value_.byte_string_data_, value_.byte_string_data_+length_, tag(), s);
                return s;
            }
            case staj_events::uint64_value:
            {
                auto s = jsoncons::make_obj_using_allocator<T>(alloc);
                jsoncons::from_integer(value_.uint64_value_, s);
                return s;
            }
            case staj_events::int64_value:
            {
                auto s = jsoncons::make_obj_using_allocator<T>(alloc);
                jsoncons::from_integer(value_.int64_value_, s);
                return s;
            }
            case staj_events::half_value:
            {
                auto s = jsoncons::make_obj_using_allocator<T>(alloc);
                jsoncons::write_double f{float_chars_format::general,0};
                double x = binary::decode_half(value_.half_value_);
                f(x, s);
                return s;
            }
            case staj_events::double_value:
            {
                auto s = jsoncons::make_obj_using_allocator<T>(alloc);
                jsoncons::write_double f{float_chars_format::general,0};
                f(value_.double_value_, s);
                return s;
            }
            case staj_events::bool_value:
            {
                return jsoncons::make_obj_using_allocator<T>(alloc, value_.bool_value_ ? true_constant : false_constant);
            }
            case staj_events::null_value:
            {
                return jsoncons::make_obj_using_allocator<T>(alloc, null_constant);
            }
            default:
            {
                ec = conv_errc::not_string;
                return T{};
            }
        }
    }

    template <typename T,typename Allocator,typename CharT_ = CharT>
    typename std::enable_if<ext_traits::is_string_view<T>::value && std::is_same<typename T::value_type, CharT_>::value, T>::type
        get_(Allocator, std::error_code& ec) const
    {
        T s;
        switch (event_type_)
        {
        case staj_events::key:
        case staj_events::string_value:
            s = T(value_.string_data_, length_);
            break;
        default:
            ec = conv_errc::not_string_view;
            break;        
        }
        return s;
    }

    template <typename T,typename Allocator>
    typename std::enable_if<std::is_same<T, byte_string_view>::value, T>::type
        get_(Allocator, std::error_code& ec) const
    {
        T s;
        switch (event_type_)
        {
            case staj_events::byte_string_value:
                s = T(value_.byte_string_data_, length_);
                break;
            default:
                ec = conv_errc::not_byte_string_view;
                break;
        }
        return s;
    }

    template <typename T,typename Allocator>
    typename std::enable_if<ext_traits::is_array_like<T>::value &&
                            std::is_same<typename T::value_type,uint8_t>::value,T>::type
    get_(Allocator alloc, std::error_code& ec) const
    {
        switch (event_type_)
        {
            case staj_events::byte_string_value:
            {
                auto v = jsoncons::make_obj_using_allocator<T>(alloc, 
                    value_.byte_string_data_, 
                    value_.byte_string_data_+length_);
                return v;
            }
            case staj_events::string_value:
            {
                auto v = jsoncons::make_obj_using_allocator<T>(alloc);
                auto r = string_to_bytes(value_.string_data_, value_.string_data_+length_, tag(), v);
                if (r.ec != conv_errc{})
                {
                    ec = conv_errc::not_byte_string;
                }
                return v;
            }
            default:
                ec = conv_errc::not_byte_string;
                return T{};
        }
    }

    template <typename IntegerType,typename Allocator>
    typename std::enable_if<ext_traits::is_integer<IntegerType>::value, IntegerType>::type
    get_(Allocator, std::error_code& ec) const
    {
        switch (event_type_)
        {
            case staj_events::string_value:
            {
                IntegerType val;
                auto result = jsoncons::to_integer(value_.string_data_, length_, val);
                if (!result)
                {
                    ec = conv_errc::not_integer;
                    return IntegerType();
                }
                return val;
            }
            case staj_events::half_value:
                return static_cast<IntegerType>(value_.half_value_);
            case staj_events::double_value:
                return static_cast<IntegerType>(value_.double_value_);
            case staj_events::int64_value:
                return static_cast<IntegerType>(value_.int64_value_);
            case staj_events::uint64_value:
                return static_cast<IntegerType>(value_.uint64_value_);
            case staj_events::bool_value:
                return static_cast<IntegerType>(value_.bool_value_ ? 1 : 0);
            default:
                ec = conv_errc::not_integer;
                return IntegerType();
        }
    }

    template <typename T,typename Allocator>
    typename std::enable_if<std::is_floating_point<T>::value, T>::type
        get_(Allocator, std::error_code& ec) const
    {
        return static_cast<T>(as_double(ec));
    }

    template <typename T,typename Allocator>
    typename std::enable_if<ext_traits::is_bool<T>::value, T>::type
        get_(Allocator, std::error_code& ec) const
    {
        return as_bool(ec);
    }

    staj_events event_type() const noexcept { return event_type_; }

    semantic_tag tag() const noexcept { return tag_; }

    uint64_t ext_tag() const noexcept { return ext_tag_; }

private:

    double as_double(std::error_code& ec) const
    {
        switch (event_type_)
        {
            case staj_events::key:
            case staj_events::string_value:
            {
                double val{0};
                jsoncons::decstr_to_double(value_.string_data_, length_, val);
                return val;
            }
            case staj_events::double_value:
                return value_.double_value_;
            case staj_events::int64_value:
                return static_cast<double>(value_.int64_value_);
            case staj_events::uint64_value:
                return static_cast<double>(value_.uint64_value_);
            case staj_events::half_value:
            {
                double x = binary::decode_half(value_.half_value_);
                return static_cast<double>(x);
            }
            default:
                ec = conv_errc::not_double;
                return double();
        }
    }

    bool as_bool(std::error_code& ec) const
    {
        switch (event_type_)
        {
            case staj_events::bool_value:
                return value_.bool_value_;
            case staj_events::double_value:
                return value_.double_value_ != 0.0;
            case staj_events::int64_value:
                return value_.int64_value_ != 0;
            case staj_events::uint64_value:
                return value_.uint64_value_ != 0;
            default:
                ec = conv_errc::not_bool;
                return bool();
        }
    }
public:
    void send_json_event(basic_json_visitor<CharT>& visitor,
        const ser_context& context,
        std::error_code& ec) const
    {
        switch (event_type())
        {
            case staj_events::begin_array:
                visitor.begin_array(tag(), context);
                break;
            case staj_events::end_array:
                visitor.end_array(context);
                break;
            case staj_events::begin_object:
                visitor.begin_object(tag(), context, ec);
                break;
            case staj_events::end_object:
                visitor.end_object(context, ec);
                break;
            case staj_events::key:
                visitor.key(string_view_type(value_.string_data_,length_), context);
                break;
            case staj_events::string_value:
                visitor.string_value(string_view_type(value_.string_data_,length_), tag(), context);
                break;
            case staj_events::byte_string_value:
                visitor.byte_string_value(byte_string_view(value_.byte_string_data_,length_), tag(), context);
                break;
            case staj_events::null_value:
                visitor.null_value(tag(), context);
                break;
            case staj_events::bool_value:
                visitor.bool_value(value_.bool_value_, tag(), context);
                break;
            case staj_events::int64_value:
                visitor.int64_value(value_.int64_value_, tag(), context);
                break;
            case staj_events::uint64_value:
                visitor.uint64_value(value_.uint64_value_, tag(), context);
                break;
            case staj_events::half_value:
                visitor.half_value(value_.half_value_, tag(), context);
                break;
            case staj_events::double_value:
                visitor.double_value(value_.double_value_, tag(), context);
                break;
            default:
                break;
        }
    }
    
    void send_value_event(basic_item_event_visitor<CharT>& visitor,
        const ser_context& context,
        std::error_code& ec) const
    {
        switch (event_type())
        {
            case staj_events::key:
                visitor.string_value(string_view_type(value_.string_data_,length_), tag(), context);
                break;
            case staj_events::begin_array:
                visitor.begin_array(tag(), context);
                break;
            case staj_events::end_array:
                visitor.end_array(context);
                break;
            case staj_events::begin_object:
                visitor.begin_object(tag(), context, ec);
                break;
            case staj_events::end_object:
                visitor.end_object(context, ec);
                break;
            case staj_events::string_value:
                visitor.string_value(string_view_type(value_.string_data_,length_), tag(), context);
                break;
            case staj_events::byte_string_value:
                visitor.byte_string_value(byte_string_view(value_.byte_string_data_,length_), tag(), context);
                break;
            case staj_events::null_value:
                visitor.null_value(tag(), context);
                break;
            case staj_events::bool_value:
                visitor.bool_value(value_.bool_value_, tag(), context);
                break;
            case staj_events::int64_value:
                visitor.int64_value(value_.int64_value_, tag(), context);
                break;
            case staj_events::uint64_value:
                visitor.uint64_value(value_.uint64_value_, tag(), context);
                break;
            case staj_events::half_value:
                visitor.half_value(value_.half_value_, tag(), context);
                break;
            case staj_events::double_value:
                visitor.double_value(value_.double_value_, tag(), context);
                break;
            default:
                break;
        }
    }

};

} // namespace jsoncons

#endif // JSONCONS_STAJ_EVENT_HPP

