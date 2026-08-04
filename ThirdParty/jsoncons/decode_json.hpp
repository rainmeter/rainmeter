// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_DECODE_JSON_HPP
#define JSONCONS_DECODE_JSON_HPP

#include <istream> // std::basic_istream
#include <tuple>
#include <type_traits>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/allocator_set.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/json_cursor.hpp>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/reflect/decode_traits.hpp>

namespace jsoncons {

// try_decode_json

template <typename T,typename StringViewLike>
typename std::enable_if<ext_traits::is_basic_json<T>::value &&
    ext_traits::is_sequence_of<StringViewLike,typename T::char_type>::value,read_result<T>>::type
try_decode_json(const StringViewLike& s,
    const basic_json_decode_options<typename StringViewLike::value_type>& options = basic_json_decode_options<typename StringViewLike::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename StringViewLike::value_type;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    basic_json_reader<char_type, string_source<char_type>> reader(s, decoder, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename StringViewLike>
typename std::enable_if<!ext_traits::is_basic_json<T>::value &&
    ext_traits::is_string_view_like<StringViewLike>::value,read_result<T>>::type
try_decode_json(const StringViewLike& s,
    const basic_json_decode_options<typename StringViewLike::value_type>& options = basic_json_decode_options<typename StringViewLike::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename StringViewLike::value_type;

    std::error_code ec;
    basic_json_cursor<char_type,string_source<char_type>> cursor(s, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }
    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

template <typename T,typename CharT>
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    basic_json_reader<CharT, stream_source<CharT>> reader(is, decoder, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type(jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column());
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename CharT>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_json_cursor<CharT> cursor(is, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }
    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

template <typename T,typename InputIt>
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(InputIt first, InputIt last,
    const basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>& options = 
    basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename std::iterator_traits<InputIt>::value_type;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    basic_json_reader<char_type, iterator_source<InputIt>> reader(iterator_source<InputIt>(first,last), decoder, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type(jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column());
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename InputIt>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(InputIt first, InputIt last,
    const basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>& options = 
    basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename std::iterator_traits<InputIt>::value_type;

    std::error_code ec;
    basic_json_cursor<char_type,iterator_source<InputIt>> cursor(iterator_source<InputIt>(first, last), 
        options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }
    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

// With leading allocator_set parameter

template <typename T,typename StringViewLike,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_basic_json<T>::value &&
    ext_traits::is_sequence_of<StringViewLike,typename T::char_type>::value,read_result<T>>::type
try_decode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const StringViewLike& s,
    const basic_json_decode_options<typename StringViewLike::value_type>& options = basic_json_decode_options<typename StringViewLike::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename StringViewLike::value_type;

    json_decoder<T,TempAlloc> decoder(aset.get_allocator(), aset.get_temp_allocator());

    std::error_code ec;   
    basic_json_reader<char_type, string_source<char_type>,TempAlloc> reader(s, decoder, options, aset.get_temp_allocator());
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type(jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column());
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename StringViewLike,typename Alloc,typename TempAlloc >
typename std::enable_if<!ext_traits::is_basic_json<T>::value &&
     ext_traits::is_string_view_like<StringViewLike>::value,read_result<T>>::type
try_decode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const StringViewLike& s,
    const basic_json_decode_options<typename StringViewLike::value_type>& options = basic_json_decode_options<typename StringViewLike::value_type>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = typename StringViewLike::value_type;

    std::error_code ec;
    basic_json_cursor<char_type,string_source<char_type>,TempAlloc> cursor(
        std::allocator_arg, aset.get_temp_allocator(), s, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }
    return reflect::decode_traits<T>::try_decode(aset, cursor);
}

template <typename T,typename CharT,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(const allocator_set<Alloc,TempAlloc>& aset,
    std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<CharT>;
    using stream_source_type = stream_source<CharT,char_allocator_type>;

    json_decoder<T,TempAlloc> decoder(aset.get_allocator(), aset.get_temp_allocator());

    std::error_code ec;   
    basic_json_reader<CharT,stream_source_type,TempAlloc> reader(stream_source_type(is,aset.get_temp_allocator()), 
        decoder, options, aset.get_temp_allocator());
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type(jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column());
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename CharT,typename Alloc,typename TempAlloc>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type
try_decode_json(const allocator_set<Alloc,TempAlloc>& aset,
    std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
{
    using value_type = T;
    using result_type = read_result<value_type>;
    using char_type = CharT;
    using char_allocator_type = typename std::allocator_traits<TempAlloc>:: template rebind_alloc<char_type>;
    using stream_source_type = stream_source<CharT,char_allocator_type>;

    std::error_code ec;   
    basic_json_cursor<char_type,stream_source_type,TempAlloc> cursor(
        std::allocator_arg, aset.get_temp_allocator(), 
        stream_source_type(is,aset.get_temp_allocator()), options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }
    return reflect::decode_traits<value_type>::try_decode(aset, cursor);
}

template <typename T, typename... Args>
T decode_json(Args&& ... args)
{
    auto result = try_decode_json<T>(std::forward<Args>(args)...); 
    if (!result)
    {
        JSONCONS_THROW(ser_error(result.error().code(), result.error().message_arg(), result.error().line(), result.error().column()));
    }
    return std::move(*result);
}

} // namespace jsoncons

#endif // JSONCONS_DECODE_JSON_HPP

