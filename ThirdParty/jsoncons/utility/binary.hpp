#ifndef JSONCONS_UTILITY_BINARY_HPP
#define JSONCONS_UTILITY_BINARY_HPP

#include <cfloat> 
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring> // std::memcpy
#include <type_traits> // std::enable_if

#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons { 
namespace binary { 

    // byte_swap

    template<class T>
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(uint8_t),T>::type
    byte_swap(T val)
    {
        return val;
    }

    template<class T>
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(uint16_t),T>::type
    byte_swap(T val)
    {
    #if defined(JSONCONS_BYTE_SWAP_16)
        return JSONCONS_BYTE_SWAP_16(val);
    #else
        return (static_cast<uint16_t>(val) >> 8) | (static_cast<uint16_t>(val) << 8);
    #endif
    }
     
    template<class T>
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(uint32_t),T>::type
    byte_swap(T val)
    {
    #if defined(JSONCONS_BYTE_SWAP_32)
        return JSONCONS_BYTE_SWAP_32(val);
    #else
        uint32_t tmp = ((static_cast<uint32_t>(val) << 8) & 0xff00ff00) | ((static_cast<uint32_t>(val) >> 8) & 0xff00ff);
        return (tmp << 16) | (tmp >> 16);
    #endif
    }

    template<class T>
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) == sizeof(uint64_t),T>::type
    byte_swap(T val)
    {
    #if defined(JSONCONS_BYTE_SWAP_64)
        return JSONCONS_BYTE_SWAP_64(val);
    #else
        uint64_t tmp = ((static_cast<uint64_t>(val) & 0x00000000ffffffffull) << 32) | ((static_cast<uint64_t>(val) & 0xffffffff00000000ull) >> 32);
        tmp = ((tmp & 0x0000ffff0000ffffull) << 16) | ((tmp & 0xffff0000ffff0000ull) >> 16);
        return ((tmp & 0x00ff00ff00ff00ffull) << 8)  | ((tmp & 0xff00ff00ff00ff00ull) >> 8);
    #endif
    }

    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == sizeof(uint32_t),T>::type
    byte_swap(T val)
    {
        uint32_t x;
        std::memcpy(&x,&val,sizeof(uint32_t));
        uint32_t y = byte_swap(x);
        T val2;
        std::memcpy(&val2,&y,sizeof(uint32_t));
        return val2;
    }

    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == sizeof(uint64_t),T>::type
    byte_swap(T val)
    {
        uint64_t x;
        std::memcpy(&x,&val,sizeof(uint64_t));
        uint64_t y = byte_swap(x);
        T val2;
        std::memcpy(&val2,&y,sizeof(uint64_t));
        return val2;
    }

    struct uint128_holder
    {
        uint64_t lo;
        uint64_t hi;
    };

    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 2*sizeof(uint64_t),T>::type
    byte_swap(T val)
    {
        uint128_holder x;
        uint8_t buf[2*sizeof(uint64_t)];
        std::memcpy(buf,&val,2*sizeof(uint64_t));
        std::memcpy(&x.lo,buf,sizeof(uint64_t));
        std::memcpy(&x.hi,buf+sizeof(uint64_t),sizeof(uint64_t));

        uint128_holder y;
        y.lo = byte_swap(x.hi);
        y.hi = byte_swap(x.lo);

        T val2;
        std::memcpy(&val2,&y,2*sizeof(uint64_t));

        return val2;
    }
    // native_to_big

    template <typename T,typename OutputIt,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,void>::type
    native_to_big(T val, OutputIt d_first)
    {
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    template <typename T,typename OutputIt,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,void>::type
    native_to_big(T val, OutputIt d_first)
    {
        T val2 = byte_swap(val);
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val2, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    // native_to_little

    template <typename T,typename OutputIt,typename Endian = endian>
    typename std::enable_if<Endian::native == Endian::little,void>::type
    native_to_little(T val, OutputIt d_first)
    {
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    template <typename T,typename OutputIt,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::big, void>::type
    native_to_little(T val, OutputIt d_first)
    {
        T val2 = byte_swap(val);
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val2, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    // big_to_native

    template <typename T,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,T>::type
    big_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return val;
    }

    template <typename T,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,T>::type
    big_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return byte_swap(val);
    }

    // little_to_native

    template <typename T,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,T>::type
    little_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return val;
    }

    template <typename T,typename Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,T>::type
    little_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return byte_swap(val);
    }

} // namespace binary
} // namespace jsoncons

#endif // JSONCONS_UTILITY_BINARY_HPP
