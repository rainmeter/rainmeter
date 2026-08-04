// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_BASIC_JSON_HPP
#define JSONCONS_BASIC_JSON_HPP

#include <algorithm> // std::swap
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list> // std::initializer_list
#include <istream> 
#include <istream> // std::basic_istream
#include <limits> // std::numeric_limits
#include <memory> // std::allocator
#include <ostream> 
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits> // std::enable_if
#include <typeinfo>
#include <utility> // std::move
#include <vector>

#include <jsoncons/allocator_set.hpp>
#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/version.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/conversion_result.hpp>
#include <jsoncons/json_array.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_encoder.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_fwd.hpp>
#include <jsoncons/json_object.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/json_reader.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/reflect/json_conv_traits.hpp>
#include <jsoncons/pretty_print.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/utility/bigint.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/heap_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

#if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
#include <memory_resource> // std::poymorphic_allocator
#endif

namespace jsoncons { 

    namespace ext_traits {

        template <typename Container>
        using 
        container_array_iterator_type_t = decltype(Container::array_iterator_type);
        template <typename Container>
        using 
        container_const_array_iterator_type_t = decltype(Container::const_array_iterator_type);
        template <typename Container>
        using 
        container_object_iterator_type_t = decltype(Container::object_iterator_type);
        template <typename Container>
        using 
        container_const_object_iterator_type_t = decltype(Container::const_object_iterator_type);

        namespace detail {

            template <typename T>
            using
            basic_json_t = basic_json<typename T::char_type,typename T::policy_type,typename T::allocator_type>;

        } // namespace detail

        template <typename T,typename Enable = void>
        struct is_basic_json : std::false_type {};

        template <typename T>
        struct is_basic_json<T,
            typename std::enable_if<ext_traits::is_detected<detail::basic_json_t,typename std::decay<T>::type>::value>::type
        > : std::true_type {};

    } // namespace ext_traits

    namespace detail {

        template <typename Iterator,typename Enable = void>
        class random_access_iterator_wrapper
        {
        };

        template <typename Iterator>
        class random_access_iterator_wrapper<Iterator,
                 typename std::enable_if<std::is_same<typename std::iterator_traits<Iterator>::iterator_category, 
                                                      std::random_access_iterator_tag>::value>::type> 
        { 
            Iterator it_; 
            bool has_value_;

            template <typename Iter,typename Enable> 
            friend class random_access_iterator_wrapper;
        public:
            using iterator_category = std::random_access_iterator_tag;

            using value_type = typename std::iterator_traits<Iterator>::value_type;
            using difference_type = typename std::iterator_traits<Iterator>::difference_type;
            using pointer = typename std::iterator_traits<Iterator>::pointer;
            using reference = typename std::iterator_traits<Iterator>::reference;
        
            random_access_iterator_wrapper() : it_(), has_value_(false) 
            { 
            }

            explicit random_access_iterator_wrapper(Iterator ptr) : it_(ptr), has_value_(true)  
            {
            }

            random_access_iterator_wrapper(const random_access_iterator_wrapper&) = default;
            random_access_iterator_wrapper(random_access_iterator_wrapper&&) = default;
            random_access_iterator_wrapper& operator=(const random_access_iterator_wrapper&) = default;
            random_access_iterator_wrapper& operator=(random_access_iterator_wrapper&&) = default;

            template <typename Iter,
                      typename=typename std::enable_if<!std::is_same<Iter,Iterator>::value && std::is_convertible<Iter,Iterator>::value>::type>
            random_access_iterator_wrapper(const random_access_iterator_wrapper<Iter>& other)
                : it_(other.it_), has_value_(other.has_value_)
            {
            }

            operator Iterator() const
            { 
                return it_; 
            }

            reference operator*() const 
            {
                return *it_;
            }

            pointer operator->() const 
            {
                return &(*it_);
            }

            random_access_iterator_wrapper& operator++() 
            {
                ++it_;
                return *this;
            }

            random_access_iterator_wrapper operator++(int) 
            {
                random_access_iterator_wrapper temp = *this;
                ++*this;
                return temp;
            }

            random_access_iterator_wrapper& operator--() 
            {
                --it_;
                return *this;
            }

            random_access_iterator_wrapper operator--(int) 
            {
                random_access_iterator_wrapper temp = *this;
                --*this;
                return temp;
            }

            random_access_iterator_wrapper& operator+=(const difference_type offset) 
            {
                it_ += offset;
                return *this;
            }

            random_access_iterator_wrapper operator+(const difference_type offset) const 
            {
                random_access_iterator_wrapper temp = *this;
                return temp += offset;
            }

            random_access_iterator_wrapper& operator-=(const difference_type offset) 
            {
                return *this += -offset;
            }

            random_access_iterator_wrapper operator-(const difference_type offset) const 
            {
                random_access_iterator_wrapper temp = *this;
                return temp -= offset;
            }

            difference_type operator-(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return it_ - rhs.it_;
            }

            reference operator[](const difference_type offset) const noexcept
            {
                return *(*this + offset);
            }

            bool operator==(const random_access_iterator_wrapper& rhs) const noexcept
            {
                if (!has_value_ || !rhs.has_value_)
                {
                    return has_value_ == rhs.has_value_ ? true : false;
                }
                else
                {
                    return it_ == rhs.it_;
                }
            }

            bool operator!=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            bool operator<(const random_access_iterator_wrapper& rhs) const noexcept
            {
                if (!has_value_ || !rhs.has_value_)
                {
                    return has_value_ == rhs.has_value_ ? false :(has_value_ ? false : true);
                }
                else
                {
                    return it_ < rhs.it_;
                }
            }

            bool operator>(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return rhs < *this;
            }

            bool operator<=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(rhs < *this);
            }

            bool operator>=(const random_access_iterator_wrapper& rhs) const noexcept
            {
                return !(*this < rhs);
            }

            inline 
            friend random_access_iterator_wrapper<Iterator> operator+(
                difference_type offset, random_access_iterator_wrapper<Iterator> next) 
            {
                return next += offset;
            }

            bool has_value() const
            {
                return has_value_;
            }
        };
    } // namespace detail

    struct sorted_policy 
    {
        template <typename KeyT,typename Json>
        using object = sorted_json_object<KeyT,Json,std::vector>;

        template <typename Json>
        using array = json_array<Json,std::vector>;
        
        template <typename CharT,typename CharTraits,typename Allocator>
        using member_key = std::basic_string<CharT, CharTraits, Allocator>;
    };

    struct order_preserving_policy
    {
        template <typename KeyT,typename Json>
        using object = order_preserving_json_object<KeyT,Json,std::vector>;

        template <typename Json>
        using array = json_array<Json,std::vector>;
        
        template <typename CharT,typename CharTraits,typename Allocator>
        using member_key = std::basic_string<CharT, CharTraits, Allocator>;
    };

    template <typename Policy,typename KeyT,typename Json,typename Enable=void>
    struct object_iterator_typedefs
    {
    };

    template <typename Policy,typename KeyT,typename Json>
    struct object_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        !ext_traits::is_detected<ext_traits::container_object_iterator_type_t, Policy>::value ||
        !ext_traits::is_detected<ext_traits::container_const_object_iterator_type_t, Policy>::value>::type>
    {
        using object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object<KeyT,Json>::iterator>;                    
        using const_object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object<KeyT,Json>::const_iterator>;
    };

    template <typename Policy,typename KeyT,typename Json>
    struct object_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        ext_traits::is_detected<ext_traits::container_object_iterator_type_t, Policy>::value &&
        ext_traits::is_detected<ext_traits::container_const_object_iterator_type_t, Policy>::value>::type>
    {
        using object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template object_iterator<KeyT,Json>>;
        using const_object_iterator_type = jsoncons::detail::random_access_iterator_wrapper<typename Policy::template const_object_iterator<KeyT,Json>>;
    };

    template <typename Policy,typename KeyT,typename Json,typename Enable=void>
    struct array_iterator_typedefs
    {
    };

    template <typename Policy,typename KeyT,typename Json>
    struct array_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        !ext_traits::is_detected<ext_traits::container_array_iterator_type_t, Policy>::value ||
        !ext_traits::is_detected<ext_traits::container_const_array_iterator_type_t, Policy>::value>::type>
    {
        using array_iterator_type = typename Policy::template array<Json>::iterator;
        using const_array_iterator_type = typename Policy::template array<Json>::const_iterator;
    };

    template <typename Policy,typename KeyT,typename Json>
    struct array_iterator_typedefs<Policy, KeyT, Json,typename std::enable_if<
        ext_traits::is_detected<ext_traits::container_array_iterator_type_t, Policy>::value &&
        ext_traits::is_detected<ext_traits::container_const_array_iterator_type_t, Policy>::value>::type>
    {
        using array_iterator_type = typename Policy::template array_iterator_type<Json>;
        using const_array_iterator_type = typename Policy::template const_array_iterator_type<Json>;
    };

    template <typename IteratorT,typename ConstIteratorT>
    class range 
    {
    public:
        using iterator = IteratorT;
        using const_iterator = ConstIteratorT;
        using reverse_iterator = std::reverse_iterator<IteratorT>;
        using const_reverse_iterator = std::reverse_iterator<ConstIteratorT>;
    private:
        iterator first_;
        iterator last_;
    public:
        range(const IteratorT& first, const IteratorT& last)
            : first_(first), last_(last)
        {
        }

        iterator begin() const noexcept
        {
            return first_;
        }
        iterator end() const noexcept
        {
            return last_;
        }
        const_iterator cbegin() const noexcept
        {
            return first_;
        }
        const_iterator cend() const noexcept
        {
            return last_;
        }
        reverse_iterator rbegin() const noexcept
        {
            return reverse_iterator(last_);
        }
        reverse_iterator rend() const noexcept
        {
            return reverse_iterator(first_);
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return reverse_iterator(last_);
        }
        const_reverse_iterator crend() const noexcept
        {
            return reverse_iterator(first_);
        }
    };

    template <typename CharT,typename Policy,typename Allocator>
    class basic_json
    {
    public:
        static_assert(std::allocator_traits<Allocator>::is_always_equal::value || ext_traits::is_propagating_allocator<Allocator>::value,
                      "Regular stateful allocators must be wrapped with std::scoped_allocator_adaptor");

        using allocator_type = Allocator; 

        using policy_type = Policy;
        using char_type = CharT;
        using char_traits_type = std::char_traits<char_type>;
        using string_view_type = jsoncons::basic_string_view<char_type,char_traits_type>;

        using char_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<char_type>;

        using string_type = std::basic_string<char_type,char_traits_type,char_allocator_type>;

        using key_type = typename policy_type::template member_key<char_type,char_traits_type,char_allocator_type>;

        using reference = basic_json&;
        using const_reference = const basic_json&;
        using pointer = basic_json*;
        using const_pointer = const basic_json*;

        using key_value_type = key_value<key_type,basic_json>;

        using array = typename policy_type::template array<basic_json>;

        using key_value_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<key_value_type>;                       

        using object = typename policy_type::template object<key_type,basic_json>;

        using object_iterator = typename object_iterator_typedefs<policy_type,key_type,basic_json>::object_iterator_type;                    
        using const_object_iterator = typename object_iterator_typedefs<policy_type,key_type,basic_json>::const_object_iterator_type;                    
        using array_iterator = typename array_iterator_typedefs<policy_type,key_type,basic_json>::array_iterator_type;                    
        using const_array_iterator = typename array_iterator_typedefs<policy_type,key_type,basic_json>::const_array_iterator_type;

        using object_range_type = range<object_iterator, const_object_iterator>;
        using const_object_range_type = range<const_object_iterator, const_object_iterator>;
        using array_range_type = range<array_iterator, const_array_iterator>;
        using const_array_range_type = range<const_array_iterator, const_array_iterator>;

    private:

        static constexpr uint8_t major_type_shift = 0x04;
        static constexpr uint8_t additional_information_mask = (1U << 4) - 1;

    public:
        struct common_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
        };

        struct null_storage 
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;

            null_storage(semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::null)), short_str_length_(0), tag_(tag)
            {
            }
        };

        struct empty_object_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;

            empty_object_storage(semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::empty_object)), short_str_length_(0), tag_(tag)
            {
            }
        };  

        struct bool_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            bool val_;

            bool_storage(bool val, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::boolean)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            bool value() const
            {
                return val_;
            }

        };

        struct int64_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            int64_t val_;

            int64_storage(int64_t val, 
                       semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::int64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            int64_t value() const
            {
                return val_;
            }
        };

        struct uint64_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            uint64_t val_;

            uint64_storage(uint64_t val, 
                        semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::uint64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            uint64_t value() const
            {
                return val_;
            }
        };

        struct half_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            uint16_t val_;

            half_storage(uint16_t val, semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::half_float)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            uint16_t value() const
            {
                return val_;
            }
        };

        struct double_storage
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            double val_;

            double_storage(double val, 
                           semantic_tag tag = semantic_tag::none)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::float64)), short_str_length_(0), tag_(tag),
                  val_(val)
            {
            }

            double value() const
            {
                return val_;
            }
        };

        struct short_string_storage
        {
            static constexpr size_t capacity = (2*sizeof(uint64_t) - 2*sizeof(uint8_t))/sizeof(char_type);
            static constexpr size_t max_length = capacity - 1;

            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            char_type data_[capacity];

            short_string_storage(const char_type* p, uint8_t length, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::short_str)), short_str_length_(length), tag_(tag)
            {
                JSONCONS_ASSERT(length <= max_length);
                std::memcpy(data_,p,length*sizeof(char_type));
                data_[length] = 0;
            }

            short_string_storage(const short_string_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(other.short_str_length_), tag_(other.tag_)
            {
                std::memcpy(data_,other.data_,other.short_str_length_*sizeof(char_type));
                data_[short_str_length_] = 0;
            }
           
            short_string_storage& operator=(const short_string_storage& other) = delete;

            uint8_t length() const
            {
                return short_str_length_;
            }

            const char_type* data() const
            {
                return data_;
            }

            const char_type* c_str() const
            {
                return data_;
            }
        };

        // long_string_storage
        struct long_string_storage
        {
            using heap_string_factory_type = jsoncons::heap::heap_string_factory<char_type,null_type,Allocator>;
            using pointer = typename heap_string_factory_type::pointer;

            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            pointer ptr_;

            long_string_storage(pointer ptr, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(tag), ptr_(ptr)
            {
            }

            long_string_storage(const long_string_storage& other)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::long_str)), short_str_length_(0), tag_(other.tag_), ptr_(other.ptr_)
            {
            }
            
            long_string_storage& operator=(const long_string_storage& other) = delete;

            semantic_tag tag() const
            {
                return tag_;
            }

            const char_type* data() const
            {
                return ptr_->data();
            }

            const char_type* c_str() const
            {
                return ptr_->c_str();
            }

            std::size_t length() const
            {
                return ptr_->length();
            }
        
            Allocator get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };

        // byte_string_storage
        struct byte_string_storage 
        {
            using heap_string_factory_type = jsoncons::heap::heap_string_factory<uint8_t,uint64_t,Allocator>;
            using pointer = typename heap_string_factory_type::pointer;

            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            pointer ptr_;

            byte_string_storage(pointer ptr, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::byte_str)), short_str_length_(0), tag_(tag), ptr_(ptr)
            {
            }

            byte_string_storage(const byte_string_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(other.ptr_)
            {
            }

            byte_string_storage& operator=(const byte_string_storage& other) = delete;

            semantic_tag tag() const
            {
                return tag_;
            }

            const uint8_t* data() const
            {
                return ptr_->data();
            }

            std::size_t length() const
            {
                return ptr_->length();
            }

            uint64_t ext_tag() const
            {
                return ptr_->extra();
            }

            Allocator get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

        struct array_storage
        {
            using allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<array>;
            using pointer = typename std::allocator_traits<allocator_type>::pointer;

            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            pointer ptr_;

            array_storage(pointer ptr, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::array)), short_str_length_(0), tag_(tag), ptr_(ptr)
            {
            }

            array_storage(const array_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(other.ptr_)
            {
            }

            void assign(const array_storage& other)
            {
                tag_ = other.tag_;
                *ptr_ = *(other.ptr_);
            }

            array_storage& operator=(const array_storage& other) = delete;

            semantic_tag tag() const
            {
                return tag_;
            }

            Allocator get_allocator() const
            {
                return ptr_->get_allocator();
            }

            array& value()
            {
                return *ptr_;
            }

            const array& value() const
            {
                return *ptr_;
            }
        };

        // object_storage
        struct object_storage
        {
            using allocator_type = typename std::allocator_traits<Allocator>:: template rebind_alloc<object>;
            using pointer = typename std::allocator_traits<allocator_type>::pointer;

            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            pointer ptr_;

            object_storage(pointer ptr, semantic_tag tag)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::object)), short_str_length_(0), tag_(tag), ptr_(ptr)
            {
            }

            explicit object_storage(const object_storage& other)
                : storage_kind_(other.storage_kind_), short_str_length_(0), tag_(other.tag_), ptr_(other.ptr_)
            {
            }

            void assign(const object_storage& other)
            {
                tag_ = other.tag_;
                *ptr_ = *(other.ptr_);
            }

            object_storage& operator=(const object_storage& other) = delete;

            semantic_tag tag() const
            {
                return tag_;
            }

            object& value()
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return *ptr_;
            }

            const object& value() const
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return *ptr_;
            }

            Allocator get_allocator() const
            {
                JSONCONS_ASSERT(ptr_ != nullptr);
                return ptr_->get_allocator();
            }
        };
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic pop
#endif

    private:
        struct json_const_reference_storage 
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            std::reference_wrapper<const basic_json> ref_;

            json_const_reference_storage(const basic_json& ref)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::json_const_ref)), short_str_length_(0), tag_(ref.tag()),
                  ref_(ref)
            {
            }

            const basic_json& value() const
            {
                return ref_.get();
            }
        };

        struct json_reference_storage 
        {
            uint8_t storage_kind_:4;
            uint8_t short_str_length_:4;
            semantic_tag tag_;
            std::reference_wrapper<basic_json> ref_;

            json_reference_storage(basic_json& ref)
                : storage_kind_(static_cast<uint8_t>(json_storage_kind::json_ref)), short_str_length_(0), tag_(ref.tag()),
                  ref_(ref)
            {
            }

            basic_json& value() 
            {
                return ref_.get();
            }

            const basic_json& value() const
            {
                return ref_.get();
            }
        };

        union 
        {
            common_storage common_;
            null_storage null_;
            bool_storage boolean_;
            int64_storage int64_;
            uint64_storage uint64_;
            half_storage half_float_;
            double_storage float64_;
            short_string_storage short_str_;
            long_string_storage long_str_;
            byte_string_storage byte_str_;
            array_storage array_;
            object_storage object_;
            empty_object_storage empty_object_;
            json_const_reference_storage json_const_pointer_;
            json_reference_storage json_ref_;
        };

        void destroy()
        {
            switch (storage_kind())
            {
                case json_storage_kind::long_str:
                {
                    if (cast<long_string_storage>().ptr_ != nullptr)
                    {
                        long_string_storage::heap_string_factory_type::destroy(cast<long_string_storage>().ptr_);
                    }
                    break;
                }
                case json_storage_kind::byte_str:
                    if (cast<byte_string_storage>().ptr_ != nullptr)
                    {
                        byte_string_storage::heap_string_factory_type::destroy(cast<byte_string_storage>().ptr_);
                    }
                    break;
                case json_storage_kind::array:
                {
                    if (cast<array_storage>().ptr_ != nullptr)
                    {
                        auto& stor = cast<array_storage>();
                        typename array_storage::allocator_type alloc{stor.ptr_->get_allocator()};
                        std::allocator_traits<typename array_storage::allocator_type>::destroy(alloc, ext_traits::to_plain_pointer(stor.ptr_));
                        std::allocator_traits<typename array_storage::allocator_type>::deallocate(alloc, stor.ptr_,1);
                    }
                    break;
                }
                case json_storage_kind::object:
                {
                    if (cast<object_storage>().ptr_ != nullptr)
                    {
                        auto& stor = cast<object_storage>();
                        typename object_storage::allocator_type alloc{stor.ptr_->get_allocator()};
                        std::allocator_traits<typename object_storage::allocator_type>::destroy(alloc, ext_traits::to_plain_pointer(stor.ptr_));
                        std::allocator_traits<typename object_storage::allocator_type>::deallocate(alloc, stor.ptr_,1);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        typename long_string_storage::pointer create_long_string(const allocator_type& alloc, const char_type* data, std::size_t length)
        {
            using heap_string_factory_type = jsoncons::heap::heap_string_factory<char_type,null_type,Allocator>;
            return heap_string_factory_type::create(data, length, null_type(), alloc); 
        }

        typename byte_string_storage::pointer create_byte_string(const allocator_type& alloc, const uint8_t* data, std::size_t length,
            uint64_t ext_tag)
        {
            using heap_string_factory_type = jsoncons::heap::heap_string_factory<uint8_t,uint64_t,Allocator>;
            return heap_string_factory_type::create(data, length, ext_tag, alloc); 
        }
        
        template <typename... Args>
        typename array_storage::pointer create_array(const allocator_type& alloc, Args&& ... args)
        {
            using stor_allocator_type = typename array_storage::allocator_type;
            stor_allocator_type stor_alloc(alloc);
            auto ptr = std::allocator_traits<stor_allocator_type>::allocate(stor_alloc, 1);
            JSONCONS_TRY
            {
                std::allocator_traits<stor_allocator_type>::construct(stor_alloc, ext_traits::to_plain_pointer(ptr), 
                    std::forward<Args>(args)...);
            }
            JSONCONS_CATCH(...)
            {
                std::allocator_traits<stor_allocator_type>::deallocate(stor_alloc, ptr,1);
                JSONCONS_RETHROW;
            }
            return ptr;
        }

        template <typename... Args>
        typename object_storage::pointer create_object(const allocator_type& alloc, Args&& ... args)
        {
            using stor_allocator_type = typename object_storage::allocator_type;
            stor_allocator_type stor_alloc(alloc);
            auto ptr = std::allocator_traits<stor_allocator_type>::allocate(stor_alloc, 1);
            JSONCONS_TRY
            {
                std::allocator_traits<stor_allocator_type>::construct(stor_alloc, ext_traits::to_plain_pointer(ptr), 
                    std::forward<Args>(args)...);
            }
            JSONCONS_CATCH(...)
            {
                std::allocator_traits<stor_allocator_type>::deallocate(stor_alloc, ptr,1);
                JSONCONS_RETHROW;
            }
            return ptr;
        }

        template <typename StorageType,typename... Args>
        void construct(Args&&... args)
        {
            ::new (&cast<StorageType>()) StorageType(std::forward<Args>(args)...);
        }

        template <typename T>
        struct identity { using type = T*; };
    public:
        template <typename T> 
        T& cast()
        {
            return cast(identity<T>());
        }

        template <typename T> 
        const T& cast() const
        {
            return cast(identity<T>());
        }
    private:
        null_storage& cast(identity<null_storage>) 
        {
            return null_;
        }

        const null_storage& cast(identity<null_storage>) const
        {
            return null_;
        }

        empty_object_storage& cast(identity<empty_object_storage>) 
        {
            return empty_object_;
        }

        const empty_object_storage& cast(identity<empty_object_storage>) const
        {
            return empty_object_;
        }

        bool_storage& cast(identity<bool_storage>) 
        {
            return boolean_;
        }

        const bool_storage& cast(identity<bool_storage>) const
        {
            return boolean_;
        }

        int64_storage& cast(identity<int64_storage>) 
        {
            return int64_;
        }

        const int64_storage& cast(identity<int64_storage>) const
        {
            return int64_;
        }

        uint64_storage& cast(identity<uint64_storage>) 
        {
            return uint64_;
        }

        const uint64_storage& cast(identity<uint64_storage>) const
        {
            return uint64_;
        }

        half_storage& cast(identity<half_storage>)
        {
            return half_float_;
        }

        const half_storage& cast(identity<half_storage>) const
        {
            return half_float_;
        }

        double_storage& cast(identity<double_storage>) 
        {
            return float64_;
        }

        const double_storage& cast(identity<double_storage>) const
        {
            return float64_;
        }

        short_string_storage& cast(identity<short_string_storage>)
        {
            return short_str_;
        }

        const short_string_storage& cast(identity<short_string_storage>) const
        {
            return short_str_;
        }

        long_string_storage& cast(identity<long_string_storage>)
        {
            return long_str_;
        }

        const long_string_storage& cast(identity<long_string_storage>) const
        {
            return long_str_;
        }

        byte_string_storage& cast(identity<byte_string_storage>)
        {
            return byte_str_;
        }

        const byte_string_storage& cast(identity<byte_string_storage>) const
        {
            return byte_str_;
        }

        object_storage& cast(identity<object_storage>)
        {
            return object_;
        }

        const object_storage& cast(identity<object_storage>) const
        {
            return object_;
        }

        array_storage& cast(identity<array_storage>)
        {
            return array_;
        }

        const array_storage& cast(identity<array_storage>) const
        {
            return array_;
        }

        json_const_reference_storage& cast(identity<json_const_reference_storage>) 
        {
            return json_const_pointer_;
        }

        json_reference_storage& cast(identity<json_reference_storage>) 
        {
            return json_ref_;
        }

        const json_const_reference_storage& cast(identity<json_const_reference_storage>) const
        {
            return json_const_pointer_;
        }

        const json_reference_storage& cast(identity<json_reference_storage>) const
        {
            return json_ref_;
        }

        template <typename TypeL,typename TypeR>
        void swap_l_r(basic_json& other) noexcept
        {
            swap_l_r(identity<TypeL>(), identity<TypeR>(), other);
        }

        template <typename TypeL,typename TypeR>
        void swap_l_r(identity<TypeL>,identity<TypeR>,basic_json& other) noexcept
        {
            TypeR temp{other.cast<TypeR>()};
            other.construct<TypeL>(cast<TypeL>());
            construct<TypeR>(temp);
        }

        template <typename TypeL>
        void swap_l(basic_json& other) noexcept
        {
            switch (other.storage_kind())
            {
                case json_storage_kind::null         : swap_l_r<TypeL, null_storage>(other); break;
                case json_storage_kind::empty_object : swap_l_r<TypeL, empty_object_storage>(other); break;
                case json_storage_kind::boolean         : swap_l_r<TypeL, bool_storage>(other); break;
                case json_storage_kind::int64        : swap_l_r<TypeL, int64_storage>(other); break;
                case json_storage_kind::uint64       : swap_l_r<TypeL, uint64_storage>(other); break;
                case json_storage_kind::half_float         : swap_l_r<TypeL, half_storage>(other); break;
                case json_storage_kind::float64       : swap_l_r<TypeL, double_storage>(other); break;
                case json_storage_kind::short_str : swap_l_r<TypeL, short_string_storage>(other); break;
                case json_storage_kind::long_str  : swap_l_r<TypeL, long_string_storage>(other); break;
                case json_storage_kind::byte_str  : swap_l_r<TypeL, byte_string_storage>(other); break;
                case json_storage_kind::array        : swap_l_r<TypeL, array_storage>(other); break;
                case json_storage_kind::object       : swap_l_r<TypeL, object_storage>(other); break;
                case json_storage_kind::json_const_ref : swap_l_r<TypeL, json_const_reference_storage>(other); break;
                case json_storage_kind::json_ref : swap_l_r<TypeL, json_reference_storage>(other); break;
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        void uninitialized_copy(const basic_json& other)
        {
            if (is_trivial_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                    {
                        const auto& stor = other.cast<long_string_storage>();
                        auto ptr = create_long_string(std::allocator_traits<Allocator>::select_on_container_copy_construction(stor.get_allocator()),
                            stor.data(), stor.length());
                        construct<long_string_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::byte_str:
                    {
                        const auto& stor = other.cast<byte_string_storage>();
                        auto ptr = create_byte_string(std::allocator_traits<Allocator>::select_on_container_copy_construction(stor.get_allocator()),
                            stor.data(), stor.length(), stor.ext_tag());
                        construct<byte_string_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::array:
                    {
                        auto ptr = create_array(
                            std::allocator_traits<Allocator>::select_on_container_copy_construction(other.cast<array_storage>().get_allocator()), 
                            other.cast<array_storage>().value());
                        construct<array_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::object:
                    {
                        auto ptr = create_object(
                            std::allocator_traits<Allocator>::select_on_container_copy_construction(other.cast<object_storage>().get_allocator()), 
                            other.cast<object_storage>().value());
                        construct<object_storage>(ptr, other.tag());
                        break;
                    }
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_copy_a(const basic_json& other, const Allocator& alloc)
        {
            if (is_trivial_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                    {
                        const auto& storage = other.cast<long_string_storage>();
                        auto ptr = create_long_string(alloc, storage.data(), storage.length());
                        construct<long_string_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::byte_str:
                    {
                        const auto& storage = other.cast<byte_string_storage>();
                        auto ptr = create_byte_string(alloc, storage.data(), storage.length(), storage.ext_tag());
                        construct<byte_string_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::array:
                    {
                        auto ptr = create_array(alloc, other.cast<array_storage>().value());
                        construct<array_storage>(ptr, other.tag());
                        break;
                    }
                    case json_storage_kind::object:
                    {
                        auto ptr = create_object(alloc, other.cast<object_storage>().value());
                        construct<object_storage>(ptr, other.tag());
                        break;
                    }
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_move(basic_json&& other) noexcept
        {
            if (is_trivial_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                        construct<long_string_storage>(other.cast<long_string_storage>());
                        other.construct<null_storage>();
                        break;
                    case json_storage_kind::byte_str:
                        construct<byte_string_storage>(other.cast<byte_string_storage>());
                        other.construct<null_storage>();
                        break;
                    case json_storage_kind::array:
                        construct<array_storage>(other.cast<array_storage>());
                        other.construct<null_storage>();
                        break;
                    case json_storage_kind::object:
                        construct<object_storage>(other.cast<object_storage>());
                        other.construct<null_storage>();
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }

        void uninitialized_move_a(std::true_type /* stateless allocator */, 
            basic_json&& other, const Allocator&) noexcept
        {
            uninitialized_move(std::move(other));
        }

        void uninitialized_move_a(std::false_type /* stateful allocator */, 
            basic_json&& other, const Allocator& alloc) noexcept
        {
            if (is_trivial_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                uninitialized_copy_a(other, alloc);
            }
        }

        void copy_assignment(const basic_json& other)
        {
            if (is_trivial_storage(other.storage_kind()))
            {
                destroy();
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else if (storage_kind() == other.storage_kind())
            {
                switch (other.storage_kind())
                {
                    case json_storage_kind::long_str:
                    {
                        auto alloc = cast<long_string_storage>().get_allocator();
                        destroy();
                        uninitialized_copy_a(other, alloc);
                        break;
                    }
                    case json_storage_kind::byte_str:
                    {
                        auto alloc = cast<byte_string_storage>().get_allocator();
                        destroy();
                        uninitialized_copy_a(other, alloc);
                        break;
                    }
                    case json_storage_kind::array:
                        cast<array_storage>().assign(other.cast<array_storage>());
                        break;
                    case json_storage_kind::object:
                        cast<object_storage>().assign(other.cast<object_storage>());
                        break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
            else if (is_trivial_storage(storage_kind())) // rhs is not trivial storage
            {
                destroy();
                uninitialized_copy(other);
            }
            else // lhs and rhs are not trivial storage
            {
                auto alloc = get_allocator();
                destroy();
                uninitialized_copy_a(other, alloc);
            }
        }

        void move_assignment(basic_json&& other) noexcept
        {
            if (is_trivial_storage(storage_kind()) && is_trivial_storage(other.storage_kind()))
            {
                std::memcpy(static_cast<void*>(this), &other, sizeof(basic_json));
            }
            else
            {
                swap(other);
            }
        }

        basic_json& evaluate_with_default() 
        {
            return *this;
        }

        basic_json& evaluate(const string_view_type& name) 
        {
            return at(name);
        }

        const basic_json& evaluate(const string_view_type& name) const
        {
            return at(name);
        }

    public:

        basic_json& evaluate() 
        {
            return *this;
        }

        const basic_json& evaluate() const
        {
            return *this;
        }

        basic_json& operator=(const basic_json& other)
        {
            if (this != &other)
            {
                copy_assignment(other);
            }
            return *this;
        }

        basic_json& operator=(basic_json&& other) noexcept
        {
            if (this != &other)
            {
                move_assignment(std::move(other));
            }
            return *this;
        }

        json_storage_kind storage_kind() const
        {
            // It is legal to access 'common_.storage_kind_' even though 
            // common_ is not the active member of the union because 'storage_kind_' 
            // is a part of the common initial sequence of all union members
            // as defined in 11.4-25 of the Standard.
            return static_cast<json_storage_kind>(common_.storage_kind_);
        }

        json_type type() const
        {
            switch(storage_kind())
            {
                case json_storage_kind::null:
                    return json_type::null;
                case json_storage_kind::boolean:
                    return json_type::boolean;
                case json_storage_kind::int64:
                    return json_type::int64;
                case json_storage_kind::uint64:
                    return json_type::uint64;
                case json_storage_kind::half_float:
                    return json_type::float16;
                case json_storage_kind::float64:
                    return json_type::float64;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return json_type::string;
                case json_storage_kind::byte_str:
                    return json_type::byte_string;
                case json_storage_kind::array:
                    return json_type::array;
                case json_storage_kind::empty_object:
                case json_storage_kind::object:
                    return json_type::object;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().type();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().type();
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        semantic_tag tag() const
        {
            // It is legal to access 'common_.tag_' even though 
            // common_ is not the active member of the union because 'tag_' 
            // is a part of the common initial sequence of all union members
            // as defined in 11.4-25 of the Standard.
            switch(storage_kind())
            {
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().tag();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().tag();
                default:
                    return common_.tag_;
            }
        }

        std::size_t size() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().size();
                case json_storage_kind::empty_object:
                    return 0;
                case json_storage_kind::object:
                    return cast<object_storage>().value().size();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().size();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().size();
                default:
                    return 0;
            }
        }

        string_view_type as_string_view() const
        {
           auto result = try_as_string_view();
           if (!result)
           {
               JSONCONS_THROW(conv_error(result.error().code()));
           }
           return *result;
        }

        conversion_result<string_view_type> try_as_string_view() const
        {
            using result_type = conversion_result<string_view_type>;

            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                    return result_type(in_place, cast<short_string_storage>().data(),cast<short_string_storage>().length());
                case json_storage_kind::long_str:
                    return result_type(in_place, cast<long_string_storage>().data(),cast<long_string_storage>().length());
                case json_storage_kind::json_const_ref:
                    return result_type(cast<json_const_reference_storage>().value().as_string_view());
                case json_storage_kind::json_ref:
                    return result_type(cast<json_reference_storage>().value().as_string_view());
                default:
                   return result_type(jsoncons::unexpect, conv_errc::not_string);
            }
        }

        template <typename T,typename Alloc, typename TempAlloc>
        typename std::enable_if<ext_traits::is_basic_byte_string<T>::value,conversion_result<T>>::type
        try_as_byte_string(const allocator_set<Alloc,TempAlloc>& aset) const
        {
            using value_type = T;
            using result_type = conversion_result<value_type>;

            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                {
                    value_type bytes = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator());
                    const auto& stor = cast<short_string_storage>();
                    auto res = string_to_bytes(stor.data(), stor.data()+stor.length(), tag(), bytes);
                    if (JSONCONS_UNLIKELY(res.ec != conv_errc{}))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_byte_string);
                    }
                    return result_type(std::move(bytes));
                }
                case json_storage_kind::long_str:
                {
                    value_type bytes = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator());
                    const auto& stor = cast<long_string_storage>();
                    auto res = string_to_bytes(stor.data(), stor.data()+stor.length(), tag(), bytes);
                    if (JSONCONS_UNLIKELY(res.ec != conv_errc{}))
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_byte_string);
                    }
                    return result_type(std::move(bytes));
                }
                case json_storage_kind::byte_str:
                {
                    auto& bs = cast<byte_string_storage>();
                    auto val = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), 
                        bs.data(), bs.length());
                    return result_type(std::move(val));
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template try_as_byte_string<T>(aset);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template try_as_byte_string<T>(aset);
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_byte_string);
            }
        }

        template <typename BytesAlloc=std::allocator<uint8_t>>
        basic_byte_string<BytesAlloc> as_byte_string() const
        {
            auto result = try_as_byte_string<basic_byte_string<BytesAlloc>>(make_alloc_set(BytesAlloc()));
            if (!result)
            {
                JSONCONS_THROW(conv_error(result.error().code()));
            }
            return *result;
        }

        conversion_result<byte_string_view> try_as_byte_string_view() const
        {
            using result_type = conversion_result<byte_string_view>;

            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return result_type(in_place, cast<byte_string_storage>().data(),cast<byte_string_storage>().length());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().try_as_byte_string_view();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().try_as_byte_string_view();
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_byte_string);
            }
        }

        byte_string_view as_byte_string_view() const
        {
            auto result = try_as_byte_string_view();
            if (!result)
            {
                JSONCONS_THROW(conv_error(result.error().code()));
            }
            return *result;
        }

        int compare(const basic_json& rhs) const noexcept
        {
            if (this == &rhs)
            {
                return 0;
            }
            switch (storage_kind())
            {
                case json_storage_kind::json_const_ref:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::json_const_ref:
                            return cast<json_const_reference_storage>().value().compare(rhs.cast<json_const_reference_storage>().value());
                        default:
                            return cast<json_const_reference_storage>().value().compare(rhs);
                    }
                    break;
                case json_storage_kind::json_ref:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::json_ref:
                            return cast<json_reference_storage>().value().compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return cast<json_reference_storage>().value().compare(rhs);
                    }
                    break;
                case json_storage_kind::null:
                    return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                case json_storage_kind::empty_object:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            return 0;
                        case json_storage_kind::object:
                            return rhs.empty() ? 0 : -1;
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::boolean:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::boolean:
                            return static_cast<int>(cast<bool_storage>().value()) - static_cast<int>(rhs.cast<bool_storage>().value());
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::int64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                        {
                            if (cast<int64_storage>().value() == rhs.cast<int64_storage>().value())
                                return 0;
                            return cast<int64_storage>().value() < rhs.cast<int64_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::uint64:
                            if (cast<int64_storage>().value() < 0) 
                                return -1;
                            else if (static_cast<uint64_t>(cast<int64_storage>().value()) == rhs.cast<uint64_storage>().value()) 
                                return 0;
                            else
                                return static_cast<uint64_t>(cast<int64_storage>().value()) < rhs.cast<uint64_storage>().value() ? -1 : 1;
                        case json_storage_kind::float64:
                        {
                            double r = static_cast<double>(cast<int64_storage>().value()) - rhs.cast<double_storage>().value();
                            return r == 0.0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::uint64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                            if (rhs.cast<int64_storage>().value() < 0) 
                                return 1;
                            else if (cast<uint64_storage>().value() == static_cast<uint64_t>(rhs.cast<int64_storage>().value()))
                                return 0;
                            else
                                return cast<uint64_storage>().value() < static_cast<uint64_t>(rhs.cast<int64_storage>().value()) ? -1 : 1;
                        case json_storage_kind::uint64:
                            if (cast<uint64_storage>().value() == static_cast<uint64_t>(rhs.cast<int64_storage>().value()))
                                return 0;
                            else
                                return cast<uint64_storage>().value() < static_cast<uint64_t>(rhs.cast<int64_storage>().value()) ? -1 : 1;
                        case json_storage_kind::float64:
                        {
                            auto r = static_cast<double>(cast<uint64_storage>().value()) - rhs.cast<double_storage>().value();
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::float64:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::int64:
                        {
                            auto r = cast<double_storage>().value() - static_cast<double>(rhs.cast<int64_storage>().value());
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::uint64:
                        {
                            auto r = cast<double_storage>().value() - static_cast<double>(rhs.cast<uint64_storage>().value());
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::float64:
                        {
                            auto r = cast<double_storage>().value() - rhs.cast<double_storage>().value();
                            return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            if (is_string_storage(rhs.storage_kind()) && is_number_tag(rhs.tag()))
                            {
                                double val1 = as_double();
                                double val2 = rhs.as_double();
                                if (val1 < 0 && val2 >= 0)
                                {
                                    return -1;
                                }
                                if (val2 < 0 && val1 >= 0)
                                {
                                    return 1;
                                }

                                return val1 == val2 ? 0 : (val1 < val2 ? -1 : 1);
                            }
                            else
                            {
                                return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                            }
                    }
                    break;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    if (is_number_tag(tag()))
                    {
                        double val1 = as_double(); 
                        switch (rhs.storage_kind())
                        {
                            case json_storage_kind::int64:
                            {
                                auto r = val1 - static_cast<double>(rhs.cast<int64_storage>().value());
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::uint64:
                            {
                                auto r = val1 - static_cast<double>(rhs.cast<uint64_storage>().value());
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::float64:
                            {
                                auto r = val1 - rhs.cast<double_storage>().value();
                                return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                            }
                            case json_storage_kind::json_const_ref:
                                return compare(rhs.cast<json_const_reference_storage>().value());
                            case json_storage_kind::json_ref:
                                return compare(rhs.cast<json_reference_storage>().value());
                            default:
                                if (is_string_storage(rhs.storage_kind()) && is_number_tag(rhs.tag()))
                                {
                                    double val2 = rhs.as_double();
                                    if (val1 == val2)
                                    {
                                        return 0;
                                    }
                                    auto r = val1 - val2; 
                                    return r == 0 ? 0 : (r < 0.0 ? -1 : 1);
                                }
                                else
                                {
                                    return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                                }
                        }
                    }
                    else
                    {
                        // compare regular text
                        switch (rhs.storage_kind())
                        {
                            case json_storage_kind::short_str:
                                return as_string_view().compare(rhs.as_string_view());
                            case json_storage_kind::long_str:
                                return as_string_view().compare(rhs.as_string_view());
                            case json_storage_kind::json_const_ref:
                                return compare(rhs.cast<json_const_reference_storage>().value());
                            case json_storage_kind::json_ref:
                                return compare(rhs.cast<json_reference_storage>().value());
                            default:
                                return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                        }
                    }
                    break;
                case json_storage_kind::byte_str:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::byte_str:
                        {
                            return as_byte_string_view().compare(rhs.as_byte_string_view());
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::array:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::array:
                        {
                            if (cast<array_storage>().value() == rhs.cast<array_storage>().value())
                                return 0; 
                            else 
                                return cast<array_storage>().value() < rhs.cast<array_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                case json_storage_kind::object:
                    switch (rhs.storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            return empty() ? 0 : 1;
                        case json_storage_kind::object:
                        {
                            if (cast<object_storage>().value() == rhs.cast<object_storage>().value())
                                return 0; 
                            else 
                                return cast<object_storage>().value() < rhs.cast<object_storage>().value() ? -1 : 1;
                        }
                        case json_storage_kind::json_const_ref:
                            return compare(rhs.cast<json_const_reference_storage>().value());
                        case json_storage_kind::json_ref:
                            return compare(rhs.cast<json_reference_storage>().value());
                        default:
                            return static_cast<int>(storage_kind()) - static_cast<int>(rhs.storage_kind());
                    }
                    break;
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        void swap(basic_json& other) noexcept
        {
            if (this == &other)
            {
                return;
            }
            if (is_trivial_storage(storage_kind()) && is_trivial_storage(other.storage_kind()))
            {
                basic_json temp;               
                std::memcpy(static_cast<void*>(&temp), static_cast<void*>(&other), sizeof(basic_json));
                std::memcpy(static_cast<void*>(&other), static_cast<void*>(this), sizeof(basic_json));
                std::memcpy(static_cast<void*>(this), static_cast<void*>(&temp), sizeof(basic_json));
            }
            else
            {
                switch (storage_kind())
                {
                    case json_storage_kind::null: swap_l<null_storage>(other); break;
                    case json_storage_kind::empty_object : swap_l<empty_object_storage>(other); break;
                    case json_storage_kind::boolean: swap_l<bool_storage>(other); break;
                    case json_storage_kind::int64: swap_l<int64_storage>(other); break;
                    case json_storage_kind::uint64: swap_l<uint64_storage>(other); break;
                    case json_storage_kind::half_float: swap_l<half_storage>(other); break;
                    case json_storage_kind::float64: swap_l<double_storage>(other); break;
                    case json_storage_kind::short_str: swap_l<short_string_storage>(other); break;
                    case json_storage_kind::long_str: swap_l<long_string_storage>(other); break;
                    case json_storage_kind::byte_str: swap_l<byte_string_storage>(other); break;
                    case json_storage_kind::array: swap_l<array_storage>(other); break;
                    case json_storage_kind::object: swap_l<object_storage>(other); break;
                    case json_storage_kind::json_const_ref: swap_l<json_const_reference_storage>(other); break;
                    case json_storage_kind::json_ref: swap_l<json_reference_storage>(other); break;
                    default:
                        JSONCONS_UNREACHABLE();
                        break;
                }
            }
        }
        // from string

        template <typename Source>
        static
        typename std::enable_if<ext_traits::is_string_view_of<Source,char_type>::value,basic_json>::type
        parse(const Source& source, 
              const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            json_decoder<basic_json> decoder;
            basic_json_parser<char_type> parser(options);

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        template <typename Source,typename TempAlloc >
        static
        typename std::enable_if<ext_traits::is_string_view_of<Source,char_type>::value,basic_json>::type
            parse(const allocator_set<allocator_type,TempAlloc>& aset, const Source& source, 
              const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            json_decoder<basic_json> decoder(aset.get_allocator(), aset.get_temp_allocator());
            basic_json_parser<char_type,TempAlloc> parser(options, aset.get_temp_allocator());

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        static basic_json parse(const char_type* str, std::size_t length, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(jsoncons::string_view(str,length), options);
        }

        static basic_json parse(const char_type* source, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(jsoncons::basic_string_view<char_type>(source), options);
        }

        template <typename TempAlloc >
        static basic_json parse(const allocator_set<allocator_type,TempAlloc>& aset, const char_type* source, 
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(aset, jsoncons::basic_string_view<char_type>(source), options);
        }

        template <typename TempAlloc >
        static basic_json parse(const allocator_set<allocator_type,TempAlloc>& aset, 
            const char_type* str, std::size_t length,
            const basic_json_decode_options<char_type>& options = basic_json_options<char_type>())
        {
            return parse(aset, jsoncons::basic_string_view<char_type>(str, length), options);
        }

        // from stream

        static basic_json parse(std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,stream_source<char_type>,Allocator> reader(is, decoder, options);
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        template <typename TempAlloc >
        static basic_json parse(const allocator_set<allocator_type,TempAlloc>& aset, std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder(aset.get_allocator(), aset.get_temp_allocator());
            basic_json_reader<char_type,stream_source<char_type>,Allocator> reader(is, decoder, options, aset.get_temp_allocator());
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        // from iterator

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,iterator_source<InputIt>,Allocator> reader(iterator_source<InputIt>(std::forward<InputIt>(first),
                std::forward<InputIt>(last)), decoder, options);
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

        template <typename InputIt,typename TempAlloc >
        static basic_json parse(const allocator_set<allocator_type,TempAlloc>& aset, InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options = basic_json_options<CharT>())
        {
            json_decoder<basic_json> decoder(aset.get_allocator(), aset.get_temp_allocator());
            basic_json_reader<char_type,iterator_source<InputIt>,Allocator> reader(iterator_source<InputIt>(std::forward<InputIt>(first),
                std::forward<InputIt>(last)), 
                decoder, options, aset.get_temp_allocator());
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

#if !defined(JSONCONS_NO_DEPRECATED)

        static basic_json parse(const char_type* s, 
            const basic_json_decode_options<char_type>& options, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(jsoncons::basic_string_view<char_type>(s), options, err_handler);
        }

        static basic_json parse(const char_type* s, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(jsoncons::basic_string_view<char_type>(s), basic_json_decode_options<char_type>(), err_handler);
        }

        static basic_json parse(std::basic_istream<char_type>& is, 
            const basic_json_decode_options<char_type>& options, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,stream_source<char_type>> reader(is, decoder, options, err_handler);
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json stream"));
            }
            return decoder.get_result();
        }

        static basic_json parse(std::basic_istream<char_type>& is, std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(is, basic_json_decode_options<CharT>(), err_handler);
        }

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                const basic_json_decode_options<char_type>& options, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_reader<char_type,iterator_source<InputIt>> reader(iterator_source<InputIt>(std::forward<InputIt>(first),std::forward<InputIt>(last)), decoder, options, err_handler);
            reader.read_next();
            reader.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json from iterator pair"));
            }
            return decoder.get_result();
        }

        template <typename Source>
        static
        typename std::enable_if<ext_traits::is_string_view_of<Source,char_type>::value,basic_json>::type
        parse(const Source& source, 
              const basic_json_decode_options<char_type>& options, 
              std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            json_decoder<basic_json> decoder;
            basic_json_parser<char_type> parser(options,err_handler);

            auto r = unicode_traits::detect_encoding_from_bom(source.data(), source.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
            {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character,parser.line(),parser.column()));
            }
            std::size_t offset = (r.ptr - source.data());
            parser.update(source.data()+offset,source.size()-offset);
            parser.parse_some(decoder);
            parser.finish_parse(decoder);
            parser.check_done();
            if (JSONCONS_UNLIKELY(!decoder.is_valid()))
            {
                JSONCONS_THROW(ser_error(json_errc::source_error, "Failed to parse json string"));
            }
            return decoder.get_result();
        }

        template <typename Source>
        static
        typename std::enable_if<ext_traits::is_string_view_of<Source,char_type>::value,basic_json>::type
        parse(const Source& source, 
            std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(source, basic_json_decode_options<CharT>(), err_handler);
        }

        template <typename InputIt>
        static basic_json parse(InputIt first, InputIt last, 
                                std::function<bool(json_errc,const ser_context&)> err_handler)
        {
            return parse(first, last, basic_json_decode_options<CharT>(), err_handler);
        }
#endif
        static basic_json make_array()
        {
            return basic_json(array());
        }

        static basic_json make_array(const array& a)
        {
            return basic_json(a);
        }

        static basic_json make_array(const array& a, allocator_type alloc)
        {
            return basic_json(a, semantic_tag::none, alloc);
        }

        static basic_json make_array(std::initializer_list<basic_json> init, const Allocator& alloc = Allocator())
        {
            return array(std::move(init),alloc);
        }

        static basic_json make_array(std::size_t n, const Allocator& alloc = Allocator())
        {
            return array(n,alloc);
        }

        template <typename T>
        static basic_json make_array(std::size_t n, const T& val, const Allocator& alloc = Allocator())
        {
            return basic_json::array(n, val,alloc);
        }

        template <std::size_t dim>
        static typename std::enable_if<dim==1,basic_json>::type make_array(std::size_t n)
        {
            return array(n);
        }

        template <std::size_t dim,typename T>
        static typename std::enable_if<dim==1,basic_json>::type make_array(std::size_t n, const T& val, const Allocator& alloc = Allocator())
        {
            return array(n,val,alloc);
        }

        template <std::size_t dim,typename... Args>
        static typename std::enable_if<(dim>1),basic_json>::type make_array(std::size_t n, Args... args)
        {
            const size_t dim1 = dim - 1;

            basic_json val = make_array<dim1>(std::forward<Args>(args)...);
            val.resize(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                val[i] = make_array<dim1>(std::forward<Args>(args)...);
            }
            return val;
        }

        static const basic_json& null()
        {
            static const basic_json a_null = basic_json(null_arg, semantic_tag::none);
            return a_null;
        }

        basic_json() 
        {
            construct<empty_object_storage>(semantic_tag::none);
        }

        explicit basic_json(const Allocator& alloc) 
        {
            auto ptr = create_object(alloc);
            construct<object_storage>(ptr, semantic_tag::none);
        }

        basic_json(semantic_tag tag, const Allocator& alloc) 
        {
            auto ptr = create_object(alloc);
            construct<object_storage>(ptr, tag);
        }

        basic_json(semantic_tag tag) 
        {
            construct<empty_object_storage>(tag);
        }

        basic_json(const basic_json& other)
        {
            uninitialized_copy(other);
        }

        basic_json(const basic_json& other, const Allocator& alloc)
        {
            uninitialized_copy_a(other,alloc);
        }

        basic_json(basic_json&& other) noexcept
        {
            uninitialized_move(std::move(other));
        }

        template <typename U = Allocator>
        basic_json(basic_json&& other, const Allocator& alloc) noexcept
        {
            uninitialized_move_a(typename std::allocator_traits<U>::is_always_equal(), std::move(other), alloc);
        }

        explicit basic_json(json_object_arg_t, 
                            semantic_tag tag,
                            const Allocator& alloc = Allocator()) 
        {
            auto ptr = create_object(alloc);
            construct<object_storage>(ptr, tag);
        }

        explicit basic_json(json_object_arg_t) 
        {
            auto ptr = create_object(Allocator{});
            construct<object_storage>(ptr, semantic_tag::none);
        }

        basic_json(json_object_arg_t, const Allocator& alloc) 
        {
            auto ptr = create_object(alloc);
            construct<object_storage>(ptr, semantic_tag::none);
        }

        template <typename InputIt>
        basic_json(json_object_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag = semantic_tag::none) 
        {
            auto ptr = create_object(Allocator(), first, last);
            construct<object_storage>(ptr, tag);
        }

        template <typename InputIt>
        basic_json(json_object_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag,
                   const Allocator& alloc) 
        {
            auto ptr = create_object(alloc, first, last);
            construct<object_storage>(ptr, tag);
        }

        basic_json(json_object_arg_t, 
                   std::initializer_list<std::pair<std::basic_string<char_type>,basic_json>> init, 
                   semantic_tag tag = semantic_tag::none) 
        {
            auto ptr = create_object(Allocator(), init);
            construct<object_storage>(ptr, tag);
        }

        basic_json(json_object_arg_t, 
                   std::initializer_list<std::pair<std::basic_string<char_type>,basic_json>> init, 
                   semantic_tag tag, 
                   const Allocator& alloc) 
        {
            auto ptr = create_object(alloc, init);
            construct<object_storage>(ptr, tag);
        }

        explicit basic_json(json_array_arg_t) 
        {
            auto ptr = create_array(Allocator{});
            construct<array_storage>(ptr, semantic_tag::none);
        }

        basic_json(json_array_arg_t, const Allocator& alloc) 
        {
            auto ptr = create_array(alloc);
            construct<array_storage>(ptr, semantic_tag::none);
        }

        basic_json(json_array_arg_t, std::size_t count, const basic_json& value,
            semantic_tag tag = semantic_tag::none) 
        {
            auto ptr = create_array(Allocator(), count, value);
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_array_arg_t, std::size_t count, const basic_json& value,
            semantic_tag tag, const Allocator& alloc) 
        {
            auto ptr = create_array(alloc, count, value);
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_array_arg_t, 
            semantic_tag tag) 
        {
            auto ptr = create_array(Allocator());
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_array_arg_t, 
            semantic_tag tag, 
            const Allocator& alloc) 
        {
            auto ptr = create_array(alloc);
            construct<array_storage>(ptr, tag);
        }

        template <typename InputIt>
        basic_json(json_array_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag = semantic_tag::none) 
        {
            auto ptr = create_array(Allocator(), first, last);
            construct<array_storage>(ptr, tag);
        }

        template <typename InputIt>
        basic_json(json_array_arg_t, 
                   InputIt first, InputIt last, 
                   semantic_tag tag, 
                   const Allocator& alloc) 
        {
            auto ptr = create_array(alloc, first, last);
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_array_arg_t, 
                   std::initializer_list<basic_json> init, 
                   semantic_tag tag = semantic_tag::none) 
        {
            auto ptr = create_array(Allocator(), init);
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_array_arg_t, 
                   std::initializer_list<basic_json> init, 
                   semantic_tag tag, 
                   const Allocator& alloc) 
        {
            auto ptr = create_array(alloc, init);
            construct<array_storage>(ptr, tag);
        }

        basic_json(json_const_pointer_arg_t, const basic_json* ptr) noexcept 
        {
            if (ptr == nullptr)
            {
                construct<null_storage>(semantic_tag::none);
            }
            else
            {
                construct<json_const_reference_storage>(*ptr);
            }
        }

        basic_json(json_pointer_arg_t, basic_json* ptr) noexcept 
        {
            if (ptr == nullptr)
            {
                construct<null_storage>(semantic_tag::none);
            }
            else
            {
                construct<json_reference_storage>(*ptr);
            }
        }

        basic_json(const array& val, semantic_tag tag = semantic_tag::none)
        {
            auto ptr = create_array(
                std::allocator_traits<Allocator>::select_on_container_copy_construction(val.get_allocator()), 
                val);
            construct<array_storage>(ptr, tag);
        }

        basic_json(const array& val, semantic_tag tag, allocator_type alloc)
        {
            auto ptr = create_array(alloc, val);
            construct<array_storage>(ptr, tag);
        }

        basic_json(array&& val, semantic_tag tag = semantic_tag::none)
        {
            auto alloc = val.get_allocator();
            auto ptr = create_array(alloc, std::move(val));
            construct<array_storage>(ptr, tag);
        }

        basic_json(const object& val, semantic_tag tag = semantic_tag::none)
        {
            auto ptr = create_object(
                std::allocator_traits<Allocator>::select_on_container_copy_construction(val.get_allocator()), 
                val);
            construct<object_storage>(ptr, tag);
        }

        basic_json(const object& val, semantic_tag tag, allocator_type alloc)
        {
            auto ptr = create_object(alloc, val);
            construct<object_storage>(ptr, tag);
        }

        basic_json(object&& val, semantic_tag tag = semantic_tag::none)
        {
            auto alloc = val.get_allocator();
            auto ptr = create_object(alloc, std::move(val));
            construct<object_storage>(ptr, tag);
        }

        template <typename T>
        basic_json(const T& val)
            : basic_json(reflect::json_conv_traits<basic_json,T>::to_json(make_alloc_set(), val))
        {
        }

        template <typename T>
        basic_json(const T& val, const Allocator& alloc)
            : basic_json(reflect::json_conv_traits<basic_json,T>::to_json(make_alloc_set(alloc), val))
        {
        }

        template <typename SAlloc>
        basic_json(const std::basic_string<char_type,std::char_traits<char_type>,SAlloc>& s, 
            semantic_tag tag = semantic_tag::none)
            : basic_json(s.data(), s.size(), tag, Allocator())
        {
        }

        template <typename SAlloc>
        basic_json(const std::basic_string<char_type, std::char_traits<char_type>, SAlloc>& s, 
            const allocator_type& alloc)
            : basic_json(s.data(), s.size(), semantic_tag::none, alloc)
        {
        }

        template <typename SAlloc>
        basic_json(const std::basic_string<char_type, std::char_traits<char_type>, SAlloc>& s, 
            semantic_tag tag, const allocator_type& alloc)
            : basic_json(s.data(), s.size(), tag, alloc)
        {
        }

        basic_json(const string_view_type& s, semantic_tag tag = semantic_tag::none)
            : basic_json(s.data(), s.size(), tag, allocator_type())
        {
        }

        basic_json(const string_view_type& s, const allocator_type& alloc)
            : basic_json(s.data(), s.size(), semantic_tag::none, alloc)
        {
        }

        basic_json(const char_type* s, semantic_tag tag = semantic_tag::none)
            : basic_json(s, char_traits_type::length(s), tag)
        {
        }

        basic_json(const char_type* s, semantic_tag tag, const allocator_type& alloc)
            : basic_json(s, char_traits_type::length(s), tag, alloc)
        {
        }

        basic_json(const char_type* s, const Allocator& alloc)
            : basic_json(s, char_traits_type::length(s), semantic_tag::none, alloc)
        {
        }

        basic_json(const char_type* s, std::size_t length, semantic_tag tag = semantic_tag::none)
            : basic_json(s, length, tag, Allocator())
        {
        }

        basic_json(const char_type* s, std::size_t length, semantic_tag tag, const Allocator& alloc)
        {
            if (length <= short_string_storage::max_length)
            {
                construct<short_string_storage>(s, static_cast<uint8_t>(length), tag);
            }
            else
            {
                auto ptr = create_long_string(alloc, s, length);
                construct<long_string_storage>(ptr, tag);
            }
        }

        basic_json(half_arg_t, uint16_t val, semantic_tag tag = semantic_tag::none)
        {
            construct<half_storage>(val, tag);
        }

        basic_json(half_arg_t, uint16_t val, semantic_tag tag, const allocator_type&)
        {
            construct<half_storage>(val, tag);
        }

        basic_json(double val, semantic_tag tag)
        {
            construct<double_storage>(val, tag);
        }

        basic_json(double val, semantic_tag tag, const allocator_type&)
        {
            construct<double_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, 
                   typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(uint64_t), int>::type = 0)
        {
            construct<uint64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, Allocator, 
                   typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(uint64_t), int>::type = 0)
        {
            construct<uint64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag, const Allocator& alloc = Allocator(),
                   typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value && sizeof(uint64_t) < sizeof(IntegerType), int>::type = 0)
        {
            std::basic_string<CharT> s;
            jsoncons::from_integer(val, s);
            if (s.length() <= short_string_storage::max_length)
            {
                construct<short_string_storage>(s.data(), static_cast<uint8_t>(s.length()), semantic_tag::bigint);
            }
            else
            {
                auto ptr = create_long_string(alloc, s.data(), s.length());
                construct<long_string_storage>(ptr, semantic_tag::bigint);
            }
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag,
                   typename std::enable_if<ext_traits::is_signed_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),int>::type = 0)
        {
            construct<int64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag tag, Allocator,
                   typename std::enable_if<ext_traits::is_signed_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),int>::type = 0)
        {
            construct<int64_storage>(val, tag);
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag,
                   typename std::enable_if<ext_traits::is_signed_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),int>::type = 0)
        {
            std::basic_string<CharT> s;
            jsoncons::from_integer(val, s);
            if (s.length() <= short_string_storage::max_length)
            {
                construct<short_string_storage>(s.data(), static_cast<uint8_t>(s.length()), semantic_tag::bigint);
            }
            else
            {
                auto ptr = create_long_string(Allocator(), s.data(), s.length());
                construct<long_string_storage>(ptr, semantic_tag::bigint);
            }
        }

        template <typename IntegerType>
        basic_json(IntegerType val, semantic_tag, const Allocator& alloc,
                   typename std::enable_if<ext_traits::is_signed_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),int>::type = 0)
        {
            std::basic_string<CharT> s;
            jsoncons::from_integer(val, s);
            if (s.length() <= short_string_storage::max_length)
            {
                construct<short_string_storage>(s.data(), static_cast<uint8_t>(s.length()), semantic_tag::bigint);
            }
            else
            {
                auto ptr = create_long_string(alloc, s.data(), s.length());
                construct<long_string_storage>(ptr, semantic_tag::bigint);
            }
        }

        basic_json(null_type, semantic_tag tag)
        {
            construct<null_storage>(tag);
        }

        basic_json(null_type, semantic_tag tag, const Allocator&)
        {
            construct<null_storage>(tag);
        }

        basic_json(bool val, semantic_tag tag)
        {
            construct<bool_storage>(val,tag);
        }

        basic_json(bool val, semantic_tag tag, const Allocator&)
        {
            construct<bool_storage>(val,tag);
        }

        basic_json(const string_view_type& sv, semantic_tag tag, const allocator_type& alloc)
            : basic_json(sv.data(), sv.length(), tag, alloc)
        {
        }

        template <typename BytesViewLike>
        basic_json(byte_string_arg_t, const BytesViewLike& source, 
                   semantic_tag tag = semantic_tag::none,
                   typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());
            
            auto ptr = create_byte_string(Allocator(), bytes.data(), bytes.size(), 0);
            construct<byte_string_storage>(ptr, tag);
        }

        template <typename BytesViewLike>
        basic_json(byte_string_arg_t, const BytesViewLike& source, 
                   semantic_tag tag,
                   const Allocator& alloc,
                   typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());

            auto ptr = create_byte_string(alloc, bytes.data(), bytes.size(), 0);
            construct<byte_string_storage>(ptr, tag);
        }

        template <typename BytesViewLike>
        basic_json(byte_string_arg_t, const BytesViewLike& source, 
                   uint64_t ext_tag,
                   typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());

            auto ptr = create_byte_string(Allocator(), bytes.data(), bytes.size(), ext_tag);
            construct<byte_string_storage>(ptr, semantic_tag::ext);
        }

        template <typename BytesViewLike>
        basic_json(byte_string_arg_t, const BytesViewLike& source, 
                   uint64_t ext_tag,
                   const Allocator& alloc,
                   typename std::enable_if<ext_traits::is_bytes_view_like<BytesViewLike>::value,int>::type = 0)
        {
            auto bytes = jsoncons::span<const uint8_t>(reinterpret_cast<const uint8_t*>(source.data()), source.size());

            auto ptr = create_byte_string(alloc, bytes.data(), bytes.size(), ext_tag);
            construct<byte_string_storage>(ptr, semantic_tag::ext);
        }

        template <typename InputIterator>
        basic_json(InputIterator first, InputIterator last)
            : basic_json(json_array_arg,first,last,Allocator())
        {
        }

        template <typename InputIterator>
        basic_json(InputIterator first, InputIterator last, const Allocator& alloc)
            : basic_json(json_array_arg,first,last,alloc)
        {
        }

        ~basic_json() noexcept
        {
             destroy();
        }

        template <typename T>
        basic_json& operator=(const T& val)
        {
            *this = reflect::json_conv_traits<basic_json,T>::to_json(make_alloc_set(), val);
            return *this;
        }

        basic_json& operator=(const char_type* s)
        {
            *this = basic_json(s, char_traits_type::length(s), semantic_tag::none);
            return *this;
        }

        basic_json& operator[](std::size_t i)
        {
            return at(i);
        }

        const basic_json& operator[](std::size_t i) const
        {
            return at(i);
        }

        basic_json& operator[](const string_view_type& key)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object: 
                    return try_emplace(key, basic_json{}).first->value();
                case json_storage_kind::object:
                {           
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        return try_emplace(key, basic_json{}).first->value();
                    }
                    else
                    {
                        return (*it).value();
                    }
                    break;
                }
                case json_storage_kind::json_ref: 
                    return cast<json_reference_storage>().value()[key];
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }               
        }

        const basic_json& operator[](const string_view_type& key) const
        {
            static const basic_json an_empty_object = basic_json();

            switch (storage_kind())
            {
                case json_storage_kind::empty_object: 
                    return an_empty_object;
                case json_storage_kind::object:
                {           
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        return an_empty_object;
                    }
                    else
                    {
                        return (*it).value();
                    }
                    break;
                }
                case json_storage_kind::json_const_ref: 
                    return cast<json_const_reference_storage>().value().at(key);
                case json_storage_kind::json_ref: 
                    return cast<json_reference_storage>().value()[key];
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
             const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump(cont, options, indenting::no_indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
             const basic_json_encode_options<char_type>& options,
             indenting indent) const
        {
            std::error_code ec;
            dump(cont, options, indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont,
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump_pretty(cont, options, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump(os, options, indenting::no_indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options,
            indenting indent) const
        {
            std::error_code ec;
            dump(os, options, indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont,
            const basic_json_encode_options<char_type>& options, 
            std::error_code& ec) const
        {
            basic_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options);
            dump(encoder, ec);
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump_pretty(CharContainer& cont, 
            std::error_code& ec) const
        {
            dump_pretty(cont, basic_json_encode_options<char_type>(), ec);
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options = basic_json_options<CharT>()) const
        {
            std::error_code ec;
            dump_pretty(os, options, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            const basic_json_encode_options<char_type>& options, 
            std::error_code& ec) const
        {
            basic_json_encoder<char_type> encoder(os, options);
            dump(encoder, ec);
        }

        void dump_pretty(std::basic_ostream<char_type>& os, 
            std::error_code& ec) const
        {
            dump_pretty(os, basic_json_encode_options<char_type>(), ec);
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, indenting indent) const
        {
            std::error_code ec;

            dump(cont, indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  indenting indent) const
        {
            std::error_code ec;

            dump(os, indent, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        void dump(basic_json_visitor<char_type>& visitor) const
        {
            std::error_code ec;
            dump(visitor, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(ser_error(ec));
            }
        }

        // dump
        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
                  const basic_json_encode_options<char_type>& options, 
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont, options);
            dump(encoder, ec);
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type,jsoncons::string_sink<CharContainer>> encoder(cont);
            dump(encoder, ec);
        }

        void dump(std::basic_ostream<char_type>& os, 
                  const basic_json_encode_options<char_type>& options,
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type> encoder(os, options);
            dump(encoder, ec);
        }

        void dump(std::basic_ostream<char_type>& os, 
                  std::error_code& ec) const
        {
            basic_compact_json_encoder<char_type> encoder(os);
            dump(encoder, ec);
        }

        // legacy
        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont,
                  const basic_json_encode_options<char_type>& options, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(cont, options, ec);
            }
            else
            {
                dump(cont, options, ec);
            }
        }

        template <typename CharContainer>
        typename std::enable_if<ext_traits::is_back_insertable_char_container<CharContainer>::value>::type
        dump(CharContainer& cont, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(cont, ec);
            }
            else
            {
                dump(cont, ec);
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  const basic_json_encode_options<char_type>& options, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(os, options, ec);
            }
            else
            {
                dump(os, options, ec);
            }
        }

        void dump(std::basic_ostream<char_type>& os, 
                  indenting indent,
                  std::error_code& ec) const
        {
            if (indent == indenting::indent)
            {
                dump_pretty(os, ec);
            }
            else
            {
                dump(os, ec);
            }
        }
    // end legacy

        void dump(basic_json_visitor<char_type>& visitor, 
                  std::error_code& ec) const
        {
            dump_noflush(visitor, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                return;
            }
            visitor.flush();
        }

        write_result try_dump(basic_json_visitor<char_type>& visitor) const
        {
            auto r = try_dump_noflush(visitor);
            visitor.flush();
            return r;
        }

        bool is_null() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::null:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_null();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_null();
                default:
                    return false;
            }
        }

        allocator_type get_default_allocator(std::false_type) const
        {
            JSONCONS_THROW(json_runtime_error<std::domain_error>("No default allocator if allocator is not default constructible."));
        }

        allocator_type get_default_allocator(std::true_type) const
        {
            return allocator_type{};
        }

        template <typename U=Allocator>
        allocator_type get_allocator() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::long_str:
                    return cast<long_string_storage>().get_allocator();
                case json_storage_kind::byte_str:
                    return cast<byte_string_storage>().get_allocator();
                case json_storage_kind::array:
                    return cast<array_storage>().get_allocator();
                case json_storage_kind::object:
                    return cast<object_storage>().get_allocator();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().get_allocator();
                default:
                    return get_default_allocator(typename std::allocator_traits<U>::is_always_equal());
            }
        }

        uint64_t ext_tag() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                {
                    return cast<byte_string_storage>().ext_tag();
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().ext_tag();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().ext_tag();
                default:
                    return 0;
            }
        }

        bool contains(const string_view_type& key) const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    return it != cast<object_storage>().value().end();
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().contains(key);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().contains(key);
                default:
                    return false;
            }
        }

        std::size_t count(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        return 0;
                    }
                    std::size_t count = 0;
                    while (it != cast<object_storage>().value().end()&& (*it).key() == key)
                    {
                        ++count;
                        ++it;
                    }
                    return count;
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().count(key);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().count(key);
                default:
                    return 0;
            }
        }

        template <typename T,typename... Args>
        bool is(Args&&... args) const noexcept
        {
            return reflect::json_conv_traits<basic_json,T>::is(*this,std::forward<Args>(args)...);
        }

        bool is_string() const noexcept
        {
            if (is_string_storage(storage_kind()))
            {
                return true;
            }
            if (storage_kind() == json_storage_kind::json_const_ref)
            {
                return cast<json_const_reference_storage>().value().is_string();
            }
            if (storage_kind() == json_storage_kind::json_ref)
            {
                return cast<json_const_reference_storage>().value().is_string();
            }
            return false;
        }

        bool is_string_view() const noexcept
        {
            return is_string();
        }

        bool is_byte_string() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_byte_string();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_byte_string();
                default:
                    return false;
            }
        }

        bool is_byte_string_view() const noexcept
        {
            return is_byte_string();
        }

        bool is_bignum() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return is_number_tag(tag());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_bignum();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_bignum();
                default:
                    return false;
            }
        }

        bool is_bool() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::boolean:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_bool();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_bool();
                default:
                    return false;
            }
        }

        bool is_object() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                case json_storage_kind::object:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_object();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_object();
                default:
                    return false;
            }
        }

        bool is_array() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_array();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_array();
                default:
                    return false;
            }
        }

        bool is_int64() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return true;
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_int64();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_int64();
                default:
                    return false;
            }
        }

        bool is_uint64() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::uint64:
                    return true;
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_uint64();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_uint64();
                default:
                    return false;
            }
        }

        bool is_half() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::half_float:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_half();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_half();
                default:
                    return false;
            }
        }

        bool is_double() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::float64:
                    return true;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_double();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_double();
                default:
                    return false;
            }
        }

        bool is_number() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                case json_storage_kind::uint64:
                case json_storage_kind::half_float:
                case json_storage_kind::float64:
                    return true;
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    return is_number_tag(tag());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().is_number();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().is_number();
                default:
                    return false;
            }
        }

        bool empty() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::byte_str:
                    return cast<byte_string_storage>().length() == 0;
                    break;
                case json_storage_kind::short_str:
                    return cast<short_string_storage>().length() == 0;
                case json_storage_kind::long_str:
                    return cast<long_string_storage>().length() == 0;
                case json_storage_kind::array:
                    return cast<array_storage>().value().empty();
                case json_storage_kind::empty_object:
                    return true;
                case json_storage_kind::object:
                    return cast<object_storage>().value().empty();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().empty();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().empty();
                default:
                    return false;
            }
        }

        std::size_t capacity() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().capacity();
                case json_storage_kind::object:
                    return cast<object_storage>().value().capacity();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().capacity();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().capacity();
                default:
                    return 0;
            }
        }

        template <typename U=Allocator>
        void create_object_implicitly()
        {
            create_object_implicitly(typename std::allocator_traits<U>::is_always_equal());
        }

        void create_object_implicitly(std::false_type)
        {
            JSONCONS_THROW(json_runtime_error<std::domain_error>("Cannot create object implicitly - allocator is stateful."));
        }

        void create_object_implicitly(std::true_type)
        {
            *this = basic_json(json_object_arg, tag());
        }

        void reserve(std::size_t n)
        {
            if (n > 0)
            {
                switch (storage_kind())
                {
                    case json_storage_kind::array:
                        cast<array_storage>().value().reserve(n);
                        break;
                    case json_storage_kind::empty_object:
                        create_object_implicitly();
                        cast<object_storage>().value().reserve(n);
                        break;
                    case json_storage_kind::object:
                        cast<object_storage>().value().reserve(n);
                        break;
                    case json_storage_kind::json_ref:
                        cast<json_reference_storage>().value().reserve(n);
                        break;
                    default:
                        break;
                }
            }
        }

        void resize(std::size_t n)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().resize(n);
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().resize(n);
                    break;
                default:
                    break;
            }
        }

        template <typename T>
        void resize(std::size_t n, T val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().resize(n, val);
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().resize(n, val);
                    break;
                default:
                    break;
            }
        }

        template <typename T>
        typename std::enable_if<reflect::is_json_conv_traits_specialized<basic_json,T>::value,T>::type
        as() const
        {
            auto r = reflect::json_conv_traits<basic_json,T>::try_as(make_alloc_set(), *this);
            if (!r)
            {
                JSONCONS_THROW(conv_error(r.error().code(), r.error().message_arg()));
            }
            return std::move(r.value());
        }

        template <typename T, typename Alloc, typename TempAlloc>
        typename std::enable_if<reflect::is_json_conv_traits_specialized<basic_json,T>::value,T>::type
        as(const allocator_set<Alloc,TempAlloc>& aset) const
        {
            auto r = reflect::json_conv_traits<basic_json,T>::try_as(aset, *this);
            if (!r)
            {
                JSONCONS_THROW(conv_error(r.error().code(), r.error().message_arg()));
            }
            return std::move(r.value());
        }

        template <typename T>
        typename std::enable_if<reflect::is_json_conv_traits_specialized<basic_json,T>::value,conversion_result<T>>::type
        try_as() const
        {
            return reflect::json_conv_traits<basic_json,T>::try_as(make_alloc_set(), *this);
        }

        template <typename T, typename Alloc, typename TempAlloc>
        typename std::enable_if<reflect::is_json_conv_traits_specialized<basic_json,T>::value,conversion_result<T>>::type
        try_as(const allocator_set<Alloc,TempAlloc>& aset) const
        {
            return reflect::json_conv_traits<basic_json,T>::try_as(aset, *this);
        }

        template <typename T>
        typename std::enable_if<(!ext_traits::is_string<T>::value && 
                                 ext_traits::is_back_insertable_byte_container<T>::value) ||
                                 ext_traits::is_basic_byte_string<T>::value,T>::type
        as(byte_string_arg_t, semantic_tag hint) const
        {
            std::error_code ec;
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    switch (tag())
                    {
                        case semantic_tag::base16:
                        case semantic_tag::base64:
                        case semantic_tag::base64url:
                        {
                            T v;
                            auto sv = as_string_view();
                            auto r = string_to_bytes(sv.begin(), sv.end(), tag(), v);
                            if (JSONCONS_UNLIKELY(r.ec != conv_errc{}))
                            {
                                JSONCONS_THROW(ser_error(conv_errc::not_byte_string));
                            }
                            return v;
                        }
                        default:
                        {
                            T v;
                            auto sv = as_string_view();
                            auto r = string_to_bytes(sv.begin(), sv.end(), hint, v);
                            if (JSONCONS_UNLIKELY(r.ec != conv_errc{}))
                            {
                                JSONCONS_THROW(ser_error(conv_errc::not_byte_string));
                            }
                            return v;
                        }
                        break;
                    }
                    break;
                }
                case json_storage_kind::byte_str:
                    return T(as_byte_string_view().begin(), as_byte_string_view().end());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template as<T>(byte_string_arg, hint);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template as<T>(byte_string_arg, hint);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a byte string"));
            }
        }

        bool as_bool() const 
        {
            switch (storage_kind())
            {
                case json_storage_kind::boolean:
                    return cast<bool_storage>().value();
                case json_storage_kind::int64:
                    return cast<int64_storage>().value() != 0;
                case json_storage_kind::uint64:
                    return cast<uint64_storage>().value() != 0;
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().as_bool();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().as_bool();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a bool"));
            }
        }

        template <typename T>
        conversion_result<T> try_as_integer() const
        {
            using result_type = conversion_result<T>;
            
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    T val;
                    auto result = jsoncons::to_integer<T>(as_string_view().data(), as_string_view().length(), val);
                    if (!result)
                    {
                        return result_type(jsoncons::unexpect, conv_errc::not_integer);
                    }
                    return val;
                }
                case json_storage_kind::half_float:
                    return result_type(static_cast<T>(cast<half_storage>().value()));
                case json_storage_kind::float64:
                    return result_type(static_cast<T>(cast<double_storage>().value()));
                case json_storage_kind::int64:
                    return result_type(static_cast<T>(cast<int64_storage>().value()));
                case json_storage_kind::uint64:
                    return result_type(static_cast<T>(cast<uint64_storage>().value()));
                case json_storage_kind::boolean:
                    return result_type(static_cast<T>(cast<bool_storage>().value() ? 1 : 0));
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template try_as_integer<T>();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template try_as_integer<T>();
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_integer);
            }
        }

        template <typename T>
        T as_integer() const
        {
            auto result = try_as_integer<T>();
            if (!result)
            {
                JSONCONS_THROW(conv_error(result.error().code()));
            }
            return *result;
        }

        template <typename T>
        typename std::enable_if<ext_traits::is_signed_integer<T>::value && sizeof(T) <= sizeof(int64_t),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return (as_integer<int64_t>() >= (ext_traits::integer_limits<T>::lowest)()) && (as_integer<int64_t>() <= (ext_traits::integer_limits<T>::max)());
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((ext_traits::integer_limits<T>::max)());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template is_integer<T>();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template is_integer<T>();
                default:
                    return false;
            }
        }

        template <typename T>
        typename std::enable_if<ext_traits::is_signed_integer<T>::value && sizeof(int64_t) < sizeof(T),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    T val;
                    auto result = jsoncons::to_integer<T>(as_string_view().data(), as_string_view().length(), val);
                    return result ? true : false;
                }
                case json_storage_kind::int64:
                    return (as_integer<int64_t>() >= (ext_traits::integer_limits<T>::lowest)()) && (as_integer<int64_t>() <= (ext_traits::integer_limits<T>::max)());
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= static_cast<uint64_t>((ext_traits::integer_limits<T>::max)());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template is_integer<T>();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template is_integer<T>();
                default:
                    return false;
            }
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value && sizeof(IntegerType) <= sizeof(int64_t),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0 && static_cast<uint64_t>(as_integer<int64_t>()) <= (ext_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= (ext_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template is_integer<IntegerType>();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        template <typename IntegerType>
        typename std::enable_if<ext_traits::is_unsigned_integer<IntegerType>::value && sizeof(int64_t) < sizeof(IntegerType),bool>::type
        is_integer() const noexcept
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    IntegerType val;
                    auto result = jsoncons::to_integer<IntegerType>(as_string_view().data(), as_string_view().length(), val);
                    return result ? true : false;
                }
                case json_storage_kind::int64:
                    return as_integer<int64_t>() >= 0 && static_cast<uint64_t>(as_integer<int64_t>()) <= (ext_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::uint64:
                    return as_integer<uint64_t>() <= (ext_traits::integer_limits<IntegerType>::max)();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template is_integer<IntegerType>();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template is_integer<IntegerType>();
                default:
                    return false;
            }
        }

        conversion_result<double> try_as_double() const
        {
            using result_type = conversion_result<double>;

            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                {
                    double x{0};
                    const char_type* s = as_cstring();
                    std::size_t len = as_string_view().length();
                    if (JSONCONS_UNLIKELY(len > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')))
                    {
                        auto result = jsoncons::hexstr_to_double(s, len, x);
                        if (result.ec == std::errc::invalid_argument)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_double);
                        }
                    }
                    else if (JSONCONS_UNLIKELY(len > 3 && s[0] == '-' && s[1] == '0' && (s[2] == 'x' || s[2] == 'X')))
                    {
                        auto result = jsoncons::hexstr_to_double(s, len, x);
                        if (result.ec == std::errc::invalid_argument)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_double);
                        }
                    }
                    else
                    {
                        auto result = jsoncons::decstr_to_double(as_cstring(), as_string_view().length(), x);
                        if (result.ec == std::errc::invalid_argument)
                        {
                            return result_type(jsoncons::unexpect, conv_errc::not_double);
                        }
                    }
                    
                    return result_type(x);
                }
                case json_storage_kind::half_float:
                    return result_type(binary::decode_half(cast<half_storage>().value()));
                case json_storage_kind::float64:
                    return result_type(cast<double_storage>().value());
                case json_storage_kind::int64:
                    return result_type(static_cast<double>(cast<int64_storage>().value()));
                case json_storage_kind::uint64:
                    return result_type(static_cast<double>(cast<uint64_storage>().value()));
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().try_as_double();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().try_as_double();
                default:
                    return result_type(jsoncons::unexpect, conv_errc::not_double);
            }
        }

        double as_double() const
        {
            auto result = try_as_double();
            if (!result)
            {
                JSONCONS_THROW(conv_error(result.error().code()));
            }
            return *result;
        }

        template <typename T,typename Alloc, typename TempAlloc>
        typename std::enable_if<ext_traits::is_string<T>::value &&
                               std::is_same<char_type,typename T::value_type>::value,conversion_result<T>>::type
        try_as_string(const allocator_set<Alloc,TempAlloc>& aset) const
        {
            using value_type = T;
            using result_type = conversion_result<value_type>;

            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                {
                    auto& stor = cast<short_string_storage>();
                    return result_type(jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), stor.data(), stor.length()));
                }
                case json_storage_kind::long_str:
                {
                    auto& stor = cast<long_string_storage>();
                    return result_type(jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator(), stor.data(), stor.length()));
                }
                case json_storage_kind::byte_str:
                {
                    auto& stor = cast<byte_string_storage>();
                    value_type s = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator());
                    bytes_to_string(stor.data(), stor.data()+stor.length(), tag(), s);
                    return result_type(std::move(s));
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template try_as_string<T>(aset);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template try_as_string<T>(aset);
                default:
                {
                    value_type s = jsoncons::make_obj_using_allocator<value_type>(aset.get_allocator());
                    basic_compact_json_encoder<char_type,jsoncons::string_sink<value_type>,TempAlloc> encoder(s, aset.get_temp_allocator());
                    std::error_code ec;
                    dump(encoder, ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return result_type(jsoncons::unexpect, ec);
                    }
                    return result_type(std::move(s));
                }
            }
        }

        template <typename CharsAlloc>
        std::basic_string<char_type,char_traits_type,CharsAlloc> as_string(const CharsAlloc& alloc) const 
        {
            using value_type = std::basic_string<char_type,char_traits_type,CharsAlloc>;
            auto r = try_as_string<value_type>(make_alloc_set(alloc));
            if (!r)
            {
                JSONCONS_THROW(conv_error(r.error().code()));
            }
            return *r;
        }

        template <typename CharsAlloc=std::allocator<char_type>>
        std::basic_string<char_type,char_traits_type,CharsAlloc> as_string() const 
        {
            return as_string(CharsAlloc());
        }

        std::basic_string<char_type,char_traits_type> as_string() const 
        {
            return as_string(std::allocator<char_type>()); 
        }

        const char_type* as_cstring() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                    return cast<short_string_storage>().c_str();
                case json_storage_kind::long_str:
                    return cast<long_string_storage>().c_str();
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().as_cstring();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().as_cstring();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not a cstring"));
            }
        }

        basic_json& at(const string_view_type& key)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    JSONCONS_THROW(key_not_found(key.data(),key.length()));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        JSONCONS_THROW(key_not_found(key.data(),key.length()));
                    }
                    return (*it).value();
                }
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().at(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        const basic_json& at(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    JSONCONS_THROW(key_not_found(key.data(),key.length()));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it == cast<object_storage>().value().end())
                    {
                        JSONCONS_THROW(key_not_found(key.data(),key.length()));
                    }
                    return (*it).value();
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().at(key);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().at(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        basic_json& at(std::size_t i)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    if (i >= cast<array_storage>().value().size())
                    {
                        JSONCONS_THROW(json_runtime_error<std::out_of_range>("Invalid array subscript"));
                    }
                    return cast<array_storage>().value().operator[](i);
                case json_storage_kind::object:
                    return cast<object_storage>().value().at(i);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().at(i);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Index on non-array value not supported"));
            }
        }

        const basic_json& at(std::size_t i) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    if (i >= cast<array_storage>().value().size())
                    {
                        JSONCONS_THROW(json_runtime_error<std::out_of_range>("Invalid array subscript"));
                    }
                    return cast<array_storage>().value().operator[](i);
                case json_storage_kind::object:
                    return cast<object_storage>().value().at(i);
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().at(i);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().at(i);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Index on non-array value not supported"));
            }
        }

        object_iterator find(const string_view_type& key)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().find(key));
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().find(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        const_object_iterator find(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return const_object_iterator(cast<object_storage>().value().find(key));
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().find(key);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().find(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        const basic_json& at_or_null(const string_view_type& key) const
        {
            switch (storage_kind())
            {
                case json_storage_kind::null:
                case json_storage_kind::empty_object:
                {
                    return null();
                }
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it != cast<object_storage>().value().end())
                    {
                        return (*it).value();
                    }
                    else
                    {
                        return null();
                    }
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().at_or_null(key);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().at_or_null(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        template <typename T,typename U>
        T get_value_or(const string_view_type& key, U&& default_value) const
        {
            static_assert(std::is_copy_constructible<T>::value,
                          "get_value_or: T must be copy constructible");
            static_assert(std::is_convertible<U&&, T>::value,
                          "get_value_or: U must be convertible to T");
            switch (storage_kind())
            {
                case json_storage_kind::null:
                case json_storage_kind::empty_object:
                    return static_cast<T>(std::forward<U>(default_value));
                case json_storage_kind::object:
                {
                    auto it = cast<object_storage>().value().find(key);
                    if (it != cast<object_storage>().value().end())
                    {
                        return (*it).value().template as<T>();
                    }
                    else
                    {
                        return static_cast<T>(std::forward<U>(default_value));
                    }
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().template get_value_or<T,U>(key,std::forward<U>(default_value));
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().template get_value_or<T,U>(key,std::forward<U>(default_value));
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        // Modifiers

        void shrink_to_fit()
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().shrink_to_fit();
                    break;
                case json_storage_kind::object:
                    cast<object_storage>().value().shrink_to_fit();
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().shrink_to_fit();
                    break;
                default:
                    break;
            }
        }

        void clear()
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().clear();
                    break;
                case json_storage_kind::object:
                    cast<object_storage>().value().clear();
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().clear();
                    break;
                default:
                    break;
            }
        }

        object_iterator erase(const_object_iterator pos)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().erase(pos));
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().erase(pos);
            default:
                JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        object_iterator erase(const_object_iterator first, const_object_iterator last)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range().end();
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().erase(first, last));
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().erase(first, last);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        array_iterator erase(const_array_iterator pos)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().erase(pos);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().erase(pos);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

        array_iterator erase(const_array_iterator first, const_array_iterator last)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().erase(first, last);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().erase(first, last);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

        // Removes all elements from an array value whose index is between from_index, inclusive, and to_index, exclusive.

        void erase(const string_view_type& key)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    cast<object_storage>().value().erase(key);
                    break;
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().erase(key);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        template <typename T>
        std::pair<object_iterator,bool> insert_or_assign(const string_view_type& key, T&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                {
                    create_object_implicitly();
                    auto result = cast<object_storage>().value().insert_or_assign(key, std::forward<T>(val));
                    return std::make_pair(object_iterator(result.first), result.second);
                }
                case json_storage_kind::object:
                {
                    auto result = cast<object_storage>().value().insert_or_assign(key, std::forward<T>(val));
                    return std::make_pair(object_iterator(result.first), result.second);
                }
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().insert_or_assign(key, std::forward<T>(val));
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        template <typename ... Args>
        std::pair<object_iterator,bool> try_emplace(const string_view_type& key, Args&&... args)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                {
                    create_object_implicitly();
                    auto result = cast<object_storage>().value().try_emplace(key, std::forward<Args>(args)...);
                    return std::make_pair(object_iterator(result.first),result.second);
                }
                case json_storage_kind::object:
                {
                    auto result = cast<object_storage>().value().try_emplace(key, std::forward<Args>(args)...);
                    return std::make_pair(object_iterator(result.first),result.second);
                }
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().try_emplace(key, std::forward<Args>(args)...);
                default:
                    JSONCONS_THROW(not_an_object(key.data(),key.length()));
            }
        }

        // merge

        void merge(const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge(source);
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge(source.cast<json_reference_storage>().value());
                    break;
               default:
                   JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
           }
        }

        void merge(basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge(std::move(source));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge(std::move(source.cast<json_reference_storage>().value()));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
           }
        }

        void merge(object_iterator hint, const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge(hint, source);
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                     break;
                case json_storage_kind::json_ref:
                    merge(hint, source.cast<json_reference_storage>().value());
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge(object_iterator hint, basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge(hint, std::move(source));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge(hint, std::move(source.cast<json_reference_storage>().value()));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        // merge_or_update

        void merge_or_update(const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge_or_update(source);
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge_or_update(source.cast<json_reference_storage>().value());
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge_or_update(std::move(source));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge_or_update(std::move(source.cast<json_reference_storage>().value()));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(object_iterator hint, const basic_json& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(hint, source.cast<object_storage>().value());
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge_or_update(hint, source);
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge_or_update(hint, source.cast<json_reference_storage>().value());
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        void merge_or_update(object_iterator hint, basic_json&& source)
        {
            switch (source.storage_kind())
            {
                case json_storage_kind::empty_object:
                    break;
                case json_storage_kind::object:
                    switch (storage_kind())
                    {
                        case json_storage_kind::empty_object:
                            create_object_implicitly();
                            cast<object_storage>().value().merge_or_update(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::object:
                            cast<object_storage>().value().merge_or_update(hint, std::move(source.cast<object_storage>().value()));
                            break;
                        case json_storage_kind::json_ref:
                            cast<json_reference_storage>().value().merge_or_update(hint, std::move(source));
                            break;
                        default:
                            JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge or update a value that is not an object"));
                    }
                    break;
                case json_storage_kind::json_ref:
                    merge_or_update(hint, std::move(source.cast<json_reference_storage>().value()));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to merge a value that is not an object"));
            }
        }

        template <typename T>
        object_iterator insert_or_assign(object_iterator hint, const string_view_type& name, T&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    create_object_implicitly();
                    return object_iterator(cast<object_storage>().value().insert_or_assign(hint, name, std::forward<T>(val)));
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().insert_or_assign(hint, name, std::forward<T>(val)));
                case json_storage_kind::json_ref:
                    return object_iterator(cast<json_reference_storage>().value().insert_or_assign(hint, name, std::forward<T>(val)));
                default:
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
            }
        }

        template <typename ... Args>
        object_iterator try_emplace(object_iterator hint, const string_view_type& name, Args&&... args)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    create_object_implicitly();
                    return object_iterator(cast<object_storage>().value().try_emplace(hint, name, std::forward<Args>(args)...));
                case json_storage_kind::object:
                    return object_iterator(cast<object_storage>().value().try_emplace(hint, name, std::forward<Args>(args)...));
                case json_storage_kind::json_ref:
                    return object_iterator(cast<json_reference_storage>().value().try_emplace(hint, name, std::forward<Args>(args)...));
                default:
                    JSONCONS_THROW(not_an_object(name.data(),name.length()));
            }
        }

        template <typename T>
        array_iterator insert(const_array_iterator pos, T&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().insert(pos, std::forward<T>(val));
                    break;
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().insert(pos, std::forward<T>(val));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename InputIt>
        array_iterator insert(const_array_iterator pos, InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().insert(pos, first, last);
                    break;
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().insert(pos, first, last);
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    create_object_implicitly();
                    cast<object_storage>().value().insert(first, last);
                    break;
                case json_storage_kind::object:
                    cast<object_storage>().value().insert(first, last);
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().insert(first, last);
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an object"));
            }
        }

        template <typename InputIt>
        void insert(sorted_unique_range_tag tag, InputIt first, InputIt last)
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    create_object_implicitly();
                    cast<object_storage>().value().insert(tag, first, last);
                    break;
                case json_storage_kind::object:
                    cast<object_storage>().value().insert(tag, first, last);
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().insert(tag, first, last);
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an object"));
            }
        }

        template <typename... Args> 
        array_iterator emplace(const_array_iterator pos, Args&&... args)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().emplace(pos, std::forward<Args>(args)...);
                    break;
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().emplace(pos, std::forward<Args>(args)...);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        template <typename... Args> 
        basic_json& emplace_back(Args&&... args)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return cast<array_storage>().value().emplace_back(std::forward<Args>(args)...);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().emplace_back(std::forward<Args>(args)...);
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        friend void swap(basic_json& a, basic_json& b) noexcept
        {
            a.swap(b);
        }

        template <typename T>
        void push_back(T&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().push_back(std::forward<T>(val));
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().push_back(std::forward<T>(val));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        void push_back(basic_json&& val)
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    cast<array_storage>().value().push_back(std::move(val));
                    break;
                case json_storage_kind::json_ref:
                    cast<json_reference_storage>().value().push_back(std::move(val));
                    break;
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Attempting to insert into a value that is not an array"));
            }
        }

        std::basic_string<char_type> to_string() const noexcept
        {
            using string_type2 = std::basic_string<char_type>;
            string_type2 s;
            basic_compact_json_encoder<char_type, jsoncons::string_sink<string_type2>> encoder(s);
            dump(encoder);
            return s;
        }

        object_range_type object_range()
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return object_range_type(object_iterator(), object_iterator());
                case json_storage_kind::object:
                    return object_range_type(object_iterator(cast<object_storage>().value().begin()),
                                                  object_iterator(cast<object_storage>().value().end()));
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().object_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        const_object_range_type object_range() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::empty_object:
                    return const_object_range_type(const_object_iterator(), const_object_iterator());
                case json_storage_kind::object:
                    return const_object_range_type(const_object_iterator(cast<object_storage>().value().begin()),
                                                        const_object_iterator(cast<object_storage>().value().end()));
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().object_range();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().object_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an object"));
            }
        }

        array_range_type array_range()
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return array_range_type(cast<array_storage>().value().begin(),
                        cast<array_storage>().value().end());
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().array_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

        const_array_range_type array_range() const
        {
            switch (storage_kind())
            {
                case json_storage_kind::array:
                    return const_array_range_type(cast<array_storage>().value().begin(),
                        cast<array_storage>().value().end());
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().array_range();
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().array_range();
                default:
                    JSONCONS_THROW(json_runtime_error<std::domain_error>("Not an array"));
            }
        }

    private:

        void dump_noflush(basic_json_visitor<char_type>& visitor, std::error_code& ec) const
        {
            const ser_context context{};
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    visitor.string_value(as_string_view(), tag(), context, ec);
                    break;
                case json_storage_kind::byte_str:
                    if (tag() == semantic_tag::ext)
                    {
                        visitor.byte_string_value(as_byte_string_view(), ext_tag(), context, ec);
                    }
                    else
                    {
                        visitor.byte_string_value(as_byte_string_view(), tag(), context, ec);
                    }
                    break;
                case json_storage_kind::half_float:
                    visitor.half_value(cast<half_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::float64:
                    visitor.double_value(cast<double_storage>().value(), 
                                         tag(), context, ec);
                    break;
                case json_storage_kind::int64:
                    visitor.int64_value(cast<int64_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::uint64:
                    visitor.uint64_value(cast<uint64_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::boolean:
                    visitor.bool_value(cast<bool_storage>().value(), tag(), context, ec);
                    break;
                case json_storage_kind::null:
                    visitor.null_value(tag(), context, ec);
                    break;
                case json_storage_kind::empty_object:
                    visitor.begin_object(0, tag(), context, ec);
                    visitor.end_object(context, ec);
                    break;
                case json_storage_kind::object:
                {
                    visitor.begin_object(size(), tag(), context, ec);
                    const object& o = cast<object_storage>().value();
                    for (auto it = o.begin(); it != o.end(); ++it)
                    {
                        visitor.key(string_view_type(((*it).key()).data(),(*it).key().length()), context, ec);
                        (*it).value().dump_noflush(visitor, ec);
                    }
                    visitor.end_object(context, ec);
                    break;
                }
                case json_storage_kind::array:
                {
                    visitor.begin_array(size(), tag(), context, ec);
                    const array& o = cast<array_storage>().value();
                    for (const_array_iterator it = o.begin(); it != o.end(); ++it)
                    {
                        (*it).dump_noflush(visitor, ec);
                    }
                    visitor.end_array(context, ec);
                    break;
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().dump_noflush(visitor, ec);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().dump_noflush(visitor, ec);
                default:
                    break;
            }
        }

        write_result try_dump_noflush(basic_json_visitor<char_type>& visitor) const
        {
            std::error_code ec;
            const ser_context context{};
            switch (storage_kind())
            {
                case json_storage_kind::short_str:
                case json_storage_kind::long_str:
                    visitor.string_value(as_string_view(), tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::byte_str:
                    if (tag() == semantic_tag::ext)
                    {
                        visitor.byte_string_value(as_byte_string_view(), ext_tag(), context, ec);
                    }
                    else
                    {
                        visitor.byte_string_value(as_byte_string_view(), tag(), context, ec);
                    }
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::half_float:
                    visitor.half_value(cast<half_storage>().value(), tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::float64:
                    visitor.double_value(cast<double_storage>().value(), 
                                         tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::int64:
                    visitor.int64_value(cast<int64_storage>().value(), tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::uint64:
                    visitor.uint64_value(cast<uint64_storage>().value(), tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::boolean:
                    visitor.bool_value(cast<bool_storage>().value(), tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::null:
                    visitor.null_value(tag(), context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::empty_object:
                    visitor.begin_object(0, tag(), context, ec);
                    visitor.end_object(context, ec);
                    return ec ? write_result{unexpect, ec} : write_result{};
                case json_storage_kind::object:
                {
                    visitor.begin_object(size(), tag(), context, ec);
                    const object& o = cast<object_storage>().value();
                    for (auto it = o.begin(); it != o.end(); ++it)
                    {
                        visitor.key(string_view_type(((*it).key()).data(),(*it).key().length()), context, ec);
                        (*it).value().dump_noflush(visitor, ec);
                        if (JSONCONS_UNLIKELY(ec))
                        {
                            return write_result{unexpect, ec};
                        }
                    }
                    visitor.end_object(context, ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return write_result{unexpect, ec};
                    }
                    return write_result{};
                }
                case json_storage_kind::array:
                {
                    visitor.begin_array(size(), tag(), context, ec);
                    const array& o = cast<array_storage>().value();
                    for (const_array_iterator it = o.begin(); it != o.end(); ++it)
                    {
                        auto r = (*it).try_dump_noflush(visitor);
                        if (JSONCONS_UNLIKELY(!r))
                        {
                            return r;
                        }
                    }
                    visitor.end_array(context, ec);
                    if (JSONCONS_UNLIKELY(ec))
                    {
                        return write_result{unexpect, ec};
                    }
                    return write_result{};
                }
                case json_storage_kind::json_const_ref:
                    return cast<json_const_reference_storage>().value().try_dump_noflush(visitor);
                case json_storage_kind::json_ref:
                    return cast<json_reference_storage>().value().try_dump_noflush(visitor);
                default:
                    JSONCONS_UNREACHABLE();
                    break;
            }
        }

        friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const basic_json& o)
        {
            o.dump(os);
            return os;
        }

        friend std::basic_istream<char_type>& operator>>(std::basic_istream<char_type>& is, basic_json& o)
        {
            json_decoder<basic_json> visitor;
            basic_json_reader<char_type,stream_source<char_type>> reader(is, visitor);
            reader.read_next();
            reader.check_done();
            if (!visitor.is_valid())
            {
                JSONCONS_THROW(json_runtime_error<std::runtime_error>("Failed to parse json stream"));
            }
            o = visitor.get_result();
            return is;
        }

        friend basic_json deep_copy(const basic_json& other)
        {
            switch (other.storage_kind())
            {
                case json_storage_kind::array:
                {
                    basic_json j(json_array_arg, other.tag(), other.get_allocator());
                    j.reserve(other.size());

                    for (const auto& item : other.array_range())
                    {
                        j.push_back(deep_copy(item));
                    }
                    return j;
                }
                case json_storage_kind::object:
                {
                    basic_json j(json_object_arg, other.tag(), other.get_allocator());
                    j.reserve(other.size());

                    for (const auto& item : other.object_range())
                    {
                        j.try_emplace(item.key(), deep_copy(item.value()));
                    }
                    return j;
                }
                case json_storage_kind::json_const_ref:
                    return deep_copy(other.cast<json_const_reference_storage>().value());
                case json_storage_kind::json_ref:
                    return deep_copy(other.cast<json_reference_storage>().value());
                default:
                    return other;
            }
        }
    };

    // operator==

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator==(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator==(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) == 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator==(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) == 0;
    }

    // operator!=

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator!=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator!=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) != 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator!=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) != 0;
    }

    // operator<

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator<(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) < 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) > 0;
    }

    // operator<=

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator<=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) <= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator<=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) >= 0;
    }

    // operator>

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator>(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) > 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) < 0;
    }

    // operator>=

    template <typename Json>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value,bool>::type
    operator>=(const Json& lhs, const Json& rhs) noexcept
    {
        return lhs.compare(rhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>=(const Json& lhs, const T& rhs) 
    {
        return lhs.compare(rhs) >= 0;
    }

    template <typename Json,typename T>
    typename std::enable_if<ext_traits::is_basic_json<Json>::value && std::is_convertible<T,Json>::value,bool>::type
    operator>=(const T& lhs, const Json& rhs) 
    {
        return rhs.compare(lhs) <= 0;
    }

    // swap

    template <typename Json>
    void swap(typename Json::key_value_type& a,typename Json::key_value_type& b) noexcept
    {
        a.swap(b);
    }

    using json = basic_json<char,sorted_policy,std::allocator<char>>;
    using wjson = basic_json<wchar_t,sorted_policy,std::allocator<char>>;
    using ojson = basic_json<char, order_preserving_policy, std::allocator<char>>;
    using wojson = basic_json<wchar_t, order_preserving_policy, std::allocator<char>>;

    inline namespace literals {

    inline 
    jsoncons::json operator ""_json(const char* s, std::size_t n)
    {
        return jsoncons::json::parse(jsoncons::json::string_view_type(s, n));
    }

    inline 
    jsoncons::wjson operator ""_json(const wchar_t* s, std::size_t n)
    {
        return jsoncons::wjson::parse(jsoncons::wjson::string_view_type(s, n));
    }

    inline
    jsoncons::ojson operator ""_ojson(const char* s, std::size_t n)
    {
        return jsoncons::ojson::parse(jsoncons::ojson::string_view_type(s, n));
    }

    inline
    jsoncons::wojson operator ""_ojson(const wchar_t* s, std::size_t n)
    {
        return jsoncons::wojson::parse(jsoncons::wojson::string_view_type(s, n));
    }

    } // inline namespace literals

    #if defined(JSONCONS_HAS_POLYMORPHIC_ALLOCATOR)
    namespace pmr {
        template< typename CharT,typename Policy>
        using basic_json = jsoncons::basic_json<CharT, Policy, std::pmr::polymorphic_allocator<char>>;
        using json = basic_json<char,sorted_policy>;
        using wjson = basic_json<wchar_t,sorted_policy>;
        using ojson = basic_json<char, order_preserving_policy>;
        using wojson = basic_json<wchar_t, order_preserving_policy>;
    } // namespace pmr
    #endif


} // namespace jsoncons

#endif // JSONCONS_BASIC_JSON_HPP
