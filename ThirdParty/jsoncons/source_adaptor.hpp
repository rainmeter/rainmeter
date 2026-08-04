// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_SOURCE_ADAPTOR_HPP
#define JSONCONS_SOURCE_ADAPTOR_HPP

#include <cstddef>
#include <system_error>

#include <jsoncons/json_error.hpp> // json_errc
#include <jsoncons/source.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

namespace jsoncons {

    // text_source_adaptor

    template <typename Source>
    class text_source_adaptor 
    {
    public:
        using value_type = typename Source::value_type;
    private:
        Source source_;
        bool bof_;

    public:
        text_source_adaptor()
            : bof_(true)
        {
        }

        template <typename Sourceable>
        text_source_adaptor(Sourceable&& source)
            : source_(std::forward<Sourceable>(source)), bof_(true)
        {
        }

        bool eof() const
        {
            return source_.eof();
        }

        bool is_error() const
        {
            return source_.is_error();  
        }

        span<const value_type> read_buffer(std::error_code& ec)
        {
            if (source_.eof())
            {
                return span<const value_type>();
            }

            auto s = source_.read_buffer();
            const value_type* data = s.data();
            std::size_t length = s.size();

            if (bof_ && length > 0)
            {
                auto r = unicode_traits::detect_encoding_from_bom(data, length);
                if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
                {
                    ec = json_errc::illegal_unicode_character;
                    return span<const value_type>();
                }
                length -= (r.ptr - data);
                data = r.ptr;
                bof_ = false;
            }
            return span<const value_type>(data, length);           
        }
    };

    // json_source_adaptor

    template <typename Source>
    class json_source_adaptor 
    {
    public:
        using value_type = typename Source::value_type;
    private:
        Source source_;
        bool bof_;

    public:
        json_source_adaptor()
            : bof_(true)
        {
        }

        template <typename Sourceable>
        json_source_adaptor(Sourceable&& source)
            : source_(std::forward<Sourceable>(source)), bof_(true)
        {
        }

        bool eof() const
        {
            return source_.eof();
        }

        bool is_error() const
        {
            return source_.is_error();  
        }

        span<const value_type> read_buffer(std::error_code& ec)
        {
            if (source_.eof())
            {
                return span<const value_type>();
            }

            auto s = source_.read_buffer();
            const value_type* data = s.data();
            std::size_t length = s.size();

            if (bof_ && length > 0)
            {
                auto r = unicode_traits::detect_json_encoding(data, length);
                if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
                {
                    ec = json_errc::illegal_unicode_character;
                    return span<const value_type>();
                }
                length -= (r.ptr - data);
                data = r.ptr;
                bof_ = false;
            }
            
            return span<const value_type>(data, length);           
        }
    };

} // namespace jsoncons

#endif // JSONCONS_SOURCE_ADAPTOR_HPP

