// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_MORE_TYPE_TRAITS_HPP
#define JSONCONS_UTILITY_MORE_TYPE_TRAITS_HPP

#include <array> // std::array
#include <climits> // CHAR_BIT
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstddef> // std::byte
#include <iterator> // std::iterator_traits
#include <memory>
#include <string>
#include <type_traits> // std::enable_if, std::true_type
#include <utility> // std::declval

#include <jsoncons/config/compiler_support.hpp>

#if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
#include <memory_resource> 
#endif

namespace jsoncons {
namespace ext_traits {
  
    template <typename T>
    struct is_std_pair : public std::false_type {};

    template <typename S, typename T>
    struct is_std_pair<std::pair<S, T> > : public std::true_type {};

    // is_char8
    template <typename CharT,typename Enable=void>
    struct is_char8 : std::false_type {};

    template <typename CharT>
    struct is_char8<CharT,typename std::enable_if<std::is_integral<CharT>::value &&
                                                   !std::is_same<CharT,bool>::value &&
                                                   sizeof(uint8_t) == sizeof(CharT)>::type> : std::true_type {};

    // is_char16
    template <typename CharT,typename Enable=void>
    struct is_char16 : std::false_type {};

    template <typename CharT>
    struct is_char16<CharT,typename std::enable_if<std::is_integral<CharT>::value &&
                                                   !std::is_same<CharT,bool>::value &&
                                                   (std::is_same<CharT,char16_t>::value || sizeof(uint16_t) == sizeof(CharT))>::type> : std::true_type {};

    // is_char32
    template <typename CharT,typename Enable=void>
    struct is_char32 : std::false_type {};

    template <typename CharT>
    struct is_char32<CharT,typename std::enable_if<std::is_integral<CharT>::value &&
                                                   !std::is_same<CharT,bool>::value &&
                                                   (std::is_same<CharT,char32_t>::value || (!std::is_same<CharT,char16_t>::value && sizeof(uint32_t) == sizeof(CharT)))>::type> : std::true_type {};

    // is_int128

    template <typename T,typename Enable=void>
    struct is_int128_type : std::false_type {};

#if defined(JSONCONS_HAS_INT128)
    template <typename T>
    struct is_int128_type<T,typename std::enable_if<std::is_same<T,int128_type>::value>::type> : std::true_type {};
#endif

    // is_unsigned_integer

    template <typename T,typename Enable=void>
    struct is_uint128_type : std::false_type {};

#if defined (JSONCONS_HAS_INT128)
    template <typename T>
    struct is_uint128_type<T,typename std::enable_if<std::is_same<T,uint128_type>::value>::type> : std::true_type {};
#endif

    template <typename T,typename Enable = void>
    class integer_limits
    {
    public:
        static constexpr bool is_specialized = false;
    };

    template <typename T>
    class integer_limits<T,typename std::enable_if<std::is_integral<T>::value && !std::is_same<T,bool>::value>::type>
    {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = std::numeric_limits<T>::is_signed;
        static constexpr int digits =  std::numeric_limits<T>::digits;
        static constexpr std::size_t buffer_size = static_cast<std::size_t>(sizeof(T)*CHAR_BIT*0.302) + 3;
        static constexpr int digits10 = std::numeric_limits<T>::digits10;
        
        static constexpr T(max)() noexcept
        {
            return (std::numeric_limits<T>::max)();
        }
        static constexpr T(min)() noexcept
        {
            return (std::numeric_limits<T>::min)();
        }
        static constexpr T lowest() noexcept
        {
            return std::numeric_limits<T>::lowest();
        }
    };

    template <typename T>
    class integer_limits<T,typename std::enable_if<!std::is_integral<T>::value && is_int128_type<T>::value>::type>
    {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr int digits =  sizeof(T)*CHAR_BIT - 1;
        static constexpr std::size_t buffer_size = (sizeof(T)*CHAR_BIT*0.302) + 3;
        static constexpr int digits10 = 38;

        static constexpr T(max)() noexcept
        {
            return (((((T)1 << (digits - 1)) - 1) << 1) + 1);
        }
        static constexpr T(min)() noexcept
        {
            return -(max)() - 1;
        }
        static constexpr T lowest() noexcept
        {
            return (min)();
        }
    };

    template <typename T>
    class integer_limits<T,typename std::enable_if<!std::is_integral<T>::value && is_uint128_type<T>::value>::type>
    {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;
        static constexpr int digits =  sizeof(T)*CHAR_BIT;
        static constexpr int digits10 = 38;

        static constexpr T(max)() noexcept
        {
            return T(T(~0));
        }
        static constexpr T(min)() noexcept
        {
            return 0;
        }
        static constexpr T lowest() noexcept
        {
            return std::numeric_limits<T>::lowest();
        }
    };

    #ifndef JSONCONS_HAS_VOID_T
    // follows https://en.cppreference.com/w/cpp/types/void_t
    template <typename... Ts> struct make_void { typedef void type;};
    template <typename... Ts> using void_t = typename make_void<Ts...>::type;
    #else
    using void_t = std::void_t; 
    #endif

    // follows http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4436.pdf

    // detector

    // primary template handles all types not supporting the archetypal Op
    template< 
        class Default, 
        class, // always void; supplied externally
        template <typename...> class Op, 
        typename... Args
    >
    struct detector
    {
        constexpr static auto value = false;
        using type = Default;
    };

    // specialization recognizes and handles only types supporting Op
    template< 
        class Default, 
        template <typename...> class Op, 
        typename... Args
    >
    struct detector<Default, void_t<Op<Args...>>, Op, Args...>
    {
        constexpr static auto value = true;
        using type = Op<Args...>;
    };

    // is_detected, is_detected_t

    template< template <typename...> class Op,typename... Args >
    using
    is_detected = detector<void, void, Op, Args...>;

    template< template <typename...> class Op,typename... Args >
    using
    is_detected_t = typename is_detected<Op, Args...>::type;

    // detected_or, detected_or_t

    template <typename Default, template <typename...> class Op,typename... Args >
    using
    detected_or = detector<Default, void, Op, Args...>;

    template <typename Default, template <typename...> class Op,typename... Args >
    using
    detected_or_t = typename detected_or<Default, Op, Args...>::type;

    // is_detected_exact

   template< class Expected, template <typename...> class Op,typename... Args >
   using
   is_detected_exact = std::is_same< Expected, is_detected_t<Op, Args...> >;

    // is_detected_convertible

    template< class To, template <typename...> class Op,typename... Args >
    using
    is_detected_convertible = std::is_convertible< is_detected_t<Op, Args...>, To >;

    // to_plain_pointer

    template <typename Pointer> inline
    typename std::pointer_traits<Pointer>::element_type* to_plain_pointer(Pointer ptr)
    {       
        return (std::addressof(*ptr));
    }

    template <typename T> inline
    T * to_plain_pointer(T * ptr)
    {       
        return (ptr);
    }  

    // is_std_byte

    template <typename T,typename Enable=void>
    struct is_std_byte : std::false_type {};
#if defined(JSONCONS_HAS_STD_BYTE)
    template <typename T>
    struct is_std_byte<T, 
           typename std::enable_if<std::is_same<T,std::byte>::value
    >::type> : std::true_type {};
#endif
    // is_byte

    template <typename T,typename Enable=void>
    struct is_byte : std::false_type {};

    template <typename T>
    struct is_byte<T, 
           typename std::enable_if<std::is_same<T,char>::value ||
                                   std::is_same<T,signed char>::value ||
                                   std::is_same<T,unsigned char>::value ||
                                   is_std_byte<T>::value
    >::type> : std::true_type {};

    // is_character

    template <typename T,typename Enable=void>
    struct is_character : std::false_type {};

    template <typename T>
    struct is_character<T, 
           typename std::enable_if<std::is_same<T,char>::value ||
#ifdef __cpp_char8_t
                                   std::is_same<T,char8_t>::value ||
#endif
                                   std::is_same<T,wchar_t>::value
    >::type> : std::true_type {};

    // is_narrow_character

    template <typename T,typename Enable=void>
    struct is_narrow_character : std::false_type {};

    template <typename T>
    struct is_narrow_character<T, 
           typename std::enable_if<is_character<T>::value && (sizeof(T) == sizeof(char))
    >::type> : std::true_type {};

    // is_wide_character

    template <typename T,typename Enable=void>
    struct is_wide_character : std::false_type {};

    template <typename T>
    struct is_wide_character<T, 
           typename std::enable_if<is_character<T>::value && (sizeof(T) != sizeof(char))
    >::type> : std::true_type {};

    // From boost
    namespace ut_detail {

    template <typename T>
    struct is_cstring_impl : public std::false_type {};

    template <typename T>
    struct is_cstring_impl<T const*> : public is_cstring_impl<T*> {};

    template <typename T>
    struct is_cstring_impl<T const* const> : public is_cstring_impl<T*> {};

    template<>
    struct is_cstring_impl<char*> : public std::true_type {};

#ifdef __cpp_char8_t
    template<>
    struct is_cstring_impl<char8_t*> : public std::true_type {};
#endif

    template<>
    struct is_cstring_impl<wchar_t*> : public std::true_type {};

    } // namespace ut_detail

    template <typename T>
    struct is_cstring : public ut_detail::is_cstring_impl<typename std::decay<T>::type> {};

    // is_bool

    template <typename T,typename Enable=void>
    struct is_bool : std::false_type {};

    template <typename T>
    struct is_bool<T, 
                   typename std::enable_if<std::is_same<T,bool>::value
    >::type> : std::true_type {};

    // is_u8_u16_u32_or_u64

    template <typename T,typename Enable=void>
    struct is_u8_u16_u32_or_u64 : std::false_type {};

    template <typename T>
    struct is_u8_u16_u32_or_u64<T, 
                                typename std::enable_if<std::is_same<T,uint8_t>::value ||
                                                        std::is_same<T,uint16_t>::value ||
                                                        std::is_same<T,uint32_t>::value ||
                                                        std::is_same<T,uint64_t>::value
    >::type> : std::true_type {};

    // is_int

    template <typename T,typename Enable=void>
    struct is_i8_i16_i32_or_i64 : std::false_type {};

    template <typename T>
    struct is_i8_i16_i32_or_i64<T, 
                                typename std::enable_if<std::is_same<T,int8_t>::value ||
                                                        std::is_same<T,int16_t>::value ||
                                                        std::is_same<T,int32_t>::value ||
                                                        std::is_same<T,int64_t>::value
    >::type> : std::true_type {};

    // is_float_or_double

    template <typename T,typename Enable=void>
    struct is_float_or_double : std::false_type {};

    template <typename T>
    struct is_float_or_double<T, 
                              typename std::enable_if<std::is_same<T,float>::value ||
                                                      std::is_same<T,double>::value
    >::type> : std::true_type {};

    // make_unsigned
    template <typename T>
    struct make_unsigned_impl {using type = typename std::make_unsigned<T>::type;};

    #if defined(JSONCONS_HAS_INT128)
    template <> 
    struct make_unsigned_impl<int128_type> {using type = uint128_type;};
    template <> 
    struct make_unsigned_impl<uint128_type> {using type = uint128_type;};
    #endif

    template <typename T>
    struct make_unsigned
       : make_unsigned_impl<typename std::remove_cv<T>::type>
    {};

    // is_integer

    template <typename T,typename Enable=void>
    struct is_integer : std::false_type {};

    template <typename T>
    struct is_integer<T,typename std::enable_if<integer_limits<T>::is_specialized>::type> : std::true_type {};

    // is_signed_integer

    template <typename T,typename Enable=void>
    struct is_signed_integer : std::false_type {};

    template <typename T>
    struct is_signed_integer<T,typename std::enable_if<integer_limits<T>::is_specialized && 
                                                        integer_limits<T>::is_signed>::type> : std::true_type {};

    // is_unsigned_integer

    template <typename T,typename Enable=void>
    struct is_unsigned_integer : std::false_type {};

    template <typename T>
    struct is_unsigned_integer<T, 
                               typename std::enable_if<integer_limits<T>::is_specialized && 
                               !integer_limits<T>::is_signed>::type> : std::true_type {};

    // is_primitive

    template <typename T,typename Enable=void>
    struct is_primitive : std::false_type {};

    template <typename T>
    struct is_primitive<T, 
           typename std::enable_if<is_integer<T>::value ||
                                   is_bool<T>::value ||
                                   std::is_floating_point<T>::value
    >::type> : std::true_type {};

    // Containers

    template <typename Container>
    using 
    container_npos_t = decltype(Container::npos);

    template <typename Container>
    using 
    container_allocator_type_t = typename Container::allocator_type;

    template <typename Container>
    using 
    container_mapped_type_t = typename Container::mapped_type;

    template <typename Container>
    using 
    container_key_type_t = typename Container::key_type;

    template <typename Container>
    using 
    container_value_type_t = typename std::iterator_traits<typename Container::iterator>::value_type;

    template <typename Container>
    using 
    container_char_traits_t = typename Container::traits_type::char_type;

    template <typename Container>
    using
    container_push_back_t = decltype(std::declval<Container>().push_back(std::declval<typename Container::value_type>()));

    template <typename Optional>
    using
    optional_has_value_t = decltype(std::declval<Optional>().has_value());

    template <typename T>
    using
    value_type_t = typename T::value_type;

    template <typename Optional>
    using
    optional_value_t = decltype(std::declval<Optional>().value());

    template <typename Optional>
    using
    optional_operator_star_t = decltype(std::declval<Optional>().operator *());

    template <typename Optional>
    using
    optional_operator_bool_t = decltype(std::declval<Optional>().operator bool());

    template <typename Container>
    using
    container_push_front_t = decltype(std::declval<Container>().push_front(std::declval<typename Container::value_type>()));

    template <typename Container>
    using
    container_insert_t = decltype(std::declval<Container>().insert(std::declval<typename Container::value_type>()));

    template <typename Container>
    using
    container_reserve_t = decltype(std::declval<Container>().reserve(typename Container::size_type()));

    template <typename Container>
    using
    container_data_t = decltype(std::declval<Container>().data());

    template <typename Container>
    using
    container_size_t = decltype(std::declval<Container>().size());

    // has_allocator_type

    template <typename T,typename Enable=void>
    struct has_allocator_type : std::false_type {};

    template <typename T>
    struct has_allocator_type<T, 
        typename std::enable_if<is_detected<container_allocator_type_t,T>::value
    >::type> : std::true_type {};

    // is_string_or_string_view

    template <typename T,typename Enable=void>
    struct is_string_or_string_view : std::false_type {};

    template <typename T>
    struct is_string_or_string_view<T, 
                     typename std::enable_if<is_character<typename T::value_type>::value &&
                                             is_detected_exact<typename T::value_type,container_char_traits_t,T>::value &&
                                             is_detected<container_npos_t,T>::value
    >::type> : std::true_type {};

    // is_string

    template <typename T,typename Enable=void>
    struct is_string : std::false_type {};

    template <typename T>
    struct is_string<T, 
                     typename std::enable_if<is_string_or_string_view<T>::value &&
                                             has_allocator_type<T>::value
    >::type> : std::true_type {};

    // is_string_view

    template <typename T,typename Enable=void>
    struct is_string_view : std::false_type {};

    template <typename T>
    struct is_string_view<T, 
                          typename std::enable_if<is_string_or_string_view<T>::value &&
                                                  !is_detected<container_allocator_type_t,T>::value
    >::type> : std::true_type {};

    // is_map_like

    template <typename T,typename Enable=void>
    struct is_map_like : std::false_type {};

    template <typename T>
    struct is_map_like<T, 
                       typename std::enable_if<is_detected<container_mapped_type_t,T>::value &&
                                               is_detected<container_allocator_type_t,T>::value &&
                                               is_detected<container_key_type_t,T>::value &&
                                               is_detected<container_value_type_t,T>::value 
        >::type> 
        : std::true_type {};

    // is_std_array
    template <typename T>
    struct is_std_array : std::false_type {};

    template <typename E, std::size_t N>
    struct is_std_array<std::array<E, N>> : std::true_type {};

    // is_constructible_from_const_pointer_and_size

    template <typename T,typename Enable=void>
    struct is_constructible_from_const_pointer_and_size : std::false_type {};

    template <typename T>
    struct is_constructible_from_const_pointer_and_size<T, 
        typename std::enable_if<std::is_constructible<T,typename T::const_pointer,typename T::size_type>::value
    >::type> 
        : std::true_type {};

    // has_reserve

    template <typename Container>
    using
    has_reserve = is_detected<container_reserve_t, Container>;

    // is_back_insertable

    template <typename Container>
    using
    is_back_insertable = is_detected<container_push_back_t, Container>;

    template <typename T>
    using
    has_has_value = is_detected_exact<bool, optional_has_value_t, T>;

    template <typename T>
    using
    has_value_type = is_detected<value_type_t, T>;

    template <typename T>
    using
    has_value = is_detected<optional_value_t, T>;

    template <typename T>
    using
    has_operator_star = is_detected<optional_operator_star_t, T>;

    template <typename T>
    using
    has_operator_bool = is_detected_exact<bool, optional_operator_bool_t, T>;

    template <typename T>
    struct is_optional
    {
        static constexpr bool value = has_value_type<T>::value && has_has_value<T>::value && has_value<T>::value && has_operator_bool<T>::value && has_operator_star<T>::value;
    };

    // is_front_insertable

    template <typename Container>
    using
    is_front_insertable = is_detected<container_push_front_t, Container>;

    // is_insertable

    template <typename Container>
    using
    is_insertable = is_detected<container_insert_t, Container>;

    // has_data, has_data_exact

    template <typename Container>
    using
    has_data = is_detected<container_data_t, Container>;

    template <typename Ret,typename Container>
    using
    has_data_exact = is_detected_exact<Ret, container_data_t, Container>;

    // has_size

    template <typename Container>
    using
    has_size = is_detected<container_size_t, Container>;

    // has_data_and_size

    template <typename Container>
    struct has_data_and_size
    {
        static constexpr bool value = has_data<Container>::value && has_size<Container>::value;
    };

    // is_array_like

    template <typename T,typename Enable=void>
    struct is_array_like : std::false_type {};

    template <typename T>
    struct is_array_like<T, 
        typename std::enable_if<is_detected<container_value_type_t,T>::value &&
                                is_detected<container_allocator_type_t,T>::value &&
                                !is_std_array<T>::value && 
                                !is_detected_exact<typename T::value_type,container_char_traits_t,T>::value &&
                                !is_map_like<T>::value 
    >::type> 
        : std::true_type {};

    template <typename Container>
    struct is_array_like_with_size
    {
        static constexpr bool value = is_array_like<Container>::value && has_size<Container>::value;
    };

    template <typename Container>
    struct is_array_like_without_size
    {
        static constexpr bool value = is_array_like<Container>::value && !has_size<Container>::value;
    };

    // is_bytes_view_like

    template <typename Container,typename Enable=void>
    struct is_bytes_view_like : std::false_type {};

    template <typename Container>
    struct is_bytes_view_like<Container, 
           typename std::enable_if<has_data_exact<const typename Container::value_type*,const Container>::value &&
                                   has_size<Container>::value &&
                                   is_byte<typename Container::value_type>::value
    >::type> : std::true_type {};

    // is_string_view_like

    template <typename StringViewLike,typename Enable=void>
    struct is_string_view_like : std::false_type {};

    template <typename StringViewLike>
    struct is_string_view_like<StringViewLike, 
           typename std::enable_if<has_data_exact<const typename StringViewLike::value_type*,const StringViewLike>::value &&
                                   has_size<StringViewLike>::value &&
                                   is_character<typename StringViewLike::value_type>::value
    >::type> : std::true_type {};

    // is_string_view_of

    template <typename StringViewLike,typename CharT,typename Enable=void>
    struct is_string_view_of : std::false_type {};

    template <typename StringViewLike,typename CharT>
    struct is_string_view_of<StringViewLike,CharT, 
        typename std::enable_if<is_string_view_like<StringViewLike>::value &&
                 std::is_same<typename StringViewLike::value_type,CharT>::value
    >::type> : std::true_type {};

    // is_sequence_of

    template <typename Container,typename ValueT,typename Enable=void>
    struct is_sequence_of : std::false_type {};

    template <typename Container,typename ValueT>
    struct is_sequence_of<Container,ValueT, 
           typename std::enable_if<has_data_exact<const typename Container::value_type*,const Container>::value &&
                                   has_size<Container>::value &&
                                   std::is_same<typename Container::value_type,ValueT>::value
    >::type> : std::true_type {};

    // is_back_insertable_byte_container

    template <typename Container,typename Enable=void>
    struct is_back_insertable_byte_container : std::false_type {};

    template <typename Container>
    struct is_back_insertable_byte_container<Container, 
           typename std::enable_if<is_back_insertable<Container>::value &&
                                   is_byte<typename Container::value_type>::value
    >::type> : std::true_type {};

    // is_back_insertable_char_container

    template <typename Container,typename Enable=void>
    struct is_back_insertable_char_container : std::false_type {};

    template <typename Container>
    struct is_back_insertable_char_container<Container, 
           typename std::enable_if<is_back_insertable<Container>::value &&
                                   is_character<typename Container::value_type>::value
    >::type> : std::true_type {};

    // is_back_insertable_container_of

    template <typename Container,typename ValueT,typename Enable=void>
    struct is_back_insertable_container_of : std::false_type {};

    template <typename Container,typename ValueT>
    struct is_back_insertable_container_of<Container, ValueT,
           typename std::enable_if<is_back_insertable<Container>::value &&
                                   std::is_same<typename Container::value_type,ValueT>::value
    >::type> : std::true_type {};

    // is_c_array

    template <typename T>
    struct is_c_array : std::false_type {};

    template <typename T>
    struct is_c_array<T[]> : std::true_type {};

    template <typename T, std::size_t N>
    struct is_c_array<T[N]> : std::true_type {};

namespace impl {

    template <typename C,typename Enable=void>
    struct is_typed_array : std::false_type {};

    template <typename T>
    struct is_typed_array
    <
        T, 
        typename std::enable_if<is_array_like<T>::value && 
                                (std::is_same<typename std::decay<typename T::value_type>::type,uint8_t>::value ||  
                                 std::is_same<typename std::decay<typename T::value_type>::type,uint16_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,uint32_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,uint64_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,int8_t>::value ||  
                                 std::is_same<typename std::decay<typename T::value_type>::type,int16_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,int32_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,int64_t>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,float>::value ||
                                 std::is_same<typename std::decay<typename T::value_type>::type,double>::value)>::type
    > : std::true_type{};

} // namespace impl
    
    template <typename T>
    using is_typed_array = impl::is_typed_array<typename std::decay<T>::type>;

    // is_compatible_element

    template <typename Container,typename Element,typename Enable=void>
    struct is_compatible_element : std::false_type {};

    template <typename Container,typename Element>
    struct is_compatible_element
    <
        Container, Element, 
        typename std::enable_if<has_data<Container>::value>::type>
            : std::is_convertible< typename std::remove_pointer<decltype(std::declval<Container>().data())>::type(*)[], Element(*)[]>
    {};

    template <typename T>
    using
    construct_from_string_t = decltype(T(std::string{}));


    template <typename T>
    using
    is_constructible_from_string = is_detected<construct_from_string_t,T>;

    template <typename T,typename Data,typename Size>
    using
    construct_from_data_size_t = decltype(T(static_cast<Data>(nullptr),Size{}));


    template <typename T,typename Data,typename Size>
    using
    is_constructible_from_data_size = is_detected<construct_from_data_size_t,T,Data,Size>;

    // is_function_object
    // is_function_object_exact

    template <typename FunctionObject,typename... Args>
        using
        function_object_t = decltype(std::declval<FunctionObject>()(std::declval<Args>()...));

    template <typename FunctionObject,typename... Args>
        using
        is_function_object = is_detected<function_object_t, FunctionObject, Args ...>;

    template <typename FunctionObject,typename T,typename... Args>
    using
    is_function_object_exact = is_detected_exact<T,function_object_t, FunctionObject, Args ...>;

    template <typename Source,typename Enable=void>
    struct is_convertible_to_string_view : std::false_type {};

    template <typename Source>
    struct is_convertible_to_string_view<Source,typename std::enable_if<is_string_or_string_view<Source>::value ||
                                                               is_cstring<Source>::value
        >::type> : std::true_type {};

    #if defined(JSONCONS_HAS_2017)
        template <typename T>
        using is_nothrow_swappable = std::is_nothrow_swappable<T>;
    #else
        template <typename T>
        struct is_nothrow_swappable {
            static const bool value = noexcept(swap(std::declval<T&>(), std::declval<T&>()));
        };
    #endif

    #if defined(JSONCONS_HAS_2014)
        template <typename T>
        using alignment_of = std::alignment_of<T>;

        template< typename T, T... Ints >
        using integer_sequence = std::integer_sequence<T,Ints...>;

        template <T ... Inds>
        using index_sequence = std::index_sequence<Inds...>;

        template <typename T, T N>
        using make_integer_sequence = std::make_integer_sequence<T,N>;

        template <std::size_t N>
        using make_index_sequence = std::make_index_sequence<N>;

        template <typename... T>
        using index_sequence_for = std::index_sequence_for<T...>;

    #else
       template <typename T>
        struct alignment_of
            : std::integral_constant<std::size_t, alignof(typename std::remove_all_extents<T>::type)> {};

        template <typename T, T... Ints>
        class integer_sequence 
        {
        public:
           using value_type = T;
           static_assert(std::is_integral<value_type>::value, "not integral type");
           static constexpr std::size_t size() noexcept 
           {
               return sizeof...(Ints);
           }
        };

        template <std::size_t... Inds>
        using index_sequence = integer_sequence<std::size_t, Inds...>;
        namespace detail_ {
        template <typename T, T Begin, T End, bool>
        struct IntSeqImpl {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value, "not integral type");
            static_assert(Begin >= 0 && Begin < End, "unexpected argument (Begin<0 || Begin<=End)");

            template <typename,typename>
            struct IntSeqCombiner;

            template <TValue... Inds0, TValue... Inds1>
            struct IntSeqCombiner<integer_sequence<TValue, Inds0...>, integer_sequence<TValue, Inds1...>> {
                using TResult = integer_sequence<TValue, Inds0..., Inds1...>;
            };

            using TResult =
                typename IntSeqCombiner<typename IntSeqImpl<TValue, Begin, Begin + (End - Begin) / 2,
                                                            (End - Begin) / 2 == 1>::TResult,
                                        typename IntSeqImpl<TValue, Begin + (End - Begin) / 2, End,
                                                            (End - Begin + 1) / 2 == 1>::TResult>::TResult;
        };

        template <typename T, T Begin>
        struct IntSeqImpl<T, Begin, Begin, false> {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value, "not integral type");
            static_assert(Begin >= 0, "unexpected argument (Begin<0)");
            using TResult = integer_sequence<TValue>;
        };

        template <typename T, T Begin, T End>
        struct IntSeqImpl<T, Begin, End, true> {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value, "not integral type");
            static_assert(Begin >= 0, "unexpected argument (Begin<0)");
            using TResult = integer_sequence<TValue, Begin>;
        };
        } // namespace detail_

        template <typename T, T N>
        using make_integer_sequence = typename detail_::IntSeqImpl<T, 0, N, (N - 0) == 1>::TResult;

        template <std::size_t N>
        using make_index_sequence = make_integer_sequence<std::size_t, N>;

        template <typename... T>
        using index_sequence_for = make_index_sequence<sizeof...(T)>;
    

    #endif

    // is_propagating_allocator

    template <typename Allocator>
    using 
    allocator_outer_allocator_type_t = typename Allocator::outer_allocator_type;

    template <typename Allocator>
    using 
    allocator_inner_allocator_type_t = typename Allocator::inner_allocator_type;

    template <typename T,typename Enable=void>
    struct is_propagating_allocator : std::false_type {};

    template <typename T,typename Enable=void>
    struct is_polymorphic_allocator : std::false_type {};

#if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
    template <typename T>
    struct is_polymorphic_allocator
    <
        T, 
        typename std::enable_if<(std::is_same<T,std::pmr::polymorphic_allocator<char>>::value) >::type
    > : std::true_type{};
#endif
    template <typename T>
    struct is_propagating_allocator
    <
        T, 
        typename std::enable_if<(is_polymorphic_allocator<T>::value) || 
            (is_detected<allocator_outer_allocator_type_t,T>::value && is_detected<allocator_inner_allocator_type_t,T>::value)>::type
    > : std::true_type{};

    
} // ext_traits
} // namespace jsoncons

#endif // JSONCONS_UTILITY_MORE_TYPE_TRAITS_HPP
