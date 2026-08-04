// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_ARRAY_HPP
#define JSONCONS_JSON_ARRAY_HPP

#include <algorithm> // std::sort, std::stable_sort, std::lower_bound, std::unique
#include <cassert> // assert
#include <cstring>
#include <initializer_list>
#include <iterator> // std::iterator_traits
#include <memory> // std::allocator
#include <type_traits> // std::enable_if
#include <utility>
#include <utility> // std::move
#include <vector>

#include <jsoncons/allocator_holder.hpp>
#include <jsoncons/json_type.hpp>

namespace jsoncons {

    // json_array

    template <typename Json,template <typename,typename> class SequenceContainer = std::vector>
    class json_array : public allocator_holder<typename Json::allocator_type>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using value_type = Json;
    private:
        using value_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<value_type>;                   
        using value_container_type = SequenceContainer<value_type,value_allocator_type>;
        value_container_type elements_;
    public:
        using iterator = typename value_container_type::iterator;
        using const_iterator = typename value_container_type::const_iterator;
        using reference = typename std::iterator_traits<iterator>::reference;
        using const_reference = typename std::iterator_traits<const_iterator>::reference;

        using allocator_holder<allocator_type>::get_allocator;

        json_array()
        {
        }

        explicit json_array(const allocator_type& alloc)
            : allocator_holder<allocator_type>(alloc), 
              elements_(value_allocator_type(alloc))
        {
        }

        explicit json_array(std::size_t n, 
                            const allocator_type& alloc = allocator_type())
            : allocator_holder<allocator_type>(alloc), 
              elements_(n,Json(),value_allocator_type(alloc))
        {
        }

        explicit json_array(std::size_t n, 
                            const Json& value, 
                            const allocator_type& alloc = allocator_type())
            : allocator_holder<allocator_type>(alloc), 
              elements_(n,value,value_allocator_type(alloc))
        {
        }

        template <typename InputIterator>
        json_array(InputIterator begin, InputIterator end, const allocator_type& alloc = allocator_type())
            : allocator_holder<allocator_type>(alloc), 
              elements_(begin,end,value_allocator_type(alloc))
        {
        }

        json_array(const json_array& other)
            : allocator_holder<allocator_type>(other.get_allocator()),
              elements_(other.elements_)
        {
        }
        json_array(const json_array& other, const allocator_type& alloc)
            : allocator_holder<allocator_type>(alloc), 
              elements_(other.elements_,value_allocator_type(alloc))
        {
        }

        json_array(json_array&& other) noexcept
            : allocator_holder<allocator_type>(other.get_allocator()), 
              elements_(std::move(other.elements_))
        {
        }
        json_array(json_array&& other, const allocator_type& alloc)
            : allocator_holder<allocator_type>(alloc), 
              elements_(std::move(other.elements_),value_allocator_type(alloc))
        {
        }

        json_array(const std::initializer_list<Json>& init, 
                   const allocator_type& alloc = allocator_type())
            : allocator_holder<allocator_type>(alloc), 
              elements_(init,value_allocator_type(alloc))
        {
        }
        ~json_array() noexcept
        {
            flatten_and_destroy();
        }

        reference back()
        {
            return elements_.back();
        }

        const_reference back() const
        {
            return elements_.back();
        }

        void pop_back()
        {
            elements_.pop_back();
        }

        bool empty() const
        {
            return elements_.empty();
        }

        void swap(json_array& other) noexcept
        {
            elements_.swap(other.elements_);
        }

        std::size_t size() const {return elements_.size();}

        std::size_t capacity() const {return elements_.capacity();}

        void clear() {elements_.clear();}

        void shrink_to_fit() 
        {
            for (std::size_t i = 0; i < elements_.size(); ++i)
            {
                elements_[i].shrink_to_fit();
            }
            elements_.shrink_to_fit();
        }

        void reserve(std::size_t n) {elements_.reserve(n);}

        void resize(std::size_t n) {elements_.resize(n);}

        void resize(std::size_t n, const Json& val) {elements_.resize(n,val);}

        iterator erase(const_iterator pos) 
        {
            return elements_.erase(pos);
        }

        iterator erase(const_iterator first, const_iterator last) 
        {
            return elements_.erase(first,last);
        }

        Json& operator[](std::size_t i) {return elements_[i];}

        const Json& operator[](std::size_t i) const {return elements_[i];}

        // push_back

        template <typename T,typename A=allocator_type>
        typename std::enable_if<std::allocator_traits<A>::is_always_equal::value,void>::type 
        push_back(T&& value)
        {
            elements_.emplace_back(std::forward<T>(value));
        }

        template <typename T,typename A=allocator_type>
        typename std::enable_if<!std::allocator_traits<A>::is_always_equal::value,void>::type 
        push_back(T&& value)
        {
            elements_.emplace_back(std::forward<T>(value));
        }

        template <typename T,typename A=allocator_type>
        typename std::enable_if<std::allocator_traits<A>::is_always_equal::value,iterator>::type 
        insert(const_iterator pos, T&& value)
        {
            return elements_.emplace(pos, std::forward<T>(value));
        }
        template <typename T,typename A=allocator_type>
        typename std::enable_if<!std::allocator_traits<A>::is_always_equal::value,iterator>::type 
        insert(const_iterator pos, T&& value)
        {
            return elements_.emplace(pos, std::forward<T>(value));
        }

        template <typename InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last)
        {
            return elements_.insert(pos, first, last);
        }

        template <typename A=allocator_type,typename... Args>
        typename std::enable_if<std::allocator_traits<A>::is_always_equal::value,iterator>::type 
        emplace(const_iterator pos, Args&&... args)
        {
            return elements_.emplace(pos, std::forward<Args>(args)...);
        }

        template <typename... Args>
        Json& emplace_back(Args&&... args)
        {
            elements_.emplace_back(std::forward<Args>(args)...);
            return elements_.back();
        }

        iterator begin() {return elements_.begin();}

        iterator end() {return elements_.end();}

        const_iterator begin() const {return elements_.begin();}

        const_iterator end() const {return elements_.end();}

        bool operator==(const json_array& rhs) const noexcept
        {
            return elements_ == rhs.elements_;
        }

        bool operator<(const json_array& rhs) const noexcept
        {
            return elements_ < rhs.elements_;
        }

        json_array& operator=(const json_array& other)
        {
            elements_ = other.elements_;
            return *this;
        }
    private:

        void flatten_and_destroy() noexcept
        {
            while (!elements_.empty())
            {
                value_type current = std::move(elements_.back());
                elements_.pop_back();
                switch (current.storage_kind())
                {
                    case json_storage_kind::array:
                    {
                        for (auto&& item : current.array_range())
                        {
                            if ((item.storage_kind() == json_storage_kind::array || item.storage_kind() == json_storage_kind::object)
                                && !item.empty()) // non-empty object or array
                            {
                                elements_.push_back(std::move(item));
                            }
                        }
                        current.clear();                           
                        break;
                    }
                    case json_storage_kind::object:
                    {
                        for (auto&& kv : current.object_range())
                        {
                            if ((kv.value().storage_kind() == json_storage_kind::array || kv.value().storage_kind() == json_storage_kind::object)
                                && !kv.value().empty()) // non-empty object or array
                            {
                                elements_.push_back(std::move(kv.value()));
                            }
                        }
                        current.clear();                           
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    };

} // namespace jsoncons

#endif // JSONCONS_JSON_ARRAY_HPP
