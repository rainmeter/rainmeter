/// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons2 for latest version

#ifndef JSONCONS_DETAIL_EXPECTED_HPP    
#define JSONCONS_DETAIL_EXPECTED_HPP    

#include <system_error>
#include <type_traits>
#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/detail/utility.hpp>
#include <cassert>

namespace jsoncons {
namespace detail {
    
struct unexpect_t
{
    explicit unexpect_t() = default; 
};

JSONCONS_INLINE_CONSTEXPR unexpect_t unexpect{};

template <typename T, typename E>
class expected
{
public:
    using value_type = T;
    using error_type = E;
private:
    bool has_value_;
    union {
        E error_;
        T value_;
    };
public:
    template <typename U=T>
    expected(typename std::enable_if<std::is_default_constructible<U>::value, int>::type = 0)
        : expected(T{})
    {
    }

    expected(const T& value) 
        : has_value_(true)
    {
        construct(value);
    }

    expected(T&& value) noexcept
        : has_value_(true)
    {
        construct(std::move(value));
    }

    template <typename... Args>    
    expected(jsoncons::detail::in_place_t, Args&& ... args) noexcept
        : has_value_(true)
    {
        ::new (&value_) T(std::forward<Args>(args)...);
    }

    template <typename... Args>    
    expected(unexpect_t, Args&& ... args) noexcept
        : has_value_(false)
    {
        ::new (&error_) E(std::forward<Args>(args)...);
    }
    
    // copy constructors
    expected(const expected<T,E>& other) 
        : has_value_(other.has_value())
    {
        if (other)
        {
            construct(other.value_);
        }
        else
        {
            ::new (&error_) E(other.error_);
        }
    }

    // move constructors
    expected(expected<T,E>&& other) noexcept
        : has_value_(other.has_value())
    {
        if (other)
        {
            construct(std::move(other.value_));
        }
        else
        {
            ::new (&error_) E(other.error_);
        }
    }

    ~expected() noexcept
    {
        destroy();
    }

    expected& operator=(const expected& other)
    {
        if (other)
        {
            assign(*other);
        }
        else
        {
            destroy();
            ::new (&error_) E(other.error_);
        }
        return *this;
    }

    expected& operator=(expected&& other)
    {
        if (other)
        {
            assign(std::move(*other));
        }
        else
        {
            destroy();
            ::new (&error_) E(other.error_);
        }
        return *this;
    }

    // value assignment
    expected& operator=(const T& v)
    {
        assign(v);
        return *this;
    }

    expected& operator=(T&& v)
    {
        assign(std::move(v));
        return *this;
    }

    constexpr operator bool() const noexcept
    {
        return has_value_;
    }
    
    constexpr bool has_value() const noexcept
    {
        return has_value_;
    }

    JSONCONS_CPP14_CONSTEXPR T& value() &
    {
        if (has_value_)
        {
            return this->value_;
        }
        JSONCONS_THROW(std::runtime_error("Bad expected access"));
    }

    JSONCONS_CPP14_CONSTEXPR const T& value() const &
    {
        if (has_value_)
        {
            return this->value_;
        }
        JSONCONS_THROW(std::runtime_error("Bad expected access"));
    }

    JSONCONS_CPP14_CONSTEXPR T&& value() &&
    {
        if (has_value_)
        {
            return std::move(this->value_);
        }
        JSONCONS_THROW(std::runtime_error("Bad expected access"));
    }

    JSONCONS_CPP14_CONSTEXPR const T&& value() const &&
    {
        if (has_value_)
        {
            return std::move(this->value_);
        }
        JSONCONS_THROW(std::runtime_error("Bad expected access"));
    }

    JSONCONS_CPP14_CONSTEXPR E& error() & noexcept
    {
        assert(!has_value_);
        return this->error_;
    }

    JSONCONS_CPP14_CONSTEXPR const E& error() const& noexcept
    {
        assert(!has_value_);
        return this->error_;
    }

    JSONCONS_CPP14_CONSTEXPR E&& error() && noexcept
    {
        assert(!has_value_);
        return std::move(this->error_);
    }

    JSONCONS_CPP14_CONSTEXPR const E&& error() const && noexcept
    {
        assert(!has_value_);
        return std::move(this->error_);
    }

    JSONCONS_CPP14_CONSTEXPR const T* operator->() const noexcept
    {
        return std::addressof(this->value_);
    }

    JSONCONS_CPP14_CONSTEXPR T* operator->() noexcept
    {
        return std::addressof(this->value_);
    }

    JSONCONS_CPP14_CONSTEXPR const T& operator*() const & noexcept
    {
        return this->value_;
    }

    JSONCONS_CPP14_CONSTEXPR T& operator*() & noexcept
    {
        return this->value_;
    }

    JSONCONS_CPP14_CONSTEXPR const T&& operator*() const && noexcept
    {
        return this->value_;
    }

    JSONCONS_CPP14_CONSTEXPR T&& operator*() && noexcept
    {
        return this->value_;
    }

    void swap(expected& other) noexcept(std::is_nothrow_move_constructible<T>::value /*&&
                                        std::is_nothrow_swappable<T>::value*/)
    {
        const bool contains_a_value = has_value();
        if (contains_a_value == other.has_value())
        {
            if (contains_a_value)
            {
                using std::swap;
                swap(**this, *other);
            }
        }
        else
        {
            expected& source = contains_a_value ? *this : other;
            expected& target = contains_a_value ? other : *this;
            target = expected<T,E>(*source);
            source.destroy();
            source.error_ = target.error_;
        }
    }
private:
    void construct(const T& value) 
    {
        ::new (&value_) T(value);
        has_value_ = true;
    }

    void construct(T&& value) noexcept
    {
        ::new (&value_) T(std::move(value));
        has_value_ = true;
    }

    void destroy() noexcept 
    {
        if (has_value_) 
        {
            value_.~T();
            has_value_ = false;
        }
        else
        {
            error_.~E();
        }
    }

    void assign(const T& u) 
    {
        if (has_value_) 
        {
            value_ = u;
        } 
        else 
        {
            construct(u);
        }
    }

    void assign(T&& u) 
    {
        if (has_value_) 
        {
            value_ = std::move(u);
        } 
        else 
        {
            construct(std::move(u));
        }
    }
};

template <typename E>
class expected<void,E>
{
public:
    using value_type = void;
    using error_type = E;
private:
    bool has_value_;
    union {
        char dummy_;
        E error_;
    };
public:

    expected()
        : has_value_(true), dummy_{}
    {
    }

    template <typename... Args>    
    expected(unexpect_t, Args&& ... args) noexcept
        : has_value_(false)
    {
        ::new (&error_) E(std::forward<Args>(args)...);
    }
    
    // copy constructors
    expected(const expected<void,E>& other) 
        : has_value_(other.has_value()), dummy_{}
    {
        if (!other)
        {
            ::new (&error_) E(other.error_);
        }
    }

    // move constructors
    expected(expected<void,E>&& other) noexcept
        : has_value_(other.has_value()), dummy_{}
    {
        if (!other)
        {
            ::new (&error_) E(other.error_);
        }
    }

    ~expected() noexcept
    {
        destroy();
    }

    expected& operator=(const expected& other)
    {
        if (other)
        {
            assign(*other);
        }
        else
        {
            destroy();
            ::new (&error_) E(other.error_);
        }
        return *this;
    }

    expected& operator=(expected&& other)
    {
        if (other)
        {
            assign(std::move(*other));
        }
        else
        {
            destroy();
            ::new (&error_) E(other.error_);
        }
        return *this;
    }

    constexpr operator bool() const noexcept
    {
        return has_value_;
    }
    
    constexpr bool has_value() const noexcept
    {
        return has_value_;
    }

    JSONCONS_CPP14_CONSTEXPR E& error() & noexcept
    {
        assert(!has_value_);
        return this->error_;
    }

    JSONCONS_CPP14_CONSTEXPR const E& error() const& noexcept
    {
        assert(!has_value_);
        return this->error_;
    }

    JSONCONS_CPP14_CONSTEXPR E&& error() && noexcept
    {
        assert(!has_value_);
        return std::move(this->error_);
    }

    JSONCONS_CPP14_CONSTEXPR const E&& error() const && noexcept
    {
        assert(!has_value_);
        return std::move(this->error_);
    }

    void swap(expected& other) noexcept
    {
        const bool contains_a_value = has_value();
        if (contains_a_value == other.has_value())
        {
            if (contains_a_value)
            {
                using std::swap;
                swap(**this, *other);
            }
        }
        else
        {
            expected& source = contains_a_value ? *this : other;
            expected& target = contains_a_value ? other : *this;
            target = expected<void,E>(*source);
            source.destroy();
            source.error_ = target.error_;
        }
    }
private:
    void destroy() noexcept 
    {
        if (!has_value_) 
        {
            error_.~E();
        }
    }
};

template <typename T,typename E>
typename std::enable_if<std::is_nothrow_move_constructible<T>::value,void>::type
swap(expected<T,E>& lhs, expected<T,E>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace detail
} // namespace jsoncons

#endif // JSONCONS_DETAIL_EXPECTED_HPP
