// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_DECODER_HPP
#define JSONCONS_JSON_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include <memory> // std::allocator
#include <system_error>
#include <utility> // std::move
#include <vector>

#include <jsoncons/json_object.hpp>
#include <jsoncons/json_type.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/ser_utils.hpp>

namespace jsoncons {

template <typename Json,typename TempAlloc =std::allocator<char>>
class json_decoder final : public basic_json_visitor<typename Json::char_type>
{
public:
    using char_type = typename Json::char_type;
    using typename basic_json_visitor<char_type>::string_view_type;

    using key_value_type = typename Json::key_value_type;
    using key_type = typename Json::key_type;
    using array = typename Json::array;
    using object = typename Json::object;
    using allocator_type = typename Json::allocator_type;
    using json_string_allocator = typename key_type::allocator_type;
    using json_array_allocator = typename array::allocator_type;
    using json_object_allocator = typename object::allocator_type;
    using json_byte_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<uint8_t>;
private:

    enum class structure_type {root_t, array_t, object_t};

    struct structure_info
    {
        structure_type type_;
        std::size_t container_index_{0};

        structure_info(structure_type type, std::size_t offset) noexcept
            : type_(type), container_index_(offset)
        {
        }
        ~structure_info() = default;
    };

    using temp_allocator_type = TempAlloc;
    using stack_item_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<index_key_value<Json>>;
    using structure_info_allocator_type = typename std::allocator_traits<temp_allocator_type>:: template rebind_alloc<structure_info>;
 
    allocator_type allocator_;

    Json result_;

    std::size_t index_{0};
    key_type name_;
    std::vector<index_key_value<Json>,stack_item_allocator_type> item_stack_;
    std::vector<structure_info,structure_info_allocator_type> structure_stack_;
    bool is_valid_{false};

public:
    json_decoder(const allocator_type& alloc = allocator_type(), 
        const temp_allocator_type& temp_alloc = temp_allocator_type())
        : allocator_(alloc),
          result_(),
          name_(alloc),
          item_stack_(alloc),
          structure_stack_(temp_alloc)
    {
        item_stack_.reserve(1000);
        structure_stack_.reserve(100);
        structure_stack_.emplace_back(structure_type::root_t, 0);
    }

    json_decoder(temp_allocator_arg_t, 
        const temp_allocator_type& temp_alloc = temp_allocator_type())
        : allocator_(),
          result_(),
          name_(),
          item_stack_(),
          structure_stack_(temp_alloc)
    {
        item_stack_.reserve(1000);
        structure_stack_.reserve(100);
        structure_stack_.emplace_back(structure_type::root_t, 0);
    }

    void reset()
    {
        is_valid_ = false;
        index_ = 0;
        item_stack_.clear();
        structure_stack_.clear();
        structure_stack_.emplace_back(structure_type::root_t, 0);
    }

    bool is_valid() const
    {
        return is_valid_;
    }

    Json get_result()
    {
        JSONCONS_ASSERT(is_valid_);
        is_valid_ = false;
        return std::move(result_);
    }

private:

    void visit_flush() override
    {
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_object(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        if (structure_stack_.back().type_ == structure_type::root_t)
        {
            index_ = 0;
            item_stack_.clear();
            is_valid_ = false;
        }
        item_stack_.emplace_back(std::move(name_), index_++, json_object_arg, tag);
        structure_stack_.emplace_back(structure_type::object_t, item_stack_.size()-1);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_end_object(const ser_context&, std::error_code&) override
    {
        JSONCONS_ASSERT(structure_stack_.size() > 0);
        JSONCONS_ASSERT(structure_stack_.back().type_ == structure_type::object_t);
        const size_t structure_index = structure_stack_.back().container_index_;
        JSONCONS_ASSERT(item_stack_.size() > structure_index);
        const size_t count = item_stack_.size() - (structure_index + 1);
        auto first = item_stack_.begin() + (structure_index+1);

        if (count > 0)
        {
            item_stack_[structure_index].value.template cast<typename Json::object_storage>().value().uninitialized_init(
                &item_stack_[structure_index+1], count);
        }

        item_stack_.erase(first, item_stack_.end());
        structure_stack_.pop_back();
        if (structure_stack_.back().type_ == structure_type::root_t)
        {
            result_.swap(item_stack_.front().value);
            item_stack_.pop_back();
            is_valid_ = true;
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_begin_array(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        if (structure_stack_.back().type_ == structure_type::root_t)
        {
            index_ = 0;
            item_stack_.clear();
            is_valid_ = false;
        }
        item_stack_.emplace_back(std::move(name_), index_++, json_array_arg, tag);
        structure_stack_.emplace_back(structure_type::array_t, item_stack_.size()-1);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_end_array(const ser_context&, std::error_code&) override
    {
        JSONCONS_ASSERT(structure_stack_.size() > 1);
        JSONCONS_ASSERT(structure_stack_.back().type_ == structure_type::array_t);
        const size_t container_index = structure_stack_.back().container_index_;
        JSONCONS_ASSERT(item_stack_.size() > container_index);

        auto& container = item_stack_[container_index].value;

        const size_t size = item_stack_.size() - (container_index + 1);
        //std::cout << "size on item stack: " << size << "\n";

        if (size > 0)
        {
            container.reserve(size);
            auto first = item_stack_.begin() + (container_index+1);
            auto last = first + size;
            for (auto it = first; it != last; ++it)
            {
                container.push_back(std::move((*it).value));
            }
            item_stack_.erase(first, item_stack_.end());
        }

        structure_stack_.pop_back();
        if (structure_stack_.back().type_ == structure_type::root_t)
        {
            result_.swap(item_stack_.front().value);
            item_stack_.pop_back();
            is_valid_ = true;
            JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_key(const string_view_type& name, const ser_context&, std::error_code&) override
    {
        name_ = key_type(name.data(),name.length(),allocator_);
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_string(const string_view_type& sv, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, sv, tag);
                break;
            case structure_type::root_t:
                result_ = Json(sv, tag, allocator_);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& b, 
                           semantic_tag tag, 
                           const ser_context&,
                           std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, byte_string_arg, b, tag);
                break;
            case structure_type::root_t:
                result_ = Json(byte_string_arg, b, tag, allocator_);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_byte_string(const byte_string_view& b, 
        uint64_t ext_tag, 
        const ser_context&,
        std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, byte_string_arg, b, ext_tag);
                break;
            case structure_type::root_t:
                result_ = Json(byte_string_arg, b, ext_tag, allocator_);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_int64(int64_t value, 
        semantic_tag tag, 
        const ser_context&,
        std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, value, tag);
                break;
            case structure_type::root_t:
                result_ = Json(value,tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_uint64(uint64_t value, 
        semantic_tag tag, 
        const ser_context&,
        std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, value, tag);
                break;
            case structure_type::root_t:
                result_ = Json(value,tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_half(uint16_t value, 
        semantic_tag tag,   
        const ser_context&,
        std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, half_arg, value, tag);
                break;
            case structure_type::root_t:
                result_ = Json(half_arg, value, tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_double(double value, 
        semantic_tag tag,   
        const ser_context&,
        std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, value, tag);
                break;
            case structure_type::root_t:
                result_ = Json(value, tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_bool(bool value, semantic_tag tag, const ser_context&, std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, value, tag);
                break;
            case structure_type::root_t:
                result_ = Json(value, tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }

    JSONCONS_VISITOR_RETURN_TYPE visit_null(semantic_tag tag, const ser_context&, std::error_code&) override
    {
        switch (structure_stack_.back().type_)
        {
            case structure_type::object_t:
            case structure_type::array_t:
                item_stack_.emplace_back(std::move(name_), index_++, null_type(), tag);
                break;
            case structure_type::root_t:
                result_ = Json(null_type(), tag);
                is_valid_ = true;
                JSONCONS_VISITOR_RETURN;
        }
        JSONCONS_VISITOR_RETURN;
    }
};

} // namespace jsoncons

#endif // JSONCONS_JSON_DECODER_HPP
