// Copyright 2018 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_BIGINT_HPP
#define JSONCONS_UTILITY_BIGINT_HPP

#include <algorithm> // std::max, std::min, std::reverse
#include <cassert> // assert
#include <climits>
#include <cmath> // std::fmod
#include <cstdint>
#include <cstring> // std::memcpy
#include <ostream>
#include <limits> // std::numeric_limits
#include <memory> // std::allocator
#include <string> // std::string
#include <system_error>
#include <type_traits> // std::enable_if
#include <vector> // std::vector

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
//#include <jsoncons/conversion_result.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons {

namespace detail {

// bits per digit in the given radix times 1024
// Rounded up to avoid underallocation.
JSONCONS_INLINE_CONSTEXPR uint64_t bits_per_digit[] = { 0, 0,
    1024, 1624, 2048, 2378, 2648, 2875, 3072, 3247, 3402, 3543, 3672,
    3790, 3899, 4001, 4096, 4186, 4271, 4350, 4426, 4498, 4567, 4633,
    4696, 4756, 4814, 4870, 4923, 4975, 5025, 5074, 5120, 5166, 5210,
    5253, 5295};

template <typename Allocator>
class bigint_storage : private std::allocator_traits<Allocator>:: template rebind_alloc<uint64_t>
{
public:
    using word_allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<uint64_t>;
    using size_type = typename std::allocator_traits<word_allocator_type>::size_type;
    using word_type = typename std::allocator_traits<word_allocator_type>::value_type;
    static constexpr word_type max_word = (std::numeric_limits<word_type>::max)();
    static constexpr size_type mem_unit = sizeof(word_type);
    static constexpr size_type word_type_bits = sizeof(word_type) * 8;  // Number of bits
    static constexpr size_type word_type_half_bits = word_type_bits/2;
    static constexpr size_type inlined_capacity = 2;
public:

    template <class ValueType>
    class storage_view
    {
        ValueType* data_;
        size_type size_;

    public:
        storage_view(ValueType* data, size_type size)
            : data_(data), size_(size)
        {
        }

        ValueType& operator[](size_type i) 
        {
            return data_[i];
        }

        ValueType operator[](size_type i) const
        {
            return data_[i];
        }

        ValueType* data()
        {
            return data_;
        }

        size_type size() const
        {
            return size_;
        }
        ValueType* begin()
        {
            return data_;
        }

        ValueType* end()
        {
            return data_ + size_;
        }
    };

    struct common_storage
    {
        uint8_t is_allocated_ : 1;
        uint8_t is_negative_ : 1;
        size_type size_;
    };

    struct inlined_storage
    {
        uint8_t is_allocated_ : 1;
        uint8_t is_negative_ : 1;
        size_type size_;
        word_type values_[inlined_capacity];

        inlined_storage()
            : is_allocated_(false),
            is_negative_(false),
            size_(0),
            values_{0, 0}
        {
        }

        template <typename T>
        inlined_storage(T n,
            typename std::enable_if<std::is_integral<T>::value &&
            sizeof(T) <= sizeof(int64_t) &&
            std::is_signed<T>::value>::type* = 0)
            : is_allocated_(false),
            is_negative_(n < 0),
            size_(n == 0 ? 0 : 1)
        {
            values_[0] = n < 0 ? (word_type(0) - static_cast<word_type>(n)) : static_cast<word_type>(n);
            values_[1] = 0;
        }

        template <typename T>
        inlined_storage(T n,
            typename std::enable_if<std::is_integral<T>::value &&
            sizeof(T) <= sizeof(int64_t) &&
            !std::is_signed<T>::value>::type* = 0)
            : is_allocated_(false),
            is_negative_(false),
            size_(n == 0 ? 0 : 1)
        {
            values_[0] = n;
            values_[1] = 0;
        }

        template <typename T>
        inlined_storage(T n,
            typename std::enable_if < std::is_integral<T>::value &&
            sizeof(int64_t) < sizeof(T) &&
            std::is_signed<T>::value > ::type* = 0)
            : is_allocated_(false),
            is_negative_(n < 0),
            size_(n == 0 ? 0 : inlined_capacity)
        {
            using unsigned_type = typename std::make_unsigned<T>::type;

            auto u = n < 0 ? (unsigned_type(0) - static_cast<unsigned_type>(n)) : static_cast<unsigned_type>(n);
            values_[0] = word_type(u & max_word);;
            u >>= word_type_bits;
            values_[1] = word_type(u & max_word);;
        }

        template <typename T>
        inlined_storage(T n,
            typename std::enable_if < std::is_integral<T>::value &&
            sizeof(int64_t) < sizeof(T) &&
            !std::is_signed<T>::value > ::type* = 0)
            : is_allocated_(false),
            is_negative_(false),
            size_(n == 0 ? 0 : inlined_capacity)
        {
            values_[0] = word_type(n & max_word);;
            n >>= word_type_bits;
            values_[1] = word_type(n & max_word);;
        }

        inlined_storage(const inlined_storage& stor)
            : is_allocated_(false),
            is_negative_(stor.is_negative_),
            size_(stor.size_)
        {
            values_[0] = stor.values_[0];
            values_[1] = stor.values_[1];
        }

        inlined_storage& operator=(const inlined_storage& stor) = delete;
        inlined_storage& operator=(inlined_storage&& stor) = delete;
    };

    struct allocated_storage
    {
        using real_allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<word_type>;
        using pointer = typename std::allocator_traits<real_allocator_type>::pointer;

        uint8_t is_allocated_ : 1;
        uint8_t is_negative_ : 1;
        size_type size_{0};
        size_type capacity_{0};
        pointer data_{nullptr};

        allocated_storage()
            : is_allocated_(true),
            is_negative_(false)
        {
        }

        allocated_storage(const allocated_storage& stor, const real_allocator_type& a)
            : is_allocated_(true),
              is_negative_(stor.is_negative_),
              size_(stor.size_),
              capacity_(round_up(stor.size_))
        {
            real_allocator_type alloc(a);

            data_ = std::allocator_traits<real_allocator_type>::allocate(alloc, capacity_);
            JSONCONS_TRY
            {
                std::allocator_traits<real_allocator_type>::construct(alloc, ext_traits::to_plain_pointer(data_));
            }
            JSONCONS_CATCH(...)
            {
                std::allocator_traits<real_allocator_type>::deallocate(alloc, data_, capacity_);
                data_ = nullptr;
                JSONCONS_RETHROW;
            }
            JSONCONS_ASSERT(stor.data_ != nullptr);
            std::memcpy(data_, stor.data_, size_type(stor.size_ * sizeof(word_type)));
        }

        allocated_storage(allocated_storage&& stor) noexcept
            : is_allocated_(stor.is_allocated_),
            is_negative_(stor.is_negative_),
            size_(stor.size_),
            capacity_(stor.capacity_),
            data_(stor.data_)
        {
            stor.is_allocated_ = false;
            stor.is_negative_ = false;
            stor.size_ = 0;
            stor.capacity_ = 0;
            stor.data_ = nullptr;
        }

        void destroy(const real_allocator_type& a) noexcept
        {
            if (data_ != nullptr)
            {
                real_allocator_type alloc(a);
                std::allocator_traits<real_allocator_type>::deallocate(alloc, data_, capacity_);
            }
        }

        void reserve(size_type n, const real_allocator_type& a)
        {
            size_type capacity_new = round_up(n);

            real_allocator_type alloc(a);
            word_type* data_new = std::allocator_traits<real_allocator_type>::allocate(alloc, capacity_new);
            if (size_ > 0)
            {
                std::memcpy(data_new, data_, size_type(size_ * sizeof(word_type)));
            }
            if (data_ != nullptr)
            {
                std::allocator_traits<real_allocator_type>::deallocate(alloc, data_, capacity_);
            }
            capacity_ = capacity_new;
            data_ = data_new;
        }

        // Find suitable new block size
        constexpr size_type round_up(size_type i) const noexcept
        {
            return ((i + 1/3) / mem_unit + 1) * mem_unit;
        }
    };

    union
    {
        common_storage common_;
        inlined_storage inlined_;
        allocated_storage allocated_;
    };

    explicit bigint_storage(const Allocator& alloc = Allocator{})
        : word_allocator_type(alloc)
    {
        ::new (&inlined_) inlined_storage();
    }

    bigint_storage(const bigint_storage& other)
        : word_allocator_type(other.get_allocator())
    {
        if (!other.is_allocated())
        {
            ::new (&inlined_) inlined_storage(other.inlined_);
        }
        else
        {
            ::new (&allocated_) allocated_storage(other.allocated_, get_allocator());
        }
    }

    bigint_storage(const bigint_storage& other, const Allocator& alloc)
        : word_allocator_type(alloc)
    {
        if (!other.is_allocated())
        {
            ::new (&inlined_) inlined_storage(other.inlined_);
        }
        else
        {
            ::new (&allocated_) allocated_storage(other.allocated_, alloc);
        }
    }

    bigint_storage(bigint_storage&& other) noexcept
        : word_allocator_type(other.get_allocator())
    {
        if (!other.is_allocated())
        {
            ::new (&inlined_) inlined_storage(other.inlined_);
        }
        else
        {
            ::new (&allocated_) allocated_storage(std::move(other.allocated_));
        }
    }

    bigint_storage(bigint_storage&& other, const Allocator& alloc) noexcept
        : word_allocator_type(alloc)
    {
        if (!other.is_allocated())
        {
            ::new (&inlined_) inlined_storage(other.inlined_);
        }
        else
        {
            ::new (&allocated_) allocated_storage(std::move(other.allocated_), get_allocator());
        }
    }

    template <typename Integer>
    bigint_storage(Integer n, const Allocator& alloc = Allocator(), 
        typename std::enable_if<std::is_integral<Integer>::value>::type* = 0)
        : word_allocator_type(alloc)
    {
        ::new (&inlined_) inlined_storage(n);
    }

    bigint_storage& operator=(const bigint_storage& other)
    {
        if (this != &other)
        {
            auto other_view = other.get_storage_view();
            resize(other_view.size());
            auto this_view = get_storage_view();
            if (other_view.size() > 0)
            {
                common_.is_negative_ = other.common_.is_negative_;
                std::memcpy(this_view.data(), other_view.data(), size_type(other_view.size()*sizeof(word_type)));
            }
        }
        return *this;
    }

    bigint_storage& operator&=(const bigint_storage& a)
    {
        auto this_view = get_storage_view();
        auto a_view = a.get_storage_view();

        const size_type old_length = this_view.size();
        const size_type new_length = (std::min)(old_length, a_view.size());

        if (new_length != old_length)
        {
            resize(new_length);
            this_view = get_storage_view();
        }

        if (new_length > 0)
        {
            const word_type* first = this_view.begin();
            word_type* p = this_view.end() - 1;
            const word_type* q = a_view.begin() + this_view.size() - 1;

            while ( p >= first )
            {
                *p-- &= *q--;
            }

            if (old_length > new_length)
            {
                if (is_allocated())
                {
                    std::memset(allocated_.data_ + new_length, 0, size_type(old_length - new_length*sizeof(word_type)));
                }
                else
                {
                    JSONCONS_ASSERT(new_length <= inlined_capacity);
                    for (size_type i = new_length; i < inlined_capacity; ++i)
                    {
                        inlined_.values_[i] = 0;
                    }
                }
            }
        }

        reduce();

        return *this;
    }

    void reduce()
    {
        if (common_.size_ > 0)
        {
            auto this_view = get_storage_view();
            word_type* p = this_view.end() - 1;
            word_type* first = this_view.begin();
            while ( p >= first )
            {
                if ( *p )
                {
                    break;
                }
                --common_.size_;
                --p;
            }
        }
        if (common_.size_ == 0)
        {
            common_.is_negative_ = false;
        }
    }

    void reserve(size_type n)
    {
       if (capacity() < n)
       {
           if (!is_allocated())
           {
               size_type size = inlined_.size_;
               size_type is_neg = inlined_.is_negative_;
               word_type values[inlined_capacity] = {inlined_.values_[0], inlined_.values_[1]};

               ::new (&allocated_) allocated_storage();
               allocated_.reserve(n, get_allocator());
               allocated_.size_ = size;
               allocated_.is_negative_ = is_neg;
               if (n >= 1)
               {
                   allocated_.data_[0] = values[0];
               }
               if (n >= 2)
               {
                   allocated_.data_[1] = values[1];
               }
           }
           else
           {
               allocated_.reserve(n, get_allocator());
           }
       }
    }

    const word_allocator_type& get_allocator() const
    {
        return static_cast<const word_allocator_type&>(*this);
    }

    void destroy() noexcept
    {
        if (is_allocated())
        {
            allocated_.destroy(get_allocator());
            allocated_.~allocated_storage();
        }
        else
        {
            inlined_.~inlined_storage();
        }
    }

    constexpr bool is_allocated() const
    {
        return common_.is_allocated_;
    }

    constexpr size_type capacity() const
    {
        return is_allocated() ? allocated_.capacity_ : inlined_capacity;
    }

    bool is_negative() const
    {
        return common_.is_negative_;
    }

    void set_negative(bool value) 
    {
        common_.is_negative_ = value;
    }

    storage_view<word_type> get_storage_view()
    {
        return common_.is_allocated_ ? 
            storage_view<word_type>{allocated_.data_, allocated_.size_} :
            storage_view<word_type>{inlined_.values_, inlined_.size_};
    }

    storage_view<const word_type> get_storage_view() const
    {
        return common_.is_allocated_ ? 
            storage_view<const word_type>{allocated_.data_, allocated_.size_} :
            storage_view<const word_type>{inlined_.values_, inlined_.size_};
    }

    void resize(size_type new_length)
    {
        size_type old_length = common_.size_;
        reserve(new_length);
        common_.size_ = new_length;

        if (old_length < new_length)
        {
            if (is_allocated())
            {
                std::memset(allocated_.data_+old_length, 0, size_type((new_length-old_length)*sizeof(word_type)));
            }
            else
            {
                JSONCONS_ASSERT(new_length <= inlined_capacity);
                for (size_type i = old_length; i < inlined_capacity; ++i)
                {
                    inlined_.values_[i] = 0;
                }
            }
        }
    }
};

} // namespace detail

template <typename CharT>
struct to_bigint_result
{
    const CharT* ptr;
    std::errc ec;
    constexpr to_bigint_result(const CharT* ptr_)
        : ptr(ptr_), ec(std::errc{})
    {
    }
    constexpr to_bigint_result(const CharT* ptr_, std::errc ec_)
        : ptr(ptr_), ec(ec_)
    {
    }

    to_bigint_result(const to_bigint_result&) = default;

    to_bigint_result& operator=(const to_bigint_result&) = default;

    constexpr explicit operator bool() const noexcept
    {
        return ec == std::errc{};
    }
    std::error_code error_code() const
    {
        return make_error_code(ec);
    }
};

template <typename Allocator>
class basic_bigint;

template <typename CharT, typename Allocator>
to_bigint_result<CharT> to_bigint(const CharT* data, std::size_t length,
    basic_bigint<Allocator>& value, const Allocator& alloc);

template <typename CharT>
to_bigint_result<CharT> to_bigint(const CharT* data, std::size_t length,
    basic_bigint<std::allocator<uint64_t>>& value);

/*
This implementation is based on Chapter 2 and Appendix A of
Ammeraal, L. (1996) Algorithms and Data Structures in C++,
Chichester: John Wiley.

*/


template <typename Allocator = std::allocator<uint64_t>>
class basic_bigint 
{
    detail::bigint_storage<Allocator> storage_; 
public:

    using allocator_type = Allocator;
    using word_allocator_type = typename detail::bigint_storage<Allocator>::word_allocator_type;
    using allocator_traits_type = std::allocator_traits<word_allocator_type>;
    using stored_allocator_type = allocator_type;
    using pointer = typename allocator_traits_type::pointer;
    using size_type = typename detail::bigint_storage<Allocator>::size_type;
    using ssize_type = typename std::make_signed<size_type>::type;
    using word_type = typename detail::bigint_storage<Allocator>::word_type;
    using storage_view_type = typename detail::bigint_storage<Allocator>::template storage_view<word_type>;
    using const_storage_view_type = typename detail::bigint_storage<Allocator>::template storage_view<const word_type>;

    static constexpr size_type inlined_capacity = 2;

    static constexpr word_type max_word = (std::numeric_limits<word_type>::max)();
    static constexpr size_type word_type_bits = sizeof(word_type) * 8;  // Number of bits
    static constexpr size_type word_type_half_bits = word_type_bits/2;

    static constexpr uint16_t word_length = 8; // Use multiples of word_length words
    static constexpr word_type r_mask = (word_type(1) << word_type_half_bits) - 1;
    static constexpr word_type l_mask = max_word - r_mask;
    static constexpr word_type l_bit = max_word - (max_word >> 1);
    static constexpr word_type max_word_type_div_10 = (std::numeric_limits<word_type>::max)()/10u ;
    static constexpr word_type max_word_type_div_16 = (std::numeric_limits<word_type>::max)()/16u ;
    static constexpr word_type max_unsigned_power_10 = 10000000000000000000u; // max_unsigned_power_10 = ::pow(10, imax_unsigned_power_10)
    static constexpr size_type imax_unsigned_power_10 = 19u;
    static constexpr word_type max_unsigned_power_16 = 1152921504606846976u; // max_unsigned_power_16 = ::pow(16, imax_unsigned_power_16)
    static constexpr size_type imax_unsigned_power_16 = 15;

public:
    basic_bigint() = default;

    explicit basic_bigint(const Allocator& alloc)
        : storage_(alloc)
    {
    }

    template <typename CharT>
    basic_bigint(const CharT* s, const Allocator& alloc = Allocator())
        : storage_(alloc)
    {
        auto r = jsoncons::to_bigint(s, std::char_traits<CharT>::length(s), *this, alloc);
        if (r.ec != std::errc{})
        {
            JSONCONS_THROW(std::system_error((int)r.ec, std::system_category()));
        }
    }

    template <typename CharT>
    basic_bigint(const CharT* s, size_type length, const Allocator& alloc = Allocator())
        : storage_(alloc)
    {
        auto r = jsoncons::to_bigint(s, length, *this, alloc);
        if (r.ec != std::errc{})
        {
            JSONCONS_THROW(std::system_error((int)r.ec, std::system_category()));
        }
    }

    basic_bigint(const basic_bigint& other)
        : storage_(other.storage_)
    {
    }

    basic_bigint(const basic_bigint& other, const Allocator& alloc)
        : storage_(other.storage_, alloc)
    {
    }

    basic_bigint(basic_bigint&& other) noexcept
        : storage_(std::move(other.storage_))
    {
    }

    basic_bigint(basic_bigint&& other, const Allocator& alloc) noexcept
        : storage_(std::move(other.storage_), alloc)
    {
    }

    template <typename Integer>
    basic_bigint(Integer n, const Allocator& alloc = Allocator(), 
                 typename std::enable_if<std::is_integral<Integer>::value>::type* = 0)
        : storage_(n, alloc)
    {
    }

    template <typename StringViewLike,typename=typename std::enable_if<ext_traits::is_string_or_string_view<StringViewLike>::value>::type>
    basic_bigint(const StringViewLike& s)
    {
        auto r = jsoncons::to_bigint(s.data(), s.size(), *this);
        if (r.ec != std::errc{})
        {
            JSONCONS_THROW(std::system_error((int)r.ec, std::system_category()));
        }
    }

    ~basic_bigint() noexcept
    {
        storage_.destroy();
    }

    word_allocator_type get_allocator() const
    {
        return storage_.get_allocator();
    }

    storage_view_type get_storage_view()
    {
        return storage_.get_storage_view();
    }

    const_storage_view_type get_storage_view() const
    {
        return storage_.get_storage_view();
    }

    bool is_negative() const
    {
        return storage_.is_negative();
    }

    void set_negative(bool value) 
    {
        storage_.set_negative(value);
    }

    template <typename CharT>
    static to_bigint_result<CharT> parse(const std::basic_string<CharT>& s, basic_bigint<Allocator>& value)
    {
        return parse<CharT>(s.data(), s.size(), value);
    }

    template <typename CharT>
    static to_bigint_result<CharT> parse(const CharT* s, basic_bigint<Allocator>& value)
    {
        auto r = parse(s, std::char_traits<CharT>::length(s), value);
        if (r.ec != std::errc{})
        {
            JSONCONS_THROW(std::system_error((int)r.ec, std::system_category()));
        }
    }

    template <typename CharT>
    static basic_bigint<Allocator> parse_radix(const CharT* data, size_type length, uint8_t radix)
    {
        if (!(radix >= 2 && radix <= 16u))
        {
            JSONCONS_THROW(std::runtime_error("Unsupported radix"));
        }

        bool neg;
        if (*data == '-')
        {
            neg = true;
            data++;
            --length;
        }
        else
        {
            neg = false;
        }

        basic_bigint<Allocator> v = 0;
        for (size_type i = 0; i < length; i++)
        {
            CharT c = data[i];
            word_type d;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    d = (word_type)(c - '0');
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    d = (word_type)(c - ('a' - 10u));
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    d = (word_type)(c - ('A' - 10u));
                    break;
                default:
                    JSONCONS_THROW(std::runtime_error(std::string("Invalid digit in radix ") + std::to_string(radix) + ": \'" + (char)c + "\'"));
            }
            if (d >= radix)
            {
                JSONCONS_THROW(std::runtime_error(std::string("Invalid digit in radix ") + std::to_string(radix) + ": \'" + (char)c + "\'"));
            }
            v = (v * radix) + d;
        }

        if ( neg )
        {
            v.set_negative(true);
        }
        return v;
    }

    static basic_bigint from_bytes_be(int signum, const uint8_t* str, size_type n)
    {
        static const double radix_log2 = std::log2(next_power_of_two(256));
        // Estimate how big the result will be, so we can pre-allocate it.
        double bits = radix_log2 * n;
        double big_digits = std::ceil(bits / 64.0);
        //std::cout << "ESTIMATED: " << big_digits << "\n";

        basic_bigint<Allocator> v = 0;
        v.reserve(static_cast<size_type>(big_digits));

        if (n > 0)
        {
            for (size_type i = 0; i < n; i++)
            {
                v = (v * 256) + (word_type)(str[i]);
            }
        }
        //std::cout << "ACTUAL: " << v.size() << "\n";

        if (signum < 0)
        {
            v.set_negative(true);
        }

        return v;
    }

    void resize(size_type new_length)
    {
        storage_.resize(new_length);
    }

    void reserve(size_type n)
    {
        storage_.reserve(n);
    }

    // operators

    bool operator!() const
    {
        return get_storage_view().size() == 0 ? true : false;
    }

    basic_bigint operator-() const
    {
        basic_bigint<Allocator> v(*this);
        v.set_negative(!v.is_negative());
        return v;
    }

    basic_bigint& operator=( const basic_bigint& y )
    {
        storage_ = y.storage_;
        return *this;
    }

    basic_bigint& operator+=( const basic_bigint& y )
    {
        auto y_view = y.get_storage_view();
        
        if ( is_negative() != y.is_negative())
            return *this -= -y;
        word_type d;
        word_type carry = 0;

        auto this_view = get_storage_view();
        resize( (std::max)(y_view.size(), this_view.size()) + 1 );
        this_view = get_storage_view();

        for (size_type i = 0; i < this_view.size(); i++ )
        {
            if ( i >= y_view.size() && carry == 0 )
                break;
            d = this_view[i] + carry;
            carry = d < carry;
            if ( i < y_view.size())
            {
                this_view[i] = d + y_view[i];
                if (this_view[i] < d)
                    carry = 1;
            }
            else
            {
                this_view[i] = d;
            }
        }
        reduce();
        return *this;
    }

    basic_bigint& operator-=(const basic_bigint& y)
    {
        auto y_view = y.get_storage_view();

        if ( is_negative() != y.is_negative())
            return *this += -y;
        if ( (!is_negative() && y > *this) || (is_negative() && y < *this) )
            return *this = -(y - *this);
        word_type borrow = 0;
        word_type d;
        auto this_view = get_storage_view();
        for (size_type i = 0; i < this_view.size(); i++ )
        {
            if ( i >= y_view.size() && borrow == 0 )
                break;
            d = this_view[i] - borrow;
            borrow = d > this_view[i];
            if ( i < y_view.size())
            {
                this_view[i] = d - y_view[i];
                if ( this_view[i] > d )
                    borrow = 1;
            }
            else 
            {
                this_view[i] = d;
            }
        }
        reduce();
        return *this;
    }

    template <typename IntegerType>
    typename std::enable_if<ext_traits::is_signed_integer<IntegerType>::value, basic_bigint<Allocator>&>::type
    operator*=(IntegerType y)
    {
        *this *= word_type(y < 0 ? -y : y);
        if ( y < 0 )
            set_negative(!is_negative());
        return *this;
    }

    template <typename IntegerType>
    typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value, basic_bigint<Allocator>&>::type
    operator*=(IntegerType y)
    {
        auto this_view = get_storage_view();
        size_type len0 = this_view.size();
        word_type dig = this_view[0];
        word_type carry = 0;

        resize(this_view.size() + 1);
        this_view = get_storage_view();

        size_type i = 0;
        for (; i < len0; i++ )
        {
            word_type hi;
            word_type lo;
            DDproduct( dig, y, hi, lo );
            this_view[i] = lo + carry;
            dig = this_view[i+1];
            carry = hi + (this_view[i] < lo);
        }
        this_view[i] = carry;
        reduce();
        return *this;
    }

    basic_bigint& operator*=(basic_bigint y) 
    {
        auto this_view = get_storage_view();
        auto y_view = y.get_storage_view();

        if (this_view.size() == 0 || y_view.size() == 0)
        {
            return *this = 0;
        }

        bool difSigns = is_negative() != y.is_negative();
        if ( this_view.size() + y_view.size() == 2 ) // size() = y.size() = 1
        {
            word_type a = this_view[0], b = y_view[0];
            this_view[0] = a * b;
            if ( this_view[0] / a != b )
            {
                resize(2);
                this_view = get_storage_view();
                DDproduct( a, b, this_view[1], this_view[0] );
            }
            set_negative(difSigns);
            return *this;
        }

        if ( this_view.size() == 1 )  //  && y.size() > 1
        {
            word_type digit = this_view[0];
            *this = y;
            *this *= digit;
        }
        else
        {
            if (y_view.size() == 1)
            {
                *this *= y_view[0];
            }
            else
            {
                size_type lenProd = this_view.size() + y_view.size();
                word_type sumHi = 0, sumLo, hi, lo,
                sumLo_old, sumHi_old, carry=0;
                basic_bigint<Allocator> x = *this;
                auto x_view = x.get_storage_view();
                resize( lenProd ); // Give *this length lenProd
                this_view = get_storage_view();

                for (size_type i = 0; i < lenProd; i++ )
                {
                    sumLo = sumHi;
                    sumHi = carry;
                    carry = 0;
                    for (size_type jA=0; jA < x_view.size(); jA++)
                    {
                        if (JSONCONS_LIKELY(i >= jA))
                        {
                            size_type jB = i - jA;
                            if (jB < y_view.size())
                            {
                                DDproduct( x_view[jA], y_view[jB], hi, lo );
                                sumLo_old = sumLo;
                                sumHi_old = sumHi;
                                sumLo += lo;
                                if ( sumLo < sumLo_old )
                                    sumHi++;
                                sumHi += hi;
                                carry += (sumHi < sumHi_old);
                            }
                        }
                    }
                    this_view[i] = sumLo;
                }
            }
        }
       reduce();
       set_negative(difSigns);
       return *this;
    }

    basic_bigint& operator/=( const basic_bigint& divisor )
    {
        basic_bigint<Allocator> r;
        divide( divisor, *this, r, false );
        return *this;
    }

    basic_bigint& operator%=( const basic_bigint& divisor )
    {
        basic_bigint<Allocator> q;
        divide( divisor, q, *this, true );
        return *this;
    }

    basic_bigint& operator<<=(size_type k)
    {
        auto this_view = get_storage_view();
        size_type q = k / word_type_bits;
        if ( q ) // Increase storage_.size() by q:
        {
            resize(this_view.size() + q);
            this_view = get_storage_view();
            for (size_type i = this_view.size(); i-- > 0; )
                this_view[i] = ( i < q ? 0 : this_view[i - q]);
            k %= word_type_bits;
        }
        if ( k )  // 0 < k < word_type_bits:
        {
            size_type k1 = word_type_bits - k;
            word_type mask = (word_type(1) << k) - word_type(1);
            resize( this_view.size() + 1 );
            this_view = get_storage_view();
            for (size_type i = this_view.size(); i-- > 0; )
            {
                this_view[i] <<= k;
                if ( i > 0 )
                    this_view[i] |= (this_view[i-1] >> k1) & mask;
            }
        }
        reduce();
        return *this;
    }

    basic_bigint& operator>>=(size_type k)
    {
        auto this_view = get_storage_view();
        size_type q = k / word_type_bits;
        if ( q >= this_view.size())
        {
            resize( 0 );
            return *this;
        }
        if (q > 0)
        {
            memmove( this_view.data(), this_view.data()+q, size_type((this_view.size() - q)*sizeof(word_type)) );
            resize( size_type(this_view.size() - q) );
            k %= word_type_bits;
            if ( k == 0 )
            {
                reduce();
                return *this;
            }
        }

        this_view = get_storage_view();
        size_type n = size_type(this_view.size() - 1);
        ssize_type k1 = word_type_bits - k;
        word_type mask = (word_type(1) << k) - 1;
        for (size_type i = 0; i <= n; i++)
        {
            this_view[i] >>= k;
            if ( i < n )
                this_view[i] |= ((this_view[i+1] & mask) << k1);
        }
        reduce();
        return *this;
    }

    basic_bigint& operator++()
    {
        *this += 1;
        return *this;
    }

    basic_bigint<Allocator> operator++(int)
    {
        basic_bigint<Allocator> old = *this;
        ++(*this);
        return old;
    }

    basic_bigint& operator--()
    {
        *this -= 1;
        return *this;
    }

    basic_bigint<Allocator> operator--(int)
    {
        basic_bigint<Allocator> old = *this;
        --(*this);
        return old;
    }

    basic_bigint& operator|=( const basic_bigint& a )
    {
        auto a_view = a.get_storage_view();

        if (a_view.size() > 0)
        {
            auto this_view = get_storage_view();

            if ( this_view.size() < a_view.size())
            {
                resize( a_view.size());
                this_view = get_storage_view();
            }

            const word_type* qfirst = a_view.begin();
            const word_type* q = a_view.end() - 1;
            word_type* p = this_view.begin() + a_view.size() - 1;

            while (q >= qfirst)
            {
                *p-- |= *q--;
            }
            reduce();
        }

        return *this;
    }

    basic_bigint& operator^=( const basic_bigint& a )
    {
        auto a_view = a.get_storage_view();

        if (a_view.size() > 0)
        {
            auto this_view = get_storage_view();
            if (this_view.size() < a_view.size())
            {
                resize(a_view.size());
                this_view = get_storage_view();
            }

            const word_type* qfirst = a_view.begin();
            const word_type* q = a_view.end() - 1;
            word_type* p = this_view.begin() + a_view.size() - 1;

            while (q >= qfirst)
            {
                *p-- ^= *q--;
            }
            reduce();
        }

        return *this;
    }

    basic_bigint& operator&=( const basic_bigint& a )
    {
        storage_ &= a.storage_;

        return *this;
    }

    explicit operator bool() const
    {
       return get_storage_view().size() != 0 ? true : false;
    }

    template <typename Integer, typename = typename std::enable_if<std::is_integral<Integer>::value && sizeof(Integer) <= sizeof(int64_t)>::type>
    explicit operator Integer() const
    {
        auto this_view = get_storage_view();
        Integer x = 0;
        if (this_view.size() > 0)
        {
            x = static_cast<Integer>(this_view[0]);
        }

        return is_negative() ? x*(-1) : x;
    }

    explicit operator double() const
    {
        double x = 0.0;
        double factor = 1.0;
        double values = (double)max_word + 1.0;

        auto this_view = get_storage_view();

        const word_type* p = this_view.begin();
        const word_type* pEnd = this_view.end();
        while ( p < pEnd )
        {
            x += *p*factor;
            factor *= values;
            ++p;
        }

       return is_negative() ? -x : x;
    }

    explicit operator long double() const
    {
        long double x = 0.0;
        long double factor = 1.0;
        long double values = (long double)max_word + 1.0;

        auto this_view = get_storage_view();

        const word_type* p = this_view.begin();
        const word_type* pEnd = this_view.end();
        while ( p < pEnd )
        {
            x += *p*factor;
            factor *= values;
            ++p;
        }

       return is_negative() ? -x : x;
    }

    template <typename Alloc>
    void write_bytes_be(int& signum, std::vector<uint8_t,Alloc>& data) const
    {
        basic_bigint<Allocator> n(*this);
        signum = (n < 0) ? -1 : (n > 0 ? 1 : 0); 

        basic_bigint<Allocator> divisor(256);

        while (n >= 256)
        {
            basic_bigint<Allocator> q;
            basic_bigint<Allocator> r;
            n.divide(divisor, q, r, true);
            n = q;
            data.push_back((uint8_t)(word_type)r);
        }
        if (n >= 0)
        {
            data.push_back((uint8_t)(word_type)n);
        }

        std::reverse(data.begin(),data.end());
    }

    std::string to_string() const
    {
        std::string s;
        write_string(s);
        return s;
    }

    template <typename Ch,typename Traits,typename Alloc>
    void write_string(std::basic_string<Ch,Traits,Alloc>& data) const
    {
        basic_bigint<Allocator> v(*this);
        auto v_view = v.get_storage_view();

        size_type len = (v_view.size() * word_type_bits / 3) + 2;
        data.reserve(len);

        if ( v_view.size() == 0 )
        {
            data.push_back('0');
        }
        else
        {
            word_type r;
            basic_bigint<Allocator> R(get_allocator());
            basic_bigint<Allocator> LP10(max_unsigned_power_10, get_allocator()); 

            do
            {
                v.divide( LP10, v, R, true );
                v_view = v.get_storage_view();

                auto R_view = R.get_storage_view();
                r = (R_view.size() ? R_view[0] : 0);
                for ( size_type j=0; j < imax_unsigned_power_10; j++ )
                {
                    data.push_back(char(r % 10u + '0'));
                    r /= 10u;
                    if ( r + v_view.size() == 0 )
                        break;
                }
            } 
            while ( v_view.size() > 0);

            if (is_negative())
            {
                data.push_back('-');
            }
            std::reverse(data.begin(),data.end());
        }
    }

    std::string to_string_hex() const
    {
        std::string s;
        write_string_hex(s);
        return s;
    }

    template <typename Ch,typename Traits,typename Alloc>
    void write_string_hex(std::basic_string<Ch,Traits,Alloc>& data) const
    {


        basic_bigint<Allocator> v(*this);
        auto v_view = v.get_storage_view();

        size_type len = (v_view.size() * basic_bigint<Allocator>::word_type_bits / 3) + 2;
        data.reserve(len);

        if ( v_view.size() == 0 )
        {
            data.push_back('0');
        }
        else
        {
            word_type r;
            basic_bigint<Allocator> R;
            basic_bigint<Allocator> LP10 = max_unsigned_power_16; // LP10 = max_unsigned_power_16 = ::pow(16, imax_unsigned_power_16)
            do
            {
                v.divide( LP10, v, R, true );
                v_view = v.get_storage_view();
                auto R_view = R.get_storage_view();
                r = (R_view.size() ? R_view[0] : 0);
                for ( size_type j=0; j < imax_unsigned_power_16; j++ )
                {
                    uint8_t c = r % 16u;
                    data.push_back((c < 10u) ? ('0' + c) : ('A' - 10u + c));
                    r /= 16u;
                    if ( r + v_view.size() == 0 )
                        break;
                }
            } 
            while (v_view.size() > 0);

            if (is_negative())
            {
                data.push_back('-');
            }
            std::reverse(data.begin(),data.end());
        }
    }

//  Global Operators

    friend bool operator==( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
        return x.compare(y) == 0 ? true : false;
    }

    friend bool operator==( const basic_bigint& x, int y ) noexcept
    {
        return x.compare(y) == 0 ? true : false;
    }

    friend bool operator!=( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
        return x.compare(y) != 0 ? true : false;
    }

    friend bool operator!=( const basic_bigint& x, int y ) noexcept
    {
        return x.compare(basic_bigint<Allocator>(y)) != 0 ? true : false;
    }

    friend bool operator<( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
       return x.compare(y) < 0 ? true : false;
    }

    friend bool operator<( const basic_bigint& x, int64_t y ) noexcept
    {
       return x.compare(y) < 0 ? true : false;
    }

    friend bool operator>( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
        return x.compare(y) > 0 ? true : false;
    }

    friend bool operator>( const basic_bigint& x, int y ) noexcept
    {
        return x.compare(basic_bigint<Allocator>(y)) > 0 ? true : false;
    }

    friend bool operator<=( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
        return x.compare(y) <= 0 ? true : false;
    }

    friend bool operator<=( const basic_bigint& x, int y ) noexcept
    {
        return x.compare(y) <= 0 ? true : false;
    }

    friend bool operator>=( const basic_bigint& x, const basic_bigint& y ) noexcept
    {
        return x.compare(y) >= 0 ? true : false;
    }

    friend bool operator>=( const basic_bigint& x, int y ) noexcept
    {
        return x.compare(y) >= 0 ? true : false;
    }

    friend basic_bigint<Allocator> operator+( basic_bigint x, const basic_bigint& y )
    {
        return x += y;
    }

    friend basic_bigint<Allocator> operator+( basic_bigint x, int64_t y )
    {
        return x += y;
    }

    friend basic_bigint<Allocator> operator-( basic_bigint x, const basic_bigint& y )
    {
        return x -= y;
    }

    friend basic_bigint<Allocator> operator-( basic_bigint x, int64_t y )
    {
        return x -= y;
    }

    friend basic_bigint<Allocator> operator*( int64_t x, const basic_bigint& y )
    {
        return basic_bigint<Allocator>(y) *= x;
    }

    friend basic_bigint<Allocator> operator*( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x *= y;
    }

    friend basic_bigint<Allocator> operator*( basic_bigint<Allocator> x, int64_t y )
    {
        return x *= y;
    }

    friend basic_bigint<Allocator> operator/( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x /= y;
    }

    friend basic_bigint<Allocator> operator/( basic_bigint<Allocator> x, int y )
    {
        return x /= y;
    }

    friend basic_bigint<Allocator> operator%( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x %= y;
    }

    friend basic_bigint<Allocator> operator<<( basic_bigint<Allocator> u, unsigned k )
    {
        return u <<= k;
    }

    friend basic_bigint<Allocator> operator<<( basic_bigint<Allocator> u, int k )
    {
        return u <<= k;
    }

    friend basic_bigint<Allocator> operator>>( basic_bigint<Allocator> u, unsigned k )
    {
        return u >>= k;
    }

    friend basic_bigint<Allocator> operator>>( basic_bigint<Allocator> u, int k )
    {
        return u >>= k;
    }

    friend basic_bigint<Allocator> operator|( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x |= y;
    }

    friend basic_bigint<Allocator> operator|( basic_bigint<Allocator> x, int y )
    {
        return x |= y;
    }

    friend basic_bigint<Allocator> operator|( basic_bigint<Allocator> x, unsigned y )
    {
        return x |= y;
    }

    friend basic_bigint<Allocator> operator^( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x ^= y;
    }

    friend basic_bigint<Allocator> operator^( basic_bigint<Allocator> x, int y )
    {
        return x ^= y;
    }

    friend basic_bigint<Allocator> operator^( basic_bigint<Allocator> x, unsigned y )
    {
        return x ^= y;
    }

    friend basic_bigint<Allocator> operator&( basic_bigint<Allocator> x, const basic_bigint& y )
    {
        return x &= y;
    }

    friend basic_bigint<Allocator> operator&( basic_bigint<Allocator> x, int y )
    {
        return x &= y;
    }

    friend basic_bigint<Allocator> operator&( basic_bigint<Allocator> x, unsigned y )
    {
        return x &= y;
    }

    template <typename CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const basic_bigint& v)
    {
        std::basic_string<CharT> s;
        v.write_string(s); 
        os << s;

        return os;
    }

    int compare( const basic_bigint& y ) const noexcept
    {
        auto this_view = get_storage_view();
        auto y_view = y.get_storage_view();

        if ( this_view.size() == 0 && y_view.size() == 0 )
            return 0;
        if ( is_negative() != y.is_negative())
            return y.is_negative() - is_negative();
        int code = 0;
        if ( this_view.size() < y_view.size())
            code = -1;
        else if ( this_view.size() > y_view.size())
            code = +1;
        else
        {
            for (size_type i = this_view.size(); i-- > 0; )
            {
                if (this_view[i] > y_view[i])
                {
                    code = 1;
                    break;
                }
                else if (this_view[i] < y_view[i])
                {
                    code = -1;
                    break;
                }
            }
        }
        return is_negative() ? -code : code;
    }

    void divide(const basic_bigint& denom_, basic_bigint& quot, basic_bigint& rem, bool remDesired ) const
    {
        basic_bigint<Allocator> denom(denom_, get_allocator());
        auto denom_view = denom.get_storage_view();

        if (denom_view.size() == 0)
        {
            JSONCONS_THROW(std::runtime_error( "Zero divide." ));
        }
        bool quot_neg = is_negative() ^ denom.is_negative();
        bool rem_neg = is_negative();
        basic_bigint<Allocator> num(*this, get_allocator());
        num.set_negative(false);
        denom.set_negative(false);
        if ( num < denom )
        {
            quot = word_type(0);
            quot.set_negative(quot_neg);
            rem = num;
            rem.set_negative(rem_neg);
            return;
        }

        auto num_view = num.get_storage_view();
        auto quot_view = quot.get_storage_view();
        auto this_view = get_storage_view();

        if ( denom_view.size() == 1 && num_view.size() == 1 )
        {
            quot = word_type( num_view[0]/denom_view[0] );
            rem = word_type( num_view[0]%denom_view[0] );
            quot.set_negative(quot_neg);
            rem.set_negative(rem_neg);
            return;
        }
        if (denom_view.size() == 1 && (denom_view[0] & l_mask) == 0 )
        {
            // Denominator fits into a half word
            word_type divisor = denom_view[0], dHi = 0, q1, r, q2, dividend;
            quot.resize(this_view.size());
            quot_view = quot.get_storage_view();
            for (size_type i=this_view.size(); i-- > 0; )
            {
                dividend = (dHi << word_type_half_bits) | (this_view[i] >> word_type_half_bits);
                q1 = dividend/divisor;
                r = dividend % divisor;
                dividend = (r << word_type_half_bits) | (this_view[i] & r_mask);
                q2 = dividend/divisor;
                dHi = dividend % divisor;
                quot_view[i] = (q1 << word_type_half_bits) | q2;
            }
            quot.reduce();
            rem = dHi;
            quot.set_negative(quot_neg);
            rem.set_negative(rem_neg);
            return;
        }
        basic_bigint<Allocator> num0(num, get_allocator());
        basic_bigint<Allocator> denom0(denom, get_allocator());
        int x = 0;
        bool second_done = normalize(denom, num, x);
        denom_view = denom.get_storage_view();
        num_view = num.get_storage_view();

        size_type l = denom_view.size() - 1;
        size_type n = num_view.size() - 1;
        quot.resize(n - l);
        quot_view = quot.get_storage_view();
        for (size_type i = quot_view.size(); i-- > 0; )
        {
            quot_view[i] = 0;
        }
        rem = num;
        auto rem_view = rem.get_storage_view();
        if ( rem_view[n] >= denom_view[l] )
        {
            rem.resize(rem_view.size() + 1);
            rem_view = rem.get_storage_view();
            n++;
            quot.resize(quot_view.size() + 1);
            quot_view = quot.get_storage_view();
        }
        word_type d = denom_view[l];

        for ( size_type k = n; k > l; k-- )
        {
            word_type q = DDquotient(rem_view[k], rem_view[k-1], d);
            subtractmul( rem_view.data() + (k - l - 1), denom_view.data(), l + 1, q );
            quot_view[k - l - 1] = q;
        }
        quot.reduce();
        quot.set_negative(quot_neg);
        if (remDesired)
        {
            unnormalize(rem, x, second_done);
            rem.set_negative(rem_neg);
        }
    }
private:

    void destroy() noexcept
    {
        storage_.destroy();
    }
    void DDproduct( word_type A, word_type B,
                    word_type& hi, word_type& lo ) const
    // Multiplying two digits: (hi, lo) = A * B
    {
        word_type hiA = A >> word_type_half_bits, loA = A & r_mask,
                   hiB = B >> word_type_half_bits, loB = B & r_mask;

        lo = loA * loB;
        hi = hiA * hiB;
        word_type mid1 = loA * hiB;
        word_type mid2 = hiA * loB;
        word_type old = lo;
        lo += mid1 << word_type_half_bits;
            hi += (lo < old) + (mid1 >> word_type_half_bits);
        old = lo;
        lo += mid2 << word_type_half_bits;
            hi += (lo < old) + (mid2 >> word_type_half_bits);
    }

    word_type DDquotient( word_type A, word_type B, word_type d ) const
    // Divide double word (A, B) by d. Quotient = (qHi, qLo)
    {
        word_type left, middle, right, qHi, qLo, x, dLo1,
                   dHi = d >> word_type_half_bits, dLo = d & r_mask;
        qHi = A/(dHi + 1);
        // This initial guess of qHi may be too small.
        middle = qHi * dLo;
        left = qHi * dHi;
        x = B - (middle << word_type_half_bits);
        A -= (middle >> word_type_half_bits) + left + (x > B);
        B = x;
        dLo1 = dLo << word_type_half_bits;
        // Increase qHi if necessary:
        while ( A > dHi || (A == dHi && B >= dLo1) )
        {
            x = B - dLo1;
            A -= dHi + (x > B);
            B = x;
            qHi++;
        }
        qLo = ((A << word_type_half_bits) | (B >> word_type_half_bits))/(dHi + 1);
        // This initial guess of qLo may be too small.
        right = qLo * dLo;
        middle = qLo * dHi;
        x = B - right;
        A -= (x > B);
        B = x;
        x = B - (middle << word_type_half_bits);
            A -= (middle >> word_type_half_bits) + (x > B);
        B = x;
        // Increase qLo if necessary:
        while ( A || B >= d )
        {
            x = B - d;
            A -= (x > B);
            B = x;
            qLo++;
        }
        return (qHi << word_type_half_bits) + qLo;
    }

    void subtractmul( word_type* a, word_type* b, size_type n, word_type& q ) const
    // a -= q * b: b in n positions; correct q if necessary
    {
        word_type hi, lo, d, carry = 0;
        size_type i;
        for ( i = 0; i < n; i++ )
        {
            DDproduct( b[i], q, hi, lo );
            d = a[i];
            a[i] -= lo;
            if ( a[i] > d )
                carry++;
            d = a[i + 1];
            a[i + 1] -= hi + carry;
            carry = a[i + 1] > d;
        }
        if ( carry ) // q was too large
        {
            q--;
            carry = 0;
            for ( i = 0; i < n; i++ )
            {
                d = a[i] + carry;
                carry = d < carry;
                a[i] = d + b[i];
                if ( a[i] < d )
                    carry = 1;
            }
            a[n] = 0;
        }
    }

    bool normalize(basic_bigint& denom, basic_bigint& num, int& x) const
    {
        auto denom_view = denom.get_storage_view();
        if (denom_view.size() == 0)
        {
            return false;
        }
        size_type r = denom_view.size() - 1;
        word_type y = denom_view[r];

        x = 0;
        while ( (y & l_bit) == 0 )
        {
            y <<= 1;
            x++;
        }
        denom <<= x;
        num <<= x;

        denom_view = denom.get_storage_view();
        if ( r > 0 && denom_view[r] < denom_view[r-1] )
        {
            denom *= max_word;
            num *= max_word;
            return true;
        }
        return false;
    }

    void unnormalize(basic_bigint& rem, int x, bool secondDone) const
    {
        if (secondDone)
        {
            rem /= max_word;
        }
        if ( x > 0 )
        {
            rem >>= x;
        }
        else
        {
            rem.reduce();
        }
    }

    void reduce()
    {
        storage_.reduce();
    }
 
    static word_type next_power_of_two(word_type n) {
        n = n - 1;
        n |= n >> 1u;
        n |= n >> 2u;
        n |= n >> 4u;
        n |= n >> 8u;
        n |= n >> 16u;
        n |= n >> 32u;
        return n + 1;
    }

    template <typename CharT>
    friend to_bigint_result<CharT> to_bigint(const CharT* s, basic_bigint& val, int radix)
    {
        return to_bigint(s, std::char_traits<CharT>::length(s), val, radix);
    }

    template <typename CharT>
    friend to_bigint_result<CharT> to_bigint(const CharT* s, size_type length, basic_bigint& val, int radix)
    {
        if (!(radix >= 2 && radix <= 16))
        {
            JSONCONS_THROW(std::runtime_error("Unsupported radix"));
        }

        const CharT* cur = s;
        const CharT* last = s + length;

        bool neg;
        if (*cur == '-')
        {
            neg = true;
            cur++;
        }
        else
        {
            neg = false;
        }

        while (cur < last)
        {
            CharT c = *cur;
            word_type d;
            switch (c)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    d = (word_type)(c - '0');
                    break;
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                    d = (word_type)(c - ('a' - 10u));
                    break;
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    d = (word_type)(c - ('A' - 10u));
                    break;
                default:
                    return to_bigint_result<CharT>(cur, std::errc::invalid_argument);
            }
            if ((int)d >= radix)
            {
                return to_bigint_result<CharT>(cur, std::errc::invalid_argument);
            }
            val = (val * radix) + d;
            ++cur;
        }

        if ( neg )
        {
            val.set_negative(true);
        }
        return to_bigint_result<CharT>(cur, std::errc{});
    }
};

template <typename Allocator>
basic_bigint<Allocator> babs( const basic_bigint<Allocator>& a )
{
    if ( a.is_negative())
    {
        return -a;
    }
    return a;
}

template <typename Allocator>
basic_bigint<Allocator> bpow(basic_bigint<Allocator> x, unsigned n)
{
    basic_bigint<Allocator> y = 1;

    while ( n )
    {
        if ( n & 1 )
        {
            y *= x;
        }
        x *= x;
        n >>= 1;
    }

    return y;
}

template <typename Allocator>
basic_bigint<Allocator> bsqrt(const basic_bigint<Allocator>& a)
{
    basic_bigint<Allocator> x = a;
    basic_bigint<Allocator> b = a;
    basic_bigint<Allocator> q;

    b <<= 1;
    while ( (void)(b >>= 2), b > 0 )
    {
        x >>= 1;
    }
    while ( x > (q = a/x) + 1 || x < q - 1 )
    {
        x += q;
        x >>= 1;
    }
    return x < q ? x : q;
}

namespace detail {

template <typename CharT, typename Allocator>
to_bigint_result<CharT> to_bigint(const CharT* data, std::size_t length,
    bool neg, basic_bigint<Allocator>& value, const Allocator& alloc)
{
    if (JSONCONS_UNLIKELY(length == 0))
    {
        return to_bigint_result<CharT>(data, std::errc::invalid_argument);
    }

    using word_type = typename basic_bigint<Allocator>::word_type;

    const CharT* last = data + length;
    const CharT* p = data;

    while (p < last && *p == '0')
    {
        ++p;
    }
    if (p == last)
    {
        value = std::move(basic_bigint<Allocator>{0, alloc});
        return to_bigint_result<CharT>(last, std::errc{});
    }
    std::size_t num_digits = last - data;
    std::size_t num_words;
    if (length < 10)
    {
        num_words = 1;
    }
    else
    {
        std::size_t num_bits = (std::size_t)(((num_digits * detail::bits_per_digit[10]) >> 10) + 1);
        num_words = (num_bits + 63) >> 6;
    }

    basic_bigint<Allocator> v(0, alloc);
    v.reserve(num_words);
    for (std::size_t i = 0; i < length; i++)
    {
        CharT c = data[i];
        switch (c)
        {
            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                v = (v * 10u) + (word_type)(c - '0');
                break;
            default:
                return to_bigint_result<CharT>(data + i, std::errc::invalid_argument);
        }
    }

    //auto view = v.get_storage_view();
    //if (num_words != view.size())
    //{
    //    std::cout << "Unexpected num_words! num_words: " << num_words << ", " << num_words << ", size: " << view.size() << "\n";
    //}

    if (neg)
    {
        v.set_negative(true);
    }

    value = std::move(v);
    return to_bigint_result<CharT>(last, std::errc{});
}

} // namespace detail

template <typename CharT, typename Allocator>
to_bigint_result<CharT> to_bigint(const CharT* data, std::size_t length,
    basic_bigint<Allocator>& value, const Allocator& alloc)
{
    if (JSONCONS_UNLIKELY(length == 0))
    {
        return to_bigint_result<CharT>(data, std::errc::invalid_argument);
    }

    if (*data == '-')
    {
        return jsoncons::detail::to_bigint(data + 1, length - 1, true, value, alloc);
    }
    else
    {
        return jsoncons::detail::to_bigint(data, length, false, value, alloc);
    }
}

template <typename CharT>
to_bigint_result<CharT> to_bigint(const CharT* s, basic_bigint<std::allocator<uint64_t>>& value)
{
    return to_bigint(s, std::char_traits<CharT>::length(s), value);
}

template <typename CharT>
to_bigint_result<CharT> to_bigint(const CharT* data, std::size_t length,
    basic_bigint<std::allocator<uint64_t>>& value)
{
    if (JSONCONS_UNLIKELY(length == 0))
    {
        return to_bigint_result<CharT>(data, std::errc::invalid_argument);
    }

    if (*data == '-')
    {
        return jsoncons::detail::to_bigint(data+1, length-1, true, value, std::allocator<uint64_t>{}); 
    }
    else
    {
        return jsoncons::detail::to_bigint(data, length, false, value, std::allocator<uint64_t>{}); 
    }
}

using bigint = basic_bigint<std::allocator<uint64_t>>;

} // namespace jsoncons

#endif // JSONCONS_UTILITY_BIGINT_HPP
