// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP
#define JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP

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
#include <jsoncons/conversion_result.hpp>
#include <jsoncons/json_type_traits.hpp>

#if defined(JSONCONS_HAS_STD_VARIANT)
  #include <variant>
#endif

namespace jsoncons {
namespace reflect {

    template <typename T>
    struct is_json_conv_traits_declared : public is_json_type_traits_declared<T>
    {};

    // json_conv_traits

    template <typename T>
    struct unimplemented : std::false_type
    {};

    template <typename Json,typename T,typename Enable=void>
    struct json_conv_traits
    {
        using result_type = conversion_result<T>;

        static constexpr bool is_compatible = false;

        static constexpr bool is(const Json& j) noexcept
        {
            return json_type_traits<Json,T>::is(j);
        }

        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            
            JSONCONS_TRY
            {
                return result_type(json_type_traits<Json,T>::as(j));
            }
            JSONCONS_CATCH (...)
            {
                return result_type(jsoncons::unexpect, conv_errc::conversion_failed );
            }
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return json_type_traits<Json,T>::to_json(val, aset.get_allocator());
        }
    };

namespace detail {

template <typename Json,typename T>
using
traits_can_convert_t = decltype(json_conv_traits<Json,T>::can_convert(Json()));

template <typename Json,typename T>
using
has_can_convert = ext_traits::is_detected<traits_can_convert_t, Json, T>;

    template <typename T>
    struct invoke_can_convert
    {
        template <typename Json>
        static 
        typename std::enable_if<has_can_convert<Json,T>::value,bool>::type
        can_convert(const Json& j) noexcept
        {
            return json_conv_traits<Json,T>::can_convert(j);
        }
        template <typename Json>
        static 
        typename std::enable_if<!has_can_convert<Json,T>::value,bool>::type
        can_convert(const Json& j) noexcept
        {
            return json_conv_traits<Json,T>::is(j);
        }
    };

    // is_json_conv_traits_unspecialized
    template <typename Json,typename T,typename Enable = void>
    struct is_json_conv_traits_unspecialized : std::false_type {};

    // is_json_conv_traits_unspecialized
    template <typename Json,typename T>
    struct is_json_conv_traits_unspecialized<Json,T,
        typename std::enable_if<!std::integral_constant<bool, json_conv_traits<Json, T>::is_compatible>::value>::type
    > : std::true_type {};

    // is_compatible_array_type
    template <typename Json,typename T,typename Enable=void>
    struct is_compatible_array_type : std::false_type {};

    template <typename Json,typename T>
    struct is_compatible_array_type<Json,T, 
        typename std::enable_if<!std::is_same<T,typename Json::array>::value &&
        ext_traits::is_array_like<T>::value 
    >::type> : std::true_type {};

} // namespace detail

    // is_json_conv_traits_specialized
    template <typename Json,typename T,typename Enable=void>
    struct is_json_conv_traits_specialized : is_json_type_traits_specialized<Json,T> {};

    template <typename Json,typename T>
    struct is_json_conv_traits_specialized<Json,T, 
        typename std::enable_if<!jsoncons::reflect::detail::is_json_conv_traits_unspecialized<Json,T>::value
    >::type> : std::true_type {};

    template <typename Json>
    struct json_conv_traits<Json, const typename std::decay<typename Json::char_type>::type*>
    {
        using char_type = typename Json::char_type;
        using result_type = conversion_result<const char_type*>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type{j.as_cstring()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const char_type* s)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), s, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename std::decay<typename Json::char_type>::type*>
    {
        using char_type = typename Json::char_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const char_type* s)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), s, semantic_tag::none);
        }
    };

    // enum

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<!is_json_conv_traits_declared<T>::value && std::is_enum<T>::value 
    >::type>
    {
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.template is_integer<T>();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            auto r = j.template try_as_integer<int64_t>();
            if (r)
            {
                return result_type{static_cast<T>(*r)};
            }
            return result_type{unexpect, r.error()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, T val)
        {
            return Json(static_cast<int64_t>(val), semantic_tag::none);
        }
    };

    // integer

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<(ext_traits::is_signed_integer<T>::value && sizeof(T) <= sizeof(int64_t)) || (ext_traits::is_unsigned_integer<T>::value && sizeof(T) <= sizeof(uint64_t)) 
    >::type>
    {
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.template is_integer<T>();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return j.template try_as_integer<T>();
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, T val)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<(ext_traits::is_signed_integer<T>::value && sizeof(T) > sizeof(int64_t)) || (ext_traits::is_unsigned_integer<T>::value && sizeof(T) > sizeof(uint64_t)) 
    >::type>
    {
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.template is_integer<T>();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return j.template try_as_integer<T>();
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, T val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val, semantic_tag::none);
        }
    };

    template <typename Json, typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<std::is_floating_point<T>::value
    >::type>
    {
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_double();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            auto result = j.try_as_double();
            if (JSONCONS_UNLIKELY(!result))
            {
                return result_type(jsoncons::unexpect, result.error().code());
            }
            return result_type(static_cast<T>(*result));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, T val)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename Json::object>
    {
        using json_object = typename Json::object;

        static bool is(const Json& j) noexcept
        {
            return j.is_object();
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, const json_object& o)
        {
            return Json(o,semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json,typename Json::array>
    {
        using json_array = typename Json::array;

        static bool is(const Json& j) noexcept
        {
            return j.is_array();
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, const json_array& a)
        {
            return Json(a, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, Json>
    {
        using result_type = conversion_result<Json>;

        static bool is(const Json&) noexcept
        {
            return true;
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type{j};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), j);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, jsoncons::null_type>
    {
        using result_type = conversion_result<typename jsoncons::null_type>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            if (!j.is_null())
            {
                return result_type{jsoncons::unexpect, conv_errc::not_jsoncons_null_type};
            }
            return result_type{jsoncons::null_type()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, jsoncons::null_type)
        {
            return Json(jsoncons::null_type{}, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, bool>
    {
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type{j.as_bool()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, bool val)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,typename std::enable_if<std::is_same<T, 
        std::conditional<!std::is_same<bool,std::vector<bool>::const_reference>::value,
                         std::vector<bool>::const_reference,
                         void>::type>::value>::type>
    {
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type{j.as_bool()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, bool val)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, std::vector<bool>::reference>
    {
        using result_type = conversion_result<bool>;

        static bool is(const Json& j) noexcept
        {
            return j.is_bool();
        }
        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type{j.as_bool()};
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, bool val)
        {
            return Json(val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
        typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                ext_traits::is_string<T>::value &&
                                std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using result_type = conversion_result<T>;
        using char_type = typename T::value_type;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            return j.template try_as_string<T>(aset);
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_string<T>::value &&
                                                    !std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using char_type = typename Json::char_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string();
        }

        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (!j.is_string())
            {
                return result_type{jsoncons::unexpect, conv_errc::not_string};
            }
            auto sv = j.as_string_view();
            T val = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
            unicode_traits::convert(sv.data(), sv.size(), val);
            return result_type(std::move(val));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            using temp_alloc_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<char_type>;
            std::basic_string<char_type,std::char_traits<char_type>,temp_alloc_type> s(aset.get_temp_allocator());
            unicode_traits::convert(val.data(), val.size(), s);
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), s, semantic_tag::none);
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    ext_traits::is_string_view<T>::value &&
                                                    std::is_same<typename Json::char_type,typename T::value_type>::value>::type>
    {
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_string_view();
        }

        template<typename Alloc,typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            auto result = j.try_as_string_view();
            return result ? result_type(in_place, result.value().data(), result.value().size()) : result_type(unexpect, conv_errc::not_string); 
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val, semantic_tag::none);
        }
    };

    // array back insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    ext_traits::is_back_insertable<T>::value 
                                                    >::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (const auto& e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        // array back insertable non-byte container

        template <typename Container = T,typename Alloc, typename TempAlloc>
        static typename std::enable_if<!ext_traits::is_byte<typename Container::value_type>::value,result_type>::type
        try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (!j.is_array())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
            T result{jsoncons::make_obj_using_allocator<T>(aset.get_allocator())};
            visit_reserve_(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(),result,j.size());
            for (const auto& item : j.array_range())
            {
                auto res = item.template try_as<value_type>(aset);
                if (JSONCONS_UNLIKELY(!res))
                {
                    return result_type(jsoncons::unexpect, res.error().code(), res.error().message_arg());
                }
                result.push_back(std::move(*res));
            }

            return result_type(std::move(result));
        }

        // array back insertable byte container

        template <typename Container = T,typename Alloc, typename TempAlloc>
        static typename std::enable_if<ext_traits::is_byte<typename Container::value_type>::value,result_type>::type
        try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            std::error_code ec;
            if (j.is_array())
            {
                T result;
                visit_reserve_(typename std::integral_constant<bool, ext_traits::has_reserve<T>::value>::type(),result,j.size());
                for (const auto& item : j.array_range())
                {
                    auto res = item.template try_as<value_type>(aset);
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.push_back(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else if (j.is_byte_string_view())
            {
                auto bs = j.as_byte_string_view();
                auto v = jsoncons::make_obj_using_allocator<T>(aset.get_allocator(), bs.begin(), bs.end()); 
                return result_type(std::move(v));
            }
            else if (j.is_string())
            {
                T v;
                auto sv = j.as_string_view();
                auto r = string_to_bytes(sv.begin(), sv.end(), j.tag(), v);
                if (JSONCONS_UNLIKELY(r.ec != conv_errc{}))
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_byte_string);
                }
                return result_type(std::move(v));
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        template <typename Alloc, typename TempAlloc, typename Container = T>
        static typename std::enable_if<!ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            auto j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }

        template <typename Alloc,typename TempAlloc,typename Container = T>
        static typename std::enable_if<ext_traits::is_std_byte<typename Container::value_type>::value,Json>::type
        to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), byte_string_arg, val, semantic_tag::none);
        }

        static void visit_reserve_(std::true_type, T& v, std::size_t size)
        {
            v.reserve(size);
        }

        static void visit_reserve_(std::false_type, T&, std::size_t)
        {
        }
    };

    // array, not back insertable but insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    !ext_traits::is_back_insertable<T>::value &&
                                                    ext_traits::is_insertable<T>::value>::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (const auto& e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_array())
            {
                T result = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
                for (const auto& item : j.array_range())
                {
                    auto res = item.template try_as<value_type>(aset);
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.insert(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else 
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            auto j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // array not back insertable or insertable, but front insertable

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
                            typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                                    detail::is_compatible_array_type<Json,T>::value &&
                                                    !ext_traits::is_back_insertable<T>::value &&
                                                    !ext_traits::is_insertable<T>::value &&
                                                    ext_traits::is_front_insertable<T>::value>::type>
    {
        typedef typename std::iterator_traits<typename T::iterator>::value_type value_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (const auto& e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_array())
            {
                T result = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());

                auto it = j.array_range().rbegin();
                auto end = j.array_range().rend();
                for (; it != end; ++it)
                {
                    auto res = (*it).template try_as<value_type>(aset);
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_vector);
                    }
                    result.push_front(std::move(*res));
                }

                return result_type(std::move(result));
            }
            else 
            {
                return result_type(jsoncons::unexpect, conv_errc::not_vector);
            }
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            auto j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first, last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // std::array

    template <typename Json,typename E, std::size_t N>
    struct json_conv_traits<Json, std::array<E, N>>
    {
        using result_type = conversion_result<std::array<E, N>>;

        using value_type = E;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array() && j.size() == N;
            if (result)
            {
                for (const auto& e : j.array_range())
                {
                    if (!e.template is<value_type>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            std::array<E, N> buff;
            if (j.size() != N)
            {
                return result_type(jsoncons::unexpect, conv_errc::not_array);
            }
            for (std::size_t i = 0; i < N; i++)
            {
                auto res = j[i].template try_as<E>(aset);
                if (JSONCONS_UNLIKELY(!res))
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_array);
                }
                buff[i] = std::move(*res);
            }
            return result_type(std::move(buff));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::array<E, N>& val)
        {
            auto j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg);
            j.reserve(N);
            for (auto it = val.begin(); it != val.end(); ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

    // map like
    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
        typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                ext_traits::is_map_like<T>::value &&
                                ext_traits::is_string<typename T::key_type>::value &&
                                is_json_conv_traits_specialized<Json,typename T::mapped_type>::value>::type
    >
    {
        using mapped_type = typename T::mapped_type;
        using value_type = typename T::value_type;
        using key_type = typename T::key_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            if (!j.is_object())
            {
                return false;
            }
            for (const auto& member : j.object_range())
            {
                if (!member.value().template is<mapped_type>())
                {
                    return false;
                }
            }
            return true;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (!j.is_object())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_map);
            }
            auto val = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
            for (const auto& item : j.object_range())
            {
                auto key = jsoncons::make_obj_using_allocator<key_type>(aset.get_allocator(), 
                    item.key().data(), item.key().size());
                auto r2 = item.value().template try_as<mapped_type>(aset);
                if (!r2)
                {
                    return result_type(jsoncons::unexpect, r2.error().code(), r2.error().message_arg());
                }
                val.emplace(std::move(key), std::move(*r2));
            }

            return result_type(std::move(val));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none);
            j.reserve(val.size());
            for (const auto& item : val)
            {
                j.try_emplace(jsoncons::make_obj_using_allocator<typename Json::key_type>(aset.get_allocator(), item.first.data(), item.first.length()), item.second);
            }
            return j;
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T, 
        typename std::enable_if<!is_json_conv_traits_declared<T>::value && 
                                ext_traits::is_map_like<T>::value &&
                                !ext_traits::is_string<typename T::key_type>::value &&
                                is_json_conv_traits_specialized<Json,typename T::key_type>::value &&
                                is_json_conv_traits_specialized<Json,typename T::mapped_type>::value>::type
    >
    {
        using mapped_type = typename T::mapped_type;
        using value_type = typename T::value_type;
        using key_type = typename T::key_type;
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept 
        {
            if (!j.is_object())
                return false;
            for (const auto& item : j.object_range())
            {
                Json k(item.key());
                if (!k.template is<key_type>())
                {
                    return false;
                }
                if (!item.value().template is<mapped_type>())
                {
                    return false;
                }
            }
            return true;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j) 
        {
            if (!j.is_object())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_map);
            }

            auto val = jsoncons::make_obj_using_allocator<T>(aset.get_allocator());
            for (const auto& item : j.object_range())
            {
                auto k = jsoncons::make_obj_using_allocator<Json>(j.get_allocator(), item.key());
                auto r1 = k.template try_as<key_type>(aset);
                if (!r1)
                {
                    return result_type(jsoncons::unexpect, r1.error());
                }
                auto r2 = item.value().template try_as<mapped_type>(aset);
                if (!r2)
                {
                    return result_type(jsoncons::unexpect, r2.error());
                }
                val.emplace(std::move(*r1), std::move(*r2));
            }

            return result_type(std::move(val));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val) 
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_object_arg, semantic_tag::none);
            j.reserve(val.size());
            for (const auto& item : val)
            {
                auto temp = json_conv_traits<Json, key_type>::to_json(aset, item.first);
                if (temp.is_string_view())
                {
                    auto sv = temp.as_string_view();
                    j.try_emplace(jsoncons::make_obj_using_allocator<typename Json::key_type>(aset.get_allocator(), sv.data(), sv.length()), item.second);
                }
                else
                {
                    auto key = jsoncons::make_obj_using_allocator<typename Json::key_type>(aset.get_allocator());
                    temp.dump(key);
                    j.try_emplace(std::move(key), item.second);
                }
            }
            return j;
        }
    };

    namespace tuple_detail
    {
        template<size_t Pos, std::size_t Size,typename Json,typename Tuple>
        struct json_tuple_helper
        {
            using element_type = typename std::tuple_element<Size-Pos, Tuple>::type;
            using next = json_tuple_helper<Pos-1, Size, Json, Tuple>;
            
            static bool is(const Json& j) noexcept
            {
                if (j[Size-Pos].template is<element_type>())
                {
                    return next::is(j);
                }
                else
                {
                    return false;
                }
            }

            template <typename Alloc,typename TempAlloc>
            static void try_as(Tuple& tuple, const allocator_set<Alloc,TempAlloc>& aset, const Json& j, std::error_code& ec)
            {
                auto res = j[Size-Pos].template try_as<element_type>(aset);
                if (!res)
                {
                    ec = res.error().code();
                    return;
                }
                std::get<Size-Pos>(tuple) = *res;
                next::try_as(tuple, aset, j, ec);
            }

            template <typename Alloc, typename TempAlloc>
            static void to_json(const allocator_set<Alloc,TempAlloc>& aset, const Tuple& tuple, Json& j)
            {
                j.push_back(json_conv_traits<Json, element_type>::to_json(aset, std::get<Size-Pos>(tuple)));
                next::to_json(aset, tuple, j);
            }
        };

        template<size_t Size,typename Json,typename Tuple>
        struct json_tuple_helper<0, Size, Json, Tuple>
        {
            static bool is(const Json&) noexcept
            {
                return true;
            }

            template <typename Alloc, typename TempAlloc>
            static void try_as(Tuple&, const allocator_set<Alloc,TempAlloc>&, const Json&, std::error_code&)
            {
            }

            template <typename Alloc, typename TempAlloc>
            static void to_json(const allocator_set<Alloc,TempAlloc>&, const Tuple&, Json&)
            {
            }
        };
    } // namespace tuple_detail

    template <typename Json,typename... E>
    struct json_conv_traits<Json, std::tuple<E...>>
    {
    private:
        using helper = tuple_detail::json_tuple_helper<sizeof...(E), sizeof...(E), Json, std::tuple<E...>>;

    public:
        using result_type = conversion_result<std::tuple<E...>>;

        static bool is(const Json& j) noexcept
        {
            return helper::is(j);
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            std::error_code ec;
            std::tuple<E...> val;
            helper::try_as(val, aset, j, ec);
            if (ec)
            {
                return result_type(jsoncons::unexpect, ec);
            }
            return result_type(std::move(val));
        }
         
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::tuple<E...>& val)
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg, semantic_tag::none);
            j.reserve(sizeof...(E));
            helper::to_json(aset, val, j);
            return j;
        }
    };

    template <typename Json,typename T1,typename T2>
    struct json_conv_traits<Json, std::pair<T1,T2>>
    {
    public:
        using result_type = conversion_result<std::pair<T1,T2>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_array() && j.size() == 2;
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (!j.is_array() || j.size() != 2)
            {
                return result_type(jsoncons::unexpect, conv_errc::not_pair);
            }
            auto res1 = j[0].template try_as<T1>(aset);
            if (!res1)
            {
                return result_type(jsoncons::unexpect, res1.error().code());
            }
            auto res2 = j[1].template try_as<T2>(aset);
            if (!res2)
            {
                return result_type(jsoncons::unexpect, res2.error().code());
            }
            return result_type(std::make_pair<T1,T2>(std::move(*res1), std::move(*res2)));
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::pair<T1, T2>& val)
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg, semantic_tag::none);
            j.reserve(2);
            j.push_back(val.first);
            j.push_back(val.second);
            return j;
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<ext_traits::is_basic_byte_string<T>::value>::type>
    {
    public:
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_byte_string();
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        { 
            return j.template try_as_byte_string<T>(aset);
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), byte_string_arg, val, semantic_tag::none);
        }
    };

    template <typename Json,typename ValueType>
    struct json_conv_traits<Json, std::shared_ptr<ValueType>,
        typename std::enable_if<!is_json_conv_traits_declared<std::shared_ptr<ValueType>>::value &&
                                !std::is_polymorphic<ValueType>::value
    >::type>
    {
        using result_type = conversion_result<std::shared_ptr<ValueType>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<ValueType>();
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j) 
        {
            if (j.is_null())
            {
                return result_type(std::shared_ptr<ValueType>(nullptr));
            }
            auto r = j.template try_as<ValueType>(aset);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            return result_type(std::allocate_shared<ValueType>(aset.get_allocator(), std::move(r.value())));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::shared_ptr<ValueType>& ptr)
        {
            if (ptr.get() != nullptr) 
            {
                return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), *ptr);
            }
            else 
            {
                return Json::null();
            }
        }
    };

    template <typename Json,typename ValueType>
    struct json_conv_traits<Json, std::unique_ptr<ValueType>,
                            typename std::enable_if<!is_json_conv_traits_declared<std::unique_ptr<ValueType>>::value &&
                                                    !std::is_polymorphic<ValueType>::value
    >::type>
    {
        using result_type = conversion_result<std::unique_ptr<ValueType>>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<ValueType>();
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j) 
        {
            if (j.is_null())
            {
                return result_type(std::unique_ptr<ValueType>(nullptr));
            }
            auto r = j.template try_as<ValueType>(aset);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            return result_type(jsoncons::make_unique<ValueType>(std::move(r.value())));
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::unique_ptr<ValueType>& ptr)
        {
            if (ptr.get() != nullptr) 
            {
                return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), *ptr, semantic_tag::none);
            }
            else 
            {
                return Json::null();
            }
        }
    };

    template <typename Json,typename T>
    struct json_conv_traits<Json, T,
        typename std::enable_if<jsoncons::ext_traits::is_optional<T>::value && !is_json_conv_traits_declared<T>::value>::type>
    {
    public:
        using result_type = conversion_result<T>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null() || j.template is<typename T::value_type>();
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        { 
            if (j.is_null())
            {
                return result_type(T());
            }
            auto r = j.template try_as<typename T::value_type>(aset);
            if (!r)
            {
                return result_type(jsoncons::unexpect, r.error());
            }
            return result_type(T(std::move(r.value())));
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const T& val)
        {
            return val.has_value() ? jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), *val) : Json::null();
        }
    };

    template <typename Json>
    struct json_conv_traits<Json, byte_string_view>
    {
    public:
        using result_type = conversion_result<byte_string_view>;

        static bool is(const Json& j) noexcept
        {
            return j.is_byte_string_view();
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            return result_type(j.try_as_byte_string_view());
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const byte_string_view& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), byte_string_arg, val, semantic_tag::none);
        }
    };

    // basic_bigint

    template <typename Json,typename Allocator>
    struct json_conv_traits<Json, basic_bigint<Allocator>>
    {
    public:
        using char_type = typename Json::char_type;
        using result_type = conversion_result<basic_bigint<Allocator>>;

        static bool is(const Json& j) noexcept
        {
            switch (j.type())
            {
                case json_type::string:
                    return jsoncons::is_base10(j.as_string_view().data(), j.as_string_view().length());
                case json_type::int64:
                case json_type::uint64:
                    return true;
                default:
                    return false;
            }
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            switch (j.type())
            {
                case json_type::string:
                {
                    auto sv = j.as_string_view();
                    std::error_code ec;
                    basic_bigint<Allocator> val;
                    auto r = to_bigint(sv.data(), sv.length(), val);
                    if (JSONCONS_UNLIKELY(!r))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_bigint);
                    }
                    return result_type(std::move(val));
                }
                case json_type::float16:
                case json_type::float64:
                {
                    auto res = j.template try_as<int64_t>(aset);
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                case json_type::int64:
                {
                    auto res = j.template try_as<int64_t>(aset);
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                case json_type::uint64:
                {
                    auto res = j.template try_as<uint64_t>(aset);
                    return res ? result_type(jsoncons::in_place, *res) : result_type(jsoncons::unexpect, conv_errc::not_bigint);
                }
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_bigint);
            }
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const basic_bigint<Allocator>& val)
        {
            using temp_alloc_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<char_type>;
            std::basic_string<char_type,std::char_traits<char_type>,temp_alloc_type> s{aset.get_temp_allocator()};
            val.write_string(s);
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), s, semantic_tag::bigint);
        }
    };

    // std::valarray

    template <typename Json,typename T>
    struct json_conv_traits<Json, std::valarray<T>>
    {
        using result_type = conversion_result<std::valarray<T>>;

        static bool is(const Json& j) noexcept
        {
            bool result = j.is_array();
            if (result)
            {
                for (const auto& e : j.array_range())
                {
                    if (!e.template is<T>())
                    {
                        result = false;
                        break;
                    }
                }
            }
            return result;
        }
        
        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_array())
            {
                std::valarray<T> v(j.size());
                for (std::size_t i = 0; i < j.size(); ++i)
                {
                    auto res = j[i].template try_as<T>(aset);
                    if (JSONCONS_UNLIKELY(!res))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_array);
                    }
                    v[i] = std::move(*res);
                }
                return result_type(std::move(v));
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_array);
            }
        }
        
        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::valarray<T>& val)
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg, semantic_tag::none);
            auto first = std::begin(val);
            auto last = std::end(val);
            std::size_t size = std::distance(first,last);
            j.reserve(size);
            for (auto it = first; it != last; ++it)
            {
                j.push_back(*it);
            }
            return j;
        }
    };

#if defined(JSONCONS_HAS_STD_VARIANT)

namespace variant_detail
{
    template<int N,typename Json,typename Variant,typename ... Args>
    typename std::enable_if<N == std::variant_size_v<Variant>, bool>::type
    is_variant(const Json& /*j*/)
    {
        return false;
    }

    template<std::size_t N,typename Json,typename Variant,typename T,typename ... U>
    typename std::enable_if<N < std::variant_size_v<Variant>, bool>::type
    is_variant(const Json& j)
    {
      if (j.template is<T>())
      {
          return true;
      }
      else
      {
          return is_variant<N+1, Json, Variant, U...>(j);
      }
    }

    template<int N,typename Json,typename Variant,typename Alloc,typename TempAlloc,typename ... Args>
    typename std::enable_if<N == std::variant_size_v<Variant>, conversion_result<Variant>>::type
    as_variant(const allocator_set<Alloc,TempAlloc>&, const Json& /*j*/)
    {
        return conversion_result<Variant>(jsoncons::unexpect, conv_errc::not_variant);
    }

    template<std::size_t N,typename Json,typename Variant,typename Alloc,typename TempAlloc,typename T,typename ... U>
    typename std::enable_if<N < std::variant_size_v<Variant>, conversion_result<Variant>>::type
    as_variant(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
    {
        using result_type = conversion_result<Variant>;
        if (j.template is<T>())
      {
          auto res = j.template try_as<T>(aset);
          if (JSONCONS_UNLIKELY(!res))
          {
              return result_type(jsoncons::unexpect, conv_errc::not_variant);
          }
          return conversion_result<Variant>(jsoncons::in_place, std::move(*res));
      }
      else
      {
          return as_variant<N+1, Json, Variant, Alloc, TempAlloc, U...>(aset, j);
      }
    }

    template <typename Json>
    struct variant_to_json_visitor
    {
        Json& j_;

        variant_to_json_visitor(Json& j) : j_(j) {}

        template <typename T>
        void operator()(const T& value) const
        {
            j_ = value;
        }
    };

} // namespace variant_detail

    template <typename Json,typename... VariantTypes>
    struct json_conv_traits<Json, std::variant<VariantTypes...>>
    {
    public:
        using variant_type = typename std::variant<VariantTypes...>;
        using result_type = conversion_result<std::variant<VariantTypes...>>;

        static bool is(const Json& j) noexcept
        {
            return variant_detail::is_variant<0,Json,variant_type, VariantTypes...>(j); 
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            return result_type(variant_detail::as_variant<0,Json,variant_type,Alloc,TempAlloc,VariantTypes...>(aset, j)); 
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::variant<VariantTypes...>& var)
        {
            Json j = jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), json_array_arg, semantic_tag::none);
            variant_detail::variant_to_json_visitor<Json> visitor(j);
            std::visit(visitor, var);
            return j;
        }
    };
#endif

    // std::chrono::duration
    template <typename Json,typename Rep,typename Period>
    struct json_conv_traits<Json,std::chrono::duration<Rep,Period>>
    {
        using duration_type = std::chrono::duration<Rep,Period>;
        using result_type = conversion_result<duration_type>;

        static constexpr int64_t nanos_in_milli = 1000000;
        static constexpr int64_t nanos_in_second = 1000000000;
        static constexpr int64_t millis_in_second = 1000;

        static bool is(const Json& j) noexcept
        {
            return (j.tag() == semantic_tag::epoch_second || j.tag() == semantic_tag::epoch_milli || j.tag() == semantic_tag::epoch_nano);
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            return from_json_(aset, j);
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const duration_type& val)
        {
            return to_json_(aset, val);
        }

        template <typename Alloc, typename TempAlloc, typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::ratio<1>>::value, result_type>::type
        from_json_(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_int64() || j.is_uint64() || j.is_double())
            {
                auto count = j.template as<Rep>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, count);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, count == 0 ? 0 : count/millis_in_second);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, count == 0 ? 0 : count/nanos_in_second);
                    default:
                        return result_type(in_place, count);
                }
            }
            else if (j.is_string())
            {
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                    case semantic_tag::epoch_milli:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / millis_in_second;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    case semantic_tag::epoch_nano:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / nanos_in_second;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    default:
                    {
                        auto res = j.template try_as<Rep>(aset);
                        if (!res)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                }
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_epoch);
            }
        }

        template <typename Alloc, typename TempAlloc, typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::milli>::value, result_type>::type
        from_json_(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_int64() || j.is_uint64())
            {
                auto res = j.template try_as<Rep>();
                if (!res)
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, *res*millis_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, *res);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res == 0 ? 0 : *res/nanos_in_milli);
                    default:
                        return result_type(in_place, *res);
                }
            }
            else if (j.is_double())
            {
                auto res = j.template try_as<double>();
                if (!res)
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, static_cast<Rep>(*res * millis_in_second));
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, static_cast<Rep>(*res));
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res == 0 ? 0 : static_cast<Rep>(*res / nanos_in_milli));
                    default:
                        return result_type(in_place, static_cast<Rep>(*res));
                }
            }
            else if (j.is_string())
            {
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                    {
                        auto res = j.template try_as<Rep>();
                        if (!res)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res*millis_in_second);
                    }
                    case semantic_tag::epoch_milli:
                    {
                        auto res = j.try_as_string_view();
                        if (!res)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        Rep n{0};
                        auto result = jsoncons::dec_to_integer((*res).data(), (*res).size(), n);
                        if (!result)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, n);
                    }
                    case semantic_tag::epoch_nano:
                    {
                        auto sv = j.as_string_view();
                        bigint n;
                        auto r = to_bigint(sv.data(), sv.length(), n);
                        if (!r) {return result_type(jsoncons::unexpect, conv_errc::not_epoch);}
                        if (n != 0)
                        {
                            n = n / nanos_in_milli;
                        }
                        return result_type(in_place, static_cast<Rep>(n));
                    }
                    default:
                    {
                        auto res = j.template try_as<Rep>(aset);
                        if (!res)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                        }
                        return result_type(in_place, *res);
                    }
                }
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_epoch);
            }
        }

        template <typename Alloc, typename TempAlloc, typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::nano>::value, result_type>::type
        from_json_(const allocator_set<Alloc,TempAlloc>& aset, const Json& j)
        {
            if (j.is_int64() || j.is_uint64() || j.is_double())
            {
                auto count = j.template as<Rep>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, count*nanos_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, count*nanos_in_milli);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, count);
                    default:
                        return result_type(in_place, count);
                }
            }
            else if (j.is_double())
            {
                auto count = j.template as<double>();
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, static_cast<Rep>(count * nanos_in_second));
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, static_cast<Rep>(count * nanos_in_milli));
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, static_cast<Rep>(count));
                    default:
                        return result_type(in_place, static_cast<Rep>(count));
                }
            }
            else if (j.is_string())
            {
                auto res = j.template try_as<Rep>(aset);
                if (!res)
                {
                    return result_type(jsoncons::unexpect, conv_errc::not_epoch);
                }
                switch (j.tag())
                {
                    case semantic_tag::epoch_second:
                        return result_type(in_place, *res*nanos_in_second);
                    case semantic_tag::epoch_milli:
                        return result_type(in_place, *res*nanos_in_milli);
                    case semantic_tag::epoch_nano:
                        return result_type(in_place, *res);
                    default:
                        return result_type(in_place, *res);
                }
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_epoch);
            }
        }

        template <typename Alloc,typename TempAlloc,typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::ratio<1>>::value,Json>::type
        to_json_(const allocator_set<Alloc,TempAlloc>& aset, const duration_type& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val.count(), semantic_tag::epoch_second);
        }

        template <typename Alloc,typename TempAlloc,typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::milli>::value,Json>::type
        to_json_(const allocator_set<Alloc,TempAlloc>& aset, const duration_type& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val.count(), semantic_tag::epoch_milli);
        }

        template <typename Alloc,typename TempAlloc,typename PeriodT=Period>
        static 
        typename std::enable_if<std::is_same<PeriodT,std::nano>::value,Json>::type
        to_json_(const allocator_set<Alloc,TempAlloc>& aset, const duration_type& val)
        {
            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), val.count(), semantic_tag::epoch_nano);
        }
    };

    // std::nullptr_t
    template <typename Json>
    struct json_conv_traits<Json,std::nullptr_t>
    {
        using result_type = conversion_result<std::nullptr_t>;

        static bool is(const Json& j) noexcept
        {
            return j.is_null();
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            if (!j.is_null())
            {
                return result_type(jsoncons::unexpect, conv_errc::not_nullptr);
            }
            return result_type(nullptr);
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>&, const std::nullptr_t&)
        {
            return Json::null();
        }
    };

    // std::bitset

    struct null_back_insertable_byte_container
    {
        using value_type = uint8_t;

        void push_back(value_type)
        {
        }
    };

    template <typename Json, std::size_t N>
    struct json_conv_traits<Json, std::bitset<N>>
    {
        using result_type = conversion_result<std::bitset<N>>;

        static bool is(const Json& j) noexcept
        {
            if (j.is_byte_string())
            {
                return true;
            }
            else if (j.is_string())
            {
                jsoncons::string_view sv = j.as_string_view();
                null_back_insertable_byte_container cont;
                auto result = base16_to_bytes(sv.begin(), sv.end(), cont);
                return result.ec == conv_errc::success ? true : false;
            }
            return false;
        }

        template <typename Alloc, typename TempAlloc>
        static result_type try_as(const allocator_set<Alloc,TempAlloc>&, const Json& j)
        {
            if (j.template is<uint64_t>())
            {
                auto bits = j.template as<uint64_t>();
                std::bitset<N> bs = static_cast<unsigned long long>(bits);
                return result_type(std::move(bs));
            }
            else if (j.is_byte_string() || j.is_string())
            {
                std::bitset<N> bs;
                std::vector<uint8_t> bits;
                if (j.is_byte_string())
                {
                    bits = j.template as<std::vector<uint8_t>>();
                }
                else
                {
                    jsoncons::string_view sv = j.as_string_view();
                    auto result = base16_to_bytes(sv.begin(), sv.end(), bits);
                    if (result.ec != conv_errc::success)
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_bitset);
                    }
                }
                std::uint8_t byte = 0;
                std::uint8_t mask  = 0;

                std::size_t pos = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (mask == 0)
                    {
                        if (pos >= bits.size())
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_bitset);
                        }
                        byte = bits.at(pos++);
                        mask = 0x80;
                    }

                    if (byte & mask)
                    {
                        bs[i] = 1;
                    }

                    mask = static_cast<std::uint8_t>(mask >> 1);
                }
                return result_type(std::move(bs));
            }
            else
            {
                return result_type(jsoncons::unexpect, conv_errc::not_bitset);
            }
        }

        template <typename Alloc, typename TempAlloc>
        static Json to_json(const allocator_set<Alloc,TempAlloc>& aset, const std::bitset<N>& val)
        {
            using temp_alloc_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<uint8_t>;
            std::vector<uint8_t,temp_alloc_type> bits(aset.get_temp_allocator());

            uint8_t byte = 0;
            uint8_t mask = 0x80;

            for (std::size_t i = 0; i < N; ++i)
            {
                if (val[i])
                {
                    byte |= mask;
                }

                mask = static_cast<uint8_t>(mask >> 1);

                if (mask == 0)
                {
                    bits.push_back(byte);
                    byte = 0;
                    mask = 0x80;
                }
            }

            // Encode remainder
            if (mask != 0x80)
            {
                bits.push_back(byte);
            }

            return jsoncons::make_obj_using_allocator<Json>(aset.get_allocator(), byte_string_arg, bits, semantic_tag::base16);
        }
    };

} // namespace reflect
} // namespace jsoncons

#endif // JSONCONS_REFLECT_JSON_CONV_TRAITS_HPP
