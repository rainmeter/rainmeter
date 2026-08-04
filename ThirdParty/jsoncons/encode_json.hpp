// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_ENCODE_JSON_HPP
#define JSONCONS_ENCODE_JSON_HPP

#include <ostream>

#include <jsoncons/basic_json.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/reflect/encode_traits.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/allocator_set.hpp>

namespace jsoncons {

// encode_json

template <typename T,typename CharT>
typename std::enable_if<ext_traits::is_basic_json<T>::value,write_result>::type
    try_encode_json(const T& val, basic_json_visitor<CharT>& encoder)
{
    return val.try_dump(encoder);
}

template <typename T,typename CharT>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,write_result>::type
    try_encode_json(const T& val, basic_json_visitor<CharT>& encoder)
{
    auto r = reflect::encode_traits<T>::try_encode(make_alloc_set(), val, encoder);
    encoder.flush();
    return r;
}

template <typename T,typename Alloc,typename TempAlloc,typename CharT>
typename std::enable_if<ext_traits::is_basic_json<T>::value, write_result>::type
    try_encode_json(const allocator_set<Alloc,TempAlloc>&,
    const T& val, basic_json_visitor<CharT>& encoder)
{
    return val.try_dump(encoder);
}

template <typename T, typename Alloc, typename TempAlloc, typename CharT>
typename std::enable_if<!ext_traits::is_basic_json<T>::value, write_result>::type
    try_encode_json(const allocator_set<Alloc, TempAlloc>& aset,
    const T& val, basic_json_visitor<CharT>& encoder)
{
    auto r = reflect::encode_traits<T>::try_encode(aset, val, encoder);
    encoder.flush();
    return r;
}

// to string

template <typename T,typename CharContainer>
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json(const T& val, CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options 
        = basic_json_encode_options<typename CharContainer::value_type>())
{
    using char_type = typename CharContainer::value_type;

    basic_compact_json_encoder<char_type, jsoncons::string_sink<CharContainer>> encoder(cont, options);
    return try_encode_json(val, encoder);
}

// to stream

template <typename T,typename CharT>
write_result try_encode_json(const T& val, std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options 
        = basic_json_encode_options<CharT>())
{
    basic_compact_json_encoder<CharT> encoder(os, options);
    return try_encode_json(val, encoder);
}

// to string with allocator_set

template <typename T,typename CharContainer,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const T& val, CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options 
        = basic_json_encode_options<typename CharContainer::value_type>())
{
    using char_type = typename CharContainer::value_type;

    basic_compact_json_encoder<char_type, jsoncons::string_sink<CharContainer>,TempAlloc> encoder(cont, options,
        aset.get_temp_allocator());
    return try_encode_json(aset, val, encoder);
}

// to stream with allocator_set

template <typename T,typename CharT,typename Alloc,typename TempAlloc >
write_result try_encode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const T& val, std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options 
        = basic_json_encode_options<CharT>())
{
    basic_compact_json_encoder<CharT,TempAlloc> encoder(os, options, aset.get_temp_allocator());
    return try_encode_json(aset, val, encoder);
}

// try_encode_json_pretty

template <typename T,typename CharContainer>
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json_pretty(const T& val,
    CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options 
        = basic_json_encode_options<typename CharContainer::value_type>())
{
    using char_type = typename CharContainer::value_type;
    basic_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options);
    return try_encode_json(val, encoder);
}

template <typename T,typename CharT>
write_result try_encode_json_pretty(const T& val,
    std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options 
        = basic_json_encode_options<CharT>())
{
    basic_json_encoder<CharT> encoder(os, options);
    return try_encode_json(val, encoder);
}

template <typename T,typename CharContainer,typename Alloc,typename TempAlloc>
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json_pretty(const allocator_set<Alloc,TempAlloc>& aset, const T& val,
    CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options 
        = basic_json_encode_options<typename CharContainer::value_type>())
{
    using char_type = typename CharContainer::value_type;
    basic_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options,
        aset.get_temp_allocator());
    return try_encode_json(aset, val, encoder);
}

template <typename T,typename CharT,typename Alloc,typename TempAlloc>
write_result try_encode_json_pretty(const allocator_set<Alloc,TempAlloc>& aset,const T& val,
    std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options 
        = basic_json_encode_options<CharT>())
{
    basic_json_encoder<CharT> encoder(os, options, aset.get_temp_allocator());
    return try_encode_json(aset, val, encoder);
}

// legacy

template <typename T,typename CharContainer>
write_result try_encode_json(const T& val, CharContainer& cont, indenting indent)
{
    if (indent == indenting::indent)
    {
        return try_encode_json_pretty(val,cont);
    }
    else
    {
        return try_encode_json(val,cont);
    }
}

template <typename T,typename CharT>
write_result try_encode_json(const T& val,
    std::basic_ostream<CharT>& os, 
    indenting indent)
{
    if (indent == indenting::indent)
    {
        return try_encode_json_pretty(val, os);
    }
    else
    {
        return try_encode_json(val, os);
    }
}


// to string

template <typename T,typename CharContainer>
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json(const T& val, CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options,
    indenting indent)
{
    using char_type = typename CharContainer::value_type;

    if (indent == indenting::no_indent)
    {
        basic_compact_json_encoder<char_type, jsoncons::string_sink<CharContainer>> encoder(cont, options);
        return try_encode_json(val, encoder);
    }
    else
    {
        basic_json_encoder<char_type, jsoncons::string_sink<CharContainer>> encoder(cont, options);
        return try_encode_json(val, encoder);
    }
}

// to stream

template <typename T,typename CharT>
write_result try_encode_json(const T& val, std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options,
    indenting indent)
{
    if (indent == indenting::no_indent)
    {
        basic_compact_json_encoder<CharT> encoder(os, options);
        return try_encode_json(val, encoder);
    }
    else
    {
        basic_json_encoder<CharT> encoder(os, options);
        return try_encode_json(val, encoder);
    }
}

// to string with allocator_set

template <typename T,typename CharContainer,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value,write_result>::type
try_encode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const T& val, CharContainer& cont, 
    const basic_json_encode_options<typename CharContainer::value_type>& options,
    indenting indent)
{
    using char_type = typename CharContainer::value_type;

    if (indent == indenting::no_indent)
    {
        basic_compact_json_encoder<char_type, jsoncons::string_sink<CharContainer>,TempAlloc> encoder(cont, options,
            aset.get_temp_allocator());
        return try_encode_json(aset, val, encoder);
    }
    else
    {
        basic_json_encoder<char_type, jsoncons::string_sink<CharContainer>, TempAlloc> encoder(cont, options, 
            aset.get_temp_allocator());
        return try_encode_json(aset, val, encoder);
    }
}

// to stream with allocator_set

template <typename T,typename CharT,typename Alloc,typename TempAlloc >
write_result try_encode_json(const allocator_set<Alloc,TempAlloc>& aset,
    const T& val, std::basic_ostream<CharT>& os, 
    const basic_json_encode_options<CharT>& options,
    indenting indent)
{
    if (indent == indenting::no_indent)
    {
        basic_compact_json_encoder<CharT,TempAlloc> encoder(os, options, aset.get_temp_allocator());
        return try_encode_json(aset, val, encoder);
    }
    else
    {
        basic_json_encoder<CharT,TempAlloc> encoder(os, options, aset.get_temp_allocator());
        return try_encode_json(aset, val, encoder);
    }
}

//end legacy

template <typename... Args>
void encode_json(Args&& ... args)
{
    auto result = try_encode_json(std::forward<Args>(args)...); 
    if (!result)
    {
        JSONCONS_THROW(ser_error(result.error()));
    }
}

template <typename... Args>
void encode_json_pretty(Args&& ... args)
{
    auto result = try_encode_json_pretty(std::forward<Args>(args)...); 
    if (!result)
    {
        JSONCONS_THROW(ser_error(result.error()));
    }
}

} // namespace jsoncons

#endif // JSONCONS_ENCODE_JSON_HPP

