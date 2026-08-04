// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_BYTE_STRING_HPP
#define JSONCONS_BYTE_STRING_HPP

#include <cmath>
#include <cstdint>
#include <cstring> // std::memcmp
#include <initializer_list>
#include <iomanip> // std::setw
#include <iterator>
#include <memory> // std::allocator
#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility> // std::move
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons {

    template <typename InputIt>
    struct to_bytes_result 
    {
        InputIt it;
        conv_errc ec;
    };

    // Algorithms

namespace detail {

    template <typename InputIt,typename Container>
    typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value,size_t>::type
    bytes_to_base64_generic(InputIt first, InputIt last, const char alphabet[65], Container& result)
    {
        std::size_t count = 0;
        unsigned char a3[3];
        unsigned char a4[4];
        unsigned char fill = alphabet[64];
        int i = 0;
        int j = 0;

        while (first != last)
        {
            a3[i++] = *first++;
            if (i == 3)
            {
                a4[0] = (a3[0] & 0xfc) >> 2;
                a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
                a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
                a4[3] = a3[2] & 0x3f;

                for (i = 0; i < 4; i++) 
                {
                    result.push_back(alphabet[a4[i]]);
                    ++count;
                }
                i = 0;
            }
        }

        if (i > 0)
        {
            for (j = i; j < 3; ++j) 
            {
                a3[j] = 0;
            }

            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);

            for (j = 0; j < i + 1; ++j) 
            {
                result.push_back(alphabet[a4[j]]);
                ++count;
            }

            if (fill != 0)
            {
                while (i++ < 3) 
                {
                    result.push_back(fill);
                    ++count;
                }
            }
        }

        return count;
    }

    template <typename InputIt,typename F,typename Container>
    typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value,to_bytes_result<InputIt>>::type 
    base64_to_bytes_generic(InputIt first, InputIt last, 
                          const uint8_t reverse_alphabet[256],
                          F f,
                          Container& result)
    {
        uint8_t a4[4], a3[3];
        uint8_t i = 0;
        uint8_t j = 0;

        while (first != last && *first != '=')
        {
            if (!f(*first))
            {
                return to_bytes_result<InputIt>{first, conv_errc::conversion_failed};
            }

            a4[i++] = static_cast<uint8_t>(*first++); 
            if (i == 4)
            {
                for (i = 0; i < 4; ++i) 
                {
                    a4[i] = reverse_alphabet[a4[i]];
                }

                a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
                a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
                a3[2] = ((a4[2] & 0x3) << 6) +   a4[3];

                for (i = 0; i < 3; i++) 
                {
                    result.push_back(a3[i]);
                }
                i = 0;
            }
        }

        if (i > 0)
        {
            for (j = 0; j < i; ++j) 
            {
                a4[j] = reverse_alphabet[a4[j]];
            }

            a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
            a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);

            for (j = 0; j < i - 1; ++j) 
            {
                result.push_back(a3[j]);
            }
        }
        return to_bytes_result<InputIt>{last, conv_errc::success};
    }

} // namespace detail

    template <typename InputIt,typename Container>
    typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value,size_t>::type
    bytes_to_base16(InputIt first, InputIt last, Container& result)
    {
        static constexpr char characters[] = "0123456789ABCDEF";

        for (auto it = first; it != last; ++it)
        {
            uint8_t c = *it;
            result.push_back(characters[c >> 4]);
            result.push_back(characters[c & 0xf]);
        }
        return (last-first)*2;
    }

    template <typename InputIt,typename Container>
    typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value,size_t>::type
    bytes_to_base64url(InputIt first, InputIt last, Container& result)
    {
        static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                      "abcdefghijklmnopqrstuvwxyz"
                                                      "0123456789-_"
                                                      "\0";
        return detail::bytes_to_base64_generic(first, last, alphabet, result);
    }

    template <typename InputIt,typename Container>
    typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value,size_t>::type
    bytes_to_base64(InputIt first, InputIt last, Container& result)
    {
        static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                   "abcdefghijklmnopqrstuvwxyz"
                                                   "0123456789+/"
                                                   "=";
        return detail::bytes_to_base64_generic(first, last, alphabet, result);
    }

    template <typename Char>
    bool is_base64(Char c) 
    {
        return (c >= 0 && c < 128) && (isalnum((int)c) || c == '+' || c == '/');
    }

    template <typename Char>
    bool is_base64url(Char c) 
    {
        return (c >= 0 && c < 128) && (isalnum((int)c) || c == '-' || c == '_');
    }

    inline 
    static bool is_base64url(int c) 
    {
        return isalnum(c) || c == '-' || c == '_';
    }

    // decode

    template <typename InputIt,typename Container>
    typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value,to_bytes_result<InputIt>>::type 
    base64url_to_bytes(InputIt first, InputIt last, Container& result)
    {
        static constexpr uint8_t reverse_alphabet[256] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 62,   0xff, 0xff,
            52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
            15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xff, 0xff, 0xff, 0xff, 63,
            0xff, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
            41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };
        auto retval = jsoncons::detail::base64_to_bytes_generic(first, last, reverse_alphabet, 
                                                              is_base64url<typename std::iterator_traits<InputIt>::value_type>, 
                                                              result);
        return retval.ec == conv_errc::success ? retval : to_bytes_result<InputIt>{retval.it, conv_errc::not_base64url};
    }

    template <typename InputIt,typename Container>
    typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value,to_bytes_result<InputIt>>::type 
    base64_to_bytes(InputIt first, InputIt last, Container& result)
    {
        static constexpr uint8_t reverse_alphabet[256] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 62,   0xff, 0xff, 0xff, 63,
            52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
            15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
            41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };
        auto retval = jsoncons::detail::base64_to_bytes_generic(first, last, reverse_alphabet, 
                                                             is_base64<typename std::iterator_traits<InputIt>::value_type>, 
                                                             result);
        return retval.ec == conv_errc::success ? retval : to_bytes_result<InputIt>{retval.it, conv_errc::not_base64};
    }

    template <typename InputIt,typename Container>
    typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value,to_bytes_result<InputIt>>::type 
    base16_to_bytes(InputIt first, InputIt last, Container& result)
    {
        std::size_t len = std::distance(first,last);
        if (len & 1) 
        {
            return to_bytes_result<InputIt>{first, conv_errc::not_base16};
        }

        InputIt it = first;
        while (it != last)
        {
            uint8_t val;
            auto a = *it++;
            if (a >= '0' && a <= '9') 
            {
                val = static_cast<uint8_t>(a - '0') << 4;
            } 
            else if ((a | 0x20) >= 'a' && (a | 0x20) <= 'f') 
            {
                val = (static_cast<uint8_t>((a | 0x20) - 'a') + 10) << 4;
            } 
            else 
            {
                return to_bytes_result<InputIt>{first, conv_errc::not_base16};
            }

            auto b = *it++;
            if (b >= '0' && b <= '9') 
            {
                val |= (b - '0');
            } 
            else if ((b | 0x20) >= 'a' && (b | 0x20) <= 'f') 
            {
                val |= ((b | 0x20) - 'a' + 10);
            } 
            else 
            {
                return to_bytes_result<InputIt>{first, conv_errc::not_base16};
            }

            result.push_back(val);
        }
        return to_bytes_result<InputIt>{last, conv_errc::success};
    }

    struct byte_traits
    {
        using char_type = uint8_t;

        static constexpr int eof() 
        {
            return std::char_traits<char>::eof();
        }

        static int compare(const char_type* s1, const char_type* s2, std::size_t count) noexcept
        {
            return std::memcmp(s1,s2,count);
        }
    };

    // basic_byte_string

    template <typename Allocator>
    class basic_byte_string;

    // byte_string_view
    class byte_string_view
    {
        const uint8_t* data_{nullptr};
        std::size_t size_{0}; 
    public:
        using traits_type = byte_traits;

        using const_iterator = const uint8_t*;
        using iterator = const_iterator;
        using size_type = std::size_t;
        using value_type = uint8_t;
        using reference = uint8_t&;
        using const_reference = const uint8_t&;
        using difference_type = std::ptrdiff_t;
        using pointer = uint8_t*;
        using const_pointer = const uint8_t*;

        constexpr byte_string_view() noexcept
        {
        }

        constexpr byte_string_view(const uint8_t* data, std::size_t length) noexcept
            : data_(data), size_(length)
        {
        }
    
        template <typename BytesViewLike>
        constexpr explicit byte_string_view(const BytesViewLike& cont,
                          typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0) 
            : data_(reinterpret_cast<const uint8_t*>(cont.data())), size_(cont.size())
        {
        }
    
        template <typename Allocator>
        constexpr byte_string_view(const basic_byte_string<Allocator>& bytes);

        constexpr byte_string_view(const byte_string_view&) = default;

        JSONCONS_CPP14_CONSTEXPR byte_string_view(byte_string_view&& other) noexcept
        {
            const_pointer temp_data = data_;
            data_ = other.data_;
            other.data_ = temp_data;

            size_type temp_size = size_;
            size_ = other.size_;
            other.size_ = temp_size;
        }

        byte_string_view& operator=(const byte_string_view&) = default;

        byte_string_view& operator=(byte_string_view&& other) noexcept
        {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            return *this;
        }

        constexpr const uint8_t* data() const noexcept
        {
            return data_;
        }
        constexpr size_t size() const noexcept
        {
            return size_;
        }

        // iterator support 
        constexpr const_iterator begin() const noexcept
        {
            return data_;
        }
        constexpr const_iterator end() const noexcept
        {
            return data_ + size_;
        }
        constexpr const_iterator cbegin() const noexcept
        {
            return data_;
        }
        constexpr const_iterator cend() const noexcept
        {
            return data_ + size_;
        }

        constexpr uint8_t operator[](size_type pos) const 
        { 
            return data_[pos]; 
        }

        JSONCONS_CPP14_CONSTEXPR byte_string_view substr(size_type pos) const 
        {
            if (pos > size_)
            {
                JSONCONS_THROW(std::out_of_range("pos exceeds size"));
            }
            std::size_t n = size_ - pos;
            return byte_string_view(data_ + pos, n);
        }

        byte_string_view substr(size_type pos, size_type n) const 
        {
            if (pos > size_)
            {
                JSONCONS_THROW(std::out_of_range("pos exceeds size"));
            }
            if (pos + n > size_)
            {
                n = size_ - pos;
            }
            return byte_string_view(data_ + pos, n);
        }

        int compare(const byte_string_view& s) const noexcept 
        {
            const int rc = traits_type::compare(data_, s.data(), (std::min)(size_, s.size()));
            return rc != 0 ? rc : (size_ == s.size() ? 0 : size_ < s.size() ? -1 : 1);
        }

        template <typename Allocator>
        int compare(const basic_byte_string<Allocator>& s) const noexcept 
        {
            const int rc = traits_type::compare(data_, s.data(), (std::min)(size_, s.size()));
            return rc != 0 ? rc : (size_ == s.size() ? 0 : size_ < s.size() ? -1 : 1);
        }

        template <typename CharT>
        friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const byte_string_view& bstr)
        {
            std::basic_ostringstream<CharT> ss;
            ss.flags(std::ios::hex);
            ss.fill('0');

            bool first = true;
            for (auto b : bstr)
            {
                if (first)
                {
                    first = false;
                }
                else 
                {
                    ss << ',';
                }
                ss << std::setw(2) << static_cast<int>(b);
            }
            os << ss.str();
            return os;
        }
    };

    // basic_byte_string
    template <typename Allocator = std::allocator<uint8_t>>
    class basic_byte_string
    {
        using byte_allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<uint8_t>;
        std::vector<uint8_t,byte_allocator_type> data_;
    public:
        using traits_type = byte_traits;
        using allocator_type = byte_allocator_type;

        using value_type = typename std::vector<uint8_t,byte_allocator_type>::value_type;
        using size_type = typename std::vector<uint8_t,byte_allocator_type>::size_type;
        using difference_type = typename std::vector<uint8_t,byte_allocator_type>::difference_type;
        using reference = typename std::vector<uint8_t,byte_allocator_type>::reference;
        using const_reference = typename std::vector<uint8_t,byte_allocator_type>::const_reference;
        using pointer = typename std::vector<uint8_t,byte_allocator_type>::pointer;
        using const_pointer = typename std::vector<uint8_t,byte_allocator_type>::const_pointer;
        using iterator = typename std::vector<uint8_t,byte_allocator_type>::iterator;
        using const_iterator = typename std::vector<uint8_t,byte_allocator_type>::const_iterator;

        basic_byte_string() = default;

        explicit basic_byte_string(const Allocator& alloc)
            : data_(alloc)
        {
        }

        basic_byte_string(std::initializer_list<uint8_t> init)
            : data_(std::move(init))
        {
        }

        basic_byte_string(std::initializer_list<uint8_t> init, const Allocator& alloc)
            : data_(std::move(init), alloc)
        {
        }

        explicit basic_byte_string(const byte_string_view& v)
            : data_(v.begin(),v.end())
        {
        }

        basic_byte_string(const basic_byte_string<Allocator>& v)
            : data_(v.data_)
        {
        }

        basic_byte_string(basic_byte_string<Allocator>&& v) noexcept
            : data_(std::move(v.data_))
        {
        }

        basic_byte_string(const byte_string_view& v, const Allocator& alloc)
            : data_(v.begin(),v.end(),alloc)
        {
        }

        basic_byte_string(const uint8_t* data, std::size_t length, const Allocator& alloc = Allocator())
            : data_(data, data+length,alloc)
        {
        }

        Allocator get_allocator() const
        {
            return data_.get_allocator();
        }

        basic_byte_string& operator=(const basic_byte_string& s) = default;

        basic_byte_string& operator=(basic_byte_string&& other) noexcept
        {
            data_.swap(other.data_);
            return *this;
        }

        void reserve(std::size_t new_cap)
        {
            data_.reserve(new_cap);
        }

        void push_back(uint8_t b)
        {
            data_.push_back(b);
        }

        void assign(const uint8_t* s, std::size_t count)
        {
            data_.clear();
            data_.insert(data_.end(), s, s+count);
        }

        void append(const uint8_t* s, std::size_t count)
        {
            data_.insert(data_.end(), s, s+count);
        }

        void clear()
        {
            data_.clear();
        }

        uint8_t operator[](size_type pos) const 
        { 
            return data_[pos]; 
        }

        // iterator support 
        iterator begin() noexcept
        {
            return data_.begin();
        }
        iterator end() noexcept
        {
            return data_.end();
        }

        const_iterator begin() const noexcept
        {
            return data_.begin();
        }
        const_iterator end() const noexcept
        {
            return data_.end();
        }

        uint8_t* data()
        {
            return data_.data();
        }

        const uint8_t* data() const
        {
            return data_.data();
        }

        std::size_t size() const
        {
            return data_.size();
        }

        int compare(const byte_string_view& s) const noexcept 
        {
            const int rc = traits_type::compare(data(), s.data(), (std::min)(size(), s.size()));
            return rc != 0 ? rc : (size() == s.size() ? 0 : size() < s.size() ? -1 : 1);
        }

        int compare(const basic_byte_string& s) const noexcept 
        {
            const int rc = traits_type::compare(data(), s.data(), (std::min)(size(), s.size()));
            return rc != 0 ? rc : (size() == s.size() ? 0 : size() < s.size() ? -1 : 1);
        }

        template <typename CharT>
        friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const basic_byte_string& o)
        {
            os << byte_string_view(o);
            return os;
        }
    };

    template <typename Allocator>
    constexpr byte_string_view::byte_string_view(const basic_byte_string<Allocator>& bytes) 
        : data_(bytes.data()), size_(bytes.size())
    {
    }

    // ==
    inline
    bool operator==(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }
    template <typename Allocator>
    bool operator==(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }
    template <typename Allocator>
    bool operator==(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) == 0;
    }
    template <typename Allocator>
    bool operator==(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) == 0;
    }

    // !=

    inline
    bool operator!=(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) != 0;
    }
    template <typename Allocator>
    bool operator!=(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) != 0;
    }
    template <typename Allocator>
    bool operator!=(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) != 0;
    }
    template <typename Allocator>
    bool operator!=(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) != 0;
    }

    // <=

    inline
    bool operator<=(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) <= 0;
    }
    template <typename Allocator>
    bool operator<=(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) <= 0;
    }
    template <typename Allocator>
    bool operator<=(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) >= 0;
    }
    template <typename Allocator>
    bool operator<=(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) >= 0;
    }

    // <

    inline
    bool operator<(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) < 0;
    }
    template <typename Allocator>
    bool operator<(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) < 0;
    }
    template <typename Allocator>
    bool operator<(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) > 0;
    }
    template <typename Allocator>
    bool operator<(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) > 0;
    }

    // >=

    inline
    bool operator>=(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) >= 0;
    }
    template <typename Allocator>
    bool operator>=(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) >= 0;
    }
    template <typename Allocator>
    bool operator>=(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) <= 0;
    }
    template <typename Allocator>
    bool operator>=(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) <= 0;
    }

    // >

    inline
    bool operator>(const byte_string_view& lhs, const byte_string_view& rhs) noexcept
    {
        return lhs.compare(rhs) > 0;
    }
    template <typename Allocator>
    bool operator>(const byte_string_view& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return lhs.compare(rhs) > 0;
    }
    template <typename Allocator>
    bool operator>(const basic_byte_string<Allocator>& lhs, const byte_string_view& rhs) noexcept
    {
        return rhs.compare(lhs) < 0;
    }
    template <typename Allocator>
    bool operator>(const basic_byte_string<Allocator>& lhs, const basic_byte_string<Allocator>& rhs) noexcept
    {
        return rhs.compare(lhs) < 0;
    }

    using byte_string = basic_byte_string<std::allocator<uint8_t>>;

    namespace ext_traits {

        template <typename T>
        struct is_basic_byte_string
        : std::false_type
        {};

        template <typename Allocator>
        struct is_basic_byte_string<basic_byte_string<Allocator>>
        : std::true_type
        {};

    } // namespace ext_traits

} // namespace jsoncons

#endif // JSONCONS_BYTE_STRING_HPP
