// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_DIAGNOSTICS_VISITOR_HPP
#define JSONCONS_DIAGNOSTICS_VISITOR_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_visitor.hpp>

namespace jsoncons {

    template <typename CharT>
    class basic_json_diagnostics_visitor : public basic_default_json_visitor<CharT>
    {
    public:
        using stream_type = std::basic_ostream<CharT>;
        using string_type = std::basic_string<CharT>;

    private:
        using supertype = basic_default_json_visitor<CharT>;
        using string_view_type = typename supertype::string_view_type;

        struct enabler {};

        static constexpr CharT visit_begin_array_name[] = {'v','i','s','i','t','_','b','e','g','i','n','_','a','r','r','a','y', 0};
        static constexpr CharT visit_end_array_name[] =  {'v','i','s','i','t','_','e','n','d','_','a','r','r','a','y', 0};
        static constexpr CharT visit_begin_object_name[] =  {'v','i','s','i','t','_','b','e','g','i','n','_','o','b','j','e','c','t', 0};
        static constexpr CharT visit_end_object_name[] =  {'v','i','s','i','t','_','e','n','d','_','o','b','j','e','c','t', 0};
        static constexpr CharT visit_key_name[] =  {'v','i','s','i','t','_','k','e','y', 0};
        static constexpr CharT visit_string_name[] =  {'v','i','s','i','t','_','s','t','r','i','n','g', 0};
        static constexpr CharT visit_byte_string_name[] =  {'v','i','s','i','t','_','b','y','t','e','_','s','t','r','i','n','g', 0};
        static constexpr CharT visit_null_name[] =  {'v','i','s','i','t','_','n','u','l','l', 0};
        static constexpr CharT visit_bool_name[] =  {'v','i','s','i','t','_','b','o','o','l', 0};
        static constexpr CharT visit_uint64_name[] =  {'v','i','s','i','t','_','u','i','n','t','6','4', 0};
        static constexpr CharT visit_int64_name[] =  {'v','i','s','i','t','_','i','n','t','6','4', 0};
        static constexpr CharT visit_half_name[] =  {'v','i','s','i','t','_','h','a','l','f', 0};
        static constexpr CharT visit_double_name[] =  {'v','i','s','i','t','_','d','o','u','b','l','e', 0};

        static constexpr CharT separator_ = ':';

        stream_type& output_;
        string_type indentation_;
        long level_{0};

    public:
        // If CharT is char, then enable the default constructor which binds to
        // std::cout.
        template <typename U = enabler>
        basic_json_diagnostics_visitor(
            typename std::enable_if<std::is_same<CharT, char>::value, U>::type = enabler{})
            : basic_json_diagnostics_visitor(std::cout)
        {
        }

        // If CharT is wchar_t, then enable the default constructor which binds
        // to std::wcout.
        template <typename U = enabler>
        basic_json_diagnostics_visitor(
            typename std::enable_if<std::is_same<CharT, wchar_t>::value, U>::type = enabler{})
            : basic_json_diagnostics_visitor(std::wcout)
        {
        }

        explicit basic_json_diagnostics_visitor(
            stream_type& output,
            string_type indentation = string_type())
            : output_(output),
              indentation_(std::move(indentation))
        {
        }

    private:
        void indent()
        {
            for (long i=0; i<level_; ++i)
                output_ << indentation_;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_begin_object_name << '\n';
            ++level_;
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t length, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_begin_object_name << separator_ << length << '\n';
            ++level_;
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) override
        {
            --level_;
            indent();
            output_ << visit_end_object_name << '\n';
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_begin_array_name << '\n';
            ++level_;
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t length, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_begin_array_name << separator_  << length << '\n';
            ++level_;
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) override
        {
            --level_;
            indent();
            output_ << visit_end_array_name << '\n';
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& s, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_key_name << separator_  << s << '\n';
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& s, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_string_name << separator_  << s << '\n';
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t val, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_int64_name << separator_  << val << '\n';
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t val, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_uint64_name << separator_ << val << '\n';
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool val, semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_bool_name << separator_ << val << '\n';
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag, const ser_context&, std::error_code&) override
        {
            indent();
            output_ << visit_null_name << '\n';
            JSONCONS_VISITOR_RETURN;
        }
    };
#if __cplusplus >= 201703L
// not needed for C++17
#else
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_begin_array_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_end_array_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_begin_object_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_end_object_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_key_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_string_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_byte_string_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_null_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_bool_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_uint64_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_int64_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_half_name[];
    template <typename C> constexpr C basic_json_diagnostics_visitor<C>::visit_double_name[];
#endif // C++17 check

    using json_diagnostics_visitor = basic_json_diagnostics_visitor<char>;
    using wjson_diagnostics_visitor = basic_json_diagnostics_visitor<wchar_t>;

} // namespace jsoncons

#endif // JSONCONS_JSON_VISITOR_HPP
