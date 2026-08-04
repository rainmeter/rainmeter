// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_JSON_LOCATION_HPP
#define JSONCONS_EXT_JSONPATH_JSON_LOCATION_HPP

#include <algorithm> // std::reverse
#include <cstddef>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits> // std::is_const
#include <utility> // std::move
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_utilities.hpp>
#include <jsoncons_ext/jsonpath/path_node.hpp>

namespace jsoncons { 
namespace jsonpath { 

    template <typename CharT,typename Allocator>
    class basic_path_element 
    {
    public:
        using char_type = CharT;
        using allocator_type = Allocator;
        using char_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<CharT>;
        using string_type = std::basic_string<char_type,std::char_traits<char_type>,char_allocator_type>;
    private:
        bool has_name_;
        string_type name_;
        std::size_t index_{0};

    public:
        basic_path_element(const char_type* name, std::size_t length, 
            const Allocator& alloc = Allocator())
            : has_name_(true), name_(name, length, alloc)
        {
        }

        explicit basic_path_element(const string_type& name)
            : has_name_(true), name_(name)
        {
        }

        explicit basic_path_element(string_type&& name)
            : has_name_(true), name_(std::move(name))
        {
        }

        basic_path_element(std::size_t index, 
            const Allocator& alloc = Allocator())
            : has_name_(false), name_(alloc), index_(index)
        {
        }

        basic_path_element(const basic_path_element& other) = default;

        basic_path_element(basic_path_element&& other) = default;
        
        ~basic_path_element() = default;

        basic_path_element& operator=(const basic_path_element& other) = default;

        basic_path_element& operator=(basic_path_element&& other) = default;

        bool has_name() const
        {
            return has_name_;
        }

        bool has_index() const
        {
            return !has_name_;
        }

        const string_type& name() const
        {
            return name_;
        }

        std::size_t index() const 
        {
            return index_;
        }

        int compare(const basic_path_element& other) const
        {
            int diff = 0;
            if (has_name_ != other.has_name_)
            {
                diff = static_cast<int>(has_name_) - static_cast<int>(other.has_name_);
            }
            else
            {
                if (has_name_)
                {
                    diff = name_.compare(other.name_);
                }
                else if (index_ < other.index_)
                {
                    diff = -1;
                }
                else if (index_ > other.index_)
                {
                    diff = 1;
                }
                else
                {
                    diff = 0;
                }
            }
            return diff;
        }
    };

    // parser
    namespace detail {
     
        enum class json_location_state 
        {
            start,
            relative_location,
            single_quoted_string,
            double_quoted_string,
            unquoted_string,
            selector,
            digit,
            expect_rbracket,
            quoted_string_escape_char
        };

        enum class selector_separator_kind{bracket,dot};

        template <typename CharT,typename Allocator>
        class json_location_parser
        {
        public:
            using allocator_type = Allocator;
            using char_type = CharT;
            using string_type = std::basic_string<CharT>;
            using string_view_type = jsoncons::basic_string_view<CharT>;
            using path_element_type = basic_path_element<CharT,Allocator>;
            using path_element_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<path_element_type>;
            using path_type = std::vector<path_element_type>;

        private:

            allocator_type alloc_;
            std::size_t line_{1};
            std::size_t column_{1};
            const char_type* input_end_{nullptr};
            const char_type* p_{nullptr};

        public:
            json_location_parser(const allocator_type& alloc = allocator_type())
                : alloc_(alloc)
            {
            }

            json_location_parser(std::size_t line, std::size_t column, 
                const allocator_type& alloc = allocator_type())
                : alloc_(alloc), line_(line), column_(column)
            {
            }

            std::size_t line() const
            {
                return line_;
            }

            std::size_t column() const
            {
                return column_;
            }

            path_type parse(const string_view_type& path)
            {
                std::error_code ec;
                auto result = parse(path, ec);
                if (JSONCONS_UNLIKELY(ec))
                {
                    JSONCONS_THROW(jsonpath_error(ec, line_, column_));
                }
                return result;
            }

            path_type parse(const string_view_type& path, std::error_code& ec)
            {
                std::vector<path_element_type> elements;

                string_type buffer(alloc_);

                input_end_ = path.data() + path.length();
                p_ = path.data();


                selector_separator_kind separator_kind = selector_separator_kind::bracket;

                json_location_state state = json_location_state::start;

                while (p_ < input_end_)
                {
                    switch (state)
                    {
                        case json_location_state::start: 
                        {
                            switch (*p_)
                            {
                                case ' ':case '\t':case '\r':case '\n':
                                    advance_past_space_character();
                                    break;
                                case '$':
                                case '@':
                                {
                                    state = json_location_state::relative_location;
                                    ++p_;
                                    ++column_;
                                    break;
                                }
                                default:
                                {
                                    ec = jsonpath_errc::expected_root_or_current_node;
                                    return path_type{};
                                }
                            }
                            break;
                        }
                        case json_location_state::relative_location: 
                            switch (*p_)
                            {
                                case ' ':case '\t':case '\r':case '\n':
                                    advance_past_space_character();
                                    break;
                                case '[':
                                    separator_kind = selector_separator_kind::bracket;
                                    state = json_location_state::selector;
                                    ++p_;
                                    ++column_;
                                    break;
                                case '.':
                                    separator_kind = selector_separator_kind::dot;
                                    state = json_location_state::selector;
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    ec = jsonpath_errc::expected_lbracket_or_dot;
                                    return path_type();
                            };
                            break;
                        case json_location_state::selector:
                            switch (*p_)
                            {
                                case ' ':case '\t':case '\r':case '\n':
                                    advance_past_space_character();
                                    break;
                                case '\'':
                                    state = json_location_state::single_quoted_string;
                                    ++p_;
                                    ++column_;
                                    break;
                                case '\"':
                                    state = json_location_state::double_quoted_string;
                                    ++p_;
                                    ++column_;
                                    break;
                                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                    state = json_location_state::digit;
                                    break;
                                case '-':
                                    ec = jsonpath_errc::expected_single_quote_or_digit;
                                    return path_type();
                                default:
                                    if (separator_kind == selector_separator_kind::dot)
                                    {
                                        state = json_location_state::unquoted_string;
                                    }
                                    else
                                    {
                                        ec = jsonpath_errc::expected_single_quote_or_digit;
                                        return path_type();
                                    }
                                    break;
                            }
                            break;
                        case json_location_state::single_quoted_string:
                            switch (*p_)
                            {
                                case '\'':
                                    elements.emplace_back(buffer);
                                    buffer.clear();
                                    if (separator_kind == selector_separator_kind::bracket)
                                    {
                                        state = json_location_state::expect_rbracket;
                                    }
                                    else
                                    {
                                        state = json_location_state::relative_location;
                                    }
                                    ++p_;
                                    ++column_;
                                    break;
                                case '\\':
                                    state = json_location_state::quoted_string_escape_char;
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    buffer.push_back(*p_);
                                    ++p_;
                                    ++column_;
                                    break;
                            };
                            break;
                        case json_location_state::double_quoted_string:
                            switch (*p_)
                            {
                                case '\"':
                                    elements.emplace_back(buffer);
                                    buffer.clear();
                                    if (separator_kind == selector_separator_kind::bracket)
                                    {
                                        state = json_location_state::expect_rbracket;
                                    }
                                    else
                                    {
                                        state = json_location_state::relative_location;
                                    }
                                    ++p_;
                                    ++column_;
                                    break;
                                case '\\':
                                    state = json_location_state::quoted_string_escape_char;
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    buffer.push_back(*p_);
                                    ++p_;
                                    ++column_;
                                    break;
                            };
                            break;
                        case json_location_state::unquoted_string:
                            switch (*p_)
                            {
                                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z':
                                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z':
                                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                case '_':
                                    buffer.push_back(*p_);
                                    ++p_;
                                    ++column_;
                                    break;
                                case '\\':
                                    state = json_location_state::quoted_string_escape_char;
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    if (typename std::make_unsigned<char_type>::type(*p_) > 127)
                                    {
                                        buffer.push_back(*p_);
                                        ++p_;
                                        ++column_;
                                    }
                                    else
                                    {
                                        elements.emplace_back(buffer);
                                        buffer.clear();
                                        advance_past_space_character();
                                        state = json_location_state::relative_location;
                                    }
                                    break;
                            };
                            break;
                        case json_location_state::expect_rbracket:
                            switch (*p_)
                            {
                                case ' ':case '\t':case '\r':case '\n':
                                    advance_past_space_character();
                                    break;
                                case ']':
                                    state = json_location_state::relative_location;
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    ec = jsonpath_errc::expected_rbracket;
                                    return path_type(alloc_);
                            }
                            break;

                        case json_location_state::digit:
                            switch(*p_)
                            {
                                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                    buffer.push_back(*p_);
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    std::size_t n{0};
                                    auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                                    if (!r)
                                    {
                                        ec = jsonpath_errc::invalid_number;
                                        return path_type(alloc_);
                                    }
                                    elements.emplace_back(n);
                                    buffer.clear();
                                    if (separator_kind == selector_separator_kind::bracket)
                                    {
                                        state = json_location_state::expect_rbracket;
                                    }
                                    else
                                    {
                                        state = json_location_state::relative_location;
                                    }
                                    break;
                            }
                            break;
                        case json_location_state::quoted_string_escape_char:
                            switch (*p_)
                            {
                                case '\"':
                                    buffer.push_back('\"');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case '\'':
                                    buffer.push_back('\'');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case '\\': 
                                    buffer.push_back('\\');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case '/':
                                    buffer.push_back('/');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 'b':
                                    buffer.push_back('\b');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 'f':
                                    buffer.push_back('\f');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 'n':
                                    buffer.push_back('\n');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 'r':
                                    buffer.push_back('\r');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 't':
                                    buffer.push_back('\t');
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                case 'u':
                                    ++p_;
                                    ++column_;
                                    state = json_location_state::single_quoted_string;
                                    break;
                                default:
                                    ec = jsonpath_errc::illegal_escaped_character;
                                    return path_type(alloc_);
                            }
                            break;
                        default:
                            ++p_;
                            ++column_;
                            break;
                    }
                }
                if (state == json_location_state::unquoted_string)
                {
                    elements.emplace_back(buffer);
                }
                else if (state == json_location_state::digit)
                {
                    std::size_t n{ 0 };
                    auto r = jsoncons::to_integer(buffer.data(), buffer.size(), n);
                    if (!r)
                    {
                        ec = jsonpath_errc::invalid_number;
                        return path_type(alloc_);
                    }
                    elements.emplace_back(n);
                }
                else if (state != json_location_state::relative_location)
                {
                    ec = jsonpath_errc::unexpected_eof;
                    return path_type();
                }
                return path_type(std::move(elements));
            }

            void advance_past_space_character()
            {
                switch (*p_)
                {
                    case ' ':case '\t':
                        ++p_;
                        ++column_;
                        break;
                    case '\r':
                        if ((p_+1 < input_end_) && (*(p_+1) == '\n'))
                        {
                            ++p_;
                        }
                        ++line_;
                        column_ = 1;
                        ++p_;
                        break;
                    case '\n':
                        ++line_;
                        column_ = 1;
                        ++p_;
                        break;
                    default:
                        break;
                }
            }
        };

    } // namespace detail

    template <typename CharT,typename Allocator = std::allocator<CharT>>
    class basic_json_location
    {
    public:
        using char_type = CharT;
        using allocator_type = Allocator;
        using string_view_type = jsoncons::basic_string_view<char_type, std::char_traits<char_type>>;
        using value_type = basic_path_element<CharT,Allocator>;
        using const_iterator = typename std::vector<value_type>::const_iterator;
        using iterator = const_iterator;
    private:
        using path_element_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<value_type>;
        std::vector<value_type,path_element_allocator_type> elements_;
    public:

        basic_json_location(const allocator_type& alloc=Allocator())
            : elements_(alloc)
        {
        }

        explicit basic_json_location(const basic_path_node<char_type>& path, const allocator_type& alloc=Allocator())
            : elements_(alloc)
        {
            auto p_node = std::addressof(path);
            while (p_node != nullptr)
            {
                switch (p_node->node_kind())
                {
                    case path_node_kind::root:
                        break;
                    case path_node_kind::name:
                        elements_.emplace_back(p_node->name().data(), p_node->name().size());
                        break;
                    case path_node_kind::index:
                        elements_.emplace_back(p_node->index());
                        break;
                }
                p_node = p_node->parent();
            }
            std::reverse(elements_.begin(), elements_.end());
        }

        basic_json_location(const basic_json_location&) = default;

        basic_json_location(basic_json_location&&) = default;

        explicit basic_json_location(std::vector<value_type,path_element_allocator_type>&& elements)
            : elements_(std::move(elements))
        {
        }
        
        ~basic_json_location() = default;

        basic_json_location& operator=(const basic_json_location&) = default;

        basic_json_location& operator=(basic_json_location&&) = default;

        // Iterators

        const_iterator begin() const
        {
            return elements_.begin();
        }

        const_iterator end() const
        {
            return elements_.end();
        }

        // Accessors

        bool empty() const
        {
            return elements_.empty();
        }

        std::size_t size() const
        {
            return elements_.size();
        }

        const value_type& operator[](std::size_t index) const
        {
            return elements_[index];
        }

        int compare(const basic_json_location& other) const
        {
            if (this == &other)
            {
               return 0;
            }

            auto it1 = elements_.begin();
            auto it2 = other.elements_.begin();
            while (it1 != elements_.end() && it2 != other.elements_.end())
            {
                int diff = it1->compare(*it2);
                if (diff != 0)
                {
                    return diff;
                }
                ++it1;
                ++it2;
            }
            if (elements_.size() < other.elements_.size())
            {
                return -1;
            }
            if (elements_.size() > other.elements_.size())
            {
                return 1;
            }
            return 0;
        }

        // Modifiers

        void clear()
        {
            elements_.clear();
        }

        basic_json_location& append(const string_view_type& s)
        {
            elements_.emplace_back(s.data(), s.size());
            return *this;
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_integer<IntegerType>::value, basic_json_location&>::type
            append(IntegerType val)
        {
            elements_.emplace_back(static_cast<std::size_t>(val));

            return *this;
        }

        basic_json_location& operator/=(const string_view_type& s)
        {
            elements_.emplace_back(s.data(), s.size());
            return *this;
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_integer<IntegerType>::value, basic_json_location&>::type
            operator/=(IntegerType val)
        {
            elements_.emplace_back(static_cast<std::size_t>(val));

            return *this;
        }

        friend bool operator==(const basic_json_location& lhs, const basic_json_location& rhs) 
        {
            return lhs.compare(rhs) == 0;
        }

        friend bool operator!=(const basic_json_location& lhs, const basic_json_location& rhs)
        {
            return !(lhs == rhs);
        }

        friend bool operator<(const basic_json_location& lhs, const basic_json_location& rhs) 
        {
            return lhs.compare(rhs) < 0;
        }

        static basic_json_location parse(const jsoncons::basic_string_view<char_type>& normalized_path)
        {
            jsonpath::detail::json_location_parser<char,std::allocator<char>> parser;

            std::vector<value_type> location = parser.parse(normalized_path);
            return basic_json_location(std::move(location));
        }

        static basic_json_location parse(const jsoncons::basic_string_view<char_type>& normalized_path, 
            std::error_code ec)
        {
            jsonpath::detail::json_location_parser<char,std::allocator<char>> parser;

            std::vector<value_type> location = parser.parse(normalized_path, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                return basic_json_location();
            }
            return basic_json_location(std::move(location));
        }
    };

    template <typename Json>
    std::size_t remove(Json& root, const basic_json_location<typename Json::char_type>& location)
    {
        std::size_t count = 0;

        Json* p_current = std::addressof(root);

        std::size_t last = location.size() == 0 ? 0 : location.size() - 1;
        for (std::size_t i = 0; i < location.size(); ++i)
        {
            const auto& element = location[i];
            if (element.has_name())
            {
                if (p_current->is_object())
                {
                    auto it = p_current->find(element.name());
                    if (it != p_current->object_range().end())
                    {
                        if (i < last)
                        {
                            p_current = std::addressof((*it).value());
                        }
                        else
                        {
                            p_current->erase(it);
                            count = 1;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else // if (element.has_index())
            {
                if (p_current->is_array() && element.index() < p_current->size())
                {
                    if (i < last)
                    {
                        p_current = std::addressof(p_current->at(element.index()));
                    }
                    else
                    {
                        p_current->erase(p_current->array_range().begin()+element.index());
                        count = 1;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        return count;
    }

    template <typename Json>
    std::pair<Json*,bool> get(Json& root, const basic_json_location<typename Json::char_type>& location)
    {
        Json* p_current = std::addressof(root);
        bool found = false;

        std::size_t last = location.size() == 0 ? 0 : location.size() - 1;
        for (std::size_t i = 0; i < location.size(); ++i)
        {
            const auto& element = location[i];
            if (element.has_name())
            {
                if (p_current->is_object())
                {
                    auto it = p_current->find(element.name());
                    if (it != p_current->object_range().end())
                    {
                        p_current = std::addressof((*it).value());
                        if (i == last)
                        {
                            found = true;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else // if (element.has_index())
            {
                if (p_current->is_array() && element.index() < p_current->size())
                {
                    p_current = std::addressof(p_current->at(element.index()));
                    if (i == last)
                    {
                        found = true;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        return std::make_pair(p_current,found);
    }

    template <typename CharT,typename Allocator = std::allocator<CharT>>
    std::basic_string<CharT, std::char_traits<CharT>, Allocator> to_basic_string(const basic_json_location<CharT,Allocator>& location, 
        const Allocator& alloc = Allocator())
    {
        std::basic_string<CharT, std::char_traits<CharT>, Allocator> buffer(alloc);

        buffer.push_back('$');
        for (const auto& element : location)
        {
            if (element.has_name())
            {
                buffer.push_back('[');
                buffer.push_back('\'');
                jsoncons::jsonpath::escape_string(element.name().data(), element.name().size(), buffer);
                buffer.push_back('\'');
                buffer.push_back(']');
            }
            else
            {
                buffer.push_back('[');
                jsoncons::from_integer(element.index(), buffer);
                buffer.push_back(']');
            }
        }

        return buffer;
    }

    template <typename Json>
    std::pair<Json*,bool> replace(Json& root, const basic_json_location<typename Json::char_type>& location, const Json& value,
        bool create_if_missing=false)
    {
        Json* p_current = std::addressof(root);
        bool found = false;

        std::size_t last = location.size() == 0 ? 0 : location.size() - 1;
        for (std::size_t i = 0; i < location.size(); ++i)
        {
            const auto& element = location[i];
            if (element.has_name() && p_current->is_object())
            {
                auto it = p_current->find(element.name());
                if (it != p_current->object_range().end())
                {
                    p_current = std::addressof((*it).value());
                    if (i == last)
                    {
                        *p_current = std::move(value);
                        found = true;
                    }
                }
                else
                {
                    if (create_if_missing)
                    {
                        if (i == last)
                        {
                            auto result = p_current->try_emplace(element.name(), value);
                            p_current = std::addressof(result.first->value());
                            found = true;
                        }
                        else
                        {
                            auto result = p_current->try_emplace(element.name(), Json{});
                            p_current = std::addressof(result.first->value());
                        }
                    }
                }
            }
            else if (element.has_index() && p_current->is_array())
            {
                if (element.index() < p_current->size())
                {
                    p_current = std::addressof(p_current->at(element.index()));
                    if (i == last)
                    {
                        *p_current = value;
                        found = true;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        if (found)
        {
            return std::make_pair(p_current,true);
        }
        return std::make_pair(p_current, false);
    }

    using json_location = basic_json_location<char>;
    using wjson_location = basic_json_location<wchar_t>;
    using path_element = basic_path_element<char,std::allocator<char>>;
    using wpath_element = basic_path_element<wchar_t,std::allocator<char>>;

    inline
    std::string to_string(const json_location& location)
    {
        return to_basic_string(location);
    }

    inline
    std::wstring to_wstring(const wjson_location& location)
    {
        return to_basic_string(location);
    }

} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_JSON_LOCATION_HPP
