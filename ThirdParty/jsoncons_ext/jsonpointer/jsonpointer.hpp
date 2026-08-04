// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPOINTER_JSONPOINTER_HPP
#define JSONCONS_EXT_JSONPOINTER_JSONPOINTER_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <system_error> // system_error
#include <type_traits> // std::enable_if, std::true_type
#include <utility> // std::move
#include <vector>
#include <map>

#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/string_utils.hpp>

#include <jsoncons_ext/jsonpointer/jsonpointer_error.hpp>

namespace jsoncons { 
namespace jsonpointer {

    namespace detail {

    enum class pointer_state 
    {
        start,
        escaped,
        new_token,
        part
    };

    } // namespace detail

    template <typename CharT,typename Allocator=std::allocator<CharT>>
    std::basic_string<CharT,std::char_traits<CharT>,Allocator> escape(jsoncons::basic_string_view<CharT> s, const Allocator& = Allocator())
    {
        std::basic_string<CharT,std::char_traits<CharT>,Allocator> result;

        for (auto c : s)
        {
            if (JSONCONS_UNLIKELY(c == '~'))
            {
                result.push_back('~');
                result.push_back('0');
            }
            else if (JSONCONS_UNLIKELY(c == '/'))
            {
                result.push_back('~');
                result.push_back('1');
            }
            else
            {
                result.push_back(c);
            }
        }
        return result;
    }

    template <typename CharT>
    std::basic_string<CharT> escape_string(const std::basic_string<CharT>& s)
    {
        std::basic_string<CharT> result;
        for (auto c : s)
        {
            switch (c)
            {
                case '~':
                    result.push_back('~');
                    result.push_back('0');
                    break;
                case '/':
                    result.push_back('~');
                    result.push_back('1');
                    break;
                default:
                    result.push_back(c);
                    break;
            }
        }
        return result;
    }

    // basic_json_pointer

    template <typename CharT>
    class basic_json_pointer
    {
    public:
        // Member types
        using char_type = CharT;
        using string_type = std::basic_string<char_type>;
        using string_view_type = jsoncons::basic_string_view<char_type>;
        using const_iterator = typename std::vector<string_type>::const_iterator;
        using iterator = const_iterator;
        using const_reverse_iterator = typename std::vector<string_type>::const_reverse_iterator;
        using reverse_iterator = const_reverse_iterator;
    private:
        std::vector<string_type> tokens_;
    public:
        // Constructors
        basic_json_pointer()
        {
        }

        basic_json_pointer(const std::vector<string_type>& tokens)
            : tokens_(tokens)
        {
        }

        basic_json_pointer(std::vector<string_type>&& tokens)
            : tokens_(std::move(tokens))
        {
        }

        explicit basic_json_pointer(const string_view_type& s)
        {
            std::error_code ec;
            auto jp = parse(s, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(jsonpointer_error(ec));
            }
            tokens_ = std::move(jp.tokens_);
        }

        explicit basic_json_pointer(const string_view_type& s, std::error_code& ec)
        {
            auto jp = parse(s, ec);
            if (!ec)
            {
                tokens_ = std::move(jp.tokens_);
            }
        }

        basic_json_pointer(const basic_json_pointer&) = default;

        basic_json_pointer(basic_json_pointer&&) = default;

        static basic_json_pointer parse(const string_view_type& input, std::error_code& ec)
        {
            std::vector<string_type> tokens;
            if (input.empty())
            {
                return basic_json_pointer<CharT>();
            }

            const char_type* p = input.data();
            const char_type* pend = input.data() + input.size();
            string_type unescaped;

            auto state = jsonpointer::detail::pointer_state::start;
            string_type buffer;

            while (p < pend)
            {
                    switch (state)
                    {
                        case jsonpointer::detail::pointer_state::start: 
                            switch (*p)
                            {
                                case '/':
                                    state = jsonpointer::detail::pointer_state::new_token;
                                    break;
                                default:
                                    ec = jsonpointer_errc::expected_slash;
                                    return basic_json_pointer();
                            };
                            break;
                        case jsonpointer::detail::pointer_state::part:
                            state = jsonpointer::detail::pointer_state::new_token; 
                            JSONCONS_FALLTHROUGH;

                        case jsonpointer::detail::pointer_state::new_token: 
                            switch (*p)
                            {
                                case '/':
                                    tokens.push_back(buffer);
                                    buffer.clear();
                                    state = jsonpointer::detail::pointer_state::part; 
                                    break;
                                case '~':
                                    state = jsonpointer::detail::pointer_state::escaped;
                                    break;
                                default:
                                    buffer.push_back(*p);
                                    break;
                            };
                            break;
                        case jsonpointer::detail::pointer_state::escaped: 
                            switch (*p)
                            {
                                case '0':
                                    buffer.push_back('~');
                                    state = jsonpointer::detail::pointer_state::new_token;
                                    break;
                                case '1':
                                    buffer.push_back('/');
                                    state = jsonpointer::detail::pointer_state::new_token;
                                    break;
                                default:
                                    ec = jsonpointer_errc::expected_0_or_1;
                                    return basic_json_pointer();
                            };
                            break;
                    }
                    ++p;
            }
            if (state == jsonpointer::detail::pointer_state::escaped)
            {
                ec = jsonpointer_errc::expected_0_or_1;
                return basic_json_pointer();
            }
            if (state == jsonpointer::detail::pointer_state::new_token || state == jsonpointer::detail::pointer_state::part)
            {
                tokens.push_back(buffer);
            }
            return basic_json_pointer(tokens);
        }

        const std::vector<string_type>& tokens() const
        {
            return tokens_;
        }

        std::vector<string_type>& tokens() 
        {
            return tokens_;
        }

        const string_type& back() const
        {
            return tokens_.back();
        }

        // operator=
        basic_json_pointer& operator=(const basic_json_pointer&) = default;

        basic_json_pointer& operator=(basic_json_pointer&&) = default;

        // Modifiers

        void clear()
        {
            tokens_.clear();
        }

        basic_json_pointer& append(const char_type* s) 
        {
            tokens_.push_back(s);
            return *this;
        }

        template <typename StringViewLike>
        typename std::enable_if<ext_traits::is_string_view_of<StringViewLike,char_type>::value,basic_json_pointer&>::type
        append(const StringViewLike& s) 
        {
            tokens_.emplace_back(s.data(), s.size());
            return *this;
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_integer<IntegerType>::value, basic_json_pointer&>::type
        append(IntegerType val)
        {
            string_type s;
            jsoncons::from_integer(val, s);
            tokens_.push_back(s);

            return *this;
        }

        basic_json_pointer& operator/=(const char_type* s) 
        {
            return append(s);
        }

        template <typename StringViewLike>
        typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value,basic_json_pointer&>::type
        operator/=(const StringViewLike& s) 
        {
            return append(s);
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_integer<IntegerType>::value, basic_json_pointer&>::type
        operator/=(IntegerType val)
        {
            string_type s;
            jsoncons::from_integer(val, s);
            tokens_.push_back(s);

            return *this;
        }

        basic_json_pointer& operator+=(const basic_json_pointer& p)
        {
            for (const auto& s : p.tokens_)
            {
                tokens_.push_back(s);
            }
            return *this;
        }

        // Accessors
        bool empty() const
        {
          return tokens_.empty();
        }

        string_type string() const
        {
            return to_string();
        }

        string_type to_string() const
        {
            string_type buffer;
            for (const auto& token : tokens_)
            {
                buffer.push_back('/');
                for (auto c : token)
                {
                    switch (c)
                    {
                        case '~':
                            buffer.push_back('~');
                            buffer.push_back('0');
                            break;
                        case '/':
                            buffer.push_back('~');
                            buffer.push_back('1');
                            break;
                        default:
                            buffer.push_back(c);
                            break;
                    }
                }
            }
            return buffer;
        }

        // Iterators
        iterator begin() const
        {
            return tokens_.begin();
        }
        iterator end() const
        {
            return tokens_.end();
        }

        reverse_iterator rbegin() const
        {
            return tokens_.rbegin();
        }
        reverse_iterator rend() const
        {
            return tokens_.rend();
        }

        // Non-member functions
        friend 
        basic_json_pointer<CharT> operator/(const basic_json_pointer<CharT>& lhs, const char_type* rhs)
        {
            basic_json_pointer<CharT> p(lhs);
            p /= rhs;
            return p;
        }

        // Non-member functions
        template <typename StringViewLike>
        friend typename std::enable_if<ext_traits::is_string_view_like<StringViewLike>::value,basic_json_pointer>::type
        operator/(const basic_json_pointer<CharT>& lhs, const StringViewLike& rhs)
        {
            basic_json_pointer<CharT> p(lhs);
            p /= rhs;
            return p;
        }

        friend basic_json_pointer<CharT> operator+( const basic_json_pointer<CharT>& lhs, const basic_json_pointer<CharT>& rhs )
        {
            basic_json_pointer<CharT> p(lhs);
            p += rhs;
            return p;
        }

        friend bool operator==( const basic_json_pointer& lhs, const basic_json_pointer& rhs )
        {
            return lhs.tokens_ == rhs.tokens_;
        }

        friend bool operator!=( const basic_json_pointer& lhs, const basic_json_pointer& rhs )
        {
            return lhs.tokens_ != rhs.tokens_;
        }

        friend bool operator<(const basic_json_pointer& lhs, const basic_json_pointer& rhs)
        {
            return lhs.tokens_ < rhs.tokens_;
        }

        friend bool operator<=(const basic_json_pointer& lhs, const basic_json_pointer& rhs)
        {
            return lhs.tokens_ <= rhs.tokens_;
        }

        friend bool operator>(const basic_json_pointer& lhs, const basic_json_pointer& rhs)
        {
            return lhs.tokens_ > rhs.tokens_;
        }

        friend bool operator>=(const basic_json_pointer& lhs, const basic_json_pointer& rhs)
        {
            return lhs.tokens_ >= rhs.tokens_;
        }

        friend std::basic_ostream<CharT>&
        operator<<(std::basic_ostream<CharT>& os, const basic_json_pointer<CharT>& p )
        {
            os << p.to_string();
            return os;
        }
    };

    template <typename CharT,typename IntegerType>
    typename std::enable_if<ext_traits::is_integer<IntegerType>::value, basic_json_pointer<CharT>>::type
    operator/(const basic_json_pointer<CharT>& lhs, IntegerType rhs)
    {
        basic_json_pointer<CharT> p(lhs);
        p /= rhs;
        return p;
    }

    using json_pointer = basic_json_pointer<char>;
    using wjson_pointer = basic_json_pointer<wchar_t>;

    inline
    std::string to_string(const json_pointer& ptr)
    {
        return ptr.to_string();
    }

    inline
    std::wstring to_wstring(const wjson_pointer& ptr)
    {
        return ptr.to_string();
    }

    namespace detail {

    template <typename Json>
    const Json* resolve(const Json* current, const typename Json::string_view_type& buffer, std::error_code& ec)
    {
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            std::size_t index{0};
            auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
            if (!result)
            {
                ec = jsonpointer_errc::invalid_index;
                return current;
            }
            if (index >= current->size())
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            current = std::addressof(current->at(index));
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                ec = jsonpointer_errc::key_not_found;
                return current;
            }
            current = std::addressof(current->at(buffer));
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return current;
        }
        return current;
    }

    template <typename Json>
    Json* resolve(Json* current, const typename Json::string_view_type& buffer, bool create_if_missing, std::error_code& ec)
    {
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            std::size_t index{0};
            auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
            if (!result)
            {
                ec = jsonpointer_errc::invalid_index;
                return current;
            }
            if (index >= current->size())
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return current;
            }
            current = std::addressof(current->at(index));
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                if (create_if_missing)
                {
                    auto r = current->try_emplace(buffer, Json());
                    current = std::addressof(r.first->value());
                }
                else
                {
                    ec = jsonpointer_errc::key_not_found;
                    return current;
                }
            }
            else
            {
                current = std::addressof(current->at(buffer));
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return current;
        }
        return current;
    }

    } // namespace detail

    // get

    template <typename Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location, 
              bool create_if_missing,
              std::error_code& ec)
    {
        if (location.empty())
        {
            return root;
        }

        Json* current = std::addressof(root);
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            current = jsoncons::jsonpointer::detail::resolve(current, *it, create_if_missing, ec);
            if (JSONCONS_UNLIKELY(ec))
                return *current;
            ++it;
        }
        return *current;
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
        const StringSource& location_str, 
        bool create_if_missing,
        std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return root;
        }
        return get(root, jsonptr, create_if_missing, ec);
    }

    template <typename Json>
    const Json& get(const Json& root, 
                    const basic_json_pointer<typename Json::char_type>& location, 
                    std::error_code& ec)
    {
        if (location.empty())
        {
            return root;
        }

        const Json* current = std::addressof(root);
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            current = jsoncons::jsonpointer::detail::resolve(current, *it, ec);
            if (JSONCONS_UNLIKELY(ec))
                return *current;
            ++it;
        }
        return *current;
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,const Json&>::type
    get(const Json& root, 
        const StringSource& location_str, 
        std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return root;
        }
        return get(root, jsonptr, ec);
    }

    template <typename Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location, 
              std::error_code& ec)
    {
        return get(root, location, false, ec);
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
        const StringSource& location_str, 
        std::error_code& ec)
    {
        return get(root, location_str, false, ec);
    }

    template <typename Json>
    Json& get(Json& root, 
              const basic_json_pointer<typename Json::char_type>& location,
              bool create_if_missing = false)
    {
        std::error_code ec;
        Json& j = get(root, location, create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,Json&>::type
    get(Json& root, 
              const StringSource& location_str,
              bool create_if_missing = false)
    {
        std::error_code ec;
        Json& result = get(root, location_str, create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return result;
    }

    template <typename Json>
    const Json& get(const Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        const Json& j = get(root, location, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,const Json&>::type
    get(const Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        const Json& j = get(root, location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
        return j;
    }

    // contains

    template <typename Json>
    bool contains(const Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        get(root, location, ec);
        return !ec ? true : false;
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,bool>::type
    contains(const Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        get(root, location_str, ec);
        return !ec ? true : false;
    }

    template <typename Json,typename T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value, 
             bool create_if_missing,
             std::error_code& ec)
    {
        if (location.empty())
        {
            root = std::forward<T>(value);
            return;
        }

        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();
        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = jsoncons::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (JSONCONS_UNLIKELY(ec))
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                current->emplace_back(std::forward<T>(value));
                current = std::addressof(current->at(current->size()-1));
            }
            else
            {
                std::size_t index{0};
                auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index > current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                if (index == current->size())
                {
                    current->emplace_back(std::forward<T>(value));
                    current = std::addressof(current->at(current->size()-1));
                }
                else
                {
                    auto it2 = current->insert(current->array_range().begin()+index,std::forward<T>(value));
                    current = std::addressof(*it2);
                }
            }
        }
        else if (current->is_object())
        {
            auto r = current->insert_or_assign(buffer,std::forward<T>(value));
            current = std::addressof(r.first->value());
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    // add
    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value, 
             bool create_if_missing,
             std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return;
        }
        add(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template <typename Json,typename T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value, 
             std::error_code& ec)
    {
        add(root, location, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value, 
             std::error_code& ec)
    {
        add(root, location_str, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename T>
    void add(Json& root, 
             const basic_json_pointer<typename Json::char_type>& location, 
             T&& value,
             bool create_if_missing = false)
    {
        std::error_code ec;
        add(root, location, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add(Json& root, 
             const StringSource& location_str, 
             T&& value,
             bool create_if_missing = false)
    {
        std::error_code ec;
        add(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // add_if_absent

    template <typename Json,typename T>
    void add_if_absent(Json& root, 
                       const basic_json_pointer<typename Json::char_type>& location, 
                       T&& value, 
                       bool create_if_missing,
                       std::error_code& ec)
    {
        if (location.empty())
        {
            root = std::forward<T>(value);
            return;
        }
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = jsoncons::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (JSONCONS_UNLIKELY(ec))
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                current->emplace_back(std::forward<T>(value));
                current = std::addressof(current->at(current->size()-1));
            }
            else
            {
                std::size_t index{0};
                auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index > current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                if (index == current->size())
                {
                    current->emplace_back(std::forward<T>(value));
                    current = std::addressof(current->at(current->size()-1));
                }
                else
                {
                    auto it2 = current->insert(current->array_range().begin()+index,std::forward<T>(value));
                    current = std::addressof(*it2);
                }
            }
        }
        else if (current->is_object())
        {
            if (current->contains(buffer))
            {
                ec = jsonpointer_errc::key_already_exists;
                return;
            }
            else
            {
                auto r = current->try_emplace(buffer,std::forward<T>(value));
                current = std::addressof(r.first->value());
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                       const StringSource& location_str, 
                       T&& value, 
                       bool create_if_missing,
                       std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return;
        }
        add_if_absent(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                const StringSource& location, 
                T&& value, 
                std::error_code& ec)
    {
        add_if_absent(root, location, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    add_if_absent(Json& root, 
                const StringSource& location_str, 
                T&& value,
                bool create_if_missing = false)
    {
        std::error_code ec;
        add_if_absent(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <typename Json,typename T>
    void add_if_absent(Json& root, 
                       const basic_json_pointer<typename Json::char_type>& location, 
                       T&& value, 
                       std::error_code& ec)
    {
        add_if_absent(root, location, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename T>
    void add_if_absent(Json& root, 
                const basic_json_pointer<typename Json::char_type>& location, 
                T&& value,
                bool create_if_missing = false)
    {
        std::error_code ec;
        add_if_absent(root, location, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // remove

    template <typename Json>
    void remove(Json& root, const basic_json_pointer<typename Json::char_type>& location, std::error_code& ec)
    {
        if (location.empty())
        {
            ec = jsonpointer_errc::cannot_remove_root;
            return;
        }

        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = jsoncons::jsonpointer::detail::resolve(current, buffer, false, ec);
                if (JSONCONS_UNLIKELY(ec))
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return;
            }
            else
            {
                std::size_t index{0};
                auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index >= current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                current->erase(current->array_range().begin()+index);
            }
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                ec = jsonpointer_errc::key_not_found;
                return;
            }
            else
            {
                current->erase(buffer);
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    remove(Json& root, const StringSource& location_str, std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return;
        }
        remove(root, jsonptr, ec);
    }

    template <typename Json,typename StringSource>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    remove(Json& root, const StringSource& location_str)
    {
        std::error_code ec;
        remove(root, location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <typename Json>
    void remove(Json& root, const basic_json_pointer<typename Json::char_type>& location)
    {
        std::error_code ec;
        remove(root, location, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    // replace

    template <typename Json,typename T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 bool create_if_missing,
                 std::error_code& ec)
    {
        if (location.empty())
        {
            root = std::forward<T>(value);
            return;
        }
        Json* current = std::addressof(root);

        std::basic_string<typename Json::char_type> buffer;
        auto it = location.begin();
        auto end = location.end();

        while (it != end)
        {
            buffer = *it;
            ++it;
            if (it != end)
            {
                current = jsoncons::jsonpointer::detail::resolve(current, buffer, create_if_missing, ec);
                if (JSONCONS_UNLIKELY(ec))
                    return;
            }
        }
        if (current->is_array())
        {
            if (buffer.size() == 1 && buffer[0] == '-')
            {
                ec = jsonpointer_errc::index_exceeds_array_size;
                return;
            }
            else
            {
                std::size_t index{};
                auto result = jsoncons::dec_to_integer(buffer.data(), buffer.length(), index);
                if (!result)
                {
                    ec = jsonpointer_errc::invalid_index;
                    return;
                }
                if (index >= current->size())
                {
                    ec = jsonpointer_errc::index_exceeds_array_size;
                    return;
                }
                current->at(index) = std::forward<T>(value);
            }
        }
        else if (current->is_object())
        {
            if (!current->contains(buffer))
            {
                if (create_if_missing)
                {
                    current->try_emplace(buffer,std::forward<T>(value));
                }
                else
                {
                    ec = jsonpointer_errc::key_not_found;
                    return;
                }
            }
            else
            {
                auto r = current->insert_or_assign(buffer,std::forward<T>(value));
                current = std::addressof(r.first->value());
            }
        }
        else
        {
            ec = jsonpointer_errc::expected_object_or_array;
            return;
        }
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 bool create_if_missing,
                 std::error_code& ec)
    {
        auto jsonptr = basic_json_pointer<typename Json::char_type>::parse(location_str, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            return;
        }
        replace(root, jsonptr, std::forward<T>(value), create_if_missing, ec);
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 std::error_code& ec)
    {
        replace(root, location_str, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename StringSource,typename T>
    typename std::enable_if<std::is_convertible<StringSource,jsoncons::basic_string_view<typename Json::char_type>>::value,void>::type
    replace(Json& root, 
                 const StringSource& location_str, 
                 T&& value, 
                 bool create_if_missing = false)
    {
        std::error_code ec;
        replace(root, location_str, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <typename Json,typename T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 std::error_code& ec)
    {
        replace(root, location, std::forward<T>(value), false, ec);
    }

    template <typename Json,typename T>
    void replace(Json& root, 
                 const basic_json_pointer<typename Json::char_type>& location, 
                 T&& value, 
                 bool create_if_missing = false)
    {
        std::error_code ec;
        replace(root, location, std::forward<T>(value), create_if_missing, ec);
        if (JSONCONS_UNLIKELY(ec))
        {
            JSONCONS_THROW(jsonpointer_error(ec));
        }
    }

    template <typename String,typename Result>
    typename std::enable_if<std::is_convertible<typename String::value_type,typename Result::value_type>::value>::type
    escape(const String& s, Result& result)
    {
        for (auto c : s)
        {
            if (c == '~')
            {
                result.push_back('~');
                result.push_back('0');
            }
            else if (c == '/')
            {
                result.push_back('~');
                result.push_back('1');
            }
            else
            {
                result.push_back(c);
            }
        }
    }

    // flatten

    template <typename Json>
    void flatten_(const std::basic_string<typename Json::char_type>& parent_key,
                  const Json& parent_value,
                  Json& result)
    {
        using char_type = typename Json::char_type;
        using string_type = std::basic_string<char_type>;

        switch (parent_value.type())
        {
            case json_type::array:
            {
                if (parent_value.empty())
                {
                    // Flatten empty array to null
                    //result.try_emplace(parent_key, null_type{});
                    //result[parent_key] = parent_value;
                    result.try_emplace(parent_key, parent_value);
                }
                else
                {
                    for (std::size_t i = 0; i < parent_value.size(); ++i)
                    {
                        string_type key(parent_key);
                        key.push_back('/');
                        jsoncons::from_integer(i,key);
                        flatten_(key, parent_value.at(i), result);
                    }
                }
                break;
            }

            case json_type::object:
            {
                if (parent_value.empty())
                {
                    // Flatten empty object to null
                    //result.try_emplace(parent_key, null_type{});
                    //result[parent_key] = parent_value;
                    result.try_emplace(parent_key, parent_value);
                }
                else
                {
                    for (const auto& item : parent_value.object_range())
                    {
                        string_type key(parent_key);
                        key.push_back('/');
                        escape(jsoncons::basic_string_view<char_type>(item.key().data(),item.key().size()), key);
                        flatten_(key, item.value(), result);
                    }
                }
                break;
            }

            default:
            {
                // add primitive parent_value with its reference string
                //result[parent_key] = parent_value;
                result.try_emplace(parent_key, parent_value);
                break;
            }
        }
    }

    template <typename Json>
    Json flatten(const Json& value)
    {
        Json result;
        std::basic_string<typename Json::char_type> parent_key;
        flatten_(parent_key, value, result);
        return result;
    }


    // unflatten

    enum class unflatten_options {none,assume_object = 1};

    template <typename Iterator,typename StringT>
    Iterator find_inner_last(Iterator first, Iterator last, std::size_t offset, const StringT& token)
    {
        Iterator it = first;
        while (it != last && *(it->first.tokens().begin() + offset) == token)
        {
            ++it;
        }
        return it;
    }

    template <typename Json, typename Iterator>
    jsoncons::optional<Json> try_unflatten_array(Iterator first, Iterator last, std::size_t offset);

    template <typename Json, typename Iterator>
    Json unflatten_object(Iterator first, Iterator last, std::size_t offset, unflatten_options options)
    {
        Json jo{json_object_arg};

        std::size_t length = std::distance(first, last);

        auto it = first;
        while (it != last)
        {
            if (it->first.tokens().size() == offset && length == 1)
            {
                return *(it->second);
            }
            if (it->first.tokens().size() == offset)
            {
                ++it;
            }
            else if (it->first.tokens().size() < offset)
            {
                return jsoncons::optional<Json>{};
            }
            else
            {
                auto jt = it->first.tokens().begin() + offset;
                if (offset + 1 == it->first.tokens().size())
                {
                    jo.try_emplace(*jt, *(it->second));
                    ++it;
                }
                else 
                {
                    auto inner_last = find_inner_last(it, last, offset, *jt);
                    if (options == unflatten_options{})
                    {
                        auto res = try_unflatten_array<Json,Iterator>(it, inner_last, offset+1);
                        if (!res)
                        {
                            jo.try_emplace(*jt, unflatten_object<Json,Iterator>(it, inner_last, offset+1, options));
                        }
                        else
                        {
                            jo.try_emplace(*jt, std::move(*res));
                        }
                    }
                    else
                    {
                        jo.try_emplace(*jt, unflatten_object<Json,Iterator>(it, inner_last, offset+1, options));
                    }
                    it = inner_last;
                }
            }
        }
        return jsoncons::optional<Json>{std::move(jo)};
    }

    template <typename Json, typename Iterator>
    jsoncons::optional<Json> try_unflatten_array(Iterator first, Iterator last, std::size_t offset)
    {
        std::map<std::size_t,Json> m;

        auto it = first;
        while (it != last)
        {
            if (offset >= it->first.tokens().size())
            {
                return unflatten_object<Json,Iterator>(first, last, offset, unflatten_options{});
            }
            auto jt = it->first.tokens().begin() + offset;
            const auto& s = *jt;
            std::size_t n;
            auto r = jsoncons::dec_to_integer(s.data(), s.size(), n);
            if (r.ec != std::errc{})
            {
                return unflatten_object<Json,Iterator>(first, last, offset, unflatten_options{});
            }
            if (offset + 1 == it->first.tokens().size())
            {
                m.emplace(std::make_pair(n,*(it->second)));
                ++it;
            }
            else 
            {
                auto inner_last = find_inner_last(it, last, offset, *jt);
                auto res = try_unflatten_array<Json,Iterator>(it, inner_last, offset+1);
                if (!res)
                {
                    m.emplace(std::make_pair(n,unflatten_object<Json,Iterator>(it, inner_last, offset+1, unflatten_options{})));
                }
                else
                {
                    m.emplace(std::make_pair(n,std::move(*res)));
                }
                it = inner_last;
            }
        }

        Json ja{json_array_arg};
        ja.reserve(m.size());
        std::size_t index = 0;
        for (const auto& item : m)
        {
            if (item.first != index)
            {
                break;
            }
            ja.push_back(std::move(item.second));
            ++index;
        }

        if (index == m.size())
        {
            return jsoncons::optional<Json>{std::move(ja)};
        }
        else
        {
            return jsoncons::optional<Json>{unflatten_object<Json,Iterator>(first, last, offset, unflatten_options{})};
        }
    }

    template <typename Json>
    Json unflatten(const Json& value, unflatten_options options = unflatten_options::none)
    {
        using char_type = typename Json::char_type;
        using map_type = std::map<basic_json_pointer<char_type>, const Json*>;

        if (JSONCONS_UNLIKELY(!value.is_object() || value.empty()))
        {
            JSONCONS_THROW(jsonpointer_error(jsonpointer_errc::invalid_argument_to_unflatten));
        }

        map_type jptrs;
        for (const auto& item : value.object_range())
        {
            jptrs.emplace(std::make_pair(item.key(), std::addressof(item.value())));
        }

        if (options == unflatten_options{})
        {
            auto result = try_unflatten_array<Json,typename map_type::iterator>(jptrs.begin(), jptrs.end(), 0);
            return result ? *result : unflatten_object<Json,typename map_type::iterator>(jptrs.begin(), jptrs.end(), 0, options);
        }
        else
        {
            return unflatten_object<Json,typename map_type::iterator>(jptrs.begin(), jptrs.end(), 0, options);
        }
    }

} // namespace jsonpointer
} // namespace jsoncons

namespace std {
    template <typename CharT>
    struct hash<jsoncons::jsonpointer::basic_json_pointer<CharT>>
    {
        std::size_t operator()(const jsoncons::jsonpointer::basic_json_pointer<CharT>& ptr) const noexcept
        {
            constexpr std::uint64_t prime{0x100000001B3};
            std::uint64_t result{0xcbf29ce484222325};
             
            for (const auto& str : ptr)
            {
                for (std::size_t i = 0; i < str.length(); ++i)
                {
                    result = (result * prime) ^ str[i];
                }
            }
            return result;
        }
    };   
    
} // namespace std

#endif
