// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_ALLOCATOR_SET_HPP
#define JSONCONS_ALLOCATOR_SET_HPP

#include <memory>

#include <jsoncons/json_type.hpp>

namespace jsoncons {

template <typename Alloc,typename TempAlloc >
class allocator_set
{
    Alloc result_alloc_;
    TempAlloc temp_alloc_;
public:
    using allocator_type = Alloc;
    using temp_allocator_type = TempAlloc;

    allocator_set(const Alloc& alloc=Alloc(), 
        const TempAlloc& temp_alloc=TempAlloc())
        : result_alloc_(alloc), temp_alloc_(temp_alloc)
    {
    }

    allocator_set(const allocator_set&)  = default;
    allocator_set(allocator_set&&)  = default;
    allocator_set& operator=(const allocator_set&)  = delete;
    allocator_set& operator=(allocator_set&&)  = delete;
    ~allocator_set() = default;

    Alloc get_allocator() const {return result_alloc_;}
    TempAlloc get_temp_allocator() const {return temp_alloc_;}
};

#if !defined(JSONCONS_NO_DEPRECATED)

inline
allocator_set<std::allocator<char>,std::allocator<char>> combine_allocators()
{
    return allocator_set<std::allocator<char>,std::allocator<char>>(std::allocator<char>(), std::allocator<char>());
}

template <typename Alloc>
allocator_set<Alloc,std::allocator<char>> combine_allocators(const Alloc& alloc)
{
    return allocator_set<Alloc,std::allocator<char>>(alloc, std::allocator<char>());
}

template <typename Alloc,typename TempAlloc >
allocator_set<Alloc,TempAlloc> combine_allocators(const Alloc& alloc, const TempAlloc& temp_alloc)
{
    return allocator_set<Alloc,TempAlloc>(alloc, temp_alloc);
}

template <typename TempAlloc >
allocator_set<std::allocator<char>,TempAlloc> temp_allocator_only(const TempAlloc& temp_alloc)
{
    return allocator_set<std::allocator<char>,TempAlloc>(std::allocator<char>(), temp_alloc);
}
#endif // !defined(JSONCONS_NO_DEPRECATED)

inline
allocator_set<std::allocator<char>,std::allocator<char>> make_alloc_set()
{
    return allocator_set<std::allocator<char>,std::allocator<char>>(std::allocator<char>(), std::allocator<char>());
}

template <typename Alloc>
allocator_set<Alloc,std::allocator<char>> make_alloc_set(const Alloc& alloc)
{
    return allocator_set<Alloc,std::allocator<char>>(alloc, std::allocator<char>());
}

template <typename Alloc,typename TempAlloc >
allocator_set<Alloc,TempAlloc> make_alloc_set(const Alloc& alloc, const TempAlloc& temp_alloc)
{
    return allocator_set<Alloc,TempAlloc>(alloc, temp_alloc);
}

template <typename TempAlloc >
allocator_set<std::allocator<char>,TempAlloc> make_alloc_set(temp_alloc_arg_t, const TempAlloc& temp_alloc)
{
    return allocator_set<std::allocator<char>,TempAlloc>(std::allocator<char>(), temp_alloc);
}

} // namespace jsoncons

#endif // JSONCONS_ALLOCATOR_SET_HPP
