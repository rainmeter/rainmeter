// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_TYPE_TRAITS_HPP
#define JSONCONS_JSON_TYPE_TRAITS_HPP

#include <algorithm> // std::swap
#include <array>
#include <bitset> // std::bitset
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator> // std::iterator_traits, std::input_iterator_tag
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits> // std::enable_if
#include <utility>
#include <valarray>
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/conversion.hpp>

#if defined(JSONCONS_HAS_STD_VARIANT)
  #include <variant>
#endif

namespace jsoncons {

template <typename T>
struct is_json_type_traits_declared : public std::false_type
{};

// json_type_traits 

template <typename T>
struct unimplemented : std::false_type
{};

template <typename Json,typename T,typename Enable=void>
struct json_type_traits
{
    using allocator_type = typename Json::allocator_type;

    static constexpr bool is_compatible = false;

    static constexpr bool is(const Json&) noexcept
    {
        return false;
    }

    static T as(const Json&)
    {
        static_assert(unimplemented<T>::value, "as not implemented");
    }

    static Json to_json(const T&)
    {
        static_assert(unimplemented<T>::value, "to_json not implemented");
    }

    static Json to_json(const T&, const allocator_type&)
    {
        static_assert(unimplemented<T>::value, "to_json not implemented");
    }
};

namespace detail {

// is_json_type_traits_unspecialized
template <typename Json,typename T,typename Enable = void>
struct is_json_type_traits_unspecialized : std::false_type {};

// is_json_type_traits_unspecialized
template <typename Json,typename T>
struct is_json_type_traits_unspecialized<Json,T,
    typename std::enable_if<!std::integral_constant<bool, json_type_traits<Json, T>::is_compatible>::value>::type
> : std::true_type {};

} // namespace detail 

// is_json_type_traits_specialized
template <typename Json,typename T,typename Enable=void>
struct is_json_type_traits_specialized : std::false_type {};

template <typename Json,typename T>
struct is_json_type_traits_specialized<Json,T, 
    typename std::enable_if<!jsoncons::detail::is_json_type_traits_unspecialized<Json,T>::value
>::type> : std::true_type {};

} // namespace jsoncons

#endif // JSONCONS_JSON_TYPE_TRAITS_HPP
