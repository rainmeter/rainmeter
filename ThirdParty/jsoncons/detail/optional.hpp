// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_DETAIL_OPTIONAL_HPP
#define JSONCONS_DETAIL_OPTIONAL_HPP

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility> // std::swap

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/detail/utility.hpp>

namespace jsoncons 
{ 
namespace detail 
{ 
    template <typename T>
    class optional;

    template <typename T1,typename T2>
    struct is_constructible_or_convertible_from_optional
        : std::integral_constant<
              bool, std::is_constructible<T1, optional<T2>&>::value ||
                    std::is_constructible<T1, optional<T2>&&>::value ||
                    std::is_constructible<T1, const optional<T2>&>::value ||
                    std::is_constructible<T1, const optional<T2>&&>::value ||
                    std::is_convertible<optional<T2>&, T1>::value ||
                    std::is_convertible<optional<T2>&&, T1>::value ||
                    std::is_convertible<const optional<T2>&, T1>::value ||
                    std::is_convertible<const optional<T2>&&, T1>::value> {};

    template <typename T1,typename T2>
    struct is_constructible_convertible_or_assignable_from_optional
        : std::integral_constant<
              bool, is_constructible_or_convertible_from_optional<T1, T2>::value ||
                    std::is_assignable<T1&, optional<T2>&>::value ||
                    std::is_assignable<T1&, optional<T2>&&>::value ||
                    std::is_assignable<T1&, const optional<T2>&>::value ||
                    std::is_assignable<T1&, const optional<T2>&&>::value> {};

    template <typename T>
    class optional
    {
    public:
        using value_type = T;
    private:
        bool has_value_;
        union {
            char dummy_;
            T value_;
        };
    public:
        constexpr optional() noexcept
            : has_value_(false), dummy_{}
        {
        }
        
        // copy constructors
        optional(const optional<T>& other)
            : has_value_(false), dummy_{}
        {
            if (other)
            {
                construct(*other);
            }
        }

        // converting
        template <typename U,
                  typename std::enable_if<!std::is_same<T,U>::value &&
                                          std::is_constructible<T, const U&>::value &&
                                          std::is_convertible<const U&,T>::value &&
                                          !is_constructible_or_convertible_from_optional<T,U>::value &&
                                          std::is_copy_constructible<typename std::decay<U>::type>::value,int>::type = 0>
        optional(const optional<U>& other)
            : has_value_(false), dummy_{}
        {
            if (other)
            {
                construct(*other);
            }
        }

        template <typename U,
                  typename std::enable_if<!std::is_same<T,U>::value &&
                                          std::is_constructible<T, const U&>::value &&
                                          !std::is_convertible<const U&,T>::value &&
                                          !is_constructible_or_convertible_from_optional<T,U>::value &&
                                          std::is_copy_constructible<typename std::decay<U>::type>::value,int>::type = 0>
        explicit optional(const optional<U>& other)
            : has_value_(false), dummy_{}
        {
            if (other)
            {
                construct(*other);
            }
        }

        // move constructors
        template <typename T2 = T>
        optional(optional<T>&& other,
                 typename std::enable_if<std::is_move_constructible<typename std::decay<T2>::type>::value>::type* = 0)
            : has_value_(false), dummy_{}
       {
            if (other)
            {
                construct(std::move(other.value_));
            }
       }

        // converting 
        template <typename U>
        optional(optional<U>&& value,
             typename std::enable_if<!std::is_same<T,U>::value &&
                                     std::is_constructible<T, U&&>::value &&
                                     !is_constructible_or_convertible_from_optional<T,U>::value &&
                                     std::is_convertible<U&&,T>::value,int>::type = 0) // (8)
            : has_value_(true), value_(std::forward<U>(value))
        {
        }

        template <typename U>
        explicit optional(optional<U>&& value,
                         typename std::enable_if<!std::is_same<T,U>::value &&
                                                 std::is_constructible<T, U&&>::value &&
                                                 !is_constructible_or_convertible_from_optional<T,U>::value &&
                                                 !std::is_convertible<U&&,T>::value,int>::type = 0) // (8)
            : has_value_(true), value_(std::forward<U>(value))
        {
        }


        // value constructors
        template <typename T2>
        optional(T2&& value,
             typename std::enable_if<!std::is_same<optional<T>,typename std::decay<T2>::type>::value &&
                                     std::is_constructible<T, T2>::value &&
                                     std::is_convertible<T2,T>::value,int>::type = 0) // (8)
            : has_value_(true), value_(std::forward<T2>(value))
        {
        }

        template <typename T2>
        explicit optional(T2&& value,
                         typename std::enable_if<!std::is_same<optional<T>,typename std::decay<T2>::type>::value &&
                                                 std::is_constructible<T, T2>::value &&
                                                 !std::is_convertible<T2,T>::value,int>::type = 0) // (8)
            : has_value_(true), value_(std::forward<T2>(value))
        {
        }

        template<typename... Args, 
            typename = typename std::enable_if<std::is_constructible<T, Args...>::value,int>::type>
        optional(jsoncons::detail::in_place_t, Args&&... args) 
          : has_value_(true), value_(std::forward<Args>(args)...)
        {
        }

        ~optional() noexcept
        {
            destroy();
        }

        optional& operator=(const optional& other)
        {
            if (other)
            {
                assign(*other);
            }
            else
            {
                reset();
            }
            return *this;
        }

        optional& operator=(optional&& other ) noexcept
        {
            if (other)
            {
                assign(std::move(*other));
            }
            else
            {
                reset();
            }
            return *this;
        }

        template <typename U>
        typename std::enable_if<!std::is_same<optional<T>, U>::value &&
                                std::is_constructible<T, const U&>::value &&
                               !is_constructible_convertible_or_assignable_from_optional<T,U>::value &&
                                std::is_assignable<T&, const U&>::value,
            optional&>::type
        operator=(const optional<U>& other)
        {
            if (other) 
            {
                assign(*other);
            } 
            else 
            {
                destroy();
            }
            return *this;
        }

        template <typename U>
        typename std::enable_if<!std::is_same<optional<T>, U>::value &&
                                std::is_constructible<T, U>::value &&
                                !is_constructible_convertible_or_assignable_from_optional<T,U>::value &&
                                std::is_assignable<T&, U>::value,
            optional&>::type
        operator=(optional<U>&& other) noexcept
        {
            if (other) 
            {
                assign(std::move(*other));
            } 
            else 
            {
                destroy();
            }
            return *this;
        }

        // value assignment
        template <typename T2>
        typename std::enable_if<!std::is_same<optional<T>,typename std::decay<T2>::type>::value &&
                                std::is_constructible<T, T2>::value &&
                                std::is_assignable<T&, T2>::value &&
                                !(std::is_scalar<T>::value && std::is_same<T,typename std::decay<T2>::type>::value),
            optional&>::type
        operator=(T2&& v)
        {
            assign(std::forward<T2>(v));
            return *this;
        }

        constexpr explicit operator bool() const noexcept
        {
            return has_value_;
        }
        constexpr bool has_value() const noexcept
        {
            return has_value_;
        }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif // _MSC_VER

        T& value() &
        {
            if (has_value_)
            {
                return get();
            }
            JSONCONS_THROW(std::runtime_error("Bad optional access"));
        }

        JSONCONS_CONSTEXPR const T& value() const &
        {
            if (has_value_)
            {
                return get();
            }
            JSONCONS_THROW(std::runtime_error("Bad optional access"));
        }

        template <typename U>
        constexpr T value_or(U&& default_value) const & 
        {
            static_assert(std::is_copy_constructible<T>::value,
                          "get_value_or: T must be copy constructible");
            static_assert(std::is_convertible<U&&, T>::value,
                          "get_value_or: U must be convertible to T");
            return static_cast<bool>(*this)
                       ? **this
                       : static_cast<T>(std::forward<U>(default_value));
        }

        template <typename U>
        T value_or(U&& default_value) && 
        {
            static_assert(std::is_move_constructible<T>::value,
                          "get_value_or: T must be move constructible");
            static_assert(std::is_convertible<U&&, T>::value,
                          "get_value_or: U must be convertible to T");
            return static_cast<bool>(*this) ? std::move(**this)
                                            : static_cast<T>(std::forward<U>(default_value));
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif  // _MSC_VER

        const T* operator->() const
        {
            return std::addressof(this->value_);
        }

        T* operator->()
        {
            return std::addressof(this->value_);
        }

        JSONCONS_CONSTEXPR const T& operator*() const&
        {
            return value();
        }

        T& operator*() &
        {
            return value();
        }

        void reset() noexcept
        {
            destroy();
        }

        void swap(optional& other) noexcept(std::is_nothrow_move_constructible<T>::value /*&&
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
                optional& source = contains_a_value ? *this : other;
                optional& target = contains_a_value ? other : *this;
                target = optional<T>(*source);
                source.reset();
            }
        }
    private:
        constexpr const T& get() const { return this->value_; }
        T& get() { return this->value_; }

        template <typename... Args>
        void construct(Args&&... args) 
        {
            ::new (static_cast<void*>(&this->value_)) T(std::forward<Args>(args)...);
            has_value_ = true;
        }

        void destroy() noexcept 
        {
            if (has_value_) 
            {
                value_.~T();
                has_value_ = false;
            }
        }

        template <typename U>
        void assign(U&& u) 
        {
            if (has_value_) 
            {
                value_ = std::forward<U>(u);
            } 
            else 
            {
                construct(std::forward<U>(u));
            }
        }
    };

    template <typename T>
    typename std::enable_if<std::is_nothrow_move_constructible<T>::value,void>::type
    swap(optional<T>& lhs, optional<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator==(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return lhs.has_value() == rhs.has_value() && (!lhs.has_value() || *lhs == *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator!=(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return lhs.has_value() != rhs.has_value() || (lhs.has_value() && *lhs != *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator<(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator>(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator<=(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return !lhs.has_value() || (rhs.has_value() && *lhs <= *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator>=(const optional<T1>& lhs, const optional<T2>& rhs) noexcept 
    {
        return !rhs.has_value() || (lhs.has_value() && *lhs >= *rhs);
    }

    template <typename T1,typename T2>
    constexpr bool operator==(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs == rhs : false;
    }
    template <typename T1,typename T2>
    constexpr bool operator==(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs == *rhs : false;
    }

    template <typename T1,typename T2>
    constexpr bool operator!=(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs != rhs : true;
    }
    template <typename T1,typename T2>
    constexpr bool operator!=(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs != *rhs : true;
    }

    template <typename T1,typename T2>
    constexpr bool operator<(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs < rhs : true;
    }
    template <typename T1,typename T2>
    constexpr bool operator<(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs < *rhs : false;
    }

    template <typename T1,typename T2>
    constexpr bool operator<=(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs <= rhs : true;
    }
    template <typename T1,typename T2>
    constexpr bool operator<=(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs <= *rhs : false;
    }

    template <typename T1,typename T2>
    constexpr bool operator>(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs > rhs : false;
    }

    template <typename T1,typename T2>
    constexpr bool operator>(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs > *rhs : true;
    }

    template <typename T1,typename T2>
    constexpr bool operator>=(const optional<T1>& lhs, const T2& rhs) noexcept 
    {
        return lhs ? *lhs >= rhs : false;
    }
    template <typename T1,typename T2>
    constexpr bool operator>=(const T1& lhs, const optional<T2>& rhs) noexcept 
    {
        return rhs ? lhs >= *rhs : true;
    }

} // namespace detail
} // namespace jsoncons

#endif // JSONCONS_DETAIL_OPTIONAL_HPP
