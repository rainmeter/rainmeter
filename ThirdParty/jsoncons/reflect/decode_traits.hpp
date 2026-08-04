// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_REFLECT_DECODE_TRAITS_HPP
#define JSONCONS_REFLECT_DECODE_TRAITS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits> // std::enable_if, std::true_type, std::false_type
#include <utility>
#include <vector>

#include <jsoncons/allocator_set.hpp>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/json_cursor.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/reflect/json_conv_traits.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/staj_cursor.hpp>
#include <jsoncons/staj_event.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons {
namespace reflect {

// decode_traits

template <typename T,typename Enable = void>
struct decode_traits
{
    using value_type = T;
    using result_type = read_result<value_type>;
    
    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset,
        basic_staj_cursor<CharT>& cursor)
    {
        std::size_t line = cursor.line(); 
        std::size_t column = cursor.column();

        using json_type = basic_json<CharT,sorted_policy,TempAlloc>;
        auto r1 = try_to_json<json_type>(make_alloc_set(aset.get_temp_allocator(), aset.get_temp_allocator()), 
            cursor);
        if (JSONCONS_UNLIKELY(!r1))
        {
            return result_type(jsoncons::unexpect, r1.error().code(), r1.error().message_arg(), line, column);
        }
        auto r2 = (*r1).template try_as<T>(aset);
        if (JSONCONS_UNLIKELY(!r2))
        {
            return result_type(jsoncons::unexpect, r2.error().code(), r2.error().message_arg(), line, column);
        }
        return result_type(std::move(*r2));
    }
};

template <typename T>
struct decode_traits<T,
    typename std::enable_if<ext_traits::is_basic_json<T>::value
>::type>
{
    using value_type = T;
    using result_type = read_result<value_type>;
    
    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        auto j_result = try_to_json<T>(aset, cursor);
        if (JSONCONS_UNLIKELY(!j_result))
        {
            return result_type(jsoncons::unexpect, std::move(j_result.error()));
        }
        return j_result;
    }
};

// specializations

// primitive

template <typename T>
struct decode_traits<T,
    typename std::enable_if<ext_traits::is_primitive<T>::value
>::type>
{
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>&, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;
        
        T v = cursor.current().template get<value_type>(ec);
        return ec ? result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()} : result_type{std::move(v)};
    }
};

// string

template <typename T>
struct decode_traits<T,
    typename std::enable_if<ext_traits::is_string<T>::value>::type>
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename T::value_type;
    using string_view_type = basic_string_view<char_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, 
        basic_staj_cursor<CharT>& cursor,
        typename std::enable_if<std::is_same<typename T::value_type,CharT>::value,int>::type = 0)
    {
        std::error_code ec;

        auto sv = cursor.current().template get<string_view_type>(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
        }

        return result_type{jsoncons::make_obj_using_allocator<T>(aset.get_allocator(), sv.data(), sv.size())};
    }

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, 
        basic_staj_cursor<CharT>& cursor,
        typename std::enable_if<!std::is_same<typename T::value_type,CharT>::value,int>::type = 0)
    {
        using temp_char_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<char_type>;
        using buffer_type = std::basic_string<char_type,std::char_traits<char_type>,temp_char_allocator_type>; 

        std::error_code ec;

        auto val = cursor.current().template get<std::basic_string<CharT>>(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        buffer_type buf(aset.get_temp_allocator());;
        unicode_traits::convert(val.data(), val.size(), buf);
        return result_type{in_place, buf.data(), buf.size()};
    }
};

// std::pair

template <typename T1,typename T2>
struct decode_traits<std::pair<T1, T2>>
{
    using value_type = std::pair<T1, T2>;
    using result_type = read_result<value_type>;
    
    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        if (cursor.current().event_type() != staj_events::begin_array)
        {
            return result_type(jsoncons::unexpect, conv_errc::not_pair, cursor.line(), cursor.column());
        }
        cursor.next(ec); // skip past array
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }

        auto r1 = decode_traits<T1>::try_decode(aset, cursor);
        if (JSONCONS_UNLIKELY(!r1))
        {
            return result_type(jsoncons::unexpect, r1.error());
        }
        cursor.next(ec);
        if (JSONCONS_UNLIKELY(ec)) 
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        auto r2 = decode_traits<T2>::try_decode(aset, cursor);
        if (JSONCONS_UNLIKELY(!r2)) 
        {
            return result_type(jsoncons::unexpect, r2.error());
        }
        cursor.next(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }

        if (cursor.current().event_type() != staj_events::end_array)
        {
            return result_type(jsoncons::unexpect, conv_errc::not_pair, cursor.line(), cursor.column()); 
        }
        return result_type{jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), std::move(*r1), std::move(*r2))};
    }
};

// vector like
template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
             ext_traits::is_array_like<T>::value &&
             ext_traits::is_back_insertable<T>::value &&
             !ext_traits::is_typed_array<T>::value 
>::type>
{
    using element_type = typename T::value_type;
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;
        T v(jsoncons::make_obj_using_allocator<T>(aset.get_allocator()));

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        if (cursor.current().event_type() != staj_events::begin_array)
        {
            return result_type(jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()); 
        }
        cursor.next(ec);
        if (JSONCONS_UNLIKELY(ec)) { return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column()); }
        while (cursor.current().event_type() != staj_events::end_array && !ec)
        {
            auto r = decode_traits<element_type>::try_decode(aset, cursor);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error()); 
            }
            v.push_back(std::move(*r));
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) { return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column()); }
        }
        return result_type{std::move(v)};
    }
};

template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
             ext_traits::is_array_like<T>::value &&
             ext_traits::is_back_insertable_byte_container<T>::value &&
             ext_traits::is_typed_array<T>::value
>::type>
{
    using element_type = typename T::value_type;
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, 
        basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        switch (cursor.current().event_type())
        {
            case staj_events::byte_string_value:
            {
                auto bytes = cursor.current().template get<byte_string_view>(ec);
                if (!ec) 
                {
                    T v;
                    if (cursor.current().size() > 0)
                    {
                        reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), v, cursor.current().size());
                    }
                    for (auto ch : bytes)
                    {
                        v.push_back(static_cast<element_type>(ch));
                    }
                    cursor.next(ec);
                    return result_type{std::move(v)};
                }
                else
                {
                    return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
                }
            }
            case staj_events::begin_array:
            {
                T v = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
                if (cursor.current().size() > 0)
                {
                    reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), v, cursor.current().size());
                }
                cursor.next(ec);
                while (cursor.current().event_type() != staj_events::end_array && !ec)
                {
                    auto r = decode_traits<element_type>::try_decode(aset, cursor);
                    if (!r)
                    {
                        return result_type(jsoncons::unexpect, r.error());
                    }
                    v.push_back(*r);
                    //v[i] = std::move(*r);
                    cursor.next(ec);
                }
                if (JSONCONS_UNLIKELY(ec)) 
                {
                    return result_type{jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()}; 
                }

                return result_type{std::move(v)};
            }
            default:
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()); 
            }
        }
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
             ext_traits::is_array_like<T>::value &&
             ext_traits::is_back_insertable<T>::value &&
             !ext_traits::is_back_insertable_byte_container<T>::value &&
             ext_traits::is_typed_array<T>::value
>::type>
{
    using element_type = typename T::value_type;
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        switch (cursor.current().event_type())
        {
            case staj_events::begin_array:
            {
                T v = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
                if (cursor.current().size() > 0)
                {
                    reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), v, cursor.current().size());
                }
                cursor.next(ec);
                while (cursor.current().event_type() != staj_events::end_array && !ec)
                {
                    auto r = decode_traits<element_type>::try_decode(aset, cursor);
                    if (!r)
                    {
                        return result_type(jsoncons::unexpect, r.error());
                    }
                    v.push_back(*r);
                    //v[i] = std::move(*r);
                    cursor.next(ec);
                }
                if (JSONCONS_UNLIKELY(ec)) 
                {
                    return result_type{jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()}; 
                }
                return result_type{std::move(v)};
            }
            default:
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()); 
            }
        }
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

// set like
template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
             ext_traits::is_array_like<T>::value &&
             !ext_traits::is_back_insertable<T>::value &&
             ext_traits::is_insertable<T>::value 
>::type>
{
    using element_type = typename T::value_type;
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;
        T v = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        if (cursor.current().event_type() != staj_events::begin_array)
        {
            return result_type(jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()); 
        }
        if (cursor.current().size() > 0)
        {
            reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), v, cursor.current().size());
        }
        cursor.next(ec);
        while (cursor.current().event_type() != staj_events::end_array && !ec)
        {
            auto r = decode_traits<element_type>::try_decode(aset, cursor);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            v.insert(std::move(*r));
            if (JSONCONS_UNLIKELY(ec)) {return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());}
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) {return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};}
        }
        return result_type{std::move(v)};
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

// std::forward_list
template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
             ext_traits::is_array_like<T>::value &&
             !ext_traits::is_back_insertable<T>::value &&
             !ext_traits::is_insertable<T>::value &&
             ext_traits::is_front_insertable<T>::value 
>::type>
{
    using element_type = typename T::value_type;
    using value_type = T;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }

        T v = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
        if (cursor.current().event_type() != staj_events::begin_array)
        {
            return result_type(jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()); 
        }
        if (cursor.current().size() > 0)
        {
            reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), v, cursor.current().size());
        }
        cursor.next(ec);
        while (cursor.current().event_type() != staj_events::end_array && !ec)
        {
            auto r = decode_traits<element_type>::try_decode(aset, cursor);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            v.push_front(std::move(*r));
            if (JSONCONS_UNLIKELY(ec)) {return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());}
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) {return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};}
        }
        return result_type{std::move(v)};
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

// std::array

template <typename T, std::size_t N>
struct decode_traits<std::array<T,N>>
{
    using element_type = typename std::array<T,N>::value_type;
    using value_type = typename std::array<T,N>;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        std::array<T,N> v;
        cursor.array_expected(ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return result_type(jsoncons::unexpect, ec, cursor.line(), cursor.column());
        }
        if (cursor.current().event_type() != staj_events::begin_array)
        {
            return result_type{jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()}; 
        }
        cursor.next(ec);
        for (std::size_t i = 0; i < N && cursor.current().event_type() != staj_events::end_array && !ec; ++i)
        {
            auto r = decode_traits<element_type>::try_decode(aset, cursor);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            v[i] = std::move(*r);
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) 
            {
                return result_type{jsoncons::unexpect, conv_errc::not_vector, cursor.line(), cursor.column()}; 
            }
        }
        return v;
    }
};

// map like

template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
                            ext_traits::is_map_like<T>::value &&
                            ext_traits::is_string<typename T::key_type>::value
>::type>
{
    using mapped_type = typename T::mapped_type;
    using value_type = T;
    using key_type = typename T::key_type;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        auto val = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
        if (cursor.current().event_type() != staj_events::begin_object)
        {
            return result_type{jsoncons::unexpect, conv_errc::not_map, cursor.line(), cursor.column()}; 
        }
        if (cursor.current().size() > 0)
        {
            reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), val, cursor.current().size());
        }
        cursor.next(ec);

        while (cursor.current().event_type() != staj_events::end_object && !ec)
        {
            if (cursor.current().event_type() != staj_events::key)
            {
                return result_type{jsoncons::unexpect, json_errc::expected_key, cursor.line(), cursor.column()}; 
            }
            auto r1 = decode_traits<key_type>::try_decode(aset, cursor);
            if (!r1)
            {
                return result_type(jsoncons::unexpect, r1.error());
            }
            if (JSONCONS_UNLIKELY(ec)) 
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) 
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
            auto r2 = decode_traits<mapped_type>::try_decode(aset, cursor);
            if (!r2)
            {
                return result_type(jsoncons::unexpect, r2.error());
            }
            val.emplace(std::move(*r1), std::move(*r2));
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) 
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
        }
        return result_type{std::move(val)};
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

template <typename T>
struct decode_traits<T,
    typename std::enable_if<!reflect::is_json_conv_traits_declared<T>::value && 
                            ext_traits::is_map_like<T>::value &&
                            std::is_integral<typename T::key_type>::value
>::type>
{
    using mapped_type = typename T::mapped_type;
    using value_type = T;
    using key_type = typename T::key_type;
    using result_type = read_result<value_type>;

    template <typename CharT,typename Alloc,typename TempAlloc>
    static result_type try_decode(const allocator_set<Alloc,TempAlloc>& aset, basic_staj_cursor<CharT>& cursor)
    {
        std::error_code ec;

        T val;
        if (cursor.current().event_type() != staj_events::begin_object)
        {
            return result_type{jsoncons::unexpect, conv_errc::not_map, cursor.line(), cursor.column()}; 
        }
        if (cursor.current().size() > 0)
        {
            reserve_storage(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(), val, cursor.current().size());
        }
        cursor.next(ec);

        while (cursor.current().event_type() != staj_events::end_object && !ec)
        {
            if (cursor.current().event_type() != staj_events::key)
            {
                return result_type{jsoncons::unexpect, json_errc::expected_key, cursor.line(), cursor.column()}; 
            }
            auto s = cursor.current().template get<jsoncons::basic_string_view<CharT>>(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
            key_type n{0};
            auto r = jsoncons::to_integer(s.data(), s.size(), n); 
            if (r.ec != std::errc{})
            {
                return result_type{jsoncons::unexpect, json_errc::invalid_number, cursor.line(), cursor.column()}; 
            }
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
            auto r1 = decode_traits<mapped_type>::try_decode(aset, cursor);
            if (!r1)
            {
                return result_type(jsoncons::unexpect, r1.error());
            }
            val.emplace(n, std::move(*r1));
            cursor.next(ec);
            if (JSONCONS_UNLIKELY(ec)) 
            {
                return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()}; 
            }
        }
        return result_type{std::move(val)};
    }

    static void reserve_storage(std::true_type, T& v, std::size_t new_cap)
    {
        v.reserve(new_cap);
    }

    static void reserve_storage(std::false_type, T&, std::size_t)
    {
    }
};

} // namespace reflect
} // namespace jsoncons

#endif // JSONCONS_REFLECT_DECODE_TRAITS_HPP

