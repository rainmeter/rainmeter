// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_JSONPATH_EXPRESSION_HPP
#define JSONCONS_EXT_JSONPATH_JSONPATH_EXPRESSION_HPP

#include <algorithm> // std::reverse
#include <cstddef>
#include <memory>
#include <system_error>
#include <type_traits> // std::is_const
#include <utility> // std::move
#include <vector>

#include <jsoncons/allocator_set.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons_ext/jsonpath/token_evaluator.hpp>
#include <jsoncons_ext/jsonpath/json_location.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_parser.hpp>
#include <jsoncons_ext/jsonpath/path_node.hpp>

namespace jsoncons { 
namespace jsonpath {

    template <typename Json,typename TempAlloc =std::allocator<char>>
    class jsonpath_expression
    {
    public:
        using allocator_type = typename jsonpath_traits<Json>::allocator_type;
        using char_type = typename jsonpath_traits<Json>::char_type;
        using string_type = typename jsonpath_traits<Json>::string_type;
        using string_view_type = typename jsonpath_traits<Json>::string_view_type;
        
        using value_type = typename jsonpath_traits<Json>::value_type;
        using reference = typename jsonpath_traits<Json>::reference;
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        
        using static_resources_type = jsoncons::jsonpath::detail::static_resources<value_type>;
        using path_expression_type = jsoncons::jsonpath::detail::path_expression<value_type,reference>;
        using const_path_expression_type = jsoncons::jsonpath::detail::path_expression<value_type,const_reference>;
        using path_node_type = basic_path_node<typename Json::char_type>;
    private:
        allocator_type alloc_;
        std::unique_ptr<static_resources_type> static_resources_;
        const_path_expression_type const_expr_;
        path_expression_type expr_;
    public:
        jsonpath_expression(const allocator_set<allocator_type,TempAlloc>& aset,
            std::unique_ptr<static_resources_type>&& resources,
            const_path_expression_type&& const_expr, 
            path_expression_type&& expr)
            : alloc_(aset.get_allocator()),
              static_resources_(std::move(resources)), 
              const_expr_(std::move(const_expr)),
              expr_(std::move(expr))
        {
        }

        jsonpath_expression(const jsonpath_expression&) = delete;
        jsonpath_expression(jsonpath_expression&&) = default;

        ~jsonpath_expression() = default;

        jsonpath_expression& operator=(const jsonpath_expression&) = delete;
        jsonpath_expression& operator=(jsonpath_expression&&) = default;

        template <typename BinaryCallback>
        typename std::enable_if<ext_traits::is_function_object<BinaryCallback,const string_type&,const_reference>::value,void>::type
        evaluate(const_reference root, BinaryCallback callback, result_options options = result_options()) const
        {
            jsoncons::jsonpath::detail::eval_context<Json,const_reference> context{alloc_};
            auto f = [&callback](const path_node_type& path, const_reference val)
            {
                callback(to_basic_string(path), val);
            };
            const_expr_.evaluate(context, root, path_node_type{}, root, f, options | result_options::path);
        }

        value_type evaluate(const_reference root, result_options options = result_options()) const
        {
            if ((options & result_options::path) == result_options::path)
            {
                jsoncons::jsonpath::detail::eval_context<value_type, const_reference> context{ alloc_ };

                value_type result(json_array_arg, semantic_tag::none, alloc_);
                auto callback = [&result](const path_node_type& p, const_reference)
                {
                    result.emplace_back(to_basic_string(p));
                };
                const_expr_.evaluate(context, root, path_node_type{}, root, callback, options);
                return result;
            }
            jsoncons::jsonpath::detail::eval_context<value_type, const_reference> context{ alloc_ };
            return const_expr_.evaluate(context, root, path_node_type{}, root, options);
        }

        value_type select(const_reference root, result_options options = result_options()) const
        {
            if ((options & result_options::path) == result_options::path)
            {
                jsoncons::jsonpath::detail::eval_context<value_type, const_reference> context{ alloc_ };

                value_type result(json_array_arg, semantic_tag::none, alloc_);
                auto callback = [&result](const path_node_type& p, const_reference)
                {
                    result.emplace_back(to_basic_string(p));
                };
                const_expr_.evaluate(context, root, path_node_type{}, root, callback, options);
                return result;
            }
            jsoncons::jsonpath::detail::eval_context<value_type, const_reference> context{ alloc_ };
            return const_expr_.evaluate(context, root, path_node_type{}, root, options);
        }

        template <typename BinaryCallback>
        typename std::enable_if<ext_traits::is_function_object<BinaryCallback,const path_node_type&,const_reference>::value,void>::type
        select(const_reference root, BinaryCallback callback, result_options options = result_options()) const
        {
            jsoncons::jsonpath::detail::eval_context<value_type,const_reference> context{alloc_};
            const_expr_.evaluate(context, root, path_node_type{}, root, callback, options | result_options::path);
        }

        template <typename BinaryCallback>
        typename std::enable_if<ext_traits::is_function_object<BinaryCallback,const path_node_type&,value_type&>::value,void>::type
        update(reference root, BinaryCallback callback) const
        {
            jsoncons::jsonpath::detail::eval_context<value_type,reference> context{alloc_};

            result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
            expr_.evaluate(context, root, path_node_type{}, root, callback, options);
        }

        std::vector<basic_json_location<char_type>> select_paths(const_reference root, 
            result_options options = result_options::nodups | result_options::sort) const
        {
            std::vector<basic_json_location<char_type>> result;

            options = options | result_options::path;

            auto callback = [&result](const path_node_type& path, const_reference)
            {
                result.emplace_back(path);
            };

            jsoncons::jsonpath::detail::eval_context<value_type,const_reference> context{alloc_};
            const_expr_.evaluate(context, root, path_node_type{}, root, callback, options);

            return result;
        }
    };

    template <typename Json>
    jsonpath_expression<Json> make_expression(const typename Json::string_view_type& path,
        const jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>& funcs = jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>())
    {
        using value_type = typename jsonpath_traits<Json>::value_type;
        using reference = typename jsonpath_traits<Json>::reference;
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        using static_resources_type = jsoncons::jsonpath::detail::static_resources<value_type>;
        using evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, reference>;
        using const_evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, const_reference>;

        auto resources = jsoncons::make_unique<static_resources_type>(funcs);
        const_evaluator_type const_evaluator;
        auto const_expr = const_evaluator.compile(*resources, path);

        evaluator_type evaluator;
        auto expr = evaluator.compile(*resources, path);

        return jsonpath_expression<Json>(jsoncons::make_alloc_set(), 
            std::move(resources), std::move(const_expr), std::move(expr));
    }

    template <typename Json>
    jsonpath_expression<Json> make_expression(const typename Json::string_view_type& expr, std::error_code& ec)
    {
        return make_expression<Json>(jsoncons::make_alloc_set(), expr, custom_functions<Json>(), ec);
    }

    template <typename Json,typename TempAlloc >
    jsonpath_expression<Json> make_expression(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
        const typename Json::string_view_type& expr, std::error_code& ec)
    {
        return make_expression<Json>(aset, expr, custom_functions<Json>(), ec);
    }

    template <typename Json,typename TempAlloc >
    jsonpath_expression<Json> make_expression(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
        const typename Json::string_view_type& path, 
        const custom_functions<Json>& funcs = custom_functions<Json>())
    {
        using value_type = typename jsonpath_traits<Json>::value_type;
        using reference = typename jsonpath_traits<Json>::reference;
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        using static_resources_type = jsoncons::jsonpath::detail::static_resources<value_type>;
        using evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, reference>;
        using const_evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, const_reference>;

        auto resources = jsoncons::make_unique<static_resources_type>(funcs,
            aset.get_allocator());
        const_evaluator_type const_evaluator{aset.get_allocator()};
        auto const_expr = const_evaluator.compile(*resources, path);

        evaluator_type evaluator{aset.get_allocator()};
        auto expr = evaluator.compile(*resources, path);

        return jsonpath_expression<Json>(aset, 
            std::move(resources), std::move(const_expr), std::move(expr));
    }

    template <typename Json,typename TempAlloc >
    jsonpath_expression<Json> make_expression(const allocator_set<typename Json::allocator_type,TempAlloc>& aset,
        const typename Json::string_view_type& path,
        const jsoncons::jsonpath::custom_functions<typename jsonpath_traits<Json>::value_type>& funcs, std::error_code& ec)
    {
        using value_type = typename jsonpath_traits<Json>::value_type;
        using reference = typename jsonpath_traits<Json>::reference;
        using const_reference = typename jsonpath_traits<Json>::const_reference;
        using static_resources_type = jsoncons::jsonpath::detail::static_resources<value_type>;
        using evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, reference>;
        using const_evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, const_reference>;

        auto resources = jsoncons::make_unique<static_resources_type>(funcs);
        const_evaluator_type const_evaluator{aset.get_allocator()};
        auto const_expr = const_evaluator.compile(*resources, path, ec);

        evaluator_type evaluator{aset.get_allocator()};
        auto expr = evaluator.compile(*resources, path, ec);

        return jsonpath_expression<Json>(aset, 
            std::move(resources), std::move(const_expr), std::move(expr));
    }

    template <typename Json>
    std::size_t remove(Json& root, const jsoncons::basic_string_view<typename Json::char_type>& path_string)
    {
        std::size_t count = 0;

        auto expr = jsonpath::make_expression<Json>(path_string);
        std::vector<jsonpath::json_location> locations = expr.select_paths(root,
            jsonpath::result_options::nodups | jsonpath::result_options::sort_descending);

        for (const auto& location : locations)
        {
            std::size_t n = jsonpath::remove(root, location);
            count += n;
        }
        return count;
    }

} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_JSONPATH_EXPRESSION_HPP
