// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_TYPE_HPP
#define JSONCONS_JSON_TYPE_HPP

#include <cstdint>
#include <ostream>

#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons {

    enum class json_type : uint8_t 
    {
        null,
        boolean,
        int64,
        uint64,
        float16,
        float64,
        string,
        byte_string,
        array,
        object
    #if !defined(JSONCONS_NO_DEPRECATED)
        , null_value = null,
        bool_value = boolean,
        int64_value = int64,
        uint64_value = uint64,
        half_value = float16,
        double_value = float64,
        string_value = string,
        byte_string_value = byte_string,
        array_value = array,
        object_value = object
    #endif
    };

    template <typename CharT>
    std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, json_type type)
    {
        static constexpr const CharT* null_value = JSONCONS_CSTRING_CONSTANT(CharT, "null");
        static constexpr const CharT* bool_value = JSONCONS_CSTRING_CONSTANT(CharT, "boolean");
        static constexpr const CharT* int64_value = JSONCONS_CSTRING_CONSTANT(CharT, "int64");
        static constexpr const CharT* uint64_value = JSONCONS_CSTRING_CONSTANT(CharT, "uint64");
        static constexpr const CharT* half_value = JSONCONS_CSTRING_CONSTANT(CharT, "float16");
        static constexpr const CharT* double_value = JSONCONS_CSTRING_CONSTANT(CharT, "float64");
        static constexpr const CharT* string_value = JSONCONS_CSTRING_CONSTANT(CharT, "string");
        static constexpr const CharT* byte_string_value = JSONCONS_CSTRING_CONSTANT(CharT, "byte_string");
        static constexpr const CharT* array_value = JSONCONS_CSTRING_CONSTANT(CharT, "array");
        static constexpr const CharT* object_value = JSONCONS_CSTRING_CONSTANT(CharT, "object");

        switch (type)
        {
            case json_type::null:
            {
                os << null_value;
                break;
            }
            case json_type::boolean:
            {
                os << bool_value;
                break;
            }
            case json_type::int64:
            {
                os << int64_value;
                break;
            }
            case json_type::uint64:
            {
                os << uint64_value;
                break;
            }
            case json_type::float16:
            {
                os << half_value;
                break;
            }
            case json_type::float64:
            {
                os << double_value;
                break;
            }
            case json_type::string:
            {
                os << string_value;
                break;
            }
            case json_type::byte_string:
            {
                os << byte_string_value;
                break;
            }
            case json_type::array:
            {
                os << array_value;
                break;
            }
            case json_type::object:
            {
                os << object_value;
                break;
            }
        }
        return os;
    }
    
    struct null_type
    {
        explicit null_type() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR null_type null_arg{};
    
    struct temp_alloc_arg_t
    {
        explicit temp_alloc_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR temp_alloc_arg_t temp_alloc_arg{};

    using temp_allocator_arg_t = temp_alloc_arg_t;

    JSONCONS_INLINE_CONSTEXPR temp_allocator_arg_t temp_allocator_arg{};
    
    struct half_arg_t
    {
        explicit half_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR half_arg_t half_arg{};
    
    struct json_array_arg_t
    {
        explicit json_array_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR json_array_arg_t json_array_arg{};
    
    struct json_object_arg_t
    {
        explicit json_object_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR json_object_arg_t json_object_arg{};
    
    struct byte_string_arg_t
    {
        explicit byte_string_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR byte_string_arg_t byte_string_arg{};
    
    struct json_const_pointer_arg_t
    {
        explicit json_const_pointer_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR json_const_pointer_arg_t json_const_pointer_arg{};
    
    struct json_pointer_arg_t
    {
        explicit json_pointer_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR json_pointer_arg_t json_pointer_arg{};

    struct raw_json_arg_t
    {
        explicit raw_json_arg_t() = default; 
    };
    
    JSONCONS_INLINE_CONSTEXPR raw_json_arg_t raw_json_arg{};
    
    struct noesc_arg_t
    {
        explicit noesc_arg_t() = default; 
    };

    enum class json_storage_kind : uint8_t 
    {
        null = 0,                 // 0000
        boolean = 1,              // 0001
        int64 = 2,                // 0010
        uint64 = 3,               // 0011
        empty_object = 4,         // 0100
        float64 = 5,              // 0101
        half_float = 6,           // 0110
        short_str = 7,            // 0111
        json_const_ref = 8, // 1000    
        json_ref = 9,       // 1001    
        byte_str = 12,            // 1100  
        object = 13,              // 1101
        array = 14,               // 1110
        long_str = 15             // 1111
    };

    inline bool is_string_storage(json_storage_kind storage_kind) noexcept
    {
        static const uint8_t mask{ uint8_t(json_storage_kind::short_str) & uint8_t(json_storage_kind::long_str) };
        return (uint8_t(storage_kind) & mask) == mask;
    }

    inline bool is_trivial_storage(json_storage_kind storage_kind) noexcept
    {
        static const uint8_t mask{ uint8_t(json_storage_kind::long_str) & uint8_t(json_storage_kind::byte_str) 
            & uint8_t(json_storage_kind::array) & uint8_t(json_storage_kind::object) };
        return (uint8_t(storage_kind) & mask) != mask;
    }

    template <typename CharT>
    std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, json_storage_kind storage)
    {
        static constexpr const CharT* null_value = JSONCONS_CSTRING_CONSTANT(CharT, "null");
        static constexpr const CharT* bool_value = JSONCONS_CSTRING_CONSTANT(CharT, "bool");
        static constexpr const CharT* int64_value = JSONCONS_CSTRING_CONSTANT(CharT, "int64");
        static constexpr const CharT* uint64_value = JSONCONS_CSTRING_CONSTANT(CharT, "uint64");
        static constexpr const CharT* half_value = JSONCONS_CSTRING_CONSTANT(CharT, "half");
        static constexpr const CharT* double_value = JSONCONS_CSTRING_CONSTANT(CharT, "double");
        static constexpr const CharT* short_string_value = JSONCONS_CSTRING_CONSTANT(CharT, "short_string");
        static constexpr const CharT* long_string_value = JSONCONS_CSTRING_CONSTANT(CharT, "string");
        static constexpr const CharT* byte_string_value = JSONCONS_CSTRING_CONSTANT(CharT, "byte_string");
        static constexpr const CharT* array_value = JSONCONS_CSTRING_CONSTANT(CharT, "array");
        static constexpr const CharT* empty_object_value = JSONCONS_CSTRING_CONSTANT(CharT, "empty_object");
        static constexpr const CharT* object_value = JSONCONS_CSTRING_CONSTANT(CharT, "object");
        static constexpr const CharT* json_const_ref = JSONCONS_CSTRING_CONSTANT(CharT, "json_const_ref");
        static constexpr const CharT* json_ref = JSONCONS_CSTRING_CONSTANT(CharT, "json_ref");

        switch (storage)
        {
            case json_storage_kind::null:
            {
                os << null_value;
                break;
            }
            case json_storage_kind::boolean:
            {
                os << bool_value;
                break;
            }
            case json_storage_kind::int64:
            {
                os << int64_value;
                break;
            }
            case json_storage_kind::uint64:
            {
                os << uint64_value;
                break;
            }
            case json_storage_kind::half_float:
            {
                os << half_value;
                break;
            }
            case json_storage_kind::float64:
            {
                os << double_value;
                break;
            }
            case json_storage_kind::short_str:
            {
                os << short_string_value;
                break;
            }
            case json_storage_kind::long_str:
            {
                os << long_string_value;
                break;
            }
            case json_storage_kind::byte_str:
            {
                os << byte_string_value;
                break;
            }
            case json_storage_kind::array:
            {
                os << array_value;
                break;
            }
            case json_storage_kind::empty_object:
            {
                os << empty_object_value;
                break;
            }
            case json_storage_kind::object:
            {
                os << object_value;
                break;
            }
            case json_storage_kind::json_const_ref:
            {
                os << json_const_ref;
                break;
            }
            case json_storage_kind::json_ref:
            {
                os << json_ref;
                break;
            }
        }
        return os;
    }

} // namespace jsoncons

#endif // JSONCONS_JSON_TYPE_HPP
