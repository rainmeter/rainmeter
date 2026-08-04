// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_REFLECT_REFLECT_TRAITS_GEN_HPP
#define JSONCONS_REFLECT_REFLECT_TRAITS_GEN_HPP

#include <utility>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp> // JSONCONS_PP_EXPAND, JSONCONS_PP_QUOTE
#include <jsoncons/conv_error.hpp>
#include <jsoncons/conversion_result.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/reflect/decode_traits.hpp>
#include <jsoncons/reflect/encode_traits.hpp>
#include <jsoncons/reflect/json_conv_traits.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#define JSONCONS_RDONLY(X)

#define JSONCONS_RDWR(X) X

namespace jsoncons {
namespace reflect {

struct always_true
{
    template< typename T>
    constexpr bool operator()(const T&) const noexcept
    {
        return true;
    }
};

struct identity
{
    template< typename T>
    constexpr T&& operator()(T&& val) const noexcept
    {
        return std::forward<T>(val);
    }
};

template <typename T>
struct json_object_name_members
{};

template <typename T>
struct reflect_type_properties
{};

template <typename T, typename U> 
void set_member(T&&, const U&) 
{ 
} 
template <typename T, typename U> 
void set_member(T&& val, U& result)
{ 
    result = std::forward<T>(val); 
} 

template <typename Json>
struct json_traits_helper
{
    using string_view_type = typename Json::string_view_type;

    template <typename T,typename Alloc,typename TempAlloc> 
    static conversion_result<T> try_get_member(const allocator_set<Alloc,TempAlloc>& aset, 
        const Json& j, string_view_type key) 
    { 
        auto it = j.find(key);
        if (it == j.object_range().end())
        {
            return conversion_result<T>(unexpect, conv_errc::missing_required_member);
        }
        auto result = it->value().template try_as<T>(aset); 
        if (!result)
        {
            return conversion_result<T>(unexpect, result.error());
        }
        return conversion_result<T>(std::move(*result));
    } 

    template <typename U> 
    static void set_optional_json_member(string_view_type key, const std::shared_ptr<U>& val, Json& j) 
    { 
        if (val) j.try_emplace(key, val); 
    } 
    template <typename U,typename Deleter> 
    static void set_optional_json_member(string_view_type key, const std::unique_ptr<U,Deleter>& val, Json& j) 
    { 
        if (val) j.try_emplace(key, val); 
    } 
    template <typename U> 
    static 
    typename std::enable_if<ext_traits::is_optional<U>::value, void>::type
    set_optional_json_member(string_view_type key, const U& val, Json& j) 
    { 
        if (val.has_value()) j.try_emplace(key, val); 
    } 
    template <typename U> 
    static         
    typename std::enable_if<!ext_traits::is_optional<U>::value, void>::type
    set_optional_json_member(string_view_type key, const U& val, Json& j) 
    { 
        j.try_emplace(key, val); 
    } 
};

template <typename CharT, typename T> 
write_result try_encode_member(const basic_string_view<CharT>& key, const T& val, basic_json_visitor<CharT>& encoder) 
{ 
    encoder.key(key);
    return encode_traits<T>::try_encode(make_alloc_set(), val, encoder); 
} 

template <typename CharT, typename T> 
write_result try_encode_optional_member(const basic_string_view<CharT>& key, const std::shared_ptr<T>& val, basic_json_visitor<CharT>& encoder) 
{ 
    if (val) 
    {
        encoder.key(key);
        return encode_traits<T>::try_encode(make_alloc_set(), *val, encoder); 
    }
    return write_result{}; 
}
 
template <typename CharT, typename T,typename Deleter> 
write_result try_encode_optional_member(const basic_string_view<CharT>& key, const std::unique_ptr<T,Deleter>& val, basic_json_visitor<CharT>& encoder) 
{ 
    if (val)
    {
        encoder.key(key);
        return encode_traits<T>::try_encode(make_alloc_set(), *val, encoder); 
    }
    return write_result{}; 
}
 
template <typename CharT, typename T> 
typename std::enable_if<ext_traits::is_optional<T>::value, write_result>::type
try_encode_optional_member(const basic_string_view<CharT>& key, const T& val, basic_json_visitor<CharT>& encoder) 
{ 
    if (val.has_value())
    {
        encoder.key(key);
        return encode_traits<T>::try_encode(make_alloc_set(), *val, encoder); 
    }
    return write_result{}; 
} 

template <typename CharT, typename T> 
typename std::enable_if<!ext_traits::is_optional<T>::value, write_result>::type
try_encode_optional_member(const basic_string_view<CharT>& key, const T& val, basic_json_visitor<CharT>& encoder)
{ 
    encoder.key(key);
    return encode_traits<T>::try_encode(make_alloc_set(), val, encoder); 
} 

template <typename T> 
bool is_optional_value_set(const std::shared_ptr<T>& val) 
{ 
    return val ? true : false; 
} 
template <typename T,typename Deleter> 
bool is_optional_value_set(const std::unique_ptr<T,Deleter>& val) 
{ 
    return val ? true : false;
} 
template <typename T> 
typename std::enable_if<ext_traits::is_optional<T>::value, bool>::type
is_optional_value_set(const T& val) 
{ 
    return val.has_value();
} 
template <typename T> 
typename std::enable_if<!ext_traits::is_optional<T>::value, bool>::type
is_optional_value_set(const T&) 
{
    return true; 
} 

} // namespace reflect

using always_true = reflect::always_true; 
using identity = reflect::identity; 

} // namespace jsoncons

#if defined(_MSC_VER)
#pragma warning( disable : 4127)
#endif

#define JSONCONS_PP_CONCAT_IMPL(a, b) a ## b
#define JSONCONS_PP_CONCAT(a, b) JSONCONS_PP_CONCAT_IMPL(a, b)

// Inspired by https://github.com/Loki-Astari/ThorsSerializer/blob/master/src/Serialize/Traits.h

#define JSONCONS_NARGS(...) JSONCONS_NARG_(__VA_ARGS__, 70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define JSONCONS_NARG_(...) JSONCONS_PP_EXPAND( JSONCONS_ARG_N(__VA_ARGS__) )
#define JSONCONS_ARG_N(e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,e15,e16,e17,e18,e19,e20,e21,e22,e23,e24,e25,e26,e27,e28,e29,e30,e31,e32,e33,e34,e35,e36,e37,e38,e39,e40,e41,e42,e43,e44,e45,e46,e47,e48,e49,e50,e51,e52,e53,e54,e55,e56,e57,e58,e59,e60,e61,e62,e63,e64,e65,e66,e67,e68,e69,e70,N,...)N

#define JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, Count) Call(P1, P2, P3, P4, Count) 
 
#define JSONCONS_VARIADIC_FOR_EACH(Call, P1, P2, P3, ...)         JSONCONS_VARIADIC_REP_OF_N(Call, P1,P2, P3, JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__)
#define JSONCONS_VARIADIC_REP_OF_N(Call, P1, P2, P3, Count, ...)  JSONCONS_VARIADIC_REP_OF_N_(Call, P1, P2, P3, Count, __VA_ARGS__)
#define JSONCONS_VARIADIC_REP_OF_N_(Call, P1, P2, P3, Count, ...) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_ ## Count(Call, P1, P2, P3, __VA_ARGS__))

#define JSONCONS_VARIADIC_REP_OF_70(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 70) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_69(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_69(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 69) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_68(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_68(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 68) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_67(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_67(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 67) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_66(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_66(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 66) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_65(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_65(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 65) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_64(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_64(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 64) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_63(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_63(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 63) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_62(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_62(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 62) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_61(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_61(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 61) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_60(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_60(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 60) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_59(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_59(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 59) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_58(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_58(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 58) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_57(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_57(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 57) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_56(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_56(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 56) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_55(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_55(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 55) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_54(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_54(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 54) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_53(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_53(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 53) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_52(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_52(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 52) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_51(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_51(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 51) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_50(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_50(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 50) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_49(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_49(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 49) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_48(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_48(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 48) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_47(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_47(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 47) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_46(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_46(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 46) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_45(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_45(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 45) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_44(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_44(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 44) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_43(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_43(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 43) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_42(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_42(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 42) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_41(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_41(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 41) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_40(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_40(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 40) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_39(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_39(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 39) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_38(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_38(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 38) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_37(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_37(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 37) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_36(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_36(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 36) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_35(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_35(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 35) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_34(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_34(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 34) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_33(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_33(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 33) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_32(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_32(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 32) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_31(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_31(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 31) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_30(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_30(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 30) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_29(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_29(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 29) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_28(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_28(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 28) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_27(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_27(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 27) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_26(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_26(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 26) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_25(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_25(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 25) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_24(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_24(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 24) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_23(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_23(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 23) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_22(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_22(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 22) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_21(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_21(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 21) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_20(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_20(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 20) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_19(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_19(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 19) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_18(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_18(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 18) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_17(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_17(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 17) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_16(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_16(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 16) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_15(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_15(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 15) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_14(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_14(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 14) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_13(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_13(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 13) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_12(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_12(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 12) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_11(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_11(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 11) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_10(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_10(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 10) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_9(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_9(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 9) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_8(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_8(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 8) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_7(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_7(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 7) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_6(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_6(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 6) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_5(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_5(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 5) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_4(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_4(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 4) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_3(Call, P1, P2, P3, __VA_ARGS__))
#define JSONCONS_VARIADIC_REP_OF_3(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 3) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_2(Call, P1, P2, P3, __VA_ARGS__)) 
#define JSONCONS_VARIADIC_REP_OF_2(Call, P1, P2, P3, P4, ...)    JSONCONS_EXPAND_CALL5(Call, P1, P2, P3, P4, 2) JSONCONS_PP_EXPAND(JSONCONS_VARIADIC_REP_OF_1(Call, P1, P2, P3, __VA_ARGS__)) 
#define JSONCONS_VARIADIC_REP_OF_1(Call, P1, P2, P3, P4)         JSONCONS_PP_EXPAND(Call ## _LAST(P1, P2, P3, P4, 1))

#define JSONCONS_TYPE_TRAITS_FRIEND \
    template <typename JSON,typename T,typename Enable> \
    friend struct jsoncons::json_type_traits; \
    template <typename JSON,typename T,typename Enable> \
    friend struct jsoncons::reflect::json_conv_traits; \
    template <typename T,typename Enable> \
    friend struct jsoncons::reflect::encode_traits; \
    template <typename T,typename Enable> \
    friend struct jsoncons::reflect::decode_traits;

#define JSONCONS_CONV_TRAITS_FRIEND \
    template <typename JSON,typename T,typename Enable> \
    friend struct jsoncons::reflect::json_conv_traits; \
    template <typename T,typename Enable> \
    friend struct jsoncons::reflect::encode_traits; \
    template <typename T,typename Enable> \
    friend struct jsoncons::reflect::decode_traits;

#define JSONCONS_EXPAND_CALL2(Call, Expr, Id) JSONCONS_PP_EXPAND(Call(Expr, Id))

#define JSONCONS_REP_OF_N(Call, Expr, Pre, App, Count)  JSONCONS_REP_OF_ ## Count(Call, Expr, Pre, App)

#define JSONCONS_REP_OF_50(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 50) JSONCONS_REP_OF_49(Call, Expr, , App)
#define JSONCONS_REP_OF_49(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 49) JSONCONS_REP_OF_48(Call, Expr, , App)
#define JSONCONS_REP_OF_48(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 48) JSONCONS_REP_OF_47(Call, Expr, , App)
#define JSONCONS_REP_OF_47(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 47) JSONCONS_REP_OF_46(Call, Expr, , App)
#define JSONCONS_REP_OF_46(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 46) JSONCONS_REP_OF_45(Call, Expr, , App)
#define JSONCONS_REP_OF_45(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 45) JSONCONS_REP_OF_44(Call, Expr, , App)
#define JSONCONS_REP_OF_44(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 44) JSONCONS_REP_OF_43(Call, Expr, , App)
#define JSONCONS_REP_OF_43(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 43) JSONCONS_REP_OF_42(Call, Expr, , App)
#define JSONCONS_REP_OF_42(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 42) JSONCONS_REP_OF_41(Call, Expr, , App)
#define JSONCONS_REP_OF_41(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 41) JSONCONS_REP_OF_40(Call, Expr, , App)
#define JSONCONS_REP_OF_40(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 40) JSONCONS_REP_OF_39(Call, Expr, , App)
#define JSONCONS_REP_OF_39(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 39) JSONCONS_REP_OF_38(Call, Expr, , App)
#define JSONCONS_REP_OF_38(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 38) JSONCONS_REP_OF_37(Call, Expr, , App)
#define JSONCONS_REP_OF_37(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 37) JSONCONS_REP_OF_36(Call, Expr, , App)
#define JSONCONS_REP_OF_36(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 36) JSONCONS_REP_OF_35(Call, Expr, , App)
#define JSONCONS_REP_OF_35(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 35) JSONCONS_REP_OF_34(Call, Expr, , App)
#define JSONCONS_REP_OF_34(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 34) JSONCONS_REP_OF_33(Call, Expr, , App)
#define JSONCONS_REP_OF_33(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 33) JSONCONS_REP_OF_32(Call, Expr, , App)
#define JSONCONS_REP_OF_32(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 32) JSONCONS_REP_OF_31(Call, Expr, , App)
#define JSONCONS_REP_OF_31(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 31) JSONCONS_REP_OF_30(Call, Expr, , App)
#define JSONCONS_REP_OF_30(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 30) JSONCONS_REP_OF_29(Call, Expr, , App)
#define JSONCONS_REP_OF_29(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 29) JSONCONS_REP_OF_28(Call, Expr, , App)
#define JSONCONS_REP_OF_28(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 28) JSONCONS_REP_OF_27(Call, Expr, , App)
#define JSONCONS_REP_OF_27(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 27) JSONCONS_REP_OF_26(Call, Expr, , App)
#define JSONCONS_REP_OF_26(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 26) JSONCONS_REP_OF_25(Call, Expr, , App)
#define JSONCONS_REP_OF_25(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 25) JSONCONS_REP_OF_24(Call, Expr, , App)
#define JSONCONS_REP_OF_24(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 24) JSONCONS_REP_OF_23(Call, Expr, , App)
#define JSONCONS_REP_OF_23(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 23) JSONCONS_REP_OF_22(Call, Expr, , App)
#define JSONCONS_REP_OF_22(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 22) JSONCONS_REP_OF_21(Call, Expr, , App)
#define JSONCONS_REP_OF_21(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 21) JSONCONS_REP_OF_20(Call, Expr, , App)
#define JSONCONS_REP_OF_20(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 20) JSONCONS_REP_OF_19(Call, Expr, , App)
#define JSONCONS_REP_OF_19(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 19) JSONCONS_REP_OF_18(Call, Expr, , App)
#define JSONCONS_REP_OF_18(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 18) JSONCONS_REP_OF_17(Call, Expr, , App)
#define JSONCONS_REP_OF_17(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 17) JSONCONS_REP_OF_16(Call, Expr, , App)
#define JSONCONS_REP_OF_16(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 16) JSONCONS_REP_OF_15(Call, Expr, , App)
#define JSONCONS_REP_OF_15(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 15) JSONCONS_REP_OF_14(Call, Expr, , App)
#define JSONCONS_REP_OF_14(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 14) JSONCONS_REP_OF_13(Call, Expr, , App)
#define JSONCONS_REP_OF_13(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 13) JSONCONS_REP_OF_12(Call, Expr, , App)
#define JSONCONS_REP_OF_12(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 12) JSONCONS_REP_OF_11(Call, Expr, , App)
#define JSONCONS_REP_OF_11(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 11) JSONCONS_REP_OF_10(Call, Expr, , App)
#define JSONCONS_REP_OF_10(Call, Expr, Pre, App)     Pre JSONCONS_EXPAND_CALL2(Call, Expr, 10) JSONCONS_REP_OF_9(Call, Expr, , App)
#define JSONCONS_REP_OF_9(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 9) JSONCONS_REP_OF_8(Call, Expr, , App)
#define JSONCONS_REP_OF_8(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 8) JSONCONS_REP_OF_7(Call, Expr, , App)
#define JSONCONS_REP_OF_7(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 7) JSONCONS_REP_OF_6(Call, Expr, , App)
#define JSONCONS_REP_OF_6(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 6) JSONCONS_REP_OF_5(Call, Expr, , App)
#define JSONCONS_REP_OF_5(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 5) JSONCONS_REP_OF_4(Call, Expr, , App)
#define JSONCONS_REP_OF_4(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 4) JSONCONS_REP_OF_3(Call, Expr, , App)
#define JSONCONS_REP_OF_3(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 3) JSONCONS_REP_OF_2(Call, Expr, , App)
#define JSONCONS_REP_OF_2(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call, Expr, 2) JSONCONS_REP_OF_1(Call, Expr, , App)
#define JSONCONS_REP_OF_1(Call, Expr, Pre, App)      Pre JSONCONS_EXPAND_CALL2(Call ## _LAST, Expr, 1) App
#define JSONCONS_REP_OF_0(Call, Expr, Pre, App)

#define JSONCONS_GENERATE_TPL_PARAMS(Call, Count) JSONCONS_REP_OF_N(Call, , , ,Count)
#define JSONCONS_GENERATE_TPL_ARGS(Call, Count) JSONCONS_REP_OF_N(Call, ,<,>,Count)
#define JSONCONS_GENERATE_TPL_PARAM(Expr, Id) typename T ## Id,
#define JSONCONS_GENERATE_TPL_PARAM_LAST(Expr, Id) typename T ## Id
#define JSONCONS_GENERATE_MORE_TPL_PARAM(Expr, Id) ,typename T ## Id
#define JSONCONS_GENERATE_MORE_TPL_PARAM_LAST(Expr, Id) ,typename T ## Id
#define JSONCONS_GENERATE_TPL_ARG(Expr, Id) T ## Id,
#define JSONCONS_GENERATE_TPL_ARG_LAST(Ex, Id) T ## Id 

#define JSONCONS_GENERATE_NAME_STR(Prefix, P2, P3, Member, Count) JSONCONS_GENERATE_NAME_STR_LAST(Prefix, P2, P3, Member, Count) 
#define JSONCONS_GENERATE_NAME_STR_LAST(Prefix, P2, P3, Member, Count) \
    static inline const string_view& Member(char) {static const string_view sv = JSONCONS_PP_QUOTE(,Member); return sv;} \
    static inline const wstring_view& Member(wchar_t) {static const wstring_view sv = JSONCONS_PP_QUOTE(L,Member); return sv;} \
    static inline const string_view& Member(unexpect_t) {static const string_view sv = # Prefix "::" # Member; return sv;} \
    /**/

#define JSONCONS_N_MEMBER_IS(Prefix, P2, P3, Member, Count) JSONCONS_N_MEMBER_IS_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_N_MEMBER_IS_LAST(Prefix, P2, P3, Member, Count) if ((num_params-Count) < num_mandatory_params1 && !ajson.contains(json_object_name_members<value_type>::Member(char_type{}))) return false;

#define JSONCONS_N_MEMBER_AS(Prefix,P2,P3, Member, Count) JSONCONS_N_MEMBER_AS_LAST(Prefix,P2,P3, Member, Count)
#define JSONCONS_N_MEMBER_AS_LAST(Prefix,P2,P3, Member, Count) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(class_instance.Member)>::type>(aset, ajson, json_object_name_members<value_type>::Member(char_type{})); \
  if (result) { \
    set_member(std::move(*result), class_instance.Member); \
  } \
  else if ((num_params-Count) < num_mandatory_params2) {return result_type(jsoncons::unexpect, result.error().code(), json_object_name_members<value_type>::Member(unexpect));} \
  else if (result.error().code() != conv_errc::missing_required_member){return result_type(jsoncons::unexpect, result.error().code(), json_object_name_members<value_type>::Member(unexpect));} \
}

#define JSONCONS_ALL_MEMBER_AS(Prefix, P2,P3,Member, Count) JSONCONS_ALL_MEMBER_AS_LAST(Prefix,P2,P3, Member, Count)
#define JSONCONS_ALL_MEMBER_AS_LAST(Prefix,P2,P3, Member, Count) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(class_instance.Member)>::type>(aset, ajson, json_object_name_members<value_type>::Member(char_type{})); \
  if (result) { \
    set_member(std::move(*result), class_instance.Member); \
  } \
  else {return result_type(jsoncons::unexpect, result.error().code(), json_object_name_members<value_type>::Member(unexpect));} \
}

#define JSONCONS_TO_JSON(Prefix, P2, P3, Member, Count) JSONCONS_TO_JSON_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_TO_JSON_LAST(Prefix, P2, P3, Member, Count) if ((num_params-Count) < num_mandatory_params2) \
    {ajson.try_emplace(json_object_name_members<value_type>::Member(char_type{}),class_instance.Member);} \
    else {json_traits_helper<Json>::set_optional_json_member(json_object_name_members<value_type>::Member(char_type{}),class_instance.Member, ajson);}

#define JSONCONS_ALL_TO_JSON(Prefix, P2, P3, Member, Count) JSONCONS_ALL_TO_JSON_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_ALL_TO_JSON_LAST(Prefix, P2, P3, Member, Count) \
    ajson.try_emplace(json_object_name_members<value_type>::Member(char_type{}),class_instance.Member);

#define JSONCONS_N_MEMBER_ENCODE(Prefix, P2, P3, Member, Count) JSONCONS_N_MEMBER_ENCODE_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_N_MEMBER_ENCODE_LAST(Prefix, P2, P3, Member, Count) \
if ((num_params-Count) < num_mandatory_params2) \
    { \
        auto r = try_encode_member(json_object_name_members<value_type>::Member(char_type{}), val.Member, encoder); \
        if (JSONCONS_UNLIKELY(!r)) {return r;} \
    } \
    else \
    { \
        auto r = try_encode_optional_member(json_object_name_members<value_type>::Member(char_type{}), val.Member, encoder); \
        if (JSONCONS_UNLIKELY(!r)) {return r;} \
    }

#define JSONCONS_ALL_MEMBER_ENCODE(Prefix, P2, P3, Member, Count) JSONCONS_ALL_MEMBER_ENCODE_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_ALL_MEMBER_ENCODE_LAST(Prefix, P2, P3, Member, Count) \
    {auto r = try_encode_member(json_object_name_members<value_type>::Member(char_type{}), val.Member, encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;}} 

#define JSONCONS_MEMBER_COUNT(Prefix, P2, P3, Member, Count) JSONCONS_MEMBER_COUNT_LAST(Prefix, P2, P3, Member, Count)
#define JSONCONS_MEMBER_COUNT_LAST(Prefix, P2, P3, Member, Count) \
if ((num_params-Count) < num_mandatory_params2) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Member)) \
    { \
        ++member_count; \
    } \
} 

#define JSONCONS_MEMBER_TRAITS_BASE(ToJson,Encode,NumTemplateParams,ClassName,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct json_object_name_members<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        JSONCONS_VARIADIC_FOR_EACH(JSONCONS_GENERATE_NAME_STR,ClassName,,, __VA_ARGS__)\
    }; \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_MEMBER_IS, ,,, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, # ClassName); \
            value_type class_instance = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator()); \
            if (num_params == num_mandatory_params2) \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_ALL_MEMBER_AS,,,, __VA_ARGS__) \
            } \
            else \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_MEMBER_AS,,,, __VA_ARGS__) \
            } \
            return result_type(std::move(class_instance)); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(ToJson, ,,, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            std::error_code ec; \
            using char_type = CharT; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_MEMBER_COUNT, ,,, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(Encode, ,,, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/

#define JSONCONS_N_MEMBER_TRAITS(ClassName,NumMandatoryParams,...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_TO_JSON, JSONCONS_N_MEMBER_ENCODE, 0, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_N_MEMBER_TRAITS(NumTemplateParams, ClassName,NumMandatoryParams, ...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_TO_JSON, JSONCONS_N_MEMBER_ENCODE,NumTemplateParams, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_ALL_MEMBER_TRAITS(ClassName, ...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_ALL_TO_JSON, JSONCONS_ALL_MEMBER_ENCODE,0,ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__),__VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_ALL_MEMBER_TRAITS(NumTemplateParams, ClassName, ...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_ALL_TO_JSON, JSONCONS_ALL_MEMBER_ENCODE,NumTemplateParams,ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__),__VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/ 

#define JSONCONS_MEMBER_NAME_IS(P1, P2, P3, Seq, Count) JSONCONS_MEMBER_NAME_IS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_MEMBER_NAME_IS_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params1 && JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_MEMBER_NAME_IS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_MEMBER_NAME_IS_2(Member, Name) !ajson.contains(Name)) return false;
#define JSONCONS_MEMBER_NAME_IS_3(Member, Name, Mode) JSONCONS_MEMBER_NAME_IS_2(Member, Name)
#define JSONCONS_MEMBER_NAME_IS_4(Member, Name, Mode, Match) JSONCONS_MEMBER_NAME_IS_6(Member, Name, Mode, Match, , )
#define JSONCONS_MEMBER_NAME_IS_5(Member, Name, Mode, Match, Into) JSONCONS_MEMBER_NAME_IS_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_MEMBER_NAME_IS_6(Member, Name, Mode, Match, Into, From) !ajson.contains(Name)) return false; \
    JSONCONS_TRY{if (!Match(ajson.at(Name).template as<typename std::decay<decltype(Into((std::declval<value_type*>())->Member))>::type>())) return false;} \
    JSONCONS_CATCH(...) {return false;}

#define JSONCONS_N_MEMBER_NAME_AS(P1, P2, P3, Seq, Count) JSONCONS_N_MEMBER_NAME_AS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_MEMBER_NAME_AS_LAST(P1, P2, P3, Seq, Count) index = num_params-Count; JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_MEMBER_NAME_AS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_MEMBER_NAME_AS_2(Member, Name) JSONCONS_N_MEMBER_NAME_AS_7(Member, Name,JSONCONS_RDWR,always_true(),,)  
#define JSONCONS_N_MEMBER_NAME_AS_3(Member, Name, Mode) Mode(JSONCONS_N_MEMBER_NAME_AS_7(Member, Name, Mode,always_true(),,))
#define JSONCONS_N_MEMBER_NAME_AS_4(Member, Name, Mode, Match) JSONCONS_N_MEMBER_NAME_AS_7(Member, Name, Mode, Match,,)
#define JSONCONS_N_MEMBER_NAME_AS_5(Member, Name, Mode, Match, Into) JSONCONS_N_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into,)
#define JSONCONS_N_MEMBER_NAME_AS_6(Member, Name, Mode, Match, Into, From) JSONCONS_N_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into, From)
#define JSONCONS_N_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into, From) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into(class_instance.Member))>::type>(aset, ajson, Name); \
  if (result && !Match(From(* result))) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} \
  Mode(JSONCONS_N_MEMBER_NAME_AS_8(Member, Name, Mode, Match, Into, From)) }
#define JSONCONS_N_MEMBER_NAME_AS_8(Member, Name, Mode, Match, Into, From) \
  if (result) { \
    set_member(From(std::move(*result)), class_instance.Member); \
  } \
  else if (index < num_mandatory_params2) {return result_type(jsoncons::unexpect, result.error().code(), class_name);} \
  else if (result.error().code() != conv_errc::missing_required_member){return result_type(jsoncons::unexpect, result.error().code(), class_name);} 

#define JSONCONS_ALL_MEMBER_NAME_AS(P1, P2, P3, Seq, Count) JSONCONS_ALL_MEMBER_NAME_AS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_ALL_MEMBER_NAME_AS_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_ALL_MEMBER_NAME_AS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_ALL_MEMBER_NAME_AS_2(Member, Name) JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name,JSONCONS_RDWR,always_true(),,)  
#define JSONCONS_ALL_MEMBER_NAME_AS_3(Member, Name, Mode) Mode(JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name,Mode,always_true(),,))
#define JSONCONS_ALL_MEMBER_NAME_AS_4(Member, Name, Mode, Match) JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name, Mode, Match,,)
#define JSONCONS_ALL_MEMBER_NAME_AS_5(Member, Name, Mode, Match, Into) JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into,)
#define JSONCONS_ALL_MEMBER_NAME_AS_6(Member, Name, Mode, Match, Into, From) JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into, From)
#define JSONCONS_ALL_MEMBER_NAME_AS_7(Member, Name, Mode, Match, Into, From) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into(class_instance.Member))>::type>(aset, ajson, Name); \
  if (result && !Match(From(* result))) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} \
  Mode(JSONCONS_ALL_MEMBER_NAME_AS_8(Member, Name, Mode, Match, Into, From)) }
#define JSONCONS_ALL_MEMBER_NAME_AS_8(Member, Name, Mode, Match, Into, From) \
  if (result) { \
    set_member(From(std::move(*result)), class_instance.Member); \
  } \
  else {return result_type(jsoncons::unexpect, result.error().code(), class_name);} 

#define JSONCONS_N_MEMBER_NAME_TO_JSON(P1, P2, P3, Seq, Count) JSONCONS_N_MEMBER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_MEMBER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_MEMBER_NAME_TO_JSON_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_MEMBER_NAME_TO_JSON_2(Member, Name) \
  {ajson.try_emplace(Name,class_instance.Member);} \
else \
  {json_traits_helper<Json>::set_optional_json_member(string_view_type(Name),class_instance.Member, ajson);}
#define JSONCONS_N_MEMBER_NAME_TO_JSON_3(Member, Name, Mode) JSONCONS_N_MEMBER_NAME_TO_JSON_2(Member, Name)
#define JSONCONS_N_MEMBER_NAME_TO_JSON_4(Member, Name, Mode, Match) JSONCONS_N_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match,,)
#define JSONCONS_N_MEMBER_NAME_TO_JSON_5(Member, Name, Mode, Match, Into) JSONCONS_N_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_N_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match, Into, From) \
  {ajson.try_emplace(Name, Into(class_instance.Member));} \
else \
  {json_traits_helper<Json>::set_optional_json_member(string_view_type(Name), Into(class_instance.Member), ajson);}

#define JSONCONS_ALL_MEMBER_NAME_TO_JSON(P1, P2, P3, Seq, Count) JSONCONS_ALL_MEMBER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_ALL_MEMBER_NAME_TO_JSON_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_2(Member, Name) ajson.try_emplace(Name,class_instance.Member);
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_3(Member, Name, Mode) JSONCONS_ALL_MEMBER_NAME_TO_JSON_2(Member, Name)
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_4(Member, Name, Mode, Match) JSONCONS_ALL_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match,,)
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_5(Member, Name, Mode, Match, Into) JSONCONS_ALL_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_ALL_MEMBER_NAME_TO_JSON_6(Member, Name, Mode, Match, Into, From) ajson.try_emplace(Name, Into(class_instance.Member));

#define JSONCONS_N_MEMBER_NAME_ENCODE(P1, P2, P3, Seq, Count) JSONCONS_N_MEMBER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_MEMBER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_MEMBER_NAME_ENCODE_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_MEMBER_NAME_ENCODE_2(Member, Name) \
    { \
        auto r = try_encode_member(string_view_type(Name), val.Member, encoder); \
        if (JSONCONS_UNLIKELY(!r)) {return r;} \
    } \
    else \
    { \
        auto r = try_encode_optional_member(string_view_type(Name), val.Member, encoder); \
        if (JSONCONS_UNLIKELY(!r)) {return r;} \
    }    
#define JSONCONS_N_MEMBER_NAME_ENCODE_3(Member, Name, Mode) JSONCONS_N_MEMBER_NAME_ENCODE_2(Member, Name)
#define JSONCONS_N_MEMBER_NAME_ENCODE_4(Member, Name, Mode, Match) JSONCONS_N_MEMBER_NAME_ENCODE_6(Member, Name, Mode, Match,,)
#define JSONCONS_N_MEMBER_NAME_ENCODE_5(Member, Name, Mode, Match, Into) JSONCONS_N_MEMBER_NAME_ENCODE_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_N_MEMBER_NAME_ENCODE_6(Member, Name, Mode, Match, Into, From) \
{ \
    auto r = try_encode_member(string_view_type(Name), Into(val.Member), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(string_view_type(Name), Into(val.Member), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
}    

#define JSONCONS_ALL_MEMBER_ENCODE_NAME(P1, P2, P3, Seq, Count) JSONCONS_ALL_MEMBER_ENCODE_NAME_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_ALL_MEMBER_ENCODE_NAME_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_2(Member, Name) \
     {auto r = try_encode_member(string_view_type(Name), val.Member, encoder); if (JSONCONS_UNLIKELY(!r)) {return r;}} 
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_3(Member, Name, Mode) JSONCONS_ALL_MEMBER_ENCODE_NAME_2(Member, Name)
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_4(Member, Name, Mode, Match) JSONCONS_ALL_MEMBER_ENCODE_NAME_6(Member, Name, Mode, Match,,)
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_5(Member, Name, Mode, Match, Into) JSONCONS_ALL_MEMBER_ENCODE_NAME_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_ALL_MEMBER_ENCODE_NAME_6(Member, Name, Mode, Match, Into, From) \
    {auto r = try_encode_member(string_view_type(Name), Into(val.Member), encoder); if (JSONCONS_UNLIKELY(!r)) {return r;}} 

#define JSONCONS_MEMBER_NAME_COUNT(P1, P2, P3, Seq, Count) JSONCONS_MEMBER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_MEMBER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_MEMBER_NAME_COUNT_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_MEMBER_NAME_COUNT_2(Member, Name) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Member)) \
    { \
        ++member_count; \
    } \
}    
#define JSONCONS_MEMBER_NAME_COUNT_3(Member, Name, Mode) JSONCONS_MEMBER_NAME_COUNT_2(Member, Name)
#define JSONCONS_MEMBER_NAME_COUNT_4(Member, Name, Mode, Match) JSONCONS_MEMBER_NAME_COUNT_6(Member, Name, Mode, Match,,)
#define JSONCONS_MEMBER_NAME_COUNT_5(Member, Name, Mode, Match, Into) JSONCONS_MEMBER_NAME_COUNT_6(Member, Name, Mode, Match, Into, )
#define JSONCONS_MEMBER_NAME_COUNT_6(Member, Name, Mode, Match, Into, From) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Member)) \
    { \
        ++member_count; \
    } \
}    

#define JSONCONS_MEMBER_NAME_TRAITS_BASE(ToJson,Encode, NumTemplateParams, ClassName,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_MEMBER_NAME_IS,,,, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            const char* class_name = # ClassName; \
            std::error_code ec; \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, # ClassName); \
            value_type class_instance = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator()); \
            if (num_params == num_mandatory_params2) \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_ALL_MEMBER_NAME_AS,,,, __VA_ARGS__) \
            } \
            else \
            { \
                std::size_t index = 0; \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_MEMBER_NAME_AS,,,, __VA_ARGS__) \
            } \
            return result_type(std::move(class_instance)); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(ToJson,,,, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::error_code ec; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_MEMBER_NAME_COUNT, ,,, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(Encode, ,,, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/


#define JSONCONS_N_MEMBER_NAME_TRAITS(ClassName,NumMandatoryParams, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_N_MEMBER_NAME_TO_JSON, JSONCONS_N_MEMBER_NAME_ENCODE, 0, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_N_MEMBER_NAME_TRAITS(NumTemplateParams, ClassName,NumMandatoryParams, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_N_MEMBER_NAME_TO_JSON, JSONCONS_N_MEMBER_NAME_ENCODE, NumTemplateParams, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_ALL_MEMBER_NAME_TRAITS(ClassName, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_ALL_MEMBER_NAME_TO_JSON, JSONCONS_ALL_MEMBER_ENCODE_NAME, 0, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_ALL_MEMBER_NAME_TRAITS(NumTemplateParams, ClassName, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_ALL_MEMBER_NAME_TO_JSON, JSONCONS_ALL_MEMBER_ENCODE_NAME, NumTemplateParams, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_CTOR_GETTER_IS(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_IS_LAST(Prefix, P2, P3, Getter, Count)
#define JSONCONS_CTOR_GETTER_IS_LAST(Prefix, P2, P3, Getter, Count) if ((num_params-Count) < num_mandatory_params1 && !ajson.contains(json_object_name_members<value_type>::Getter(char_type{}))) return false;

#define JSONCONS_CTOR_GETTER_GET(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_GET_LAST(Prefix, P2, P3, Getter, Count)
#define JSONCONS_CTOR_GETTER_GET_LAST(Prefix, P2, P3, Getter, Count) \
  auto _r ## Getter = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype((std::declval<value_type*>())->Getter())>::type>(aset, ajson, json_object_name_members<value_type>::Getter(char_type{})); \
  if (!_r ## Getter && (num_params-Count) < num_mandatory_params2) {return result_type(jsoncons::unexpect, _r ## Getter.error().code(), json_object_name_members<value_type>::Getter(unexpect));}

#define JSONCONS_CTOR_GETTER_AS(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_AS_LAST(Prefix, P2, P3, Getter, Count),
#define JSONCONS_CTOR_GETTER_AS_LAST(Prefix, P2, P3, Getter, Count) \
  _r ## Getter ? std::move(*_r ## Getter) : jsoncons::make_obj_using_allocator<typename std::decay<decltype((std::declval<value_type*>())->Getter())>::type>(aset.get_allocator())

#define JSONCONS_CTOR_GETTER_TO_JSON(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_TO_JSON_LAST(Prefix, P2, P3, Getter, Count)
#define JSONCONS_CTOR_GETTER_TO_JSON_LAST(Prefix, P2, P3, Getter, Count) \
if ((num_params-Count) < num_mandatory_params2) { \
       ajson.try_emplace(json_object_name_members<value_type>::Getter(char_type{}),class_instance.Getter()); \
  } \
else { \
  json_traits_helper<Json>::set_optional_json_member(json_object_name_members<value_type>::Getter(char_type{}),class_instance.Getter(), ajson); \
}

#define JSONCONS_CTOR_GETTER_COUNT(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_COUNT_LAST(Prefix, P2, P3, Getter, Count)
#define JSONCONS_CTOR_GETTER_COUNT_LAST(Prefix, P2, P3, Getter, Count) \
if ((num_params-Count) < num_mandatory_params2) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
} 

#define JSONCONS_CTOR_GETTER_ENCODE(Prefix, P2, P3, Getter, Count) JSONCONS_CTOR_GETTER_ENCODE_LAST(Prefix, P2, P3, Getter, Count)
#define JSONCONS_CTOR_GETTER_ENCODE_LAST(Prefix, P2, P3, Getter, Count) \
if ((num_params-Count) < num_mandatory_params2) \
{ \
    auto r = try_encode_member(json_object_name_members<value_type>::Getter(char_type{}), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(json_object_name_members<value_type>::Getter(char_type{}), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} 

#define JSONCONS_CTOR_GETTER_TRAITS_BASE(NumTemplateParams, ClassName,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct json_object_name_members<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        JSONCONS_VARIADIC_FOR_EACH(JSONCONS_GENERATE_NAME_STR,ClassName,,, __VA_ARGS__)\
    }; \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_IS, ,,, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, # ClassName); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_GET,ClassName,,, __VA_ARGS__) \
            return result_type(jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_AS, ,,, __VA_ARGS__) )); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_TO_JSON, ,,, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::error_code ec; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_COUNT, ,,, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_ENCODE, ,,, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/
 
#define JSONCONS_ALL_CTOR_GETTER_TRAITS(ClassName, ...)  \
    JSONCONS_CTOR_GETTER_TRAITS_BASE(0, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_TPL_ALL_CTOR_GETTER_TRAITS(NumTemplateParams, ClassName, ...)  \
    JSONCONS_CTOR_GETTER_TRAITS_BASE(NumTemplateParams, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_N_CTOR_GETTER_TRAITS(ClassName,NumMandatoryParams, ...)  \
    JSONCONS_CTOR_GETTER_TRAITS_BASE(0, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_N_ALL_CTOR_GETTER_TRAITS(NumTemplateParams, ClassName,NumMandatoryParams, ...)  \
    JSONCONS_CTOR_GETTER_TRAITS_BASE(NumTemplateParams, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_CTOR_GETTER_NAME_IS(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_IS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_IS_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params1 && JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_IS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_IS_2(Getter, Name) !ajson.contains(Name)) return false;
#define JSONCONS_CTOR_GETTER_NAME_IS_3(Getter, Name, Mode) JSONCONS_CTOR_GETTER_NAME_IS_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_IS_4(Getter, Name, Mode, Match) JSONCONS_CTOR_GETTER_NAME_IS_6(Getter, Name, Mode, Match, , )
#define JSONCONS_CTOR_GETTER_NAME_IS_5(Getter, Name, Mode, Match, Into) JSONCONS_CTOR_GETTER_NAME_IS_6(Getter, Name, Mode, Match, Into, )
#define JSONCONS_CTOR_GETTER_NAME_IS_6(Getter, Name, Mode, Match, Into, From) !ajson.contains(Name)) return false; \
    JSONCONS_TRY{if (!Match(ajson.at(Name).template as<typename std::decay<decltype(Into((std::declval<value_type*>())->Getter()))>::type>())) return false;} \
    JSONCONS_CATCH(...) {return false;}
 
#define JSONCONS_CTOR_GETTER_NAME_MATCH(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_MATCH_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_MATCH_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_MATCH_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_MATCH_2(Getter, Name) 
#define JSONCONS_CTOR_GETTER_NAME_MATCH_3(Getter, Name, Mode) 
#define JSONCONS_CTOR_GETTER_NAME_MATCH_4(Getter, Name, Mode, Match) JSONCONS_CTOR_GETTER_NAME_MATCH_6(Getter, Name, Mode, Match, , )
#define JSONCONS_CTOR_GETTER_NAME_MATCH_5(Getter, Name, Mode, Match, Into) JSONCONS_CTOR_GETTER_NAME_MATCH_6(Getter, Name, Mode, Match, Into, )
#define JSONCONS_CTOR_GETTER_NAME_MATCH_6(Getter, Name, Mode, Match, Into, From) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into((std::declval<value_type*>())->Getter()))>::type>(aset, ajson, Name); \
  if (result && !Match(* result)) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} \
}

#define JSONCONS_COMMA ,

#define JSONCONS_CTOR_GETTER_NAME_GET(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_GET_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_GET_LAST(P1, P2, P3, Seq, Count) index = num_params-Count; JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_GET_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_GET_2(Getter, Name) JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name,JSONCONS_RDWR,always_true(),,)
#define JSONCONS_CTOR_GETTER_NAME_GET_3(Getter, Name, Mode) Mode(JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name, Mode, always_true(),,))
#define JSONCONS_CTOR_GETTER_NAME_GET_4(Getter, Name, Mode, Match) Mode(JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name, Mode, Match,,))
#define JSONCONS_CTOR_GETTER_NAME_GET_5(Getter, Name, Mode, Match, Into) Mode(JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name, Mode, Match, Into,))
#define JSONCONS_CTOR_GETTER_NAME_GET_6(Getter, Name, Mode, Match, Into, From) Mode(JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name, Mode, Match, Into, From))
#define JSONCONS_CTOR_GETTER_NAME_GET_7(Getter, Name, Mode, Match, Into, From) \
  auto _r ## Getter = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into((std::declval<value_type*>())->Getter()))>::type>(aset, ajson, Name); \
  if (!_r ## Getter && index < num_mandatory_params2) {return result_type(jsoncons::unexpect, _r ## Getter.error().code(), class_name);} \
  if (_r ## Getter && !Match(* _r ## Getter)) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} 

#define JSONCONS_CTOR_GETTER_NAME_AS(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_AS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_AS_2(Getter, Name) JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name,,,, ) JSONCONS_COMMA
#define JSONCONS_CTOR_GETTER_NAME_AS_3(Getter, Name, Mode) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name, Mode,,, ) JSONCONS_COMMA) 
#define JSONCONS_CTOR_GETTER_NAME_AS_4(Getter, Name, Mode, Match) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name, Mode, Match,, ) JSONCONS_COMMA) 
#define JSONCONS_CTOR_GETTER_NAME_AS_5(Getter, Name, Mode, Match, Into) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name, Mode, Match, Into, ) JSONCONS_COMMA) 
#define JSONCONS_CTOR_GETTER_NAME_AS_6(Getter, Name, Mode, Match, Into, From) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter,Name,Mode,Match,Into,From) JSONCONS_COMMA)

#define JSONCONS_CTOR_GETTER_NAME_AS_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_AS_LAST_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_2(Getter, Name) JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name,,,,)
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_3(Getter, Name, Mode) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name,Mode,,,))
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_4(Getter, Name, Mode, Match) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_6(Getter, Name, Mode, Match,,))
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_5(Getter, Name, Mode, Match, Into) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_6(Getter, Name, Mode, Match, Into, ))
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_6(Getter, Name, Mode, Match, Into, From) Mode(JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name, Mode, Match, Into, From))
#define JSONCONS_CTOR_GETTER_NAME_AS_LAST_7(Getter, Name, Mode, Match, Into, From) \
  _r ## Getter ? From(std::move(*_r ## Getter)) : From(jsoncons::make_obj_using_allocator<typename std::decay<decltype(Into((std::declval<value_type*>())->Getter()))>::type>(aset.get_allocator()))

#define JSONCONS_CTOR_GETTER_NAME_TO_JSON(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_TO_JSON_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_2(Getter, Name) \
{ \
  ajson.try_emplace(Name,class_instance.Getter()); \
} \
else { \
  json_traits_helper<Json>::set_optional_json_member(string_view_type(Name),class_instance.Getter(), ajson); \
}
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_3(Getter, Name, Mode) JSONCONS_CTOR_GETTER_NAME_TO_JSON_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_4(Getter, Name, Mode, Match) JSONCONS_CTOR_GETTER_NAME_TO_JSON_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_5(Getter, Name, Mode, Match, Into) JSONCONS_CTOR_GETTER_NAME_TO_JSON_6(Getter, Name, Mode, Match, Into, )
#define JSONCONS_CTOR_GETTER_NAME_TO_JSON_6(Getter, Name, Mode, Match, Into, From) \
{ \
  ajson.try_emplace(Name, Into(class_instance.Getter()) ); \
} \
else { \
  json_traits_helper<Json>::set_optional_json_member(string_view_type(Name), Into(class_instance.Getter()), ajson); \
}

#define JSONCONS_CTOR_GETTER_NAME_COUNT(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_COUNT_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_COUNT_2(Getter, Name) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
}    
#define JSONCONS_CTOR_GETTER_NAME_COUNT_3(Getter, Name, Mode) JSONCONS_CTOR_GETTER_NAME_COUNT_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_COUNT_4(Getter, Name, Mode, Match) JSONCONS_CTOR_GETTER_NAME_COUNT_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_COUNT_5(Getter, Name, Mode, Match, Into) JSONCONS_CTOR_GETTER_NAME_COUNT_6(Getter, Name, Mode, Match, Into, )
#define JSONCONS_CTOR_GETTER_NAME_COUNT_6(Getter, Name, Mode, Match, Into, From) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
}    

#define JSONCONS_CTOR_GETTER_NAME_ENCODE(P1, P2, P3, Seq, Count) JSONCONS_CTOR_GETTER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_CTOR_GETTER_NAME_ENCODE_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_2(Getter, Name) \
{ \
    auto r = try_encode_member(string_view_type(Name), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(string_view_type(Name), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
}
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_3(Getter, Name, Mode) JSONCONS_CTOR_GETTER_NAME_ENCODE_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_4(Getter, Name, Mode, Match) JSONCONS_CTOR_GETTER_NAME_ENCODE_2(Getter, Name)
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_5(Getter, Name, Mode, Match, Into) JSONCONS_CTOR_GETTER_NAME_ENCODE_6(Getter, Name, Mode, Match, Into, )
#define JSONCONS_CTOR_GETTER_NAME_ENCODE_6(Getter, Name, Mode, Match, Into, From) \
{ \
    auto r = try_encode_member(string_view_type(Name), Into(val.Getter()), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(string_view_type(Name), Into(val.Getter()), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
}

#define JSONCONS_CTOR_GETTER_NAME_TRAITS_BASE(NumTemplateParams, ClassName,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_IS,,,, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            const char* class_name = # ClassName; \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, # ClassName); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_MATCH,,,, __VA_ARGS__)\
            std::size_t index = 0; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_GET,ClassName,,, __VA_ARGS__) \
            return result_type(jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_AS,,,, __VA_ARGS__))); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_TO_JSON,,,, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::error_code ec; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_COUNT,,,, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_CTOR_GETTER_NAME_ENCODE,,,, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/
                                                                       
#define JSONCONS_ALL_CTOR_GETTER_NAME_TRAITS(ClassName, ...)  \
    JSONCONS_CTOR_GETTER_NAME_TRAITS_BASE(0, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_TPL_ALL_CTOR_GETTER_NAME_TRAITS(NumTemplateParams, ClassName, ...)  \
    JSONCONS_CTOR_GETTER_NAME_TRAITS_BASE(NumTemplateParams, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_N_CTOR_GETTER_NAME_TRAITS(ClassName,NumMandatoryParams, ...)  \
    JSONCONS_CTOR_GETTER_NAME_TRAITS_BASE(0, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_TPL_N_CTOR_GETTER_NAME_TRAITS(NumTemplateParams, ClassName,NumMandatoryParams, ...)  \
JSONCONS_CTOR_GETTER_NAME_TRAITS_BASE(NumTemplateParams, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_ENUM_PAIR(Prefix, P2, P3, Member, Count) JSONCONS_ENUM_PAIR_LAST(Prefix, P2, P3, Member, Count),
#define JSONCONS_ENUM_PAIR_LAST(Prefix, P2, P3, Member, Count) {value_type::Member, json_object_name_members<value_type>::Member(char_type{})}

#define JSONCONS_ENUM_TRAITS_BASE(EnumType, ...)  \
namespace jsoncons { \
namespace reflect { \
    template<> \
    struct json_object_name_members<EnumType> \
    { \
        JSONCONS_VARIADIC_FOR_EACH(JSONCONS_GENERATE_NAME_STR, ,,, __VA_ARGS__)\
    }; \
    template<> \
    struct reflect_type_properties<EnumType> \
    { \
        using value_type = EnumType; \
        static constexpr std::size_t count = JSONCONS_NARGS(__VA_ARGS__); \
        template <typename CharT> \
        static const std::pair<EnumType,basic_string_view<CharT>>* values() \
        { \
            using char_type = CharT; \
            static const std::pair<EnumType,basic_string_view<CharT>> values[] = { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_ENUM_PAIR, ,,, __VA_ARGS__)\
            };\
            return values; \
        } \
    }; \
    template <typename Json> \
    struct json_conv_traits<Json, EnumType> \
    { \
        static_assert(std::is_enum<EnumType>::value, # EnumType " must be an enum"); \
        using value_type = EnumType; \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_type = std::basic_string<char_type>; \
        using string_view_type = basic_string_view<char_type>; \
        using mapped_type = std::pair<value_type,string_view_type>; \
        \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_string()) return false; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            auto rs = ajson.try_as_string_view(); \
            if (!rs) return false; \
            const string_view_type s = *rs; \
            if (s.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return true; \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == s; }); \
            return it != last; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& /*aset*/, const Json& ajson) \
        { \
            if (!is(ajson)) return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType); \
            auto rs = ajson.try_as_string_view(); \
            if (!rs) return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType); \
            const string_view_type s = *rs; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            if (s.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return value_type(); \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == s; }); \
            if (it == last) \
            { \
                if (s.empty()) \
                { \
                    return result_type(value_type()); \
                } \
                else \
                { \
                    return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType); \
                } \
            } \
            return result_type((*it).first); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, value_type class_instance) \
        { \
            static constexpr char_type empty_string[] = {0}; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            auto it = std::find_if(first, last, \
                                   [class_instance](const mapped_type& item) -> bool \
                                   { return item.first == class_instance; }); \
            if (it == last) \
            { \
                if (class_instance == value_type()) \
                { \
                    return Json(empty_string); \
                } \
                else \
                { \
                    JSONCONS_THROW(conv_error(conv_errc::conversion_failed, # EnumType)); \
                } \
            } \
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), (*it).second, semantic_tag::none); \
        } \
    }; \
    template <> struct encode_traits<EnumType> \
    { \
        using value_type = EnumType; \
        using result_type = conversion_result<value_type>; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            using mapped_type = std::pair<value_type,string_view_type>; \
            static const char_type empty_string[] = {0}; \
            std::error_code ec; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            auto it = std::find_if(first, last, \
                                   [val](const mapped_type& item) -> bool \
                                   { return item.first == val; }); \
            if (it == last) \
            { \
                if (val == value_type()) \
                { \
                    encoder.string_value(empty_string, semantic_tag::none, ser_context(), ec); \
                    if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
                    return write_result{}; \
                } \
                else \
                { \
                    return write_result{unexpect, conv_errc::conversion_failed}; \
                } \
            } \
            encoder.string_value((*it).second, semantic_tag::none, ser_context(), ec); \
            return write_result{}; \
        } \
    }; \
    template <> struct decode_traits<EnumType> \
    { \
        using value_type = EnumType; \
        using result_type = read_result<value_type>; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static result_type try_decode(const allocator_set<Alloc,TempAlloc>&, basic_staj_cursor<CharT>& cursor) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            using mapped_type = std::pair<value_type,string_view_type>; \
            std::error_code ec; \
            auto sv = cursor.current().template get<string_view_type>(ec); \
            if (ec) \
            { \
                return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType, cursor.line(), cursor.column()); \
            } \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            if (sv.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return value_type(); \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == sv; }); \
            if (it == last) \
            { \
                if (sv.empty()) \
                { \
                    return result_type(value_type()); \
                } \
                else \
                { \
                    return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType, cursor.line(), cursor.column()); \
                } \
            } \
            return result_type((*it).first); \
        } \
    }; \
} \
} \
    /**/

#define JSONCONS_ENUM_TRAITS(EnumType, ...)  \
    JSONCONS_ENUM_TRAITS_BASE(EnumType,__VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<EnumType> : public std::true_type {}; } \
  /**/

#define JSONCONS_NAME_ENUM_PAIR(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_NAME_ENUM_PAIR_ Seq),
#define JSONCONS_NAME_ENUM_PAIR_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_NAME_ENUM_PAIR_ Seq)
#define JSONCONS_NAME_ENUM_PAIR_(Member, Name) {value_type::Member, Name}

#define JSONCONS_ENUM_NAME_TRAITS(EnumType, ...)  \
namespace jsoncons { \
namespace reflect { \
    template<> \
    struct reflect_type_properties<EnumType> \
    { \
        using value_type = EnumType; \
        static constexpr std::size_t count = JSONCONS_NARGS(__VA_ARGS__); \
        template <typename CharT> \
        static const std::pair<EnumType,basic_string_view<CharT>>* values() \
        { \
            static const std::pair<EnumType,basic_string_view<CharT>> values[] = { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_NAME_ENUM_PAIR, ,,, __VA_ARGS__)\
            };\
            return values; \
        } \
    }; \
    template <typename Json> \
    struct json_conv_traits<Json, EnumType> \
    { \
        static_assert(std::is_enum<EnumType>::value, # EnumType " must be an enum"); \
        using value_type = EnumType; \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_type = std::basic_string<char_type>; \
        using string_view_type = basic_string_view<char_type>; \
        using mapped_type = std::pair<value_type,string_view_type>; \
        \
        static bool is(const Json& ajson) noexcept \
        { \
            auto rs = ajson.try_as_string_view(); \
            if (!rs) {return false;} \
            const string_view_type s = *rs; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            if (s.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return true; \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == s; }); \
            return it != last; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& /*aset*/, const Json& ajson) \
        { \
            auto rs = ajson.try_as_string_view(); \
            if (!rs) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType);} \
            const string_view_type s = *rs; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            if (s.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return result_type(value_type()); \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == s; }); \
            if (it == last) \
            { \
                if (s.empty()) \
                { \
                    return result_type(value_type()); \
                } \
                else \
                { \
                    return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType); \
                } \
            } \
            return (*it).first; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, value_type class_instance) \
        { \
            static constexpr char_type empty_string[] = {0}; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            auto it = std::find_if(first, last, \
                                   [class_instance](const mapped_type& item) -> bool \
                                   { return item.first == class_instance; }); \
            if (it == last) \
            { \
                if (class_instance == value_type()) \
                { \
                    return Json(empty_string); \
                } \
                else \
                { \
                    JSONCONS_THROW(conv_error(conv_errc::conversion_failed, # EnumType)); \
                } \
            } \
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), (*it).second, semantic_tag::none); \
        } \
    }; \
    template <> struct encode_traits<EnumType> \
    { \
        using value_type = EnumType; \
        using result_type = conversion_result<value_type>; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            using mapped_type = std::pair<value_type,string_view_type>; \
            static const char_type empty_string[] = {0}; \
            std::error_code ec; \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            auto it = std::find_if(first, last, \
                                   [val](const mapped_type& item) -> bool \
                                   { return item.first == val; }); \
            if (it == last) \
            { \
                if (val == value_type()) \
                { \
                    encoder.string_value(empty_string, semantic_tag::none, ser_context(), ec); \
                    if (JSONCONS_UNLIKELY(ec)) return write_result{unexpect, ec}; \
                    return write_result{}; \
                } \
                else \
                { \
                    return write_result{unexpect, conv_errc::conversion_failed}; \
                } \
            } \
            encoder.string_value((*it).second, semantic_tag::none, ser_context(), ec); \
            return write_result{}; \
        } \
    }; \
    template <> struct decode_traits<EnumType> \
    { \
        using value_type = EnumType; \
        using result_type = read_result<value_type>; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static result_type try_decode(const allocator_set<Alloc,TempAlloc>&, basic_staj_cursor<CharT>& cursor) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            using mapped_type = std::pair<value_type,string_view_type>; \
            std::error_code ec; \
            auto sv = cursor.current().template get<string_view_type>(ec); \
            if (ec) \
            { \
                return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType, cursor.line(), cursor.column()); \
            } \
            auto first = reflect_type_properties<value_type>::values<char_type>(); \
            auto last = first + reflect_type_properties<value_type>::count; \
            if (sv.empty() && std::find_if(first, last, \
                                          [](const mapped_type& item) -> bool \
                                          { return item.first == value_type(); }) == last) \
            { \
                return value_type(); \
            } \
            auto it = std::find_if(first, last, \
                                   [&](const mapped_type& item) -> bool \
                                   { return item.second == sv; }); \
            if (it == last) \
            { \
                if (sv.empty()) \
                { \
                    return result_type(value_type()); \
                } \
                else \
                { \
                    return result_type(jsoncons::unexpect, conv_errc::conversion_failed, # EnumType, cursor.line(), cursor.column()); \
                } \
            } \
            return result_type((*it).first); \
        } \
    }; \
} \
    template <> struct is_json_type_traits_declared<EnumType> : public std::true_type {}; \
} \
    /**/

#define JSONCONS_N_GETTER_SETTER_AS(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_AS_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_AS_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_AS_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count)  
#define JSONCONS_N_GETTER_SETTER_AS_(Prefix, Getter, Setter, Property, Count) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(class_instance.Getter())>::type>(aset, ajson, json_object_name_members<value_type>::Property(char_type{})); \
  if (result) {class_instance.Setter(std::move(* result));} \
  else if ((num_params-Count) < num_mandatory_params2) {return result_type(jsoncons::unexpect, result.error().code(), # Prefix);} \
  else if (result.error().code() != conv_errc::missing_required_member){return result_type(jsoncons::unexpect, result.error().code(), # Prefix);} \
}

#define JSONCONS_ALL_GETTER_SETTER_AS(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_ALL_GETTER_SETTER_AS_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_ALL_GETTER_SETTER_AS_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_ALL_GETTER_SETTER_AS_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_ALL_GETTER_SETTER_AS_(Prefix, Getter, Setter, Property, Count) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(class_instance.Getter())>::type>(aset, ajson, json_object_name_members<value_type>::Property(char_type{})); \
  if (!result) {return result_type(jsoncons::unexpect, result.error().code(), # Prefix "::" # Property);} \
  class_instance.Setter(std::move(* result)); \
}

#define JSONCONS_N_GETTER_SETTER_TO_JSON(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_TO_JSON_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_TO_JSON_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_TO_JSON_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_TO_JSON_(Prefix, Getter, Setter, Property, Count) \
if ((num_params-Count) < num_mandatory_params2) \
  {ajson.try_emplace(json_object_name_members<value_type>::Property(char_type{}),class_instance.Getter());} \
else \
  {json_traits_helper<Json>::set_optional_json_member(json_object_name_members<value_type>::Property(char_type{}),class_instance.Getter(), ajson);}

#define JSONCONS_ALL_GETTER_SETTER_TO_JSON(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_ALL_GETTER_SETTER_TO_JSON_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_ALL_GETTER_SETTER_TO_JSON_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_ALL_GETTER_SETTER_TO_JSON_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_ALL_GETTER_SETTER_TO_JSON_(Prefix, Getter, Setter, Property, Count) ajson.try_emplace(json_object_name_members<value_type>::Property(char_type{}),class_instance.Getter());

#define JSONCONS_N_GETTER_SETTER_COUNT(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_COUNT_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_COUNT_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_COUNT_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_COUNT_(Prefix, Getter, Setter, Property, Count) \
if ((num_params-Count) < num_mandatory_params2) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
} 

#define JSONCONS_N_GETTER_SETTER_ENCODE(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_ENCODE_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_ENCODE_LAST(Prefix, GetPrefix, SetPrefix, Property, Count) JSONCONS_N_GETTER_SETTER_ENCODE_(Prefix, GetPrefix ## Property, SetPrefix ## Property, Property, Count) 
#define JSONCONS_N_GETTER_SETTER_ENCODE_(Prefix, Getter, Setter, Property, Count) \
if ((num_params-Count) < num_mandatory_params2) \
{ \
    auto r = try_encode_member(json_object_name_members<value_type>::Property(char_type{}), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(json_object_name_members<value_type>::Property(char_type{}), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} 

#define JSONCONS_GETTER_SETTER_TRAITS_BASE(ToJson,NumTemplateParams, ClassName,GetPrefix,SetPrefix,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct json_object_name_members<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        JSONCONS_VARIADIC_FOR_EACH(JSONCONS_GENERATE_NAME_STR, ,,, __VA_ARGS__)\
    }; \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_MEMBER_IS, ,GetPrefix,SetPrefix, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, # ClassName); \
            value_type class_instance = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator()); \
            if (num_params == num_mandatory_params2) \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_ALL_GETTER_SETTER_AS,ClassName,GetPrefix,SetPrefix, __VA_ARGS__) \
            } \
            else \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_AS,ClassName,GetPrefix,SetPrefix, __VA_ARGS__) \
            } \
            return result_type(std::move(class_instance)); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(ToJson, ,GetPrefix,SetPrefix, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::error_code ec; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_COUNT, ,GetPrefix,SetPrefix, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_ENCODE, ,GetPrefix,SetPrefix, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/

#define JSONCONS_N_GETTER_SETTER_TRAITS(ClassName,GetPrefix,SetPrefix,NumMandatoryParams, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_N_GETTER_SETTER_TO_JSON,0, ClassName,GetPrefix,SetPrefix,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_N_GETTER_SETTER_TRAITS(NumTemplateParams, ClassName,GetPrefix,SetPrefix,NumMandatoryParams, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_N_GETTER_SETTER_TO_JSON,NumTemplateParams, ClassName,GetPrefix,SetPrefix,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_ALL_GETTER_SETTER_TRAITS(ClassName,GetPrefix,SetPrefix, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_ALL_GETTER_SETTER_TO_JSON,0,ClassName,GetPrefix,SetPrefix, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__),__VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/

#define JSONCONS_TPL_ALL_GETTER_SETTER_TRAITS(NumTemplateParams, ClassName,GetPrefix,SetPrefix, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_ALL_GETTER_SETTER_TO_JSON,NumTemplateParams,ClassName,GetPrefix,SetPrefix, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__),__VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_GETTER_SETTER_NAME_IS(P1, P2, P3, Seq, Count) JSONCONS_GETTER_SETTER_NAME_IS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_GETTER_SETTER_NAME_IS_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params1 && JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_GETTER_SETTER_NAME_IS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_GETTER_SETTER_NAME_IS_3(Getter, Setter, Name) !ajson.contains(Name)) return false;
#define JSONCONS_GETTER_SETTER_NAME_IS_5(Getter, Setter, Name, Mode, Match) JSONCONS_GETTER_SETTER_NAME_IS_7(Getter, Setter, Name, Mode, Match,, )
#define JSONCONS_GETTER_SETTER_NAME_IS_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_GETTER_SETTER_NAME_IS_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_GETTER_SETTER_NAME_IS_7(Getter, Setter, Name, Mode, Match, Into, From) !ajson.contains(Name)) return false; \
    JSONCONS_TRY{if (!Match(ajson.at(Name).template as<typename std::decay<decltype(Into((std::declval<value_type*>())->Getter()))>::type>())) return false;} \
    JSONCONS_CATCH(...) {return false;}

#define JSONCONS_N_GETTER_SETTER_NAME_AS(P1, P2, P3, Seq, Count) JSONCONS_N_GETTER_SETTER_NAME_AS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_GETTER_SETTER_NAME_AS_LAST(P1, P2, P3, Seq, Count) index = num_params-Count; JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_GETTER_SETTER_NAME_AS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_GETTER_SETTER_NAME_AS_3(Getter, Setter, Name) JSONCONS_N_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name,JSONCONS_RDWR,always_true(),,)
#define JSONCONS_N_GETTER_SETTER_NAME_AS_4(Getter, Setter, Name, Mode) Mode(JSONCONS_N_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, always_true(),,))
#define JSONCONS_N_GETTER_SETTER_NAME_AS_5(Getter, Setter, Name, Mode, Match) JSONCONS_N_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match, , )
#define JSONCONS_N_GETTER_SETTER_NAME_AS_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_N_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_N_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match, Into, From) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into(class_instance.Getter()))>::type>(aset, ajson, Name); \
  if (result && !Match(From(* result))) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} \
  Mode(JSONCONS_N_GETTER_SETTER_NAME_AS_8(Getter, Setter, Name, Mode, Match, Into, From)) \
}
#define JSONCONS_N_GETTER_SETTER_NAME_AS_8(Getter, Setter, Name, Mode, Match, Into, From) \
  if (result) { \
    class_instance.Setter(From(std::move(* result))); \
  } \
  else if (index < num_mandatory_params2) {return result_type(jsoncons::unexpect, result.error().code(), class_name);} \
  else if (result.error().code() != conv_errc::missing_required_member){return result_type(jsoncons::unexpect, result.error().code(), class_name);} 

#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON(P1, P2, P3, Seq, Count) JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_3(Getter, Setter, Name) \
    ajson.try_emplace(Name,class_instance.Getter());
#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_5(Getter, Setter, Name, Mode, Match) JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, , )
#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_N_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, Into, From) \
    ajson.try_emplace(Name, Into(class_instance.Getter()) );

#define JSONCONS_ALL_GETTER_SETTER_NAME_AS(P1, P2, P3, Seq, Count) JSONCONS_ALL_GETTER_SETTER_NAME_AS_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_LAST(P1, P2, P3, Seq, Count) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_ALL_GETTER_SETTER_NAME_AS_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_3(Getter, Setter, Name) JSONCONS_ALL_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name,JSONCONS_RDWR, always_true(),,)
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_4(Getter, Setter, Name, Mode) Mode(JSONCONS_ALL_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name,Mode, always_true(),,))
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_5(Getter, Setter, Name, Mode, Match) JSONCONS_ALL_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match,,)
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_ALL_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match, Into,)
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_7(Getter, Setter, Name, Mode, Match, Into, From) { \
  auto result = json_traits_helper<Json>::template try_get_member<typename std::decay<decltype(Into(class_instance.Getter()))>::type>(aset, ajson, Name); \
  if (result && !Match(From(* result))) {return result_type(jsoncons::unexpect, conv_errc::conversion_failed, class_name);} \
  Mode(JSONCONS_ALL_GETTER_SETTER_NAME_AS_8(Getter, Setter, Name, Mode, Match, Into, From)) \
}
#define JSONCONS_ALL_GETTER_SETTER_NAME_AS_8(Getter, Setter, Name, Mode, Match, Into, From) \
  if (result) { \
    class_instance.Setter(From(std::move(* result))); \
  } \
  else {return result_type(jsoncons::unexpect, result.error().code(), class_name);} 

#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON(P1, P2, P3, Seq, Count) JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_3(Getter, Setter, Name) \
  ajson.try_emplace(Name,class_instance.Getter()); \
else \
  {json_traits_helper<Json>::set_optional_json_member(string_view_type(Name),class_instance.Getter(), ajson);}
#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_5(Getter, Setter, Name, Mode, Match) JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, , )
#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON_7(Getter, Setter, Name, Mode, Match, Into, From) \
  ajson.try_emplace(Name, Into(class_instance.Getter())); \
else \
  {json_traits_helper<Json>::set_optional_json_member(string_view_type(Name), Into(class_instance.Getter()), ajson);}
 
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT(P1, P2, P3, Seq, Count) JSONCONS_N_GETTER_SETTER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_GETTER_SETTER_NAME_COUNT_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT_3(Getter, Setter, Name) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
}    
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT_5(Getter, Setter, Name, Mode, Match) JSONCONS_N_GETTER_SETTER_NAME_COUNT_7(Getter, Setter, Name, Mode, Match, , )
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_N_GETTER_SETTER_NAME_COUNT_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_N_GETTER_SETTER_NAME_COUNT_7(Getter, Setter, Name, Mode, Match, Into, From) \
{ \
    ++member_count; \
} \
else \
{ \
    if (is_optional_value_set(val.Getter())) \
    { \
        ++member_count; \
    } \
}    

#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE(P1, P2, P3, Seq, Count) JSONCONS_N_GETTER_SETTER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count)
#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE_LAST(P1, P2, P3, Seq, Count) if ((num_params-Count) < num_mandatory_params2) JSONCONS_PP_EXPAND(JSONCONS_PP_CONCAT(JSONCONS_N_GETTER_SETTER_NAME_ENCODE_,JSONCONS_NARGS Seq) Seq)
#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE_3(Getter, Setter, Name) \
{ \
    auto r = try_encode_member(string_view_type(Name), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(string_view_type(Name), val.Getter(), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
}
 
#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE_5(Getter, Setter, Name, Mode, Match) JSONCONS_N_GETTER_SETTER_NAME_ENCODE_7(Getter, Setter, Name, Mode, Match, , )
#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE_6(Getter, Setter, Name, Mode, Match, Into) JSONCONS_N_GETTER_SETTER_NAME_ENCODE_7(Getter, Setter, Name, Mode, Match, Into, )
#define JSONCONS_N_GETTER_SETTER_NAME_ENCODE_7(Getter, Setter, Name, Mode, Match, Into, From) \
{ \
    auto r = try_encode_member(string_view_type(Name), Into(val.Getter()), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
} \
else \
{ \
    auto r = try_encode_optional_member(string_view_type(Name), Into(val.Getter()), encoder); \
    if (JSONCONS_UNLIKELY(!r)) {return r;} \
}

#define JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(ToJson, NumTemplateParams, ClassName,NumMandatoryParams1,NumMandatoryParams2, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <typename Json JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_MORE_TPL_PARAM, NumTemplateParams)> \
    struct json_conv_traits<Json, ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        using result_type = conversion_result<value_type>; \
        using char_type = typename Json::char_type; \
        using string_view_type = typename Json::string_view_type; \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        static bool is(const Json& ajson) noexcept \
        { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_GETTER_SETTER_NAME_IS,,,, __VA_ARGS__)\
            return true; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) \
        { \
            const char* class_name = # ClassName; \
            std::error_code ec; \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map, class_name); \
            value_type class_instance = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator()); \
            if (num_params == num_mandatory_params2) \
            { \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_ALL_GETTER_SETTER_NAME_AS,,,, __VA_ARGS__) \
            } \
            else \
            { \
                std::size_t index = 0; \
                JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_NAME_AS,,,, __VA_ARGS__) \
            } \
            return result_type(std::move(class_instance)); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& class_instance) \
        { \
            Json ajson = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none); \
            JSONCONS_VARIADIC_FOR_EACH(ToJson,,,, __VA_ARGS__) \
            return ajson; \
        } \
    }; \
    template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> \
    struct encode_traits<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> \
    { \
        using value_type = ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams); \
        constexpr static size_t num_params = JSONCONS_NARGS(__VA_ARGS__); \
        constexpr static size_t num_mandatory_params1 = NumMandatoryParams1; \
        constexpr static size_t num_mandatory_params2 = NumMandatoryParams2; \
        template <typename CharT,typename Alloc,typename TempAlloc> \
        static write_result try_encode(const allocator_set<Alloc,TempAlloc>&, const value_type& val, \
            basic_json_visitor<CharT>& encoder) \
        { \
            using char_type = CharT; \
            using string_view_type = basic_string_view<char_type>; \
            (void)num_params; (void)num_mandatory_params1; (void)num_mandatory_params2; \
            std::error_code ec; \
            std::size_t member_count{0}; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_NAME_COUNT,,,, __VA_ARGS__) \
            encoder.begin_object(member_count, semantic_tag::none, ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_N_GETTER_SETTER_NAME_ENCODE,,,, __VA_ARGS__) \
            encoder.end_object(ser_context(), ec); \
            if (JSONCONS_UNLIKELY(ec)) {return write_result{unexpect, ec};} \
            return write_result{}; \
        } \
    }; \
} \
} \
  /**/
 
#define JSONCONS_N_GETTER_SETTER_NAME_TRAITS(ClassName,NumMandatoryParams, ...)  \
    JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_N_GETTER_SETTER_NAME_TO_JSON, 0, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_TPL_N_GETTER_SETTER_NAME_TRAITS(NumTemplateParams, ClassName,NumMandatoryParams, ...)  \
    JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_N_GETTER_SETTER_NAME_TO_JSON, NumTemplateParams, ClassName,NumMandatoryParams,NumMandatoryParams, __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_ALL_GETTER_SETTER_NAME_TRAITS(ClassName, ...)  \
    JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON, 0, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <> struct is_json_type_traits_declared<ClassName> : public std::true_type {}; } \
  /**/
 
#define JSONCONS_TPL_ALL_GETTER_SETTER_NAME_TRAITS(NumTemplateParams, ClassName, ...)  \
    JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_ALL_GETTER_SETTER_NAME_TO_JSON, NumTemplateParams, ClassName, JSONCONS_NARGS(__VA_ARGS__), JSONCONS_NARGS(__VA_ARGS__), __VA_ARGS__) \
    namespace jsoncons { template <JSONCONS_GENERATE_TPL_PARAMS(JSONCONS_GENERATE_TPL_PARAM, NumTemplateParams)> struct is_json_type_traits_declared<ClassName JSONCONS_GENERATE_TPL_ARGS(JSONCONS_GENERATE_TPL_ARG, NumTemplateParams)> : public std::true_type {}; } \
  /**/

#define JSONCONS_POLYMORPHIC_IS(BaseClass, P2, P3, DerivedClass, Count) if (ajson.template is<DerivedClass>()) return true;
#define JSONCONS_POLYMORPHIC_IS_LAST(BaseClass, P2, P3, DerivedClass, Count)  if (ajson.template is<DerivedClass>()) return true;

#define JSONCONS_POLYMORPHIC_AS_UNIQUE_PTR(BaseClass, P2, P3, DerivedClass, Count) { \
  auto result = ajson.template try_as<DerivedClass>(aset); \
  if (result) { \
  using rebind = typename std::allocator_traits<Alloc>::template rebind_alloc<DerivedClass>; \
  auto alloc = rebind(aset.get_allocator()); \
  auto* ptr = alloc.allocate(1); \
  JSONCONS_TRY {ptr = new(ptr) DerivedClass(*result);} JSONCONS_CATCH(...) {alloc.deallocate(ptr,1); throw;} \
  return result_type{jsoncons::in_place, ptr, jsoncons::make_obj_using_allocator<Deleter>(alloc)};} \
} /**/

#define JSONCONS_POLYMORPHIC_AS_UNIQUE_PTR_LAST(BaseClass, P2, P3, DerivedClass, Count) { \
  auto result = ajson.template try_as<DerivedClass>(aset); \
  if (result) { \
  using rebind = typename std::allocator_traits<Alloc>::template rebind_alloc<DerivedClass>; \
  auto alloc = rebind(aset.get_allocator()); \
  auto* ptr = alloc.allocate(1); \
  JSONCONS_TRY {ptr = new(ptr) DerivedClass(*result);} JSONCONS_CATCH(...) {alloc.deallocate(ptr,1); throw;} \
  return result_type{jsoncons::in_place, ptr, jsoncons::make_obj_using_allocator<Deleter>(alloc)};} \
} /**/

#define JSONCONS_POLYMORPHIC_AS_SHARED_PTR(BaseClass, P2, P3, DerivedClass, Count) { \
  auto result = ajson.template try_as<DerivedClass>(aset); \
  if (result) {return result_type(std::allocate_shared<DerivedClass>(aset.get_allocator(), std::move(*result)));} \
} /**/

#define JSONCONS_POLYMORPHIC_AS_SHARED_PTR_LAST(BaseClass, P2, P3, DerivedClass, Count) { \
  auto result = ajson.template try_as<DerivedClass>(aset); \
  if (result) {return result_type(std::allocate_shared<DerivedClass>(aset.get_allocator(), std::move(*result)));} \
} /**/
 
#define JSONCONS_POLYMORPHIC_TO_JSON(BaseClass, P2, P3, DerivedClass, Count) if (DerivedClass* p = dynamic_cast<DerivedClass*>(ptr.get())) {return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), *p);}
#define JSONCONS_POLYMORPHIC_TO_JSON_LAST(BaseClass, P2, P3, DerivedClass, Count) if (DerivedClass* p = dynamic_cast<DerivedClass*>(ptr.get())) {return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), *p);}

#define JSONCONS_POLYMORPHIC_TRAITS(BaseClass, ...)  \
namespace jsoncons { \
namespace reflect { \
    template <typename Json> \
    struct json_conv_traits<Json, std::shared_ptr<BaseClass>> { \
        using value_type = std::shared_ptr<BaseClass>; \
        using result_type = conversion_result<value_type>; \
        static bool is(const Json& ajson) noexcept { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_IS, BaseClass,,, __VA_ARGS__)\
            return false; \
        } \
\
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) { \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_AS_SHARED_PTR, BaseClass,,, __VA_ARGS__)\
            return result_type(jsoncons::unexpect, conv_errc::conversion_failed); \
        } \
\
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& ptr) { \
            if (ptr.get() == nullptr) {return Json::null();} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_TO_JSON, BaseClass,,, __VA_ARGS__)\
            return Json::null(); \
        } \
    }; \
    template <typename Json,typename Deleter> \
    struct json_conv_traits<Json, std::unique_ptr<BaseClass,Deleter>> { \
        using value_type = std::unique_ptr<BaseClass,Deleter>; \
        using result_type = conversion_result<value_type>; \
        static bool is(const Json& ajson) noexcept { \
            if (!ajson.is_object()) return false; \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_IS, BaseClass,,, __VA_ARGS__)\
            return false; \
        } \
        template <typename Alloc,typename TempAlloc> \
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& ajson) { \
            if (!ajson.is_object()) return result_type(jsoncons::unexpect, conv_errc::not_map); \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_AS_UNIQUE_PTR, BaseClass,,, __VA_ARGS__)\
            return result_type(jsoncons::unexpect, conv_errc::conversion_failed); \
        } \
        template <typename Alloc,typename TempAlloc> \
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const value_type& ptr) { \
            if (ptr.get() == nullptr) {return Json::null();} \
            JSONCONS_VARIADIC_FOR_EACH(JSONCONS_POLYMORPHIC_TO_JSON, BaseClass,,, __VA_ARGS__)\
            return Json::null(); \
        } \
    }; \
} \
} \
  /**/

#endif // JSONCONS_REFLECT_REFLECT_TRAITS_GEN_HPP
