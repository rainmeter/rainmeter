// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_TOKEN_EVALUATOR_HPP
#define JSONCONS_EXT_JSONPATH_TOKEN_EVALUATOR_HPP

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string> // std::basic_string
#include <system_error>
#include <type_traits>
#include <unordered_map> // std::unordered_map
#include <utility> // std::move
#include <vector> // std::vector

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/read_number.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/path_node.hpp>

#if defined(JSONCONS_HAS_STD_REGEX)
#include <regex>
#endif

namespace jsoncons { 
namespace jsonpath {

    template <typename Json>
    struct jsonpath_traits
    {
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using value_type = typename std::remove_const<Json>::type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
    };

    struct reference_arg_t
    {
        explicit reference_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR reference_arg_t reference_arg{};

    struct const_reference_arg_t
    {
        explicit const_reference_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR const_reference_arg_t const_reference_arg{};

    struct literal_arg_t
    {
        explicit literal_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR literal_arg_t literal_arg{};

    struct end_of_expression_arg_t
    {
        explicit end_of_expression_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_of_expression_arg_t end_of_expression_arg{};

    struct separator_arg_t
    {
        explicit separator_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR separator_arg_t separator_arg{};

    struct lparen_arg_t
    {
        explicit lparen_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR lparen_arg_t lparen_arg{};

    struct rparen_arg_t
    {
        explicit rparen_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR rparen_arg_t rparen_arg{};

    struct begin_union_arg_t
    {
        explicit begin_union_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR begin_union_arg_t begin_union_arg{};

    struct end_union_arg_t
    {
        explicit end_union_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_union_arg_t end_union_arg{};

    struct begin_filter_arg_t
    {
        explicit begin_filter_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR begin_filter_arg_t begin_filter_arg{};

    struct end_filter_arg_t
    {
        explicit end_filter_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_filter_arg_t end_filter_arg{};

    struct begin_expression_arg_t
    {
        explicit begin_expression_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR begin_expression_arg_t begin_expression_arg{};

    struct end_index_expression_arg_t
    {
        explicit end_index_expression_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_index_expression_arg_t end_index_expression_arg{};

    struct end_argument_expression_arg_t
    {
        explicit end_argument_expression_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_argument_expression_arg_t end_argument_expression_arg{};

    struct current_node_arg_t
    {
        explicit current_node_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR current_node_arg_t current_node_arg{};

    struct root_node_arg_t
    {
        explicit root_node_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR root_node_arg_t root_node_arg{};

    struct end_function_arg_t
    {
        explicit end_function_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR end_function_arg_t end_function_arg{};

    struct argument_arg_t
    {
        explicit argument_arg_t() = default;
    };
    JSONCONS_INLINE_CONSTEXPR argument_arg_t argument_arg{};

    enum class result_options : unsigned {
#if !defined(JSONCONS_NO_DEPRECATED)
        value=0, 
#endif
        nodups=1, sort=2, sort_descending=4, path=8};


JSONCONS_ATTRIBUTE_NODISCARD
constexpr result_options
operator|(result_options lhs, result_options rhs) noexcept
{ return (result_options)((unsigned)lhs | (unsigned)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr result_options
operator&(result_options lhs, result_options rhs) noexcept
{ return (result_options)((unsigned)lhs & (unsigned)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr result_options
operator^(result_options lhs, result_options rhs) noexcept
{ return (result_options)((unsigned)lhs ^ (unsigned)rhs); }

JSONCONS_ATTRIBUTE_NODISCARD
constexpr result_options
operator~(result_options flags) noexcept
{ return (result_options)~(unsigned)flags; }

constexpr result_options&
operator|=(result_options& lhs, result_options rhs) noexcept
{ return lhs = lhs | rhs; }

constexpr result_options&
operator&=(result_options& lhs, result_options rhs) noexcept
{ return lhs = lhs & rhs; }

constexpr result_options&
operator^=(result_options& lhs, result_options rhs) noexcept
{ return lhs = lhs ^ rhs; }

/*
    inline constexpr result_options operator~(result_options a)
    {
        return static_cast<result_options>(~static_cast<unsigned int>(a));
    }

    inline constexpr result_options operator&(result_options a, result_options b)
    {
        return static_cast<result_options>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
    }

    inline constexpr result_options operator^(result_options a, result_options b)
    {
        return static_cast<result_options>(static_cast<unsigned int>(a) ^ static_cast<unsigned int>(b));
    }

    inline constexpr result_options operator|(result_options a, result_options b)
    {
        return static_cast<result_options>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline constexpr result_options operator&=(result_options& a, result_options b)
    {
        return (a = a & b);
    }

    inline constexpr result_options operator^=(result_options& a, result_options b)
    {
        return (a = a ^ b);
    }

    inline constexpr result_options operator|=(result_options& a, result_options b)
    {
        return (a = a | b);
    }
*/
    template <typename Json>
    class parameter;

    template <typename Json,typename JsonReference>
    class value_or_pointer
    {
    public:
        friend class parameter<Json>;
        using value_type = Json;
        using reference = JsonReference;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<reference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
    private:
        bool is_value_;
        union
        {
            value_type val_;
            pointer ptr_;
        };
    public:
        value_or_pointer(value_type&& val)
            : is_value_(true), val_(std::move(val))
        {
        }

        value_or_pointer(pointer ptr)
            : is_value_(false), ptr_(std::move(ptr))
        {
        }

        value_or_pointer(const value_or_pointer& other) = delete;

        value_or_pointer(value_or_pointer&& other) noexcept
            : is_value_(other.is_value_)
        {
            if (is_value_)
            {
                new(&val_)value_type(std::move(other.val_));
            }
            else
            {
                ptr_ = other.ptr_;
            }
        }

        ~value_or_pointer() noexcept
        {
            if (is_value_)
            {
                val_.~value_type();
            }
        }

        value_or_pointer& operator=(const value_or_pointer& other) noexcept = delete;

        value_or_pointer& operator=(value_or_pointer&& other) noexcept
        {
            if (is_value_)
            {
                val_.~value_type();
            }
            is_value_ = other.is_value_;

            if (is_value_)
            {
                new(&val_)value_type(std::move(other.val_));
            }
            else
            {
                ptr_ = other.ptr_;
            }
            return *this;
        }

        reference value() 
        {
            return is_value_ ? val_ : *ptr_;
        }

        pointer ptr() 
        {
            return is_value_ ? &val_ : ptr_;
        }
    };

    template <typename Json>
    class parameter
    {
        using value_type = Json;
        using reference = const Json&;
        using pointer = const Json*;
    private:
        value_or_pointer<Json,reference> data_{nullptr};
    public:
        template <typename JsonReference>
        parameter(value_or_pointer<Json,JsonReference>&& data) noexcept
        {
            data_.is_value_ = data.is_value_;
            if (data.is_value_)
            {
                data_.val_ = std::move(data.val_);
            }
            else
            {
                data_.ptr_ = data.ptr_;
            }
        }

        parameter(const parameter& other) = default;

        parameter(parameter&& other) = default;
        
        ~parameter() = default; 

        parameter& operator=(const parameter& other) = default;

        parameter& operator=(parameter&& other) = default;

        const Json& value() const
        {
            return data_.is_value_ ? data_.val_ : *data_.ptr_;
        }
    };

    template <typename Json>
    class custom_function
    {
    public:
        using value_type = Json;
        using char_type = typename Json::char_type;
        using parameter_type = parameter<Json>;
        using function_type = std::function<value_type(jsoncons::span<const parameter_type>, std::error_code& ec)>;
        using string_type = typename Json::string_type;

        string_type function_name_;
        optional<std::size_t> arity_;
        function_type f_;

        custom_function(const custom_function&) = default;
        custom_function(custom_function&&) = default;

        custom_function(const string_type& function_name,
                        const optional<std::size_t>& arity,
                        const function_type& f)
            : function_name_(function_name),
              arity_(arity),
              f_(f)
        {
        }

        custom_function(string_type&& function_name,
                        optional<std::size_t>&& arity,
                        function_type&& f)
            : function_name_(std::move(function_name)),
              arity_(arity),
              f_(std::move(f))
        {
        }
        
        ~custom_function() = default; 

        custom_function& operator=(const custom_function&) = default;
        custom_function& operator=(custom_function&&) = default;

        const string_type& name() const 
        {
            return function_name_;
        }

        optional<std::size_t> arity() const 
        {
            return arity_;
        }

        const function_type& function() const 
        {
            return f_;
        }
    };

    template <typename Json>
    class custom_functions
    {
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using function_type = std::function<value_type(jsoncons::span<const parameter_type>, std::error_code& ec)>;
        using const_iterator = typename std::vector<custom_function<Json>>::const_iterator;

        std::vector<custom_function<Json>> functions_;
    public:
        void register_function(const string_type& name,
                               jsoncons::optional<std::size_t> arity,
                               const function_type& f)
        {
            functions_.emplace_back(name, arity, f);
        }

        const_iterator begin() const
        {
            return functions_.begin();
        }

        const_iterator end() const
        {
            return functions_.end();
        }
    };

namespace detail {

    template <typename Json>
    struct unary_operator
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        
        std::size_t precedence_level_;
        bool is_right_associative_;

        unary_operator(std::size_t precedence_level,
                       bool is_right_associative)
            : precedence_level_(precedence_level),
              is_right_associative_(is_right_associative)
        {
        }
        
        unary_operator(const unary_operator& other) = default;
        unary_operator(unary_operator&& other) = default;

        virtual ~unary_operator() = default;

        unary_operator& operator=(const unary_operator& other) = default;
        unary_operator& operator=(unary_operator&& other) = default;

        std::size_t precedence_level() const 
        {
            return precedence_level_;
        }
        bool is_right_associative() const
        {
            return is_right_associative_;
        }

        virtual Json evaluate(const_reference, std::error_code&) const = 0;
    };

    template <typename Json>
    bool is_false(const Json& val)
    {
        return ((val.is_array() && val.empty()) ||
                 (val.is_object() && val.empty()) ||
                 (val.is_string() && val.as_string_view().empty()) ||
                 (val.is_bool() && !val.as_bool()) ||
                 val.is_null());
    }

    template <typename Json>
    bool is_true(const Json& val)
    {
        return !is_false(val);
    }

    template <typename Json>
    class unary_not_operator final : public unary_operator<Json>
    {
    public:
        using const_reference = typename jsonpath_traits<Json>::const_reference;

        unary_not_operator()
            : unary_operator<Json>(1, true)
        {}

        Json evaluate(const_reference val, std::error_code&) const override
        {
            return is_false(val) ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
        }
    };

    template <typename Json>
    class unary_minus_operator final : public unary_operator<Json>
    {
    public:
        using const_reference = typename jsonpath_traits<Json>::const_reference;

        unary_minus_operator()
            : unary_operator<Json>(1, true)
        {}

        Json evaluate(const_reference val, 
                      std::error_code&) const override
        {
            if (val.is_int64())
            {
                return Json(-val.template as<int64_t>(), semantic_tag::none);
            }
            if (val.is_double())
            {
                return Json(-val.as_double(), semantic_tag::none);
            }
            return Json::null();
        }
    };

    template <typename Json>
    class regex_operator final : public unary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        std::basic_regex<char_type> pattern_;
    public:
        regex_operator(std::basic_regex<char_type>&& pattern)
            : unary_operator<Json>(2, true),
              pattern_(std::move(pattern))
        {
        }

        Json evaluate(const_reference val, std::error_code&) const override
        {
            if (!val.is_string())
            {
                return Json::null();
            }
            return std::regex_search(val.as_string(), pattern_) ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
        }
    };

    template <typename Json>
    struct binary_operator
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        std::size_t precedence_level_;
        bool is_right_associative_;

        binary_operator(std::size_t precedence_level,
                        bool is_right_associative = false)
            : precedence_level_(precedence_level),
              is_right_associative_(is_right_associative)
        {
        }
        
        virtual ~binary_operator() = default;

        std::size_t precedence_level() const 
        {
            return precedence_level_;
        }
        bool is_right_associative() const
        {
            return is_right_associative_;
        }

        virtual Json evaluate(const_reference lhs, const_reference rhs, 
            std::error_code&) const = 0;

        virtual std::string to_string(int) const
        {
            return "binary operator";
        }
    };

    // Implementations

    template <typename Json>
    class or_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        or_operator()
            : binary_operator<Json>(9)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (lhs.is_null() && rhs.is_null())
            {
                return Json::null();
            }
            if (!is_false(lhs))
            {
                return lhs;
            }
            return rhs;
        }
        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                //s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("or operator");
            return s;
        }
    };

    template <typename Json>
    class and_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        and_operator()
            : binary_operator<Json>(8)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (is_true(lhs))
            {
                return rhs;
            }
            return lhs;
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("and operator");
            return s;
        }
    };

    template <typename Json>
    class eq_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        eq_operator()
            : binary_operator<Json>(6)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override 
        {
            return lhs == rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("equal operator");
            return s;
        }
    };

    template <typename Json>
    class ne_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        ne_operator()
            : binary_operator<Json>(6)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override 
        {
            return lhs != rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("not equal operator");
            return s;
        }
    };

    template <typename Json>
    class lt_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        lt_operator()
            : binary_operator<Json>(5)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override 
        {
            if (lhs.is_number() && rhs.is_number())
            {
                return lhs < rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            if (lhs.is_string() && rhs.is_string())
            {
                return lhs < rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            return Json::null();
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("less than operator");
            return s;
        }
    };

    template <typename Json>
    class lte_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        lte_operator()
            : binary_operator<Json>(5)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override 
        {
            if (lhs.is_number() && rhs.is_number())
            {
                return lhs <= rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            if (lhs.is_string() && rhs.is_string())
            {
                return lhs <= rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            return Json::null();
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("less than or equal operator");
            return s;
        }
    };

    template <typename Json>
    class gt_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        gt_operator()
            : binary_operator<Json>(5)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            //std::cout << "operator> lhs: " << lhs << ", rhs: " << rhs << "\n";

            if (lhs.is_number() && rhs.is_number())
            {
                return lhs > rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            if (lhs.is_string() && rhs.is_string())
            {
                return lhs > rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            return Json::null();
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("greater than operator");
            return s;
        }
    };

    template <typename Json>
    class gte_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        gte_operator()
            : binary_operator<Json>(5)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (lhs.is_number() && rhs.is_number())
            {
                return lhs >= rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            if (lhs.is_string() && rhs.is_string())
            {
                return lhs >= rhs ? Json(true, semantic_tag::none) : Json(false, semantic_tag::none);
            }
            return Json::null();
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("greater than or equal operator");
            return s;
        }
    };

    template <typename Json>
    class plus_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        plus_operator()
            : binary_operator<Json>(4)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (!(lhs.is_number() && rhs.is_number()))
            {
                return Json::null();
            }
            if (lhs.is_int64() && rhs.is_int64())
            {
                return Json(((lhs.template as<int64_t>() + rhs.template as<int64_t>())), semantic_tag::none);
            }
            if (lhs.is_uint64() && rhs.is_uint64())
            {
                return Json((lhs.template as<uint64_t>() + rhs.template as<uint64_t>()), semantic_tag::none);
            }
            return Json((lhs.as_double() + rhs.as_double()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("plus operator");
            return s;
        }
    };

    template <typename Json>
    class minus_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        minus_operator()
            : binary_operator<Json>(4)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (!(lhs.is_number() && rhs.is_number()))
            {
                return Json::null();
            }
            if (lhs.is_int64() && rhs.is_int64())
            {
                return Json(((lhs.template as<int64_t>() - rhs.template as<int64_t>())), semantic_tag::none);
            }
            if (lhs.is_uint64() && rhs.is_uint64())
            {
                return Json((lhs.template as<uint64_t>() - rhs.template as<uint64_t>()), semantic_tag::none);
            }
            return Json((lhs.as_double() - rhs.as_double()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("minus operator");
            return s;
        }
    };

    template <typename Json>
    class mult_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        mult_operator()
            : binary_operator<Json>(3)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            if (!(lhs.is_number() && rhs.is_number()))
            {
                return Json::null();
            }
            if (lhs.is_int64() && rhs.is_int64())
            {
                return Json(((lhs.template as<int64_t>() * rhs.template as<int64_t>())), semantic_tag::none);
            }
            if (lhs.is_uint64() && rhs.is_uint64())
            {
                return Json((lhs.template as<uint64_t>() * rhs.template as<uint64_t>()), semantic_tag::none);
            }
            return Json((lhs.as_double() * rhs.as_double()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("multiply operator");
            return s;
        }
    };

    template <typename Json>
    class div_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        div_operator()
            : binary_operator<Json>(3)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            //std::cout << "operator/ lhs: " << lhs << ", rhs: " << rhs << "\n";

            if (!(lhs.is_number() && rhs.is_number()))
            {
                return Json::null();
            }
            if (lhs.is_int64() && rhs.is_int64())
            {
                return Json(((lhs.template as<int64_t>() / rhs.template as<int64_t>())), semantic_tag::none);
            }
            if (lhs.is_uint64() && rhs.is_uint64())
            {
                return Json((lhs.template as<uint64_t>() / rhs.template as<uint64_t>()), semantic_tag::none);
            }
            return Json((lhs.as_double() / rhs.as_double()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("divide operator");
            return s;
        }
    };

    template <typename Json>
    class modulus_operator final : public binary_operator<Json>
    {
        using const_reference = typename jsonpath_traits<Json>::const_reference;

    public:
        modulus_operator()
            : binary_operator<Json>(3)
        {
        }

        Json evaluate(const_reference lhs, const_reference rhs, std::error_code&) const override
        {
            //std::cout << "operator/ lhs: " << lhs << ", rhs: " << rhs << "\n";

            if (!(lhs.is_number() && rhs.is_number()))
            {
                return Json::null();
            }
            if (lhs.is_int64() && rhs.is_int64())
            {
                return Json(((lhs.template as<int64_t>() % rhs.template as<int64_t>())), semantic_tag::none);
            }
            if (lhs.is_uint64() && rhs.is_uint64())
            {
                return Json((lhs.template as<uint64_t>() % rhs.template as<uint64_t>()), semantic_tag::none);
            }
            return Json(fmod(lhs.as_double(), rhs.as_double()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("modulus operator");
            return s;
        }
    };

    // function_base
    template <typename Json>
    class function_base
    {
        jsoncons::optional<std::size_t> arg_count_;
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        function_base(jsoncons::optional<std::size_t> arg_count)
            : arg_count_(arg_count)
        {
        }

        function_base(const function_base&) = default;
        function_base(function_base&&) = default;
        
        virtual ~function_base() = default;

        function_base& operator=(const function_base&) = default;
        function_base& operator=(function_base&&) = default;

        jsoncons::optional<std::size_t> arity() const
        {
            return arg_count_;
        }

        virtual value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const = 0;

        virtual std::string to_string(int level) const
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("function");
            return s;
        }
    };  

    template <typename Json>
    class decorator_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using string_view_type = typename Json::string_view_type;
        using function_type = std::function<value_type(jsoncons::span<const parameter_type>, std::error_code& ec)>;
    private:
        function_type f_;
    public:
        decorator_function(jsoncons::optional<std::size_t> arity,
            const function_type& f)
            : function_base<Json>(arity), f_(f)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args,
            std::error_code& ec) const override
        {
            return f_(args, ec);
        }
    };

    template <typename Json>
    class contains_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using string_view_type = typename Json::string_view_type;

        contains_function()
            : function_base<Json>(2)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            auto arg1= args[1].value();

            switch (arg0.type())
            {
                case json_type::array:
                    for (auto& j : arg0.array_range())
                    {
                        if (j == arg1)
                        {
                            return value_type(true, semantic_tag::none);
                        }
                    }
                    return value_type(false, semantic_tag::none);
                case json_type::string:
                {
                    if (!arg1.is_string())
                    {
                        ec = jsonpath_errc::invalid_type;
                        return value_type::null();
                    }
                    auto sv0 = arg0.template as<string_view_type>();
                    auto sv1 = arg1.template as<string_view_type>();
                    return sv0.find(sv1) != string_view_type::npos ? value_type(true, semantic_tag::none) : value_type(false, semantic_tag::none);
                }
                default:
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("contains function");
            return s;
        }
    };

    template <typename Json>
    class ends_with_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using string_view_type = typename Json::string_view_type;

        ends_with_function()
            : function_base<Json>(2)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_string())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            auto arg1= args[1].value();
            if (!arg1.is_string())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            auto sv0 = arg0.template as<string_view_type>();
            auto sv1 = arg1.template as<string_view_type>();

            if (sv1.length() <= sv0.length() && sv1 == sv0.substr(sv0.length() - sv1.length()))
            {
                return value_type(true, semantic_tag::none);
            }
            return value_type(false, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("ends_with function");
            return s;
        }
    };

    template <typename Json>
    class starts_with_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using string_view_type = typename Json::string_view_type;

        starts_with_function()
            : function_base<Json>(2)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_string())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            auto arg1= args[1].value();
            if (!arg1.is_string())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            auto sv0 = arg0.template as<string_view_type>();
            auto sv1 = arg1.template as<string_view_type>();

            if (sv1.length() <= sv0.length() && sv1 == sv0.substr(0, sv1.length()))
            {
                return value_type(true, semantic_tag::none);
            }
            return value_type(false, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("starts_with function");
            return s;
        }
    };

    template <typename Json>
    class sum_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        sum_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_array())
            {
                //std::cout << "arg: " << arg0 << "\n";
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            //std::cout << "sum function arg: " << arg0 << "\n";

            double sum = 0;
            for (auto& j : arg0.array_range())
            {
                if (!j.is_number())
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
                sum += j.template as<double>();
            }

            return value_type(sum, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("sum function");
            return s;
        }
    };

#if defined(JSONCONS_HAS_STD_REGEX)

    template <typename Json>
    class tokenize_function : public function_base<Json>
    {
        using allocator_type = typename Json::allocator_type;

        allocator_type alloc_;

    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;

        tokenize_function(const allocator_type& alloc)
            : function_base<Json>(2), alloc_(alloc)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            if (!args[0].value().is_string() || !args[1].value().is_string())
            {
                //std::cout << "arg: " << arg0 << "\n";
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            auto arg0 = args[0].value().template as<string_view_type>();
            auto arg1 = args[1].value().template as<string_view_type>();

            auto s0 = string_type(arg0.begin(), arg0.end(), alloc_);
            auto s1 = string_type(arg1.begin(), arg1.end(), alloc_);

            std::regex::flag_type options = std::regex_constants::ECMAScript; 
            std::basic_regex<char_type> pieces_regex(s1, options);

            std::regex_token_iterator<typename string_type::const_iterator> rit ( s0.begin(), s0.end(), pieces_regex, -1);
            std::regex_token_iterator<typename string_type::const_iterator> rend;

            value_type j(json_array_arg, semantic_tag::none, alloc_);
            while (rit != rend) 
            {
                auto s = (*rit).str();
                j.emplace_back(s.c_str(), semantic_tag::none);
                ++rit;
            }
            return j;
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("tokenize function");
            return s;
        }
    };

#endif // defined(JSONCONS_HAS_STD_REGEX)

    template <typename Json>
    class ceil_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        ceil_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            switch (arg0.type())
            {
                case json_type::uint64:
                case json_type::int64:
                {
                    return value_type(arg0.template as<double>(), semantic_tag::none);
                }
                case json_type::float64:
                {
                    return value_type(std::ceil(arg0.template as<double>()), semantic_tag::none);
                }
                default:
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("ceil function");
            return s;
        }
    };

    template <typename Json>
    class floor_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        floor_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            switch (arg0.type())
            {
                case json_type::uint64:
                case json_type::int64:
                {
                    return value_type(arg0.template as<double>(), semantic_tag::none);
                }
                case json_type::float64:
                {
                    return value_type(std::floor(arg0.template as<double>()), semantic_tag::none);
                }
                default:
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("floor function");
            return s;
        }
    };

    template <typename Json>
    class to_number_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        to_number_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            switch (arg0.type())
            {
                case json_type::int64:
                case json_type::uint64:
                case json_type::float64:
                    return arg0;
                case json_type::string:
                {
                    auto sv = arg0.as_string_view();
                    uint64_t un{0};
                    auto result1 = jsoncons::to_integer(sv.data(), sv.length(), un);
                    if (result1)
                    {
                        return value_type(un, semantic_tag::none);
                    }
                    int64_t sn{0};
                    auto result2 = jsoncons::to_integer(sv.data(), sv.length(), sn);
                    if (result2)
                    {
                        return value_type(sn, semantic_tag::none);
                    }
                    auto s = arg0.as_string();
                    double d;
                    auto result = jsoncons::decstr_to_double(s.c_str(), s.length(), d);
                    if (result.ec == std::errc::invalid_argument)
                    {
                        return value_type::null();
                    }
                    return value_type(d, semantic_tag::none);
                }
                default:
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("to_number function");
            return s;
        }
    };

    template <typename Json>
    class prod_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        prod_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_array() || arg0.empty())
            {
                //std::cout << "arg: " << arg0 << "\n";
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            double prod = 1;
            for (auto& j : arg0.array_range())
            {
                if (!j.is_number())
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
                prod *= j.template as<double>();
            }

            return value_type(prod, semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("prod function");
            return s;
        }
    };

    template <typename Json>
    class avg_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        avg_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_array())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            if (arg0.empty())
            {
                return value_type::null();
            }
            double sum = 0;
            for (auto& j : arg0.array_range())
            {
                if (!j.is_number())
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
                sum += j.template as<double>();
            }

            return value_type(sum / static_cast<double>(arg0.size()), semantic_tag::none);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("to_string function");
            return s;
        }
    };

    template <typename Json>
    class min_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        min_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_array())
            {
                //std::cout << "arg: " << arg0 << "\n";
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            if (arg0.empty())
            {
                return value_type::null();
            }
            bool is_number = arg0.at(0).is_number();
            bool is_string = arg0.at(0).is_string();
            if (!is_number && !is_string)
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            std::size_t index = 0;
            for (std::size_t i = 1; i < arg0.size(); ++i)
            {
                if (!(arg0.at(i).is_number() == is_number && arg0.at(i).is_string() == is_string))
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
                if (arg0.at(i) < arg0.at(index))
                {
                    index = i;
                }
            }

            return arg0.at(index);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("min function");
            return s;
        }
    };

    template <typename Json>
    class max_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        max_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            if (!arg0.is_array())
            {
                //std::cout << "arg: " << arg0 << "\n";
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }
            if (arg0.empty())
            {
                return value_type::null();
            }

            bool is_number = arg0.at(0).is_number();
            bool is_string = arg0.at(0).is_string();
            if (!is_number && !is_string)
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            std::size_t index = 0;
            for (std::size_t i = 1; i < arg0.size(); ++i)
            {
                if (!(arg0.at(i).is_number() == is_number && arg0.at(i).is_string() == is_string))
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
                if (arg0.at(i) > arg0.at(index))
                {
                    index = i;
                }
            }

            return arg0.at(index);
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("max function");
            return s;
        }
    };

    template <typename Json>
    class abs_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;

        abs_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
                 std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0= args[0].value();
            switch (arg0.type())
            {
                case json_type::uint64:
                    return arg0;
                case json_type::int64:
                {
                    return arg0.template as<int64_t>() >= 0 ? arg0 : value_type(std::abs(arg0.template as<int64_t>()), semantic_tag::none);
                }
                case json_type::float64:
                {
                    return arg0.template as<double>() >= 0 ? arg0 : value_type(std::abs(arg0.template as<double>()), semantic_tag::none);
                }
                default:
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("abs function");
            return s;
        }
    };

    template <typename Json>
    class length_function : public function_base<Json>
    {
    public:
        using value_type = Json;
        using string_view_type = typename Json::string_view_type;
        using parameter_type = parameter<Json>;

        length_function()
            : function_base<Json>(1)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0 = args[0].value();
            //std::cout << "length function arg: " << arg0 << "\n";

            switch (arg0.type())
            {
                case json_type::object:
                case json_type::array:
                    //std::cout << "LENGTH ARG: " << arg0 << "\n";
                    return value_type(arg0.size(), semantic_tag::none);
                case json_type::string:
                {
                    auto sv0 = arg0.template as<string_view_type>();
                    auto length = unicode_traits::count_codepoints(sv0.data(), sv0.size());
                    return value_type(length, semantic_tag::none);
                }
                default:
                {
                    ec = jsonpath_errc::invalid_type;
                    return value_type::null();
                }
            }
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("length function");
            return s;
        }
    };

    template <typename Json>
    class keys_function : public function_base<Json>
    {
        using allocator_type = typename Json::allocator_type;

        allocator_type alloc_;
    public:
        using value_type = Json;
        using parameter_type = parameter<Json>;
        using string_view_type = typename Json::string_view_type;

        keys_function(const allocator_type& alloc)
            : function_base<Json>(1), alloc_(alloc)
        {
        }

        value_type evaluate(const std::vector<parameter_type>& args, 
            std::error_code& ec) const override
        {
            if (args.size() != *this->arity())
            {
                ec = jsonpath_errc::invalid_arity;
                return value_type::null();
            }

            auto arg0 = args[0].value();
            if (!arg0.is_object())
            {
                ec = jsonpath_errc::invalid_type;
                return value_type::null();
            }

            value_type result(json_array_arg, semantic_tag::none, alloc_);
            result.reserve(args.size());

            for (auto& item : arg0.object_range())
            {
                auto s = item.key();
                result.emplace_back(s.c_str(), semantic_tag::none);
            }
            return result;
        }

        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("keys function");
            return s;
        }
    };

    enum class jsonpath_token_kind
    {
        root_node,
        current_node,
        expression,
        lparen,
        rparen,
        begin_union,
        end_union,
        begin_filter,
        end_filter,
        begin_expression,
        end_index_expression,
        end_argument_expression,
        separator,
        literal,
        selector,
        function,
        end_function,
        argument,
        unary_operator,
        binary_operator
    };

    inline std::string to_string(jsonpath_token_kind kind)
    {
        switch (kind)
        {
            case jsonpath_token_kind::root_node:
                return "root_node";
            case jsonpath_token_kind::current_node:
                return "current_node";
            case jsonpath_token_kind::lparen:
                return "lparen";
            case jsonpath_token_kind::rparen:
                return "rparen";
            case jsonpath_token_kind::begin_union:
                return "begin_union";
            case jsonpath_token_kind::end_union:
                return "end_union";
            case jsonpath_token_kind::begin_filter:
                return "begin_filter";
            case jsonpath_token_kind::end_filter:
                return "end_filter";
            case jsonpath_token_kind::begin_expression:
                return "begin_expression";
            case jsonpath_token_kind::end_index_expression:
                return "end_index_expression";
            case jsonpath_token_kind::end_argument_expression:
                return "end_argument_expression";
            case jsonpath_token_kind::separator:
                return "separator";
            case jsonpath_token_kind::literal:
                return "literal";
            case jsonpath_token_kind::selector:
                return "selector";
            case jsonpath_token_kind::function:
                return "function";
            case jsonpath_token_kind::end_function:
                return "end_function";
            case jsonpath_token_kind::argument:
                return "argument";
            case jsonpath_token_kind::unary_operator:
                return "unary_operator";
            case jsonpath_token_kind::binary_operator:
                return "binary_operator";
            default:
                return "";
        }
    }

    template <typename Json,typename JsonReference>
    struct path_value_pair
    {
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using value_type = Json;
        using reference = JsonReference;
        using value_pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_pointer = const path_node_type*;

        path_pointer path_ptr_;
        value_pointer value_ptr_;

        path_value_pair(const path_node_type& path, reference value) noexcept
            : path_ptr_(std::addressof(path)), value_ptr_(std::addressof(value))
        {
        }

        path_value_pair(const path_value_pair&) = default;
        path_value_pair(path_value_pair&& other) = default;
        path_value_pair& operator=(const path_value_pair&) = default;
        path_value_pair& operator=(path_value_pair&& other) = default;
        
        ~path_value_pair() = default;

        path_node_type path() const
        {
            return *path_ptr_;
        }

        reference value() 
        {
            return *value_ptr_;
        }
    };
 
    template <typename Json,typename JsonReference>
    struct path_value_pair_less
    {
        bool operator()(const path_value_pair<Json,JsonReference>& lhs,
                        const path_value_pair<Json,JsonReference>& rhs) const noexcept
        {
            return lhs.path() < rhs.path();
        }
    };

    template <typename Json,typename JsonReference>
    struct path_value_pair_greater
    {
        bool operator()(const path_value_pair<Json,JsonReference>& lhs,
                        const path_value_pair<Json,JsonReference>& rhs) const noexcept
        {
            return rhs.path() < lhs.path();
        }
    };

    template <typename Json,typename JsonReference>
    struct path_value_pair_equal
    {
        bool operator()(const path_value_pair<Json,JsonReference>& lhs,
                        const path_value_pair<Json,JsonReference>& rhs) const noexcept
        {
            return lhs.path() == rhs.path();
        }
    };

    template <typename Json,typename JsonReference>
    struct path_component_value_pair
    {
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using value_type = Json;
        using reference = JsonReference;
        using value_pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_pointer = const path_node_type*;
    private:
        const path_node_type* last_ptr_;
        value_pointer value_ptr_;
    public:
        path_component_value_pair(const path_node_type& last, reference value) noexcept
            : last_ptr_(std::addressof(last)), value_ptr_(std::addressof(value))
        {
        }

        const path_node_type& last() const
        {
            return *last_ptr_;
        }

        reference value() const
        {
            return *value_ptr_;
        }
    };

    template <typename Json,typename JsonReference>
    class node_receiver
    {
    public:
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using reference = JsonReference;
        using path_node_type = basic_path_node<typename Json::char_type>;

        node_receiver() = default;
        node_receiver(const node_receiver&) = default;
        node_receiver(node_receiver&&) = default;

        virtual ~node_receiver() = default;

        node_receiver& operator=(const node_receiver&) = default;
        node_receiver& operator=(node_receiver&&) = default;

        virtual void add(const path_node_type& base_path, reference value) = 0;
    };

    template <typename Json,typename JsonReference>
    class path_value_receiver : public node_receiver<Json,JsonReference>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using reference = JsonReference;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_value_pair_type = path_value_pair<Json,JsonReference>;

        allocator_type alloc_;
        std::vector<path_value_pair_type> nodes;

        path_value_receiver(const allocator_type& alloc)
            : alloc_(alloc)
        {
        }

        void add(const path_node_type& base_path, reference value) override
        {
            nodes.emplace_back(base_path, value);
        }
    };

    template <typename Json,typename JsonReference>
    class path_component_value_receiver : public node_receiver<Json,JsonReference>
    {
    public:
        using reference = JsonReference;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_component_value_pair_type = path_component_value_pair<Json,JsonReference>;

        std::vector<path_component_value_pair_type> nodes;

        void add(const path_node_type& base_path, reference value) override
        {
            nodes.emplace_back(base_path, value);
        }
    };

    template <typename Json,typename JsonReference>
    class eval_context
    {
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using reference = JsonReference;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<reference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using path_node_type = basic_path_node<typename Json::char_type>;

        allocator_type alloc_;
        std::vector<std::unique_ptr<Json>> temp_json_values_;
        std::vector<std::unique_ptr<path_node_type>> temp_node_values_;
        std::unordered_map<std::size_t,pointer> cache_;
        string_type length_label_;
    public:
        eval_context(const allocator_type& alloc = allocator_type())
            : alloc_(alloc), length_label_{JSONCONS_CSTRING_CONSTANT(char_type, "length"), alloc}
        {
        }

        allocator_type get_allocator() const
        {
            return alloc_;
        }

        bool is_cached(std::size_t id) const
        {
            return cache_.find(id) != cache_.end();
        }
        void add_to_cache(std::size_t id, reference val) 
        {
            cache_.emplace(id, std::addressof(val));
        }
        reference get_from_cache(std::size_t id) 
        {
            return *cache_[id];
        }

        reference null_value()
        {
            static Json a_null = Json(null_type(), semantic_tag::none);
            return a_null;
        }

        template <typename... Args>
        Json* create_json(Args&& ... args)
        {
            auto temp = jsoncons::make_unique<Json>(std::forward<Args>(args)...);
            Json* ptr = temp.get();
            temp_json_values_.push_back(std::move(temp));
            return ptr;
        }

        const string_type& length_label() const
        {
            return length_label_;
        }

        template <typename... Args>
        const path_node_type* create_path_node(Args&& ... args)
        {
            auto temp = jsoncons::make_unique<path_node_type>(std::forward<Args>(args)...);
            path_node_type* ptr = temp.get();
            temp_node_values_.push_back(std::move(temp));
            return ptr;
        }
    };

    template <typename Json,typename JsonReference>
    struct node_less
    {
        bool operator()(const path_value_pair<Json,JsonReference>& a, const path_value_pair<Json,JsonReference>& b) const
        {
            return *(a.ptr) < *(b.ptr);
        }
    };

    template <typename Json,typename JsonReference>
    class jsonpath_selector
    {
        bool is_path_;
        std::size_t precedence_level_;

    public:
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = jsoncons::basic_string_view<char_type, std::char_traits<char_type>>;
        using value_type = Json;
        using reference = JsonReference;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using path_value_pair_type = path_value_pair<Json,JsonReference>;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using node_receiver_type = node_receiver<Json,JsonReference>;
        using selector_type = jsonpath_selector<Json,JsonReference>;

        jsonpath_selector(bool is_path,
                          std::size_t precedence_level = 0)
            : is_path_(is_path), 
              precedence_level_(precedence_level)
        {
        }

        virtual ~jsonpath_selector() = default;

        bool is_path() const 
        {
            return is_path_;
        }

        std::size_t precedence_level() const
        {
            return precedence_level_;
        }

        bool is_right_associative() const
        {
            return true;
        }

        virtual void select(eval_context<Json,JsonReference>& context,
            reference root,
            const path_node_type& base_path, 
            reference val, 
            node_receiver_type& receiver,
            result_options options) const = 0;

        virtual reference evaluate(eval_context<Json,JsonReference>& context,
            reference root,
            const path_node_type& base_path, 
            reference current, 
            result_options options,
            std::error_code& ec) const = 0;

        virtual void append_selector(jsonpath_selector*) 
        {
        }

        virtual std::string to_string(int) const
        {
            return std::string();
        }
    };

    template <typename Json>
    struct static_resources
    {
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using value_type = Json;
        using reference = typename jsonpath_traits<Json>::reference;
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        using function_base_type = function_base<Json>;
        using selector_type = jsonpath_selector<Json,reference>;
        using const_selector_type = jsonpath_selector<Json,const_reference>;

        struct MyHash
        {
            std::uintmax_t operator()(string_type const& s) const noexcept
            {
                const int p = 31;
                const int m = static_cast<int>(1e9) + 9;
                std::uintmax_t hash_value = 0;
                std::uintmax_t p_pow = 1;
                for (char_type c : s) {
                    hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
                    p_pow = (p_pow * p) % m;
                }
                return hash_value;   
            }
        };

        allocator_type alloc_;
        std::vector<std::unique_ptr<selector_type>> selectors_;
        std::vector<std::unique_ptr<const_selector_type>> const_selectors_;
        std::vector<std::unique_ptr<Json>> temp_json_values_;
        std::vector<std::unique_ptr<unary_operator<Json>>> unary_operators_;

        std::unordered_map<string_type,std::unique_ptr<function_base_type>,MyHash> functions_;
        std::unordered_map<string_type,std::unique_ptr<function_base_type>,MyHash> custom_functions_;

        static_resources(const static_resources&) = delete;
        static_resources(static_resources&&) = default;

        static_resources(const allocator_type& alloc = allocator_type())
            : alloc_(alloc)
        {
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "abs"), alloc_}, 
                    jsoncons::make_unique<abs_function<Json>>());
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "contains"), alloc_ },
                    jsoncons::make_unique<contains_function<Json>>());
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "starts_with"), alloc_ },
                    jsoncons::make_unique<starts_with_function<Json>>());
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "ends_with"), alloc_ },
                    jsoncons::make_unique<ends_with_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "ceil"), alloc_}, 
                    jsoncons::make_unique<ceil_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "floor"), alloc_}, 
                    jsoncons::make_unique<floor_function<Json>>());
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "to_number"), alloc_ },
                    jsoncons::make_unique<to_number_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "sum"), alloc_}, 
                    jsoncons::make_unique<sum_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "prod"), alloc_}, 
                    jsoncons::make_unique<prod_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "avg"), alloc_}, 
                    jsoncons::make_unique<avg_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "min"), alloc_}, 
                    jsoncons::make_unique<min_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "max"), alloc_}, 
                    jsoncons::make_unique<max_function<Json>>());
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "length"), alloc_}, 
                    jsoncons::make_unique<length_function<Json>>());
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "keys"), alloc_ },
                    jsoncons::make_unique<keys_function<Json>>(alloc_));
#if defined(JSONCONS_HAS_STD_REGEX)
                functions_.emplace(string_type{ JSONCONS_CSTRING_CONSTANT(char_type, "tokenize"), alloc_ },
                    jsoncons::make_unique<tokenize_function<Json>>(alloc_));
#endif
                functions_.emplace(string_type{JSONCONS_CSTRING_CONSTANT(char_type, "count"), alloc_}, 
                    jsoncons::make_unique<length_function<Json>>());
        }

        static_resources(const custom_functions<Json>& functions, 
            const allocator_type& alloc = allocator_type())
            : static_resources(alloc)
        {
            for (const auto& item : functions)
            {
                custom_functions_.emplace(item.name(),
                                          jsoncons::make_unique<decorator_function<Json>>(item.arity(),item.function()));
            }
        }
        
        ~static_resources() = default;

        static_resources operator=(const static_resources&) = delete;
        static_resources operator=(static_resources&&) = delete;

        const function_base_type* get_function(const string_type& name, std::error_code& ec) const
        {
            auto it = functions_.find(name);
            if (it == functions_.end())
            {
                auto it2 = custom_functions_.find(name);
                if (it2 == custom_functions_.end())
                {
                    ec = jsonpath_errc::unknown_function;
                    return nullptr;
                }
                return it2->second.get();
            }
            return (*it).second.get();
        }

        const unary_operator<Json>* get_unary_not() const
        {
            static unary_not_operator<Json> oper;
            return &oper;
        }

        const unary_operator<Json>* get_unary_minus() const
        {
            static unary_minus_operator<Json> oper;
            return &oper;
        }

        const unary_operator<Json>* get_regex_operator(std::basic_regex<char_type>&& pattern) 
        {
            unary_operators_.push_back(jsoncons::make_unique<regex_operator<Json>>(std::move(pattern)));
            return unary_operators_.back().get();
        }

        const binary_operator<Json>* get_or_operator() const
        {
            static or_operator<Json> oper;

            return &oper;
        }

        const binary_operator<Json>* get_and_operator() const
        {
            static and_operator<Json> oper;

            return &oper;
        }

        const binary_operator<Json>* get_eq_operator() const
        {
            static eq_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_ne_operator() const
        {
            static ne_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_lt_operator() const
        {
            static lt_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_lte_operator() const
        {
            static lte_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_gt_operator() const
        {
            static gt_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_gte_operator() const
        {
            static gte_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_plus_operator() const
        {
            static plus_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_minus_operator() const
        {
            static minus_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_mult_operator() const
        {
            static mult_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_div_operator() const
        {
            static div_operator<Json> oper;
            return &oper;
        }

        const binary_operator<Json>* get_modulus_operator() const
        {
            static modulus_operator<Json> oper;
            return &oper;
        }

        template <typename T>
        typename std::enable_if<std::is_const<typename std::remove_reference<typename T::reference>::type>::value,const_selector_type*>::type    
        new_selector(T&& val)
        {
            const_selectors_.push_back(jsoncons::make_unique<T>(std::forward<T>(val)));
            return const_selectors_.back().get();
        }

        template <typename T>
        typename std::enable_if<!std::is_const<typename std::remove_reference<typename T::reference>::type>::value,selector_type*>::type    
        new_selector(T&& val)
        {
            selectors_.push_back(jsoncons::make_unique<T>(std::forward<T>(val)));
            return selectors_.back().get();
        }

        template <typename... Args>
        Json* create_json(Args&& ... args)
        {
            auto temp = jsoncons::make_unique<Json>(std::forward<Args>(args)...);
            Json* ptr = temp.get();
            temp_json_values_.emplace_back(std::move(temp));
            return ptr;
        }
    };

    template <typename Json,typename JsonReference>
    class expression_base
    {
    public:
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = jsoncons::basic_string_view<char_type, std::char_traits<char_type>>;
        using value_type = Json;
        using reference = JsonReference;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using path_value_pair_type = path_value_pair<Json,JsonReference>;
        using path_node_type = basic_path_node<typename Json::char_type>;

        expression_base() = default;
        
        expression_base(const expression_base&) = default;
        expression_base(expression_base&&) = default;

        virtual ~expression_base() = default;

        expression_base& operator=(const expression_base&) = default;
        expression_base& operator=(expression_base&&) = default;

        virtual value_type evaluate(eval_context<Json,JsonReference>& context,
            reference root,
            reference val, 
            result_options options,
            std::error_code& ec) const = 0;

        virtual std::string to_string(int level) const = 0;
    };

    template <typename Json,typename JsonReference>
    class token
    {
    public:
        using selector_type = jsonpath_selector<Json,JsonReference>;
        using expression_base_type = expression_base<Json,JsonReference>;

        jsonpath_token_kind token_kind_;

        union
        {
            char dummy_;
            selector_type* selector_;
            std::unique_ptr<expression_base_type> expression_;
            const unary_operator<Json>* unary_operator_;
            const binary_operator<Json>* binary_operator_;
            const function_base<Json>* function_;
            Json value_;
        };
    public:

        token(const token& other) = delete;

        token(token&& other) noexcept
        {
            construct(std::move(other));
        }

        token(const unary_operator<Json>* expr) noexcept
            : token_kind_(jsonpath_token_kind::unary_operator),
              unary_operator_(expr)
        {
        }

        token(const binary_operator<Json>* expr) noexcept
            : token_kind_(jsonpath_token_kind::binary_operator),
              binary_operator_(expr)
        {
        }

        token(current_node_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::current_node), dummy_{}
        {
        }

        token(root_node_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::root_node), dummy_{}
        {
        }

        token(end_function_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::end_function), dummy_{}
        {
        }

        token(separator_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::separator), dummy_{}
        {
        }

        token(lparen_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::lparen), dummy_{}
        {
        }

        token(rparen_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::rparen), dummy_{}
        {
        }

        token(begin_union_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::begin_union), dummy_{}
        {
        }

        token(end_union_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::end_union), dummy_{}
        {
        }

        token(begin_filter_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::begin_filter), dummy_{}
        {
        }

        token(end_filter_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::end_filter), dummy_{}
        {
        }

        token(begin_expression_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::begin_expression), dummy_{}
        {
        }

        token(end_index_expression_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::end_index_expression), dummy_{}
        {
        }

        token(end_argument_expression_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::end_argument_expression), dummy_{}
        {
        }

        token(selector_type* selector)
            : token_kind_(jsonpath_token_kind::selector), selector_(selector)
        {
        }

        token(std::unique_ptr<expression_base_type>&& expr)
            : token_kind_(jsonpath_token_kind::expression)
        {
            new (&expression_) std::unique_ptr<expression_base_type>(std::move(expr));
        }

        token(const function_base<Json>* function) noexcept
            : token_kind_(jsonpath_token_kind::function),
              function_(function)
        {
        }

        token(argument_arg_t) noexcept
            : token_kind_(jsonpath_token_kind::argument), dummy_{}
        {
        }

        token(literal_arg_t, Json&& value) noexcept
            : token_kind_(jsonpath_token_kind::literal), value_(std::move(value))
        {
        }

        const Json& get_value(const_reference_arg_t, eval_context<Json,JsonReference>&) const
        {
            return value_;
        }

        Json& get_value(reference_arg_t, eval_context<Json,JsonReference>& context) const
        {
            return *context.create_json(value_);
        }

#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        token& operator=(const token& other) = delete;

        token& operator=(token&& other) noexcept
        {

            if (&other != this)
            {
                if (token_kind_ == other.token_kind_)
                {
                    switch (token_kind_)
                    {
                        case jsonpath_token_kind::selector:
                            selector_ = other.selector_;
                            break;
                        case jsonpath_token_kind::expression:
                            expression_ = std::move(other.expression_);
                            break;
                        case jsonpath_token_kind::unary_operator:
                            unary_operator_ = other.unary_operator_;
                            break;
                        case jsonpath_token_kind::binary_operator:
                            binary_operator_ = other.binary_operator_;
                            break;
                        case jsonpath_token_kind::function:
                            function_ = other.function_;
                            break;
                        case jsonpath_token_kind::literal:
                            value_ = std::move(other.value_);
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    destroy();
                    construct(std::move(other));
                }
            }
            return *this;
        }
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic pop
#endif

        ~token() noexcept
        {
            destroy();
        }

        jsonpath_token_kind token_kind() const
        {
            return token_kind_;
        }

        bool is_lparen() const
        {
            return token_kind_ == jsonpath_token_kind::lparen; 
        }

        bool is_rparen() const
        {
            return token_kind_ == jsonpath_token_kind::rparen; 
        }

        bool is_current_node() const
        {
            return token_kind_ == jsonpath_token_kind::current_node; 
        }

        bool is_path() const
        {
            if (token_kind_ == jsonpath_token_kind::selector)
            {
                JSONCONS_ASSERT(selector_ != nullptr);
                return selector_->is_path(); 
            }
            return false;
        }

        bool is_operator() const
        {
            return token_kind_ == jsonpath_token_kind::unary_operator || 
                   token_kind_ == jsonpath_token_kind::binary_operator; 
        }

        std::size_t precedence_level() const
        {
            switch(token_kind_)
            {
                case jsonpath_token_kind::selector:
                    JSONCONS_ASSERT(selector_ != nullptr);
                    return selector_->precedence_level();
                case jsonpath_token_kind::unary_operator:
                    JSONCONS_ASSERT(unary_operator_ != nullptr);
                    return unary_operator_->precedence_level();
                case jsonpath_token_kind::binary_operator:
                    JSONCONS_ASSERT(binary_operator_ != nullptr);
                    return binary_operator_->precedence_level();
                default:
                    return 0;
            }
        }

        jsoncons::optional<std::size_t> arity() const
        {
            if (token_kind_ == jsonpath_token_kind::function)
            {
                JSONCONS_ASSERT(function_ != nullptr);
                return function_->arity();
            }
            return jsoncons::optional<std::size_t>();
        }

        bool is_right_associative() const
        {
            switch(token_kind_)
            {
                case jsonpath_token_kind::selector:
                    JSONCONS_ASSERT(selector_ != nullptr);
                    return selector_->is_right_associative();
                case jsonpath_token_kind::unary_operator:
                    JSONCONS_ASSERT(unary_operator_ != nullptr);
                    return unary_operator_->is_right_associative();
                case jsonpath_token_kind::binary_operator:
                    JSONCONS_ASSERT(binary_operator_ != nullptr);
                    return binary_operator_->is_right_associative();
                default:
                    return false;
            }
        }

#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        void construct(token&& other) noexcept
        {
            token_kind_ = other.token_kind_;
            switch (token_kind_)
            {
                case jsonpath_token_kind::selector:
                    selector_ = other.selector_;
                    break;
                case jsonpath_token_kind::expression:
                    new (&expression_) std::unique_ptr<expression_base_type>(std::move(other.expression_));
                    break;
                case jsonpath_token_kind::unary_operator:
                    unary_operator_ = other.unary_operator_;
                    break;
                case jsonpath_token_kind::binary_operator:
                    binary_operator_ = other.binary_operator_;
                    break;
                case jsonpath_token_kind::function:
                    function_ = other.function_;
                    break;
                case jsonpath_token_kind::literal:
                    new (&value_) Json(std::move(other.value_));
                    break;
                default:
                    break;
            }
        }
#if defined(__GNUC__) && JSONCONS_GCC_AVAILABLE(12,0,0)
# pragma GCC diagnostic pop
#endif

        void destroy() noexcept 
        {
            switch(token_kind_)
            {
                case jsonpath_token_kind::expression:
                    expression_.~unique_ptr();
                    break;
                case jsonpath_token_kind::literal:
                    value_.~Json();
                    break;
                default:
                    break;
            }
        }

        std::string to_string(int level) const
        {
            std::string s;
            switch (token_kind_)
            {
                case jsonpath_token_kind::root_node:
                    if (level > 0)
                    {
                        s.append("\n");
                        s.append(std::size_t(level)*2, ' ');
                    }
                    s.append("root node");
                    break;
                case jsonpath_token_kind::current_node:
                    if (level > 0)
                    {
                        s.append("\n");
                        s.append(std::size_t(level)*2, ' ');
                    }
                    s.append("current node");
                    break;
                case jsonpath_token_kind::argument:
                    if (level > 0)
                    {
                        s.append("\n");
                        s.append(std::size_t(level)*2, ' ');
                    }
                    s.append("argument");
                    break;
                case jsonpath_token_kind::selector:
                    JSONCONS_ASSERT(selector_ != nullptr);
                    s.append(selector_->to_string(level));
                    break;
                case jsonpath_token_kind::expression:
                    s.append(expression_->to_string(level));
                    break;
                case jsonpath_token_kind::literal:
                {
                    if (level > 0)
                    {
                        s.append("\n");
                        s.append(std::size_t(level)*2, ' ');
                    }
                    auto sbuf = value_.to_string();
                    unicode_traits::convert(sbuf.data(), sbuf.size(), s);
                    break;
                }
                case jsonpath_token_kind::binary_operator:
                    s.append(binary_operator_->to_string(level));
                    break;
                case jsonpath_token_kind::function:
                    s.append(function_->to_string(level));
                    break;
                default:
                    if (level > 0)
                    {
                        s.append("\n");
                        s.append(std::size_t(level)*2, ' ');
                    }
                    s.append("token kind: ");
                    s.append(jsoncons::jsonpath::detail::to_string(token_kind_));
                    break;
            }
            //s.append("\n");
            return s;
        }
    };

    template <typename Callback,typename Json,typename JsonReference>
    class callback_receiver : public node_receiver<Json,JsonReference>
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using reference = JsonReference;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using path_node_type = basic_path_node<typename Json::char_type>;
    private:
        allocator_type alloc_;
        Callback& callback_;
    public:

        callback_receiver(Callback& callback, const allocator_type& alloc)
            : alloc_(alloc), callback_(callback)
        {
        }

        void add(const path_node_type& base_path, 
                 reference value) override
        {
            callback_(base_path, value);
        }
    };

    template <typename Json,typename JsonReference>
    class path_expression
    {
    public:
        using allocator_type = typename Json::allocator_type;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using path_value_pair_type = path_value_pair<Json,JsonReference>;
        using path_value_pair_less_type = path_value_pair_less<Json,JsonReference>;
        using path_value_pair_greater_type = path_value_pair_greater<Json,JsonReference>;
        using path_value_pair_equal_type = path_value_pair_equal<Json,JsonReference>;
        using value_type = Json;
        using reference = typename path_value_pair_type::reference;
        using pointer = typename path_value_pair_type::value_pointer;
        using token_type = token<Json,JsonReference>;
        using reference_arg_type = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,
            const_reference_arg_t,reference_arg_t>::type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using selector_type = jsonpath_selector<Json,JsonReference>;
    private:
        allocator_type alloc_;
        selector_type* selector_;
        result_options required_options_;
    public:

        path_expression(selector_type* selector, bool paths_required, const allocator_type& alloc)
            : alloc_(alloc), selector_(selector), required_options_()
        {
            if (paths_required)
            {
                required_options_ |= result_options::path;
            }
        }

        path_expression(const allocator_type& alloc)
            : alloc_(alloc), selector_(nullptr), required_options_()
        {
        }

        path_expression(const path_expression& expr) = delete;
        path_expression(path_expression&& expr) = default;

        ~path_expression() = default;
        
        path_expression& operator=(const path_expression& expr) = delete;
        path_expression& operator=(path_expression&& expr) = default;

        Json evaluate(eval_context<Json,JsonReference>& context, 
            reference root,
            const path_node_type& path, 
            reference instance,
            result_options options) const
        {
            Json result(json_array_arg, semantic_tag::none, alloc_);

            if ((options & result_options::path) == result_options::path)
            {
                auto callback = [&result](const path_node_type& pathp, reference)
                {
                    result.emplace_back(to_basic_string(pathp)); 
                };
                evaluate(context, root, path, instance, callback, options);
            }
            else
            {
                auto callback = [&result](const path_node_type&, reference val)
                {
                    result.push_back(val);
                };
                evaluate(context, root, path, instance, callback, options);
            }

            return result;
        }

        template <typename Callback>
        typename std::enable_if<ext_traits::is_function_object<Callback,const path_node_type&,reference>::value,void>::type
        evaluate(eval_context<Json,JsonReference>& context, 
            reference root,
            const path_node_type& path, 
            reference current, 
            Callback callback,
            result_options options) const
        {
            std::error_code ec;

            options |= required_options_;

            const result_options require_more = result_options::nodups | result_options::sort | result_options::sort_descending;

            if (selector_ != nullptr)
            {
                if ((options & require_more) != result_options())
                {
                    path_value_receiver<Json,JsonReference> receiver{alloc_};
                    selector_->select(context, root, path, current, receiver, options);

                    if (receiver.nodes.size() > 1) 
                    {
                        if ((options & result_options::sort_descending) == result_options::sort_descending)
                        {
                            std::sort(receiver.nodes.begin(), receiver.nodes.end(), path_value_pair_greater_type());
                        } 
                        else if ((options & result_options::sort) == result_options::sort)
                        {
                            std::sort(receiver.nodes.begin(), receiver.nodes.end(), path_value_pair_less_type());
                        }
                    }

                    if (receiver.nodes.size() > 1 && (options & result_options::nodups) == result_options::nodups)
                    {
                        if ((options & result_options::sort_descending) == result_options::sort_descending)
                        {
                            auto last = std::unique(receiver.nodes.rbegin(),receiver.nodes.rend(),path_value_pair_equal_type());
                            receiver.nodes.erase(receiver.nodes.begin(), last.base());
                            for (auto& node : receiver.nodes)
                            {
                                callback(node.path(), node.value());
                            }
                        }
                        else if ((options & result_options::sort) == result_options::sort)
                        {
                            auto last = std::unique(receiver.nodes.begin(),receiver.nodes.end(),path_value_pair_equal_type());
                            receiver.nodes.erase(last,receiver.nodes.end());
                            for (auto& node : receiver.nodes)
                            {
                                callback(node.path(), node.value());
                            }
                        }
                        else
                        {
                            std::vector<path_value_pair_type> index(receiver.nodes);
                            std::sort(index.begin(), index.end(), path_value_pair_less_type());
                            auto last = std::unique(index.begin(),index.end(),path_value_pair_equal_type());
                            index.erase(last,index.end());

                            std::vector<path_value_pair_type> temp2;
                            temp2.reserve(index.size());
                            for (auto&& node : receiver.nodes)
                            {
                                auto it = std::lower_bound(index.begin(),index.end(),node, path_value_pair_less_type());
                                if (it != index.end() && (*it).path() == node.path()) 
                                {
                                    temp2.emplace_back(std::move(node));
                                    index.erase(it);
                                }
                            }
                            for (auto& node : temp2)
                            {
                                callback(node.path(), node.value());
                            }
                        }
                    }
                    else
                    {
                        for (auto& node : receiver.nodes)
                        {
                            callback(node.path(), node.value());
                        }
                    }
                }
                else
                {
                    callback_receiver<Callback,Json,JsonReference> receiver(callback, alloc_);
                    selector_->select(context, root, path, current, receiver, options);
                }
            }
        }

        std::string to_string(int level) const
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("expression ");
            if (selector_ != nullptr)
            {
                s.append(selector_->to_string(level+1));
            }

            return s;

        }
    };

    template <typename Json,typename JsonReference>
    class token_evaluator : public expression_base<Json,JsonReference>
    {
    public:
        using path_value_pair_type = path_value_pair<Json,JsonReference>;
        using value_type = Json;
        using reference = typename path_value_pair_type::reference;
        using pointer = typename path_value_pair_type::value_pointer;
        using const_pointer = const value_type*;
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using path_value_pair_less_type = path_value_pair_less<Json,reference>;
        using path_value_pair_greater_type = path_value_pair_greater<Json,reference>;
        using path_value_pair_equal_type = path_value_pair_equal<Json,reference>;
        using parameter_type = parameter<Json>;
        using token_type = token<Json,reference>;
        using reference_arg_type = typename std::conditional<std::is_const<typename std::remove_reference<reference>::type>::value,
            const_reference_arg_t,reference_arg_t>::type;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using stack_item_type = value_or_pointer<Json,JsonReference>;
    private:
        std::vector<token_type> token_list_;
    public:

        token_evaluator()
        {
        }

        token_evaluator(const token_evaluator& expr) = delete;
        token_evaluator(token_evaluator&& expr) = default;

        token_evaluator(std::vector<token_type>&& token_stack)
            : token_list_(std::move(token_stack))
        {
        }

        token_evaluator& operator=(const token_evaluator& expr) = delete;
        token_evaluator& operator=(token_evaluator&& expr) = default;
        
        ~token_evaluator() = default;

        value_type evaluate(eval_context<Json,reference>& context, 
            reference root,
            reference current,
            result_options options,
            std::error_code& ec) const override
        {
            std::vector<stack_item_type> stack;
            std::vector<parameter_type> arg_stack;

            //std::cout << "EVALUATE TOKENS\n";
            //for (auto& tok : token_list_)
            //{
            //    std::cout << tok.to_string() << "\n";
            //}
            //std::cout << "\n";

            if (!token_list_.empty())
            {
                for (auto& tok : token_list_)
                {
                    //std::cout << "Token: " << tok.to_string() << "\n";
                    switch (tok.token_kind())
                    { 
                        case jsonpath_token_kind::literal:
                        {
                            stack.emplace_back(std::addressof(tok.get_value(reference_arg_type(), context)));
                            break;
                        }
                        case jsonpath_token_kind::unary_operator:
                        {
                            JSONCONS_ASSERT(!stack.empty());
                            auto item = std::move(stack.back());
                            stack.pop_back();

                            auto val = tok.unary_operator_->evaluate(item.value(), ec);
                            stack.emplace_back(std::move(val));
                            break;
                        }
                        case jsonpath_token_kind::binary_operator:
                        {
                            //std::cout << "binary operator: " << stack.size() << "\n";
                            JSONCONS_ASSERT(stack.size() >= 2);
                            auto rhs = std::move(stack.back());
                            //std::cout << "rhs: " << *rhs << "\n";
                            stack.pop_back();
                            auto lhs = std::move(stack.back());
                            //std::cout << "lhs: " << *lhs << "\n";
                            stack.pop_back();

                            auto val = tok.binary_operator_->evaluate(lhs.value(), rhs.value(), ec);
                            //std::cout << "Evaluate binary expression: " << r << "\n";
                            stack.emplace_back(std::move(val));
                            break;
                        }
                        case jsonpath_token_kind::root_node:
                            //std::cout << "root: " << root << "\n";
                            stack.emplace_back(std::addressof(root));
                            break;
                        case jsonpath_token_kind::current_node:
                            stack.emplace_back(std::addressof(current));
                            break;
                        case jsonpath_token_kind::argument:
                            JSONCONS_ASSERT(!stack.empty());
                            arg_stack.emplace_back(std::move(stack.back()));
                            stack.pop_back();
                            break;
                        case jsonpath_token_kind::function:
                        {
                            if (tok.function_->arity() && *(tok.function_->arity()) != arg_stack.size())
                            {
                                ec = jsonpath_errc::invalid_arity;
                                return Json::null();
                            }

                            value_type val = tok.function_->evaluate(arg_stack, ec);
                            if (JSONCONS_UNLIKELY(ec))
                            {
                                return Json::null();
                            }
                            //std::cout << "function result: " << val << "\n";
                            arg_stack.clear();
                            stack.emplace_back(std::move(val));
                            break;
                        }
                        case jsonpath_token_kind::expression:
                        {
                            value_type val = tok.expression_->evaluate(context, root, current, options, ec);
                            stack.emplace_back(std::move(val));
                            break;
                        }
                        case jsonpath_token_kind::selector:
                        {
                            JSONCONS_ASSERT(!stack.empty());
                            auto item = std::move(stack.back());
                            //for (auto& item : stack)
                            //{
                                //std::cout << "selector stack input:\n";
                                //switch (item.tag)
                                //{
                                //    case node_set_tag::single:
                                //        std::cout << "single: " << *(item.node.ptr) << "\n";
                                //        break;
                                //    case node_set_tag::multi:
                                //        for (auto& node : stack.back().ptr().nodes)
                                //        {
                                //            std::cout << "multi: " << *node.ptr << "\n";
                                //        }
                                //        break;
                                //    default:
                                //        break;
                            //}
                            //std::cout << "\n";
                            //}
                            //std::cout << "selector item: " << *ptr << "\n";

                            reference val = tok.selector_->evaluate(context, root, path_node_type{}, item.value(), options, ec);

                            stack.pop_back();
                            stack.emplace_back(stack_item_type(std::addressof(val)));
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            //if (stack.size() != 1)
            //{
            //    std::cout << "Stack size: " << stack.size() << "\n";
            //}
            return stack.empty() ? Json::null() : stack.back().value();
        }
 
        std::string to_string(int level) const override
        {
            std::string s;
            if (level > 0)
            {
                s.append("\n");
                s.append(std::size_t(level)*2, ' ');
            }
            s.append("token_evaluator ");
            for (const auto& item : token_list_)
            {
                s.append(item.to_string(level+1));
            }

            return s;

        }
    private:
    };

} // namespace detail
} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_TOKEN_EVALUATOR_HPP
