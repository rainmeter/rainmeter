// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/unicode_traits for latest version

/*
 * Includes code derived from Unicode, Inc decomposition code in ConvertUTF.h and ConvertUTF.c 
 * http://www.unicode.org/  
 *  
 * "Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard."
*/

#ifndef JSONCONS_UTILITY_UNICODE_TRAITS_HPP
#define JSONCONS_UTILITY_UNICODE_TRAITS_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <system_error>
#include <type_traits>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons { 
namespace unicode_traits {

    enum class encoding_kind {undetected,utf8,utf16le,utf16be,utf32le,utf32be};

    inline
    std::string to_string(encoding_kind encoding)
    {
        switch (encoding)
        {
            case encoding_kind::utf8:
                return "utf8";
            case encoding_kind::utf16le:
                return "utf16le";
            case encoding_kind::utf16be:
                return "utf16be";
            case encoding_kind::utf32le:
                return "utf32le";
            case encoding_kind::utf32be:
                return "utf32be";
            default:
                return "undetected";
        }
    }

    template <typename Byte>
    struct detect_encoding_result
    {
        const Byte* ptr;
        encoding_kind encoding;
    };

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value,detect_encoding_result<CharT>>::type
    detect_encoding_from_bom(const CharT* data, std::size_t length)
    {
        const uint8_t bom_utf8[] = {0xef,0xbb,0xbf}; 
        const uint8_t bom_utf16le[] = {0xff,0xfe}; 
        const uint8_t bom_utf16be[] = {0xfe,0xff}; 
        const uint8_t bom_utf32le[] = {0xff,0xfe,0x00,0x00}; 
        const uint8_t bom_utf32be[] = {0x00,0x00,0xfe,0xff}; 

        if (length >= 4 && !memcmp(data,bom_utf32le,4))
        {
            return detect_encoding_result<CharT>{data+4,encoding_kind::utf32le};
        }
        else if (length >= 4 && !memcmp(data,bom_utf32be,4))
        {
            return detect_encoding_result<CharT>{data+4,encoding_kind::utf32be};
        }
        else if (length >= 2 && !memcmp(data,bom_utf16le,2))
        {
            return detect_encoding_result<CharT>{data+2,encoding_kind::utf16le};
        }
        else if (length >= 2 && !memcmp(data,bom_utf16be,2))
        {
            return detect_encoding_result<CharT>{data+2,encoding_kind::utf16be};
        }
        else if (length >= 3 && !memcmp(data,bom_utf8,3))
        {
            return detect_encoding_result<CharT>{data+3,encoding_kind::utf8};
        }
        else
        {
            return detect_encoding_result<CharT>{data,encoding_kind::undetected};
        }
    }

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char16<CharT>::value || ext_traits::is_char32<CharT>::value,detect_encoding_result<CharT>>::type
    detect_encoding_from_bom(const CharT* data, std::size_t)
    {
        return detect_encoding_result<CharT>{data,encoding_kind::undetected};
    }

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value,detect_encoding_result<CharT>>::type
    detect_json_encoding(const CharT* data, std::size_t length)
    {
        detect_encoding_result<CharT> r = detect_encoding_from_bom(data,length);
        if (r.encoding != encoding_kind::undetected)
        {
            return r;
        }
        else if (length < 4)
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf8};
        }
        else if (*data == 0 && *(data+1) == 0 && *(data+2) == 0)
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf32be};
        }
        else if (*data == 0 && *(data+2) == 0)
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf16be};
        }
        else if (*(data+1) == 0 && *(data+2) == 0 && *(data+3) == 0)
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf32le};
        }
        else if (*(data+1) == 0 && *(data+3) == 0)
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf16le};
        }
        else
        {
            return detect_encoding_result<CharT>{data,encoding_kind::utf8};
        }
    }

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char16<CharT>::value || ext_traits::is_char32<CharT>::value,detect_encoding_result<CharT>>::type
    detect_json_encoding(const CharT* data, std::size_t)
    {
        return detect_encoding_result<CharT>{data,encoding_kind::undetected};
    }

    /*
     * Magic values subtracted from a buffer value during UTF8 conversion.
     * This table contains as many values as there might be trailing bytes
     * in a UTF-8 sequence. Source: ConvertUTF.c
     */
    const uint32_t offsets_from_utf8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
                  0x03C82080UL, 0xFA082080UL, 0x82082080UL };

    /*
     * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
     * into the first byte, depending on how many bytes follow.  There are
     * as many entries in this table as there are UTF-8 sequence types.
     * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
     * for *legal* UTF-8 will be 4 or fewer bytes total. Source: ConvertUTF.c
     */
    const uint8_t first_byte_mark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    /*
     * Index into the table below with the first byte of a UTF-8 sequence to
     * get the number of trailing bytes that are supposed to follow it.
     * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
     * left as-is for anyone who may want to do such conversion, which was
     * allowed in earlier algorithms. Source: ConvertUTF.c
     */
    const uint8_t trailing_bytes_for_utf8[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
    };

    // Some fundamental constants.  Source: ConvertUTF.h 
    const uint32_t replacement_char = 0x0000FFFD;
    const uint32_t max_bmp = 0x0000FFFF;
    const uint32_t max_utf16 = 0x0010FFFF;
    const uint32_t max_utf32 = 0x7FFFFFFF;
    const uint32_t max_legal_utf32 = 0x0010FFFF;

    const int half_shift  = 10; // used for shifting by 10 bits
    const uint32_t half_base = 0x0010000UL;
    const uint32_t half_mask = 0x3FFUL;

    const uint16_t sur_high_start = 0xD800;
    const uint16_t sur_high_end = 0xDBFF;
    const uint16_t sur_low_start = 0xDC00;
    const uint16_t sur_low_end = 0xDFFF;

    inline
    static bool is_continuation_byte(unsigned char ch)
    {
        return (ch & 0xC0) == 0x80;
    }

    inline
    bool is_high_surrogate(uint32_t ch) noexcept
    {
        return (ch >= sur_high_start && ch <= sur_high_end);
    }

    inline
    bool is_low_surrogate(uint32_t ch) noexcept
    {
        return (ch >= sur_low_start && ch <= sur_low_end);
    }

    inline
    bool is_surrogate(uint32_t ch) noexcept
    {
        return (ch >= sur_high_start && ch <= sur_low_end);
    }

    enum class conv_flags 
    {
        strict = 0,
        lenient
    };

    // conv_errc

    enum class conv_errc 
    {
        success = 0,
        over_long_utf8_sequence = 1, // over long utf8 sequence
        expected_continuation_byte,  // expected continuation byte    
        unpaired_high_surrogate,     // unpaired high surrogate UTF-16
        illegal_surrogate_value,     // UTF-16 surrogate values are illegal in UTF-32
        source_exhausted,            // partial character in source, but hit end
        source_illegal               // source sequence is illegal/malformed
    };

    class Unicode_traits_error_category_impl_
       : public std::error_category
    {
    public:
        virtual const char* name() const noexcept
        {
            return "unicode_traits conversion error";
        }
        virtual std::string message(int ev) const
        {
            switch (static_cast<conv_errc>(ev))
            {
            case conv_errc::over_long_utf8_sequence:
                return "Over long utf8 sequence";
            case conv_errc::expected_continuation_byte:
                return "Expected continuation byte";
            case conv_errc::unpaired_high_surrogate:
                return "Unpaired high surrogate UTF-16";
            case conv_errc::illegal_surrogate_value:
                return "UTF-16 surrogate values are illegal in UTF-32";
            case conv_errc::source_exhausted:
                return "Partial character in source, but hit end";
            case conv_errc::source_illegal:
                return "Source sequence is illegal/malformed";
            default:
                return "";
                break;
            }
        }
    };

    inline
    const std::error_category& unicode_traits_error_category() noexcept
    {
      static Unicode_traits_error_category_impl_ instance;
      return instance;
    }

    inline 
    std::error_code make_error_code(conv_errc result) noexcept
    {
        return std::error_code(static_cast<int>(result),unicode_traits_error_category());
    }

} // unicode_traits
} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::unicode_traits::conv_errc> : public true_type
    {
    };
} // namespace std

namespace jsoncons { 
namespace unicode_traits {

    // utf8

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value, conv_errc>::type
    is_legal_utf8(const CharT* first, std::size_t length) 
    {
        uint8_t a;
        const CharT* srcptr = first+length;
        switch (length) {
        default:
            return conv_errc::over_long_utf8_sequence;
        case 4:
            if (((a = (*--srcptr))& 0xC0) != 0x80)
                return conv_errc::expected_continuation_byte;
            JSONCONS_FALLTHROUGH;
        case 3:
            if (((a = (*--srcptr))& 0xC0) != 0x80)
                return conv_errc::expected_continuation_byte;
            JSONCONS_FALLTHROUGH;
        case 2:
            if (((a = (*--srcptr))& 0xC0) != 0x80)
                return conv_errc::expected_continuation_byte;

            switch (static_cast<uint8_t>(*first)) 
            {
                // no fall-through in this inner switch
                case 0xE0: if (a < 0xA0) return conv_errc::source_illegal; break;
                case 0xED: if (a > 0x9F) return conv_errc::source_illegal; break;
                case 0xF0: if (a < 0x90) return conv_errc::source_illegal; break;
                case 0xF4: if (a > 0x8F) return conv_errc::source_illegal; break;
                default:   if (a < 0x80) return conv_errc::source_illegal;
            }

            JSONCONS_FALLTHROUGH;
        case 1:
            if (static_cast<uint8_t>(*first) >= 0x80 && static_cast<uint8_t>(*first) < 0xC2)
                return conv_errc::source_illegal;
            break;
        }
        if (static_cast<uint8_t>(*first) > 0xF4) 
            return conv_errc::source_illegal;

        return conv_errc();
    }

    template <typename...> using void_t = void;

    template <typename,typename,typename = void>
    struct is_output_iterator : std::false_type {};

    template <typename I,typename E>
    struct is_output_iterator<I, E, void_t<
        typename std::iterator_traits<I>::iterator_category,
        decltype(*std::declval<I>() = std::declval<E>())>> : std::true_type {};

    // is_same_size fixes issue with vs2013

    // primary template
    template <typename T1,typename T2,typename Enable = void>
    struct is_same_size : std::false_type 
    {
    };
     
    // specialization for non void types
    template <typename T1,typename T2>
    struct is_same_size<T1, T2,typename std::enable_if<!std::is_void<T1>::value && !std::is_void<T2>::value>::type>
    {
        static constexpr bool value = (sizeof(T1) == sizeof(T2));
    }; 

    // convert

    template <typename CharT>
    struct convert_result
    {
        const CharT* ptr;
        conv_errc ec;
    };

    // to_codepoint

    template <typename CharT,typename CodepointT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value && ext_traits::is_char32<CodepointT>::value,
                            convert_result<CharT>>::type 
    to_codepoint(const CharT* first, const CharT* last, 
                 CodepointT& ch, 
                 conv_flags flags = conv_flags::strict) noexcept
    {
        ch = 0;
        if (first >= last)
        {
            return convert_result<CharT>{first, conv_errc::source_exhausted};
        }
        conv_errc  result = conv_errc();

        unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[static_cast<uint8_t>(*first)];
        if (extra_bytes_to_read >= last - first) 
        {
            result = conv_errc::source_exhausted; 
            return convert_result<CharT>{first, result};
        }
        // Do this check whether lenient or strict 
        if ((result=is_legal_utf8(first, extra_bytes_to_read+1)) != conv_errc()) 
        {
            return convert_result<CharT>{first, result};
        }
        // The cases all fall through. See "Note A" below.
        switch (extra_bytes_to_read) 
        {
            case 5: 
                ch += static_cast<uint8_t>(*first++); 
                ch <<= 6;
                JSONCONS_FALLTHROUGH;
            case 4: 
                ch += static_cast<uint8_t>(*first++); 
                ch <<= 6;
                JSONCONS_FALLTHROUGH;
            case 3: 
                ch += static_cast<uint8_t>(*first++); 
                ch <<= 6;
                JSONCONS_FALLTHROUGH;
            case 2: 
                ch += static_cast<uint8_t>(*first++); 
                ch <<= 6;
                JSONCONS_FALLTHROUGH;
            case 1: 
                ch += static_cast<uint8_t>(*first++); 
                ch <<= 6;
                JSONCONS_FALLTHROUGH;
            case 0: 
                ch += static_cast<uint8_t>(*first++);
                break;
        }
        ch -= offsets_from_utf8[extra_bytes_to_read];

        if (ch <= max_legal_utf32) {
            /*
             * UTF-16 surrogate values are illegal in UTF-32, and anything
             * over Plane 17 (> 0x10FFFF) is illegal.
             */
            if (is_surrogate(ch) ) 
            {
                if (flags == conv_flags::strict) 
                {
                    first -= (extra_bytes_to_read+1); // return to the illegal value itself
                    result = conv_errc::source_illegal;
                    return convert_result<CharT>{first, result};
                } 
                else
                {
                    ch = replacement_char;
                }
            }
        } 
        else // i.e., ch > max_legal_utf32
        { 
            result = conv_errc::source_illegal;
            ch = replacement_char;
        }

        return convert_result<CharT>{first,result} ;
    }

    template <typename CharT,typename CodepointT>
    typename std::enable_if<ext_traits::is_char16<CharT>::value && ext_traits::is_char32<CodepointT>::value,
                            convert_result<CharT>>::type 
    to_codepoint(const CharT* first, const CharT* last, 
                 CodepointT& ch, 
                 conv_flags flags = conv_flags::strict) noexcept
    {
        ch = 0;
        if (first >= last)
        {
            return convert_result<CharT>{first, conv_errc::source_exhausted};
        }
        conv_errc  result = conv_errc();

        ch = *first++;
        // If we have a surrogate pair, convert to UTF32 first. 
        if (is_high_surrogate(ch)) 
        {
            // If the 16 bits following the high surrogate are in the first buffer... 
            if (first < last) 
            {
                uint32_t ch2 = *first;
                // If ptr's a low surrogate, convert to UTF32. 
                if (ch2 >= sur_low_start && ch2 <= sur_low_end ) 
                {
                    ch = ((ch - sur_high_start) << half_shift)
                        + (ch2 - sur_low_start) + half_base;
                    ++first;
                } 
                else if (flags == conv_flags::strict) // ptr's an unpaired high surrogate 
                { 
                    --first; /* return to the illegal value itself */
                    result = conv_errc::source_illegal;
                    return convert_result<CharT>{first, result};
                }
            } 
            else 
            { /* We don't have the 16 bits following the high surrogate. */
                --first; /* return to the high surrogate */
                result = conv_errc::source_exhausted;
                return convert_result<CharT>{first, result};
            }
        } else if (flags == conv_flags::strict) {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (is_low_surrogate(ch) ) 
            {
                --first; /* return to the illegal value itself */
                result = conv_errc::source_illegal;
                return convert_result<CharT>{first, result};
            }
        }
        
        return convert_result<CharT>{first,result} ;
    }

    template <typename CharT,typename CodepointT>
    typename std::enable_if<ext_traits::is_char32<CharT>::value && ext_traits::is_char32<CodepointT>::value,
                            convert_result<CharT>>::type 
    to_codepoint(const CharT* first, const CharT* last, 
                 CodepointT& ch, 
                 conv_flags flags = conv_flags::strict) noexcept
    {
        ch = 0;
        if (first >= last)
        {
            return convert_result<CharT>{first, conv_errc::source_exhausted};
        }
        conv_errc  result = conv_errc();

        ch = *first++;
        if (flags == conv_flags::strict ) 
        {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (is_surrogate(ch)) 
            {
                --first; /* return to the illegal value itself */
                result = conv_errc::illegal_surrogate_value;
                return convert_result<CharT>{first,result} ;
            }
        }
        if (!(ch <= max_legal_utf32))
        {
            ch = replacement_char;
            result = conv_errc::source_illegal;
        }

        return convert_result<CharT>{first,result} ;
    }

    // convert

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char8<CharT>::value
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char8<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, Container& target, conv_flags flags=conv_flags::strict) 
    {
        (void)flags;

        conv_errc  result = conv_errc();
        const CharT* last = data + length;
        while (data != last) 
        {
            std::size_t len = trailing_bytes_for_utf8[static_cast<uint8_t>(*data)] + 1;
            if (len > (std::size_t)(last - data))
            {
                return convert_result<CharT>{data, conv_errc::source_exhausted};
            }
            if ((result=is_legal_utf8(data, len)) != conv_errc())
            {
                return convert_result<CharT>{data,result};
            }

            switch (len) {
                case 4: target.push_back(static_cast<uint8_t>(*data++));
                    JSONCONS_FALLTHROUGH;
                case 3: target.push_back(static_cast<uint8_t>(*data++));
                    JSONCONS_FALLTHROUGH;
                case 2: target.push_back(static_cast<uint8_t>(*data++));
                    JSONCONS_FALLTHROUGH;
                case 1: target.push_back(static_cast<uint8_t>(*data++));
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char8<CharT>::value
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char16<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[static_cast<uint8_t>(*data)];
            if (extra_bytes_to_read >= last - data) 
            {
                result = conv_errc::source_exhausted; 
                break;
            }
            /* Do this check whether lenient or strict */
            if ((result=is_legal_utf8(data, extra_bytes_to_read+1)) != conv_errc())
            {
                break;
            }
            /*
             * The cases all fall through. See "Note A" below.
             */
            uint32_t ch = 0;
            switch (extra_bytes_to_read) {
                case 5: ch += static_cast<uint8_t>(*data++); ch <<= 6; /* remember, illegal UTF-8 */
                    JSONCONS_FALLTHROUGH;
                case 4: ch += static_cast<uint8_t>(*data++); ch <<= 6; /* remember, illegal UTF-8 */
                    JSONCONS_FALLTHROUGH;
                case 3: ch += static_cast<uint8_t>(*data++); ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 2: ch += static_cast<uint8_t>(*data++); ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 1: ch += static_cast<uint8_t>(*data++); ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 0: ch += static_cast<uint8_t>(*data++);
                    break;
            }
            ch -= offsets_from_utf8[extra_bytes_to_read];

            if (ch <= max_bmp) { /* Target is a character <= 0xFFFF */
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (is_surrogate(ch) ) 
                {
                    if (flags == conv_flags::strict) {
                        data -= (extra_bytes_to_read+1); /* return to the illegal value itself */
                        result = conv_errc::source_illegal;
                        break;
                    } else {
                        target.push_back(replacement_char);
                    }
                } else {
                    target.push_back((uint16_t)ch); /* normal case */
                }
            } else if (ch > max_utf16) {
                if (flags == conv_flags::strict) {
                    result = conv_errc::source_illegal;
                    data -= (extra_bytes_to_read+1); /* return to the start */
                    break; /* Bail out; shouldn't continue */
                } else {
                    target.push_back(replacement_char);
                }
            } else {
                /* target is a character in range 0xFFFF - 0x10FFFF. */
                ch -= half_base;
                target.push_back((uint16_t)((ch >> half_shift) + sur_high_start));
                target.push_back((uint16_t)((ch & half_mask) + sur_low_start));
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char8<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char32<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data < last) 
        {
            uint32_t ch = 0;
            unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[static_cast<uint8_t>(*data)];
            if (extra_bytes_to_read >= last - data) 
            {
                result = conv_errc::source_exhausted; 
                break;
            }
            /* Do this check whether lenient or strict */
            if ((result=is_legal_utf8(data, extra_bytes_to_read+1)) != conv_errc()) 
            {
                break;
            }
            /*
             * The cases all fall through. See "Note A" below.
             */
            switch (extra_bytes_to_read) 
            {
                case 5: 
                    ch += static_cast<uint8_t>(*data++); 
                    ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 4: 
                    ch += static_cast<uint8_t>(*data++); 
                    ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 3: 
                    ch += static_cast<uint8_t>(*data++); 
                    ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 2: 
                    ch += static_cast<uint8_t>(*data++); 
                    ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 1: 
                    ch += static_cast<uint8_t>(*data++); 
                    ch <<= 6;
                    JSONCONS_FALLTHROUGH;
                case 0: 
                    ch += static_cast<uint8_t>(*data++);
                    break;
            }
            ch -= offsets_from_utf8[extra_bytes_to_read];

            if (ch <= max_legal_utf32) {
                /*
                 * UTF-16 surrogate values are illegal in UTF-32, and anything
                 * over Plane 17 (> 0x10FFFF) is illegal.
                 */
                if (is_surrogate(ch) ) 
                {
                    if (flags == conv_flags::strict) {
                        data -= (extra_bytes_to_read+1); /* return to the illegal value itself */
                        result = conv_errc::source_illegal;
                        break;
                    } else {
                        target.push_back(replacement_char);
                    }
                } else {
                    target.push_back(ch);
                }
            } else { /* i.e., ch > max_legal_utf32 */
                result = conv_errc::source_illegal;
                target.push_back(replacement_char);
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    // utf16

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char16<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char8<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
                     Container& target, 
                     conv_flags flags = conv_flags::strict) {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data < last) {
            unsigned short bytes_to_write = 0;
            const uint32_t byteMask = 0xBF;
            const uint32_t byteMark = 0x80; 
            uint32_t ch = *data++;
            /* If we have a surrogate pair, convert to uint32_t data. */
            if (is_high_surrogate(ch)) 
            {
                /* If the 16 bits following the high surrogate are in the data buffer... */
                if (data < last) {
                    uint32_t ch2 = *data;
                    /* If ptr's a low surrogate, convert to uint32_t. */
                    if (ch2 >= sur_low_start && ch2 <= sur_low_end) {
                        ch = ((ch - sur_high_start) << half_shift)
                            + (ch2 - sur_low_start) + half_base;
                        ++data;
                    } else if (flags == conv_flags::strict) { /* ptr's an unpaired high surrogate */
                        --data; /* return to the illegal value itself */
                        result = conv_errc::unpaired_high_surrogate;
                        break;
                    }
                } else { /* We don't have the 16 bits following the high surrogate. */
                    --data; /* return to the high surrogate */
                    result = conv_errc::source_exhausted;
                    break;
                }
            } else if (flags == conv_flags::strict) {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (is_low_surrogate(ch)) 
                {
                    --data; /* return to the illegal value itself */
                    result = conv_errc::source_illegal;
                    break;
                }
            }
            /* Figure out how many bytes the result will require */
            if (ch < (uint32_t)0x80) {      
                bytes_to_write = 1;
            } else if (ch < (uint32_t)0x800) {     
                bytes_to_write = 2;
            } else if (ch < (uint32_t)0x10000) {   
                bytes_to_write = 3;
            } else if (ch < (uint32_t)0x110000) {  
                bytes_to_write = 4;
            } else {                            
                bytes_to_write = 3;
                ch = replacement_char;
            }
            
            uint8_t byte1 = 0;
            uint8_t byte2 = 0;
            uint8_t byte3 = 0;
            uint8_t byte4 = 0;

            switch (bytes_to_write) { // note: everything falls through
                case 4: byte4 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                    JSONCONS_FALLTHROUGH;
                case 3: byte3 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                    JSONCONS_FALLTHROUGH;
                case 2: byte2 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                    JSONCONS_FALLTHROUGH;
                case 1: byte1 = (uint8_t)(ch | first_byte_mark[bytes_to_write]);
                    break;
            }
            switch (bytes_to_write) 
            {
            case 4: 
                target.push_back(byte1);
                target.push_back(byte2);
                target.push_back(byte3);
                target.push_back(byte4);
                break;
            case 3: 
                target.push_back(byte1);
                target.push_back(byte2);
                target.push_back(byte3);
                break;
            case 2: 
                target.push_back(byte1);
                target.push_back(byte2);
                break;
            case 1: 
                target.push_back(byte1);
                break;
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char16<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char16<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            /* If we have a surrogate pair, convert to uint32_t data. */
            if (is_high_surrogate(ch)) 
            {
                /* If the 16 bits following the high surrogate are in the data buffer... */
                if (data < last) {
                    uint32_t ch2 = *data;
                    /* If ptr's a low surrogate, */
                    if (ch2 >= sur_low_start && ch2 <= sur_low_end) {
                        target.push_back((uint16_t)ch);
                        target.push_back((uint16_t)ch2);
                        ++data;
                    } else if (flags == conv_flags::strict) { /* ptr's an unpaired high surrogate */
                        --data; /* return to the illegal value itself */
                        result = conv_errc::unpaired_high_surrogate;
                        break;
                    }
                } else { /* We don't have the 16 bits following the high surrogate. */
                    --data; /* return to the high surrogate */
                    result = conv_errc::source_exhausted;
                    break;
                }
            } else if (is_low_surrogate(ch)) 
            {
                // illegal leading low surrogate
                if (flags == conv_flags::strict) {
                    --data; /* return to the illegal value itself */
                    result = conv_errc::source_illegal;
                    break;
                }
                else
                {
                    target.push_back((uint16_t)ch);
                }
            }
            else
            {
                target.push_back((uint16_t)ch);
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char16<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char32<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            /* If we have a surrogate pair, convert to UTF32 data. */
            if (is_high_surrogate(ch)) 
            {
                /* If the 16 bits following the high surrogate are in the data buffer... */
                if (data < last) {
                    uint32_t ch2 = *data;
                    /* If ptr's a low surrogate, convert to UTF32. */
                    if (ch2 >= sur_low_start && ch2 <= sur_low_end ) 
                    {
                        ch = ((ch - sur_high_start) << half_shift)
                            + (ch2 - sur_low_start) + half_base;
                        ++data;
                    } else if (flags == conv_flags::strict) { /* ptr's an unpaired high surrogate */
                        --data; /* return to the illegal value itself */
                        result = conv_errc::source_illegal;
                        break;
                    }
                } else { /* We don't have the 16 bits following the high surrogate. */
                    --data; /* return to the high surrogate */
                    result = conv_errc::source_exhausted;
                    break;
                }
            } else if (flags == conv_flags::strict) {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (is_low_surrogate(ch) ) 
                {
                    --data; /* return to the illegal value itself */
                    result = conv_errc::source_illegal;
                    break;
                }
            }
            target.push_back(ch);
        }
        return convert_result<CharT>{data,result} ;
    }

    // utf32

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char32<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char8<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();
        const CharT* last = data + length;
        while (data < last) 
        {
            unsigned short bytes_to_write = 0;
            const uint32_t byteMask = 0xBF;
            const uint32_t byteMark = 0x80; 
            uint32_t ch = *data++;
            if (flags == conv_flags::strict ) 
            {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (is_surrogate(ch)) 
                {
                    --data; /* return to the illegal value itself */
                    result = conv_errc::illegal_surrogate_value;
                    break;
                }
            }
            /*
             * Figure out how many bytes the result will require. Turn any
             * illegally large UTF32 things (> Plane 17) into replacement chars.
             */
            if (ch < (uint32_t)0x80) {      bytes_to_write = 1;
            } else if (ch < (uint32_t)0x800) {     bytes_to_write = 2;
            } else if (ch < (uint32_t)0x10000) {   bytes_to_write = 3;
            } else if (ch <= max_legal_utf32) {  bytes_to_write = 4;
            } else {                            
                bytes_to_write = 3;
                ch = replacement_char;
                result = conv_errc::source_illegal;
            }

            uint8_t byte1 = 0;
            uint8_t byte2 = 0;
            uint8_t byte3 = 0;
            uint8_t byte4 = 0;

            switch (bytes_to_write) {
            case 4:
                byte4 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                JSONCONS_FALLTHROUGH;
            case 3:
                byte3 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                JSONCONS_FALLTHROUGH;
            case 2:
                byte2 = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                JSONCONS_FALLTHROUGH;
            case 1:
                byte1 = (uint8_t) (ch | first_byte_mark[bytes_to_write]);
                break;
            }

            switch (bytes_to_write) 
            {
            case 4: 
                target.push_back(byte1);
                target.push_back(byte2);
                target.push_back(byte3);
                target.push_back(byte4);
                break;
            case 3: 
                target.push_back(byte1);
                target.push_back(byte2);
                target.push_back(byte3);
                break;
            case 2: 
                target.push_back(byte1);
                target.push_back(byte2);
                break;
            case 1: 
                target.push_back(byte1);
                break;
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char32<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char16<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            if (ch <= max_bmp) { /* Target is a character <= 0xFFFF */
                /* UTF-16 surrogate values are illegal in UTF-32; 0xffff or 0xfffe are both reserved values */
                if (is_surrogate(ch) ) 
                {
                    if (flags == conv_flags::strict) {
                        --data; /* return to the illegal value itself */
                        result = conv_errc::source_illegal;
                        break;
                    } else {
                        target.push_back(replacement_char);
                    }
                } else {
                    target.push_back((uint16_t)ch); /* normal case */
                }
            } else if (ch > max_legal_utf32) {
                if (flags == conv_flags::strict) {
                    result = conv_errc::source_illegal;
                } else {
                    target.push_back(replacement_char);
                }
            } else {
                /* target is a character in range 0xFFFF - 0x10FFFF. */
                ch -= half_base;
                target.push_back((uint16_t)((ch >> half_shift) + sur_high_start));
                target.push_back((uint16_t)((ch & half_mask) + sur_low_start));
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    template <typename CharT,typename Container>
    typename std::enable_if<ext_traits::is_char32<CharT>::value                            
                            && ext_traits::is_back_insertable<Container>::value
                            && ext_traits::is_char32<typename Container::value_type>::value,
                            convert_result<CharT>>::type 
    convert(const CharT* data, std::size_t length, 
            Container& target, 
            conv_flags flags = conv_flags::strict) 
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            if (flags == conv_flags::strict ) 
            {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (is_surrogate(ch)) 
                {
                    --data; /* return to the illegal value itself */
                    result = conv_errc::illegal_surrogate_value;
                    break;
                }
            }
            if (ch <= max_legal_utf32)
            {
                target.push_back(ch);
            }
            else
            {
                target.push_back(replacement_char);
                result = conv_errc::source_illegal;
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    // validate

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value,
                            convert_result<CharT>>::type 
    validate(const CharT* data, std::size_t length) noexcept
    {
        conv_errc  result = conv_errc();
        const CharT* last = data + length;
        while (data != last) 
        {
            std::size_t len = static_cast<std::size_t>(trailing_bytes_for_utf8[static_cast<uint8_t>(*data)]) + 1;
            if (len > (std::size_t)(last - data))
            {
                return convert_result<CharT>{data, conv_errc::source_exhausted};
            }
            if ((result=is_legal_utf8(data, len)) != conv_errc())
            {
                return convert_result<CharT>{data,result} ;
            }
            data += len;
        }
        return convert_result<CharT>{data,result} ;
    }

    // utf16

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char16<CharT>::value,
                            convert_result<CharT>>::type 
    validate(const CharT* data, std::size_t length)  noexcept
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            /* If we have a surrogate pair, validate to uint32_t data. */
            if (is_high_surrogate(ch)) 
            {
                /* If the 16 bits following the high surrogate are in the data buffer... */
                if (data < last) {
                    uint32_t ch2 = *data;
                    /* If ptr's a low surrogate, */
                    if (ch2 >= sur_low_start && ch2 <= sur_low_end) {
                        ++data;
                    } else {
                        --data; /* return to the illegal value itself */
                        result = conv_errc::unpaired_high_surrogate;
                        break;
                    }
                } 
                else // We don't have the 16 bits following the high surrogate.  
                { 
                    --data; /* return to the high surrogate */
                    result = conv_errc::source_exhausted;
                    break;
                }
            } 
            else if (is_low_surrogate(ch)) 
            {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                --data; /* return to the illegal value itself */
                result = conv_errc::source_illegal;
                break;
            }
        }
        return convert_result<CharT>{data,result} ;
    }

    // utf32

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char32<CharT>::value,
                            convert_result<CharT>>::type 
    validate(const CharT* data, std::size_t length) noexcept
    {
        conv_errc  result = conv_errc();

        const CharT* last = data + length;
        while (data != last) 
        {
            uint32_t ch = *data++;
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (is_surrogate(ch)) 
            {
                --data; /* return to the illegal value itself */
                result = conv_errc::illegal_surrogate_value;
                break;
            }
            if (!(ch <= max_legal_utf32))
            {
                result = conv_errc::source_illegal;
            }
        }
        return convert_result<CharT>{data, result} ;
    }

    enum class encoding {u8,u16le,u16be,u32le,u32be,undetected};

    template <typename Iterator>
    struct determine_encoding_result
    {
        Iterator it;
        encoding ec;
    };

    template <typename Iterator>
    typename std::enable_if<std::is_integral<typename std::iterator_traits<Iterator>::value_type>::value && sizeof(typename std::iterator_traits<Iterator>::value_type) == sizeof(uint8_t),
                            determine_encoding_result<Iterator>>::type 
    detect_encoding(Iterator first, Iterator last) noexcept
    {
        Iterator it1 = first;
        if (std::distance(first,last) < 4)
        {
            if (std::distance(first,last) == 3)
            {
                Iterator it2 = ++first;
                Iterator it3 = ++first;
                if (static_cast<uint8_t>(*it1) == 0xEF && static_cast<uint8_t>(*it2) == 0xBB && static_cast<uint8_t>(*it3) == 0xBF)
                {
                    return determine_encoding_result<Iterator>{last,encoding::u8};
                }
            }
            return determine_encoding_result<Iterator>{it1,encoding::undetected};
        }
        else
        {
            Iterator it2 = ++first;
            Iterator it3 = ++first;
            Iterator it4 = ++first;

            uint32_t bom = static_cast<uint8_t>(*it1) | (static_cast<uint8_t>(*it2) << 8) | (static_cast<uint8_t>(*it3) << 16) | (static_cast<uint8_t>(*it4) << 24);
            if (bom == 0xFFFE0000)                  
            { 
                return determine_encoding_result<Iterator>{it4++,encoding::u32be};
            }
            else if (bom == 0x0000FEFF) 
            {
                return determine_encoding_result<Iterator>{first,encoding::u32le};
            }
            else if ((bom & 0xFFFF) == 0xFFFE)     
            {
                return determine_encoding_result<Iterator>{it3,encoding::u16be};
            }
            else if ((bom & 0xFFFF) == 0xFEFF)      
            {
                return determine_encoding_result<Iterator>{it3,encoding::u16le};
            }
            else if ((bom & 0xFFFFFF) == 0xBFBBEF)  
            {
                return determine_encoding_result<Iterator>{it4,encoding::u8};
            }
            else
            {
                uint32_t pattern = (static_cast<uint8_t>(*it1) ? 1 : 0) | (static_cast<uint8_t>(*it2) ? 2 : 0) | (static_cast<uint8_t>(*it3) ? 4 : 0) | (static_cast<uint8_t>(*it4) ? 8 : 0);
                switch (pattern) {
                case 0x08: 
                    return determine_encoding_result<Iterator>{it1,encoding::u32be};
                case 0x0A: 
                    return determine_encoding_result<Iterator>{it1,encoding::u16be};
                case 0x01: 
                    return determine_encoding_result<Iterator>{it1,encoding::u32le};
                case 0x05: 
                    return determine_encoding_result<Iterator>{it1,encoding::u16le};
                case 0x0F: 
                    return determine_encoding_result<Iterator>{it1,encoding::u8};
                default:
                    return determine_encoding_result<Iterator>{it1,encoding::undetected};
                }
            }
        }
    }

    // count_codepoints

    template <typename CharT>
    typename std::enable_if<ext_traits::is_char8<CharT>::value || ext_traits::is_char16<CharT>::value || ext_traits::is_char32<CharT>::value, std::size_t>::type 
    count_codepoints(const CharT* data, std::size_t length, 
                     conv_flags flags = conv_flags::strict) noexcept
    {
        conv_errc ec = conv_errc();

        std::size_t count = 0;
        const CharT* ptr = data;
        const CharT* last = data + length;

        for (; ptr < last; ++count) 
        {
            uint32_t cp = 0;
            auto r = to_codepoint(ptr, last, cp, flags);
            if (r.ec != conv_errc())
            {
                ec = r.ec;
                break;
            }
            ptr = r.ptr;
        }
        return ec == conv_errc() && ptr == last ? count : 0;
    }

} // unicode_traits
} // namespace jsoncons

#endif //JSONCONS_UTILITY_UNICODE_TRAITS_HPP

