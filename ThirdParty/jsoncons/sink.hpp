// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_SINK_HPP
#define JSONCONS_SINK_HPP

#include <cmath>
#include <cstdint>
#include <cstring> // std::memcpy
#include <memory> // std::addressof
#include <ostream>
#include <vector>

#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

namespace jsoncons { 

    // stream_sink

    template <typename CharT>
    class stream_sink
    {
    public:
        using value_type = CharT;
        using container_type = std::basic_ostream<CharT>;

    private:
        static constexpr size_t default_buffer_length = 16384;

        std::basic_ostream<CharT>* stream_ptr_;
        std::vector<CharT> buffer_;
        CharT * begin_buffer_;
        const CharT* end_buffer_;
        CharT* p_;

    public:

        // Noncopyable
        stream_sink(const stream_sink&) = delete;
        stream_sink(stream_sink&&) = default;

        stream_sink(std::basic_ostream<CharT>& os)
            : stream_ptr_(std::addressof(os)), buffer_(default_buffer_length), begin_buffer_(buffer_.data()), end_buffer_(begin_buffer_+buffer_.size()), p_(begin_buffer_)
        {
        }
        stream_sink(std::basic_ostream<CharT>& os, std::size_t buflen)
        : stream_ptr_(std::addressof(os)), buffer_(buflen), begin_buffer_(buffer_.data()), end_buffer_(begin_buffer_+buffer_.size()), p_(begin_buffer_)
        {
        }
        ~stream_sink() noexcept
        {
            stream_ptr_->write(begin_buffer_, buffer_length());
            stream_ptr_->flush();
        }

        // Movable
        stream_sink& operator=(const stream_sink&) = delete;
        stream_sink& operator=(stream_sink&&) = default;

        void flush()
        {
            stream_ptr_->write(begin_buffer_, buffer_length());
            stream_ptr_->flush();
            p_ = buffer_.data();
        }

        void append(const CharT* s, std::size_t length)
        {
            std::size_t diff = end_buffer_ - p_;
            if (diff >= length)
            {
                std::memcpy(p_, s, length*sizeof(CharT));
                p_ += length;
            }
            else
            {
                stream_ptr_->write(begin_buffer_, buffer_length());
                stream_ptr_->write(s,length);
                p_ = begin_buffer_;
            }
        }

        void append(std::size_t count, const CharT ch)
        {
            if (count <= std::size_t(end_buffer_ - p_))
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    *p_++ = ch;
                }
            }
            else
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    push_back(ch);
                }
            }
        }

        void push_back(CharT ch)
        {
            if (p_ < end_buffer_)
            {
                *p_++ = ch;
            }
            else
            {
                stream_ptr_->write(begin_buffer_, buffer_length());
                p_ = begin_buffer_;
                *p_++ = ch;
            }
        }
    private:

        std::size_t buffer_length() const
        {
            return p_ - begin_buffer_;
        }
    };

    // binary_stream_sink

    class binary_stream_sink
    {
    public:
        typedef uint8_t value_type;
        using container_type = std::basic_ostream<char>;
    private:
        static constexpr size_t default_buffer_length = 16384;

        std::basic_ostream<char>* stream_ptr_;
        std::vector<uint8_t> buffer_;
        uint8_t * begin_buffer_;
        const uint8_t* end_buffer_;
        uint8_t* p_;

    public:

        // Noncopyable
        binary_stream_sink(const binary_stream_sink&) = delete;

        binary_stream_sink(binary_stream_sink&&) = default;

        binary_stream_sink(std::basic_ostream<char>& os)
            : stream_ptr_(std::addressof(os)), 
              buffer_(default_buffer_length), 
              begin_buffer_(buffer_.data()), 
              end_buffer_(begin_buffer_+buffer_.size()), 
              p_(begin_buffer_)
        {
        }
        binary_stream_sink(std::basic_ostream<char>& os, std::size_t buflen)
            : stream_ptr_(std::addressof(os)), 
              buffer_(buflen), 
              begin_buffer_(buffer_.data()), 
              end_buffer_(begin_buffer_+buffer_.size()), 
              p_(begin_buffer_)
        {
        }
        ~binary_stream_sink() noexcept
        {
            stream_ptr_->write((char*)begin_buffer_, buffer_length());
            stream_ptr_->flush();
        }

        binary_stream_sink& operator=(const binary_stream_sink&) = delete;
        binary_stream_sink& operator=(binary_stream_sink&&) = default;

        void flush()
        {
            stream_ptr_->write((char*)begin_buffer_, buffer_length());
            p_ = buffer_.data();
        }

        void append(const uint8_t* s, std::size_t length)
        {
            std::size_t diff = end_buffer_ - p_;
            if (diff >= length)
            {
                std::memcpy(p_, s, length*sizeof(uint8_t));
                p_ += length;
            }
            else
            {
                stream_ptr_->write((char*)begin_buffer_, buffer_length());
                stream_ptr_->write((const char*)s,length);
                p_ = begin_buffer_;
            }
        }

        void append(std::size_t count, uint8_t ch)
        {
            if (count <= std::size_t(end_buffer_ - p_))
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    *p_++ = ch;
                }
            }
            else
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    push_back(ch);
                }
            }
        }

        void push_back(uint8_t ch)
        {
            if (p_ < end_buffer_)
            {
                *p_++ = ch;
            }
            else
            {
                stream_ptr_->write((char*)begin_buffer_, buffer_length());
                p_ = begin_buffer_;
                *p_++ = ch;
            }
        }
    private:

        std::size_t buffer_length() const
        {
            return p_ - begin_buffer_;
        }
    };

    // string_sink

    template <typename StringT>
    class string_sink 
    {
    public:
        using value_type = typename StringT::value_type;
        using container_type = StringT;
    private:
        container_type* buf_ptr{nullptr};
    public:

        // Noncopyable
        string_sink(const string_sink&) = delete;

        string_sink(string_sink&& other) noexcept
        {
            std::swap(buf_ptr,other.buf_ptr);
        }

        string_sink(container_type& buf)
            : buf_ptr(std::addressof(buf))
        {
        }
        
        ~string_sink() = default;

        string_sink& operator=(const string_sink&) = delete;

        string_sink& operator=(string_sink&& other) noexcept
        {
            // TODO: Shouldn't other.buf_ptr be nullified?
            //       Also see move constructor above.
            std::swap(buf_ptr,other.buf_ptr);
            return *this;
        }

        void flush()
        {
        }

        void append(const value_type* s, std::size_t length)
        {
            buf_ptr->append(s, length);
        }

        void append(std::size_t count, value_type ch)
        {
            buf_ptr->append(count, ch);
        }

        void push_back(value_type ch)
        {
            buf_ptr->push_back(ch);
        }
    };

    // bytes_sink

    template <typename Container,typename = void>
    class bytes_sink
    {
    };

    template <typename Container>
    class bytes_sink<Container,typename std::enable_if<ext_traits::is_back_insertable_byte_container<Container>::value>::type> 
    {
    public:
        using container_type = Container;
        using value_type = typename Container::value_type;
    private:
        container_type* buf_ptr;
    public:

        // Noncopyable
        bytes_sink(const bytes_sink&) = delete;

        bytes_sink(bytes_sink&&) = default;

        bytes_sink(container_type& buf)
            : buf_ptr(std::addressof(buf))
        {
        }
        
        ~bytes_sink() = default; 

        bytes_sink& operator=(const bytes_sink&) = delete;
        bytes_sink& operator=(bytes_sink&&) = default;

        void flush()
        {
        }

        void push_back(uint8_t ch)
        {
            buf_ptr->push_back(static_cast<value_type>(ch));
        }
    };

} // namespace jsoncons

#endif // JSONCONS_SINK_HPP
