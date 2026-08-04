// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_JSONPATH_JSON_QUERY_HPP
#define JSONCONS_EXT_JSONPATH_JSON_QUERY_HPP

#include <type_traits>

#include <jsoncons/allocator_set.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/reflect/json_conv_traits.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons_ext/jsonpath/token_evaluator.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_expression.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_parser.hpp>
#include <jsoncons_ext/jsonpath/path_node.hpp>

namespace jsoncons { 
namespace jsonpath {

    template <typename Json,typename JsonReference = const Json&>
    struct legacy_jsonpath_traits
    {
        using char_type = typename Json::char_type;
        using string_type = typename Json::string_type;
        using string_view_type = typename Json::string_view_type;
        using element_type = Json;
        using value_type = typename std::remove_cv<Json>::type;
        using reference = JsonReference;
        using const_reference = const value_type&;
        using pointer = typename std::conditional<std::is_const<typename std::remove_reference<reference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;
        using allocator_type = typename value_type::allocator_type;
        using evaluator_type = typename jsoncons::jsonpath::detail::jsonpath_evaluator<value_type, reference>;
        using path_node_type = basic_path_node<typename Json::char_type>;
        using path_expression_type = jsoncons::jsonpath::detail::path_expression<value_type,reference>;
        using path_pointer = const path_node_type*;
    };
     
    template <typename Json>
    Json json_query(const Json& root,
                    const typename Json::string_view_type& path, 
                    result_options options = result_options(),
                    const custom_functions<Json>& functions = custom_functions<Json>())
    {
        auto expr = make_expression<Json>(path, functions);
        return expr.evaluate(root, options);
    }

    template <typename Json,typename Callback>
    typename std::enable_if<ext_traits::is_function_object<Callback,const typename Json::string_type&,const Json&>::value,void>::type
    json_query(const Json& root, 
               const typename Json::string_view_type& path, 
               Callback callback,
               result_options options = result_options(),
               const custom_functions<Json>& functions = custom_functions<Json>())
    {
        auto expr = make_expression<Json>(path, functions);
        expr.evaluate(root, callback, options);
    }

    template <typename Json,typename TempAlloc >
    Json json_query(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
        const Json& root, const typename Json::string_view_type& path, 
        result_options options = result_options(),
        const custom_functions<Json>& functions = custom_functions<Json>())
    {
        auto expr = make_expression<Json>(aset, path, functions);
        return expr.evaluate(root, options);
    }

    template <typename Json,typename Callback,typename TempAlloc >
    typename std::enable_if<ext_traits::is_function_object<Callback,const typename Json::string_type&,const Json&>::value,void>::type
    json_query(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
        const Json& root, const typename Json::string_view_type& path, 
        Callback callback,
        result_options options = result_options(),
        const custom_functions<Json>& functions = custom_functions<Json>())
    {
        auto expr = make_expression<Json>(aset, path, functions);
        expr.evaluate(root, callback, options);
    }

    template <typename Json,typename T>
    typename std::enable_if<reflect::is_json_conv_traits_specialized<Json,T>::value,void>::type
        json_replace(Json& root, const typename Json::string_view_type& path, T&& new_value,
                     const custom_functions<Json>& funcs = custom_functions<Json>())
    {
        using jsonpath_traits_type = jsoncons::jsonpath::legacy_jsonpath_traits<Json, Json&>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;

        auto resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type>>(funcs);
        evaluator_type evaluator;
        path_expression_type expr = evaluator.compile(*resources, path);

        jsoncons::jsonpath::detail::eval_context<Json,reference> context;
        auto callback = [&new_value](const path_node_type&, reference v)
        {
            v = std::forward<T>(new_value);
        };

        result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
        expr.evaluate(context, root, path_node_type{}, root, callback, options);
    }

    template <typename Json,typename T,typename TempAlloc >
    typename std::enable_if<reflect::is_json_conv_traits_specialized<Json,T>::value,void>::type
        json_replace(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
            Json& root, const typename Json::string_view_type& path, T&& new_value,
            const custom_functions<Json>& funcs = custom_functions<Json>())
    {
        using jsonpath_traits_type = jsoncons::jsonpath::legacy_jsonpath_traits<Json, Json&>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;

        auto resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type>>(funcs, aset.get_allocator());
        evaluator_type evaluator{aset.get_allocator()};
        path_expression_type expr = evaluator.compile(*resources, path);

        jsoncons::jsonpath::detail::eval_context<Json,reference> context{aset.get_allocator()};
        auto callback = [&new_value](const path_node_type&, reference v)
        {
            v = Json(std::forward<T>(new_value), semantic_tag::none);
        };
        result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
        expr.evaluate(context, root, path_node_type{}, root, callback, options);
    }

    template <typename Json,typename BinaryCallback>
    typename std::enable_if<ext_traits::is_function_object<BinaryCallback,const typename Json::string_type&,Json&>::value,void>::type
    json_replace(Json& root, const typename Json::string_view_type& path , BinaryCallback callback, 
                 const custom_functions<Json>& funcs = custom_functions<Json>())
    {
        using jsonpath_traits_type = jsoncons::jsonpath::legacy_jsonpath_traits<Json, Json&>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;

        auto resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type>>(funcs);
        evaluator_type evaluator;
        path_expression_type expr = evaluator.compile(*resources, path);

        jsoncons::jsonpath::detail::eval_context<Json,reference> context;

        auto f = [&callback](const path_node_type& path, reference val)
        {
            callback(to_basic_string(path), val);
        };
        result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
        expr.evaluate(context, root, path_node_type{}, root, f, options);
    }

    template <typename Json,typename BinaryCallback,typename TempAlloc >
    typename std::enable_if<ext_traits::is_function_object<BinaryCallback,const typename Json::string_type&,Json&>::value,void>::type
    json_replace(const allocator_set<typename Json::allocator_type,TempAlloc>& aset, 
        Json& root, const typename Json::string_view_type& path , BinaryCallback callback, 
        const custom_functions<Json>& funcs = custom_functions<Json>())
    {
        using jsonpath_traits_type = jsoncons::jsonpath::legacy_jsonpath_traits<Json, Json&>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;

        auto resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type>>(funcs, aset.get_allocator());
        evaluator_type evaluator{aset.get_allocator()};
        path_expression_type expr = evaluator.compile(*resources, path);

        jsoncons::jsonpath::detail::eval_context<Json,reference> context{aset.get_allocator()};

        auto f = [&callback](const path_node_type& path, reference val)
        {
            callback(to_basic_string(path), val);
        };
        result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
        expr.evaluate(context, root, path_node_type{}, root, f, options);
    }

    // Legacy replace function
    template <typename Json,typename UnaryCallback>
    typename std::enable_if<ext_traits::is_function_object<UnaryCallback,Json>::value,void>::type
    json_replace(Json& root, const typename Json::string_view_type& path , UnaryCallback callback)
    {
        using jsonpath_traits_type = jsoncons::jsonpath::legacy_jsonpath_traits<Json, Json&>;

        using value_type = typename jsonpath_traits_type::value_type;
        using reference = typename jsonpath_traits_type::reference;
        using evaluator_type = typename jsonpath_traits_type::evaluator_type;
        using path_expression_type = typename jsonpath_traits_type::path_expression_type;
        using path_node_type = typename jsonpath_traits_type::path_node_type;

        auto resources = jsoncons::make_unique<jsoncons::jsonpath::detail::static_resources<value_type>>();
        evaluator_type evaluator;
        path_expression_type expr = evaluator.compile(*resources, path);

        jsoncons::jsonpath::detail::eval_context<Json,reference> context;
        auto f = [callback](const path_node_type&, reference v)
        {
            v = callback(v);
        };
        result_options options = result_options::nodups | result_options::path | result_options::sort_descending;
        expr.evaluate(context, root, path_node_type{}, root, f, options);
    }

} // namespace jsonpath
} // namespace jsoncons

#endif // JSONCONS_EXT_JSONPATH_JSON_QUERY_HPP
