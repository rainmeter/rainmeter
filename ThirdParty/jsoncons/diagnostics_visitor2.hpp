// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_DIAGNOSTICS_VISITOR2_HPP
#define JSONCONS_DIAGNOSTICS_VISITOR2_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/item_event_visitor.hpp>

namespace jsoncons { 

    class diagnostics_visitor2 : public basic_default_item_event_visitor<char>
    {
        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_begin_object" << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(std::size_t length, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_begin_object " << length << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) override
        {
            std::cout << "visit_end_object" << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_begin_array" << '\n';
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(std::size_t length, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_begin_array " << length << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) override
        {
            std::cout << "visit_end_array" << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& s, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_string " << s << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t val, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_int64 " << val << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t val, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_uint64 " << val << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool val, semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_bool " << val << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag, const ser_context&, std::error_code&) override
        {
            std::cout << "visit_null " << '\n'; 
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(const jsoncons::span<const uint16_t>& s, 
            semantic_tag tag, 
            const ser_context&, 
            std::error_code&) override  
        {
            std::cout << "visit_typed_array uint16_t " << tag << '\n'; 
            for (auto val : s)
            {
                std::cout << val << "" << '\n';
            }
            std::cout << "" << '\n';
            JSONCONS_VISITOR_RETURN;
        }

        JSONCONS_VISITOR_RETURN_TYPE visit_typed_array(half_arg_t, const jsoncons::span<const uint16_t>& s,
            semantic_tag tag,
            const ser_context&,
            std::error_code&) override
        {
            std::cout << "visit_typed_array half_arg_t uint16_t " << tag << "" << '\n';
            for (auto val : s)
            {
                std::cout << val << "" << '\n';
            }
            std::cout << "" << '\n';
            JSONCONS_VISITOR_RETURN;
        }
    };

} // namespace jsoncons

#endif // JSONCONS_DIAGNOSTICS_VISITOR2_HPP
