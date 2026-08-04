// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_STAJ_ITERATOR_HPP
#define JSONCONS_STAJ_ITERATOR_HPP

#include <exception>
#include <ios>
#include <iterator> // std::input_iterator_tag
#include <memory>
#include <new> // placement new
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/reflect/decode_traits.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/staj_event.hpp>
#include <jsoncons/staj_cursor.hpp>

namespace jsoncons {

    // staj_array_iterator

    template <typename T,typename CharT=char>
    class staj_array_iterator
    {
        using char_type = CharT;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::input_iterator_tag;
        
    private:
        basic_staj_cursor<char_type>* cursor_ptr_{nullptr};
        jsoncons::optional<T> value_;
        bool done_{true};
    public:

        staj_array_iterator() noexcept = default;

        staj_array_iterator(basic_staj_cursor<char_type>& cursor)
            : cursor_ptr_(std::addressof(cursor)), done_(false)
        {
            if (cursor_ptr_->done())
            {
                done_ = true;
            }
            else if (cursor_ptr_->current().event_type() == staj_events::begin_array)
            {
                next();
            }
            else
            {
                done_ = true;
            }
        }

        staj_array_iterator(basic_staj_cursor<char_type>& cursor, std::error_code& ec)
            : cursor_ptr_(std::addressof(cursor)), done_(false)
        {
            if (cursor_ptr_->done())
            {
                done_ = true;
            }
            else if (cursor_ptr_->current().event_type() == staj_events::begin_array)
            {
                next(ec);
            }
            else
            {
                done_ = true;
            }
        }

        staj_array_iterator(const staj_array_iterator& iter) = default;

        ~staj_array_iterator() noexcept = default;

        staj_array_iterator& operator=(const staj_array_iterator& iter) = default;

        const T& operator*() const
        {
            return *value_;
        }

        const T* operator->() const
        {
            return value_.operator->();
        }

        staj_array_iterator& operator++()
        {
            next();
            return *this;
        }

        staj_array_iterator& increment(std::error_code& ec)
        {
            next(ec);
            if (JSONCONS_UNLIKELY(ec)) {done_ = true;}
            return *this;
        }

        staj_array_iterator operator++(int) // postfix increment
        {
            staj_array_iterator temp(*this);
            next();
            return temp;
        }

        friend bool operator==(const staj_array_iterator& a, const staj_array_iterator& b)
        {
            return (a.done() && b.done());
        }

        friend bool operator!=(const staj_array_iterator& a, const staj_array_iterator& b)
        {
            return !(a == b);
        }

    private:

        bool done() const
        {
            return done_;
        }

        void next()
        {
            if (JSONCONS_UNLIKELY(done_))
            {
                return;
            }
            std::error_code ec;
            next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, cursor_ptr_->context().line(), cursor_ptr_->context().column()));
            }
        }

        void next(std::error_code& ec)
        {
            ec.clear();
            if (JSONCONS_UNLIKELY(done_))
            {
                return;
            }
            if (cursor_ptr_->done())
            {
                done_ = true;
                return;
            }
            cursor_ptr_->next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                done_ = true;
                return;
            }
            if (JSONCONS_UNLIKELY(cursor_ptr_->current().event_type() == staj_events::end_array))
            {
                done_ = true;
                return;
            }
            auto result = reflect::decode_traits<T>::try_decode(make_alloc_set(), *cursor_ptr_);
            if (JSONCONS_UNLIKELY(!result))
            {
                ec = result.error().code();
                return;
            }
            value_ = std::move(*result);
        }            
    };

    template <typename T,typename CharT>
    staj_array_iterator<T,CharT> begin(staj_array_iterator<T,CharT> iter)
    {
        return iter;
    }

    template <typename T,typename CharT>
    staj_array_iterator<T,CharT> end(staj_array_iterator<T,CharT>) noexcept
    {
        return staj_array_iterator<T,CharT>();
    }

    // staj_object_iterator
    
    template <typename Key,typename T,typename CharT=char>
    class staj_object_iterator
    {
        using char_type = CharT;
        using key_type = std::basic_string<char_type>;
        using value_type = std::pair<key_type,T>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::input_iterator_tag;

    private:
        basic_staj_cursor<char_type>* cursor_ptr_{nullptr};
        jsoncons::optional<value_type> key_value_;
        bool done_{true};
    public:

        staj_object_iterator() noexcept = default;

        staj_object_iterator(basic_staj_cursor<char_type>& cursor)
            : cursor_ptr_(std::addressof(cursor)), done_(false)
        {
            if (cursor_ptr_->done())
            {
                done_ = true;
            }
            else if (cursor_ptr_->current().event_type() == staj_events::begin_object)
            {
                next();
            }
            else
            {
                done_ = true;
            }
        }

        staj_object_iterator(basic_staj_cursor<char_type>& cursor, std::error_code& ec)
                : cursor_ptr_(std::addressof(cursor)), done_(false)
        {
            if (cursor_ptr_->done())
            {
                done_ = true;
            }
            else if (cursor_ptr_->current().event_type() == staj_events::begin_object)
            {
                next(ec);
                if (JSONCONS_UNLIKELY(ec)) {done_ = true;}
            }
            else
            {
                done_ = true;
            }
        }

        staj_object_iterator(const staj_object_iterator& iter) = default; 

        ~staj_object_iterator() noexcept = default;

        staj_object_iterator& operator=(const staj_object_iterator& iter) = default; 

        const value_type& operator*() const
        {
            return *key_value_;
        }

        const value_type* operator->() const
        {
            return key_value_.operator->();
        }

        staj_object_iterator& operator++()
        {
            next();
            return *this;
        }

        staj_object_iterator& increment(std::error_code& ec)
        {
            next(ec);
            if (JSONCONS_UNLIKELY(ec)){done_ = true;}
            return *this;
        }

        staj_object_iterator operator++(int) // postfix increment
        {
            staj_object_iterator temp(*this);
            next();
            return temp;
        }

        friend bool operator==(const staj_object_iterator& a, const staj_object_iterator& b)
        {
            return (a.done() && b.done());
        }

        friend bool operator!=(const staj_object_iterator& a, const staj_object_iterator& b)
        {
            return !(a == b);
        }

    private:

        bool done() const
        {
            return done_;
        }

        void next()
        {
            if (JSONCONS_UNLIKELY(done_))
            {
                return;
            }
            std::error_code ec;
            next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec, cursor_ptr_->context().line(), cursor_ptr_->context().column()));
            }
        }

        void next(std::error_code& ec)
        {
            if (JSONCONS_UNLIKELY(done_))
            {
                return;
            }
            if (cursor_ptr_->done())
            {
                done_ = true;
                return;
            }
            
            cursor_ptr_->next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                done_ = true;
                return;
            }
            if (JSONCONS_UNLIKELY(cursor_ptr_->current().event_type() == staj_events::end_object))
            {
                done_ = true;
                return;
            }
            JSONCONS_ASSERT(cursor_ptr_->current().event_type() == staj_events::key);
            auto key = cursor_ptr_->current(). template get<key_type>();
            cursor_ptr_->next(ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                done_ = true;
                return;
            }
            auto result = reflect::decode_traits<T>::try_decode(make_alloc_set(), *cursor_ptr_);
            if (JSONCONS_UNLIKELY(!result))
            {
                ec = result.error().code();
                done_ = true;
                return;
            }
            key_value_ = value_type(std::move(key), std::move(*result));
        }
    };

    template <typename Key, typename T, typename CharT>
    staj_object_iterator<Key, T, CharT> begin(staj_object_iterator<Key, T, CharT> iter)
    {
        return iter;
    }

    template <typename Key, typename T, typename CharT>
    staj_object_iterator<Key, T, CharT> end(staj_object_iterator<Key, T, CharT>) noexcept
    {
        return staj_object_iterator<Key, T, CharT>();
    }

} // namespace jsoncons

#endif // JSONCONS_STAJ_ITERATOR_HPP

