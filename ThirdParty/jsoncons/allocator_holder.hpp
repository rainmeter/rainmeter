// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_ALLOCATOR_HOLDER_HPP
#define JSONCONS_ALLOCATOR_HOLDER_HPP

namespace jsoncons {

template <typename Allocator>
class allocator_holder
{
public:
    using allocator_type = Allocator;
private:
    allocator_type alloc_;
public:
    allocator_holder() = default;
    allocator_holder(const allocator_holder&)  = default;
    allocator_holder(allocator_holder&&)  = default;
    allocator_holder& operator=(const allocator_holder&) = delete;
    allocator_holder(const allocator_type& alloc)
        : alloc_(alloc)
        {}
    ~allocator_holder() = default;
    
    allocator_type get_allocator() const
    {
        return alloc_;
    }
};

} // namespace jsoncons

#endif // JSONCONS_ALLOCATOR_HOLDER_HPP
