// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_STRING_UTILS_HPP
#define JSONCONS_UTILITY_STRING_UTILS_HPP

#include <string> // std::string

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons { 

template <typename Key>
struct transparent_string_less {
    using is_transparent = void;
    using char_type = typename Key::value_type;

    bool operator()(const Key& lhs, const Key& rhs) const
    {
        return lhs < rhs;
    }

    bool operator()(const Key& lhs, jsoncons::string_view rhs) const
    {
        return lhs < rhs;
    }

    bool operator()(const char_type* lhs, const Key& rhs) const
    {
        return lhs < rhs;
    }

    bool operator()(jsoncons::string_view lhs, const Key& rhs) const
    {
        return lhs < rhs;
    }

    bool operator()(const Key& lhs, const char_type* rhs) const
    {
        return lhs < rhs;
    }
};

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value,bool>::type
starts_with(const StringViewLike& sv, const StringViewLike& prefix)
{
    using char_type = typename StringViewLike::value_type;

    if (JSONCONS_UNLIKELY(sv.size() < prefix.size()))
    {
        return false;
    }
    const char_type* p = sv.data();
    const char_type* q = prefix.data();
    const char_type* last = prefix.data() + prefix.size();

    while (q < last)
    {
        if (*q++ != *p++)
        {
            return false;
        }
    }
    return true;
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value,bool>::type
starts_with(const StringViewLike& s, const typename StringViewLike::value_type* prefix)
{
    return starts_with(s, StringViewLike(prefix));
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value,bool>::type
starts_with(const StringViewLike& s, typename StringViewLike::value_type prefix)
{
    return starts_with(s, StringViewLike(&prefix, 1));
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value, bool>::type
ends_with(const StringViewLike& sv, const StringViewLike& suffix)
{
    using char_type = typename StringViewLike::value_type;

    if (JSONCONS_UNLIKELY(sv.size() < suffix.size()))
    {
        return false;
    }
    const char_type* p = sv.data() + (sv.size() - suffix.size());
    const char_type* q = suffix.data();
    const char_type* last = suffix.data() + suffix.size();

    while (q < last)
    {
        if (*q++ != *p++)
        {
            return false;
        }
    }
    return true;
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value, bool>::type
ends_with(const StringViewLike& s, const typename StringViewLike::value_type* suffix)
{
    return ends_with(s, StringViewLike(suffix));
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value, bool>::type
ends_with(const StringViewLike& s, typename StringViewLike::value_type suffix)
{
    return ends_with(s, StringViewLike(&suffix, 1));
}

template <typename StringViewLike>
typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value, StringViewLike>::type
strip(const StringViewLike& sv)
{
    using char_type = typename StringViewLike::value_type;

    const char_type* first = sv.data();
    const char_type* last = first + sv.size();
    const char_type* p = first;

    while (p < last)
    {
        char_type c = *p;
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r'))
        {
            break;
        }
        ++p;
    }
    if (p == last)
    {
        return StringViewLike{};
    }

    const char_type* q = last;
    do
    {
        --q;
        char_type c = *q;
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r'))
        {
            break;
        }
    } while (q > p);

    std::size_t size = (p - first) + ((last-q)-1);
    return StringViewLike(p, sv.size() - size);
}

} // namespace jsoncons

#endif // JSONCONS_UTILITY_STRING_UTILS_HPP
