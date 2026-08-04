// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_UTILITY_URI_HPP
#define JSONCONS_UTILITY_URI_HPP

#include <algorithm> 
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string> // std::string
#include <system_error>
#include <type_traits>
#include <utility>

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/config/jsoncons_config.hpp>
#include <jsoncons/utility/read_number.hpp>
#include <jsoncons/utility/write_number.hpp>
#include <jsoncons/json_exception.hpp>

namespace jsoncons { 

    enum class uri_errc
    {
        success = 0,
        invalid_uri = 1,
        invalid_character_in_scheme = 2,
        invalid_port = 3,
        invalid_character_in_userinfo = 4,
        invalid_character_in_host = 5,
        invalid_character_in_path = 6,
        invalid_character_in_fragment = 7
    };


    class uri_error_category_impl
        : public std::error_category
    {
    public:
        const char* name() const noexcept override
        {
            return "jsoncons/uri";
        }
        std::string message(int ev) const override
        {
            switch (static_cast<uri_errc>(ev))
            {
                case uri_errc::invalid_uri:
                    return "Invalid URI";
                case uri_errc::invalid_character_in_scheme:
                    return "Invalid characters in scheme";
                case uri_errc::invalid_port:
                    return "'port' argument must be a number >= 0 and < 65536";
                case uri_errc::invalid_character_in_userinfo:
                    return "Invalid characters in userinfo";
                case uri_errc::invalid_character_in_host:
                    return "Invalid characters in host";
                case uri_errc::invalid_character_in_path:
                    return "Invalid characters in path";
                case uri_errc::invalid_character_in_fragment:
                    return "Invalid characters in fragment";
                default:
                    return "Unknown uri error";
            }
        }
    };

    inline
        const std::error_category& uri_error_category() noexcept
    {
        static uri_error_category_impl instance;
        return instance;
    }

    inline
        std::error_code make_error_code(uri_errc result) noexcept
    {
        return std::error_code(static_cast<int>(result), uri_error_category());
    }

} // namespace jsoncons

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::uri_errc> : public true_type
    {
    };
} // namespace std

namespace jsoncons {

    struct uri_fragment_part_t
    {
        explicit uri_fragment_part_t() = default; 
    };

    constexpr uri_fragment_part_t uri_fragment_part{};

    struct uri_encoded_part_t
    {
        explicit uri_encoded_part_t() = default; 
    };

    constexpr uri_encoded_part_t uri_encoded_part{};

    class uri
    {
        using part_type = std::pair<std::size_t,std::size_t>;

        std::string uri_string_;
        part_type scheme_part_;
        part_type userinfo_part_;
        part_type host_part_;
        part_type port_part_;
        part_type path_part_;
        part_type query_part_;
        part_type fragment_part_;
    public:

        uri()
            : uri_string_{}, scheme_part_{0,0},userinfo_part_{0,0},host_part_{0,0},port_part_{0,0},path_part_{0,0},query_part_{0,0},fragment_part_{0,0} 
        {
        }

        uri(const uri& other)
            : uri_string_(other.uri_string_), scheme_part_(other.scheme_part_), userinfo_part_(other.userinfo_part_), host_part_(other.host_part_), 
              port_part_(other.port_part_), path_part_(other.path_part_), query_part_(other.query_part_), fragment_part_(other.fragment_part_)
        {
        }

        uri(uri&& other) noexcept
            : uri_string_(std::move(other.uri_string_)), scheme_part_(other.scheme_part_), userinfo_part_(other.userinfo_part_), host_part_(other.host_part_), 
              port_part_(other.port_part_), path_part_(other.path_part_), query_part_(other.query_part_), fragment_part_(other.fragment_part_)
        {
        }

        uri(const uri& other, uri_fragment_part_t, jsoncons::string_view fragment)
            : uri_string_(other.uri_string_), scheme_part_(other.scheme_part_), userinfo_part_(other.userinfo_part_), host_part_(other.host_part_), 
              port_part_(other.port_part_), path_part_(other.path_part_), query_part_(other.query_part_)
        {
            uri_string_.erase(query_part_.second);
            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_part_.first = uri_string_.length();
                encode_illegal_characters(fragment, uri_string_);
                fragment_part_.second = uri_string_.length();
            }
            else
            {
                fragment_part_.first = fragment_part_.second = uri_string_.length();
            }
        }

        explicit uri(jsoncons::string_view str)
        {
            std::error_code ec;
            *this = parse(str, ec);
            if (JSONCONS_UNLIKELY(ec))
            {
                JSONCONS_THROW(std::system_error(ec, std::string(str)));
            }
        }

        uri(jsoncons::string_view scheme,
            jsoncons::string_view userinfo,
            jsoncons::string_view host,
            jsoncons::string_view port,
            jsoncons::string_view path,
            jsoncons::string_view query = "",
            jsoncons::string_view fragment = "")
        {
            if (!scheme.empty()) 
            {
                uri_string_.append(scheme.data(), scheme.size());
                scheme_part_.second = uri_string_.length();
            }
            if (!userinfo.empty() || !host.empty() || !port.empty()) 
            {
                if (!scheme.empty()) 
                {
                    uri_string_.append("://");
                }

                if (!userinfo.empty()) 
                {
                    userinfo_part_.first = uri_string_.length();
                    encode_userinfo(userinfo, uri_string_);
                    userinfo_part_.second = uri_string_.length();
                    uri_string_.append("@");
                }
                else
                {
                    userinfo_part_.first = userinfo_part_.second = uri_string_.length();
                }

                if (!host.empty()) 
                {
                    host_part_.first = uri_string_.length();
                    uri_string_.append(host.data(), host.size());
                    host_part_.second = uri_string_.length();
                } 
                else 
                {
                    JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                }

                if (!port.empty()) 
                {
                    if (!validate_port(port))
                    {
                        JSONCONS_THROW(std::system_error(uri_errc::invalid_port, std::string(port)));
                    }

                    uri_string_.append(":");
                    port_part_.first = uri_string_.length();
                    uri_string_.append(port.data(), port.size());
                    port_part_.second = uri_string_.length();
                }
                else
                {
                    port_part_.first = port_part_.second = uri_string_.length();
                }
            }
            else 
            {
                userinfo_part_.first = userinfo_part_.second = uri_string_.length();
                host_part_.first = host_part_.second = uri_string_.length();
                port_part_.first = port_part_.second = uri_string_.length();
                if (!scheme.empty())
                {
                    if (!path.empty() || !query.empty() || !fragment.empty()) 
                    {
                        uri_string_.append(":");
                    } 
                    else 
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                    }
                }
            }

            if (!path.empty()) 
            {
                // if the URI is not opaque and the path is not already prefixed
                // with a '/', add one.
                path_part_.first = uri_string_.length();
                if (!host.empty() && (path.front() != '/')) 
                {
                    uri_string_.push_back('/');
                }
                encode_path(path, uri_string_);
                path_part_.second = uri_string_.length();
            }
            else
            {
                path_part_.first = path_part_.second = uri_string_.length();
            }

            if (!query.empty()) 
            {
                uri_string_.append("?");
                query_part_.first = uri_string_.length();
                encode_illegal_characters(query, uri_string_);
                query_part_.second = uri_string_.length();
            }
            else
            {
                query_part_.first = query_part_.second = uri_string_.length();
            }

            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_part_.first = uri_string_.length();
                encode_illegal_characters(fragment, uri_string_);
                fragment_part_.second = uri_string_.length();
            }
            else
            {
                fragment_part_.first = fragment_part_.second = uri_string_.length();
            }
        }

        uri(uri_encoded_part_t,
            jsoncons::string_view scheme,
            jsoncons::string_view userinfo,
            jsoncons::string_view host,
            jsoncons::string_view port,
            jsoncons::string_view path,
            jsoncons::string_view query,
            jsoncons::string_view fragment)
        {
            if (!scheme.empty()) 
            {
                uri_string_.append(scheme.data(), scheme.size());
                scheme_part_.second = uri_string_.length();
            }
            if (!userinfo.empty() || !host.empty() || !port.empty()) 
            {
                if (!scheme.empty()) 
                {
                    uri_string_.append("://");
                }

                if (!userinfo.empty()) 
                {
                    userinfo_part_.first = uri_string_.length();
                    uri_string_.append(userinfo.data(), userinfo.size());
                    userinfo_part_.second = uri_string_.length();
                    uri_string_.append("@");
                }
                else
                {
                    userinfo_part_.first = userinfo_part_.second = uri_string_.length();
                }

                if (!host.empty()) 
                {
                    host_part_.first = uri_string_.length();
                    uri_string_.append(host.data(), host.size());
                    host_part_.second = uri_string_.length();
                } 
                else 
                {
                    JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                }

                if (!port.empty()) 
                {
                    uri_string_.append(":");
                    port_part_.first = uri_string_.length();
                    uri_string_.append(port.data(), port.size());
                    port_part_.second = uri_string_.length();
                }
                else
                {
                    port_part_.first = port_part_.second = uri_string_.length();
                }
            }
            else 
            {
                userinfo_part_.first = userinfo_part_.second = uri_string_.length();
                host_part_.first = host_part_.second = uri_string_.length();
                port_part_.first = port_part_.second = uri_string_.length();
                if (!scheme.empty())
                {
                    if (!path.empty() || !query.empty() || !fragment.empty()) 
                    {
                        uri_string_.append(":");
                    } 
                    else 
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>("uri error."));
                    }
                }
            }

            if (!path.empty()) 
            {
                // if the URI is not opaque and the path is not already prefixed
                // with a '/', add one.
                path_part_.first = uri_string_.length();
                if (!host.empty() && (path.front() != '/')) 
                {
                    uri_string_.push_back('/');
                }
                uri_string_.append(path.data(), path.size());
                path_part_.second = uri_string_.length();
            }
            else
            {
                path_part_.first = path_part_.second = uri_string_.length();
            }

            if (!query.empty()) 
            {
                uri_string_.append("?");
                query_part_.first = uri_string_.length();
                uri_string_.append(query.data(), query.size());
                query_part_.second = uri_string_.length();
            }
            else
            {
                query_part_.first = query_part_.second = uri_string_.length();
            }

            if (!fragment.empty()) 
            {
                uri_string_.append("#");
                fragment_part_.first = uri_string_.length();
                uri_string_.append(fragment.data(), fragment.size());
                fragment_part_.second = uri_string_.length();
            }
            else
            {
                fragment_part_.first = fragment_part_.second = uri_string_.length();
            }
        }

        uri& operator=(const uri& other) 
        {
            if (&other != this)
            {
                uri_string_ = other.uri_string_;
                scheme_part_ = other.scheme_part_;
                userinfo_part_ = other.userinfo_part_;
                host_part_ = other.host_part_;
                port_part_ = other.port_part_;
                path_part_ = other.path_part_;
                query_part_ = other.query_part_;
                fragment_part_ = other.fragment_part_;
            }
            return *this;
        }

        uri& operator=(uri&& other) noexcept
        {
            if (&other != this)
            {
                uri_string_ = std::move(other.uri_string_);
                scheme_part_ = other.scheme_part_;
                userinfo_part_ = other.userinfo_part_;
                host_part_ = other.host_part_;
                port_part_ = other.port_part_;
                path_part_ = other.path_part_;
                query_part_ = other.query_part_;
                fragment_part_ = other.fragment_part_;
            }
            return *this;
        }


        const std::string& string() const noexcept
        {
            return uri_string_;
        }

        bool is_absolute() const noexcept
        {
            return scheme_part_.second > scheme_part_.first;
        }

        bool is_opaque() const noexcept 
        {
          return is_absolute() && !encoded_authority().empty();
        }

        uri base() const noexcept 
        { 
            return uri{uri_encoded_part, scheme(), encoded_userinfo(), host(), port(), encoded_path(), 
                jsoncons::string_view{}, jsoncons::string_view{}};
        }

        string_view scheme() const noexcept { return string_view(uri_string_.data()+scheme_part_.first,(scheme_part_.second-scheme_part_.first)); }

        std::string userinfo() const 
        {
            return decode_part(encoded_userinfo());
        }

        string_view encoded_userinfo() const noexcept { return string_view(uri_string_.data()+userinfo_part_.first,(userinfo_part_.second-userinfo_part_.first)); }

        string_view host() const noexcept { return string_view(uri_string_.data()+host_part_.first,(host_part_.second-host_part_.first)); }

        string_view port() const noexcept { return string_view(uri_string_.data()+port_part_.first,(port_part_.second-port_part_.first)); }

        std::string authority() const
        {
            return decode_part(encoded_authority());
        }

        string_view encoded_authority() const noexcept { return string_view(uri_string_.data()+userinfo_part_.first,(port_part_.second-userinfo_part_.first)); }

        std::string path() const
        {
            return decode_part(encoded_path());
        }

        string_view encoded_path() const noexcept { return string_view(uri_string_.data()+path_part_.first,(path_part_.second-path_part_.first)); }

        std::string query() const
        {
            return decode_part(encoded_query());
        }

        string_view encoded_query() const noexcept { return string_view(uri_string_.data()+query_part_.first,(query_part_.second-query_part_.first)); }

        std::string fragment() const
        {
            return decode_part(encoded_fragment());
        }

        string_view encoded_fragment() const noexcept 
        { 
            return string_view(uri_string_.data()+fragment_part_.first,(fragment_part_.second-fragment_part_.first)); 
        }

        bool has_scheme() const noexcept
        {
            return !scheme().empty();
        }

        bool has_userinfo() const noexcept
        {
            return !encoded_userinfo().empty();
        }

        bool has_authority() const noexcept
        {
            return !encoded_authority().empty();
        }

        bool has_host() const noexcept
        {
            return !host().empty();
        }

        bool has_port() const noexcept
        {
            return !port().empty();
        }

        bool has_path() const noexcept
        {
            return !encoded_path().empty();
        }

        bool has_query() const noexcept
        {
            return !encoded_query().empty();
        }

        bool has_fragment() const noexcept
        {
            return !encoded_fragment().empty();
        }

        uri resolve(string_view reference) const
        {
            return resolve(uri(reference));
        }
        
        uri resolve(const uri& reference) const
        {
            // This implementation uses the psuedo-code given in
            // http://tools.ietf.org/html/rfc3986#section-5.2.2

            if (reference.is_absolute() && !reference.is_opaque()) 
            {
                return reference;
            }

            if (reference.is_opaque()) 
            {
                return reference;
            }

            std::string userinfo, host, port, path, query, fragment;

            if (reference.has_authority()) 
            {
              // g -> http://g
              if (reference.has_userinfo()) 
              {
                  userinfo = std::string(reference.encoded_userinfo());
              }

              if (reference.has_host()) 
              {
                  host = std::string(reference.host());
              }

              if (reference.has_port()) 
              {
                  port = std::string(reference.port());
              }

              if (reference.has_path()) 
              {
                  path = remove_dot_segments(std::string(reference.encoded_path()));
              }

              if (reference.has_query()) 
              {
                  query = std::string(reference.encoded_query());
              }
            } 
            else 
            {
              if (!reference.has_path()) 
              {
                if (has_path()) 
                {
                    path = std::string(encoded_path());
                }

                if (reference.has_query()) 
                {
                    query = std::string(reference.encoded_query());
                } 
                else if (has_query()) 
                {
                    query = std::string(encoded_query());
                }
              } 
              else 
              {
                  if (reference.encoded_path().front() == '/') 
                  {
                    path = remove_dot_segments(std::string(reference.encoded_path()));
                  } 
                  else 
                  {
                      path = merge_paths(*this, reference);
                  }

                  if (reference.has_query()) 
                  {
                      query = std::string(reference.encoded_query());
                  }
              }

              if (has_userinfo()) 
              {
                  userinfo = std::string(encoded_userinfo());
              }

              if (has_host()) 
              {
                  host = std::string(this->host());
              }

              if (has_port()) 
              {
                  port = std::string(this->port());
              }
            }

            if (reference.has_fragment()) 
            {
                fragment = std::string(reference.encoded_fragment());
            }

            return uri(uri_encoded_part, std::string(scheme()), userinfo, host, port, path, query, fragment);
        }

        int compare(const uri& other) const
        {
            int result = scheme().compare(other.scheme());
            if (result != 0) return result;
            result = encoded_userinfo().compare(other.encoded_userinfo());
            if (result != 0) return result;
            result = host().compare(other.host());
            if (result != 0) return result;
            result = port().compare(other.port());
            if (result != 0) return result;
            result = encoded_path().compare(other.encoded_path());
            if (result != 0) return result;
            result = encoded_query().compare(other.encoded_query());
            if (result != 0) return result;
            result = encoded_fragment().compare(other.encoded_fragment());

            return result;
        }

        friend bool operator==(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) == 0;
        }

        friend bool operator!=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) != 0;
        }

        friend bool operator<(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) < 0;
        }

        friend bool operator<=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) <= 0;
        }

        friend bool operator>(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) > 0;
        }

        friend bool operator>=(const uri& lhs, const uri& rhs)
        {
            return lhs.compare(rhs) >= 0;
        }

        static std::string decode_part(const jsoncons::string_view& encoded)
        {
            std::string decoded;

            std::size_t length = encoded.size();
            for (std::size_t i = 0; i < length;)
            {
                if (encoded[i] == '%' && (length - i) >= 3)
                {
                    auto hex = encoded.substr(i + 1, 2);

                    uint8_t n;
                    jsoncons::hex_to_integer<uint8_t>(hex.data(), hex.size(), n);
                    decoded.push_back((char)n);
                    i += 3;
                }
                else
                {
                    decoded.push_back(encoded[i]);
                    ++i;
                }
            }
            return decoded;
        }
        static uri parse(string_view str, std::error_code& ec)
        {
            part_type scheme_part{ 0,0 };
            part_type userinfo_part{0,0};
            part_type host_part{0,0};
            part_type port_part{0,0};
            part_type path_part{0,0};
            part_type query_part{0,0};
            part_type fragment_part{0,0};

            std::size_t start = 0;

            parse_state state = parse_state::start;
            std::size_t colon_pos = 0; 
            
            std::size_t i = 0;
            while (i < str.size())
            {
                char c = str[i];
                switch (state)
                {
                    case parse_state::start:
                        switch (c)
                        {
                            case '/':
                                state = parse_state::expect_path;
                                break;
                            case '#':
                                state = parse_state::expect_fragment;
                                start = ++i; 
                                break;
                            default:
                                state = parse_state::expect_scheme;
                                break;
                        }
                        break;
                    case parse_state::expect_scheme:
                        switch (c)
                        {
                            case ':':
                                if (!validate_scheme(string_view{str.data() + start, i-start}))
                                {
                                    ec = uri_errc::invalid_character_in_scheme;
                                    return uri{};
                                }
                                else
                                {
                                    scheme_part = std::make_pair(start,i);
                                    state = parse_state::expect_first_slash;
                                    start = i;
                                }
                                ++i;
                                break;
                            case '?':
                                path_part = std::make_pair(start, i);
                                state = parse_state::expect_query;
                                start = i + 1;
                                ++i;
                                break;
                            case '#':
                                userinfo_part = std::make_pair(start,start);
                                host_part = std::make_pair(start,start);
                                port_part = std::make_pair(start,start);
                                path_part = std::make_pair(start,i);
                                query_part = std::make_pair(i,i);
                                state = parse_state::expect_fragment;
                                start = i+1; 
                                ++i;
                                break;
                            default:
                                if (++i == str.size()) // end of string, haven't found a colon, try path_part
                                {
                                    i = 0;
                                    state = parse_state::expect_path;
                                }
                                break;
                        }
                        break;
                    case parse_state::expect_first_slash:
                        switch (c)
                        {
                            case '/':
                                state = parse_state::expect_second_slash;
                                ++i;
                                break;
                            default:
                                start = i;
                                state = parse_state::expect_path;
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_second_slash:
                        switch (c)
                        {
                            case '/':
                                state = parse_state::expect_authority;
                                start = i+1;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_authority:
                        switch (c)
                        {
                            case '[':
                                state = parse_state::expect_host_ipv6;
                                start = i+1;
                                ++i;
                                break;
                            default:
                                state = parse_state::expect_userinfo;
                                start = i;
                                // i unchanged;
                                break;
                        }
                        break;
                    case parse_state::expect_host_ipv6:
                        switch (c)
                        {
                            case ']':
                                userinfo_part = std::make_pair(start,start);
                                host_part = std::make_pair(start,i);
                                port_part = std::make_pair(i,i);
                                state = parse_state::expect_path;
                                start = i+1;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_userinfo:
                        switch (c)
                        {
                            case '@':
                                if (!validate_userinfo(string_view{str.data() + start, i-start}))
                                {
                                    ec = uri_errc::invalid_character_in_userinfo;
                                    return uri{};
                                }
                                userinfo_part = std::make_pair(start,i);
                                state = parse_state::expect_host;
                                start = i+1;
                                ++i;
                                break;
                            case ':':
                                colon_pos = i;
                                state = parse_state::expect_password;
                                ++i;
                                break;
                            case '/':
                                userinfo_part = std::make_pair(start,start);
                                host_part = std::make_pair(start,i);
                                port_part = std::make_pair(i,i);
                                state = parse_state::expect_path;
                                start = i;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_password:
                        switch (c)
                        {
                            case '@':
                                if (!validate_host(string_view{str.data() + start, i-start}))
                                {
                                    ec = uri_errc::invalid_character_in_host;
                                    return uri{};
                                }
                                userinfo_part = std::make_pair(start,i);
                                state = parse_state::expect_host;
                                start = i+1;
                                ++i;
                                break;
                            case '/':
                            {
                                if (!validate_host(string_view{str.data() + start, colon_pos-start}))
                                {
                                    ec = uri_errc::invalid_character_in_host;
                                    return uri{};
                                }
                                if (!validate_port(string_view{str.data() + (colon_pos+1), i-(colon_pos+1)}))
                                {
                                    ec = uri_errc::invalid_port;
                                    return uri{};
                                }
                                userinfo_part = std::make_pair(start,start);
                                host_part = std::make_pair(start,colon_pos);
                                port_part = std::make_pair(colon_pos+1,i);
                                state = parse_state::expect_path;
                                start = i;
                                ++i;
                                break;
                            }
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_host:
                        switch (c)
                        {
                            case ':':
                                if (!validate_host(string_view{str.data() + start, i-start}))
                                {
                                    ec = uri_errc::invalid_character_in_host;
                                    return uri{};
                                }
                                host_part = std::make_pair(start,i);
                                state = parse_state::expect_port;
                                start = i+1;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_port:
                        switch (c)
                        {
                            case '/':
                                if (!validate_port(string_view{str.data() + start, i-start}))
                                {
                                    ec = uri_errc::invalid_port;
                                    return uri{};
                                }
                                port_part = std::make_pair(start,i);
                                state = parse_state::expect_path;
                                start = i;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_path:
                        switch (c)
                        {
                            case '?':
                                path_part = std::make_pair(start,i);
                                state = parse_state::expect_query;
                                start = i+1;
                                ++i;
                                break;
                            case '#':
                                path_part = std::make_pair(start,i);
                                query_part = std::make_pair(i,i);
                                state = parse_state::expect_fragment;
                                start = i+1;
                                ++i;
                                break;
                            default:
                            {
                                auto first = str.cbegin() + i;
                                auto result = is_pchar(first, str.cend());
                                if (result.second)
                                {
                                    i += (result.first - first);
                                }
                                else if (c == '/')
                                {
                                    ++i;
                                }
                                else
                                {
                                    ec = uri_errc::invalid_character_in_path;
                                    return uri{};
                                }
                                break;
                            }
                        }
                        break;
                    case parse_state::expect_query:
                        switch (c)
                        {
                            case '#':
                                query_part = std::make_pair(start,i);
                                state = parse_state::expect_fragment;
                                start = i+1;
                                ++i;
                                break;
                            default:
                                ++i;
                                break;
                        }
                        break;
                    case parse_state::expect_fragment:
                        ++i;
                        break;
                }
            }
            switch (state)
            {
                case parse_state::expect_userinfo:
                    userinfo_part = std::make_pair(start,start);
                    if (!validate_host(string_view{str.data() + start, str.size()-start}))
                    {
                        ec = uri_errc::invalid_character_in_host;
                        return uri{};
                    }
                    host_part = std::make_pair(start,str.size());
                    port_part = std::make_pair(str.size(), str.size());
                    path_part = std::make_pair(str.size(), str.size());
                    query_part = std::make_pair(str.size(), str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_password:
                    userinfo_part = std::make_pair(start,start);
                    if (!validate_host(string_view{str.data() + start, colon_pos-start}))
                    {
                        ec = uri_errc::invalid_character_in_host;
                        return uri{};
                    }
                    host_part = std::make_pair(start,colon_pos);
                    if (!validate_port(string_view{str.data() + (colon_pos+1), str.size() - (colon_pos+1)}))
                    {
                        ec = uri_errc::invalid_port;
                        return uri{};
                    }
                    port_part = std::make_pair(colon_pos+1, str.size());
                    path_part = std::make_pair(str.size(), str.size());
                    query_part = std::make_pair(str.size(), str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_host:
                    if (!validate_host(string_view{str.data() + start, str.size()-start}))
                    {
                        ec = uri_errc::invalid_character_in_host;
                        return uri{};
                    }
                    host_part = std::make_pair(start, str.size());
                    port_part = std::make_pair(str.size(), str.size());
                    path_part = std::make_pair(str.size(), str.size());
                    query_part = std::make_pair(str.size(), str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_port:
                    if (!validate_port(string_view{str.data() + start, str.size() - start}))
                    {
                        ec = uri_errc::invalid_port;
                        return uri{};
                    }
                    port_part = std::make_pair(start, str.size());
                    path_part = std::make_pair(str.size(), str.size());
                    query_part = std::make_pair(str.size(), str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_path:
                    path_part = std::make_pair(start,str.size());
                    query_part = std::make_pair(str.size(), str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_query:
                    query_part = std::make_pair(start,str.size());
                    fragment_part = std::make_pair(str.size(), str.size());
                    break;
                case parse_state::expect_fragment:
                    fragment_part = std::make_pair(start,str.size());
                    if (!validate_fragment(string_view{str.data() + fragment_part.first, (fragment_part.second - fragment_part.first)}))
                    {
                        ec = uri_errc::invalid_character_in_fragment;
                        return uri{};
                    }
                    break;
                default:
                    ec = uri_errc::invalid_uri;
                    break;
            }

            return uri(std::string(str), scheme_part, userinfo_part, host_part, port_part, path_part, query_part, fragment_part);
        }

    private:
        enum class parse_state {
            start,
            expect_scheme,
            expect_first_slash,
            expect_second_slash,
            expect_authority,
            expect_host_ipv6,
            expect_userinfo,
            expect_password,
            expect_host,
            expect_port,
            expect_path,
            expect_query,
            expect_fragment};

        uri(const std::string& uri, part_type scheme, part_type userinfo, 
            part_type host, part_type port, part_type path, 
            part_type query, part_type fragment)
            : uri_string_(uri), scheme_part_(scheme), userinfo_part_(userinfo), 
              host_part_(host), port_part_(port), path_part_(path), 
              query_part_(query), fragment_part_(fragment)
        {
        }

/*
5.2.4.  Remove Dot Segments

   The pseudocode also refers to a "remove_dot_segments" routine for
   interpreting and removing the special "." and ".." complete path
   segments from a referenced path.  This is done after the path is
   extracted from a reference, whether or not the path was relative, in
   order to remove any invalid or extraneous dot-segments prior to
   forming the target URI.  Although there are many ways to accomplish
   this removal process, we describe a simple method using two string
   buffers.

   1.  The input buffer is initialized with the now-appended path
       components and the output buffer is initialized to the empty
       string.

   2.  While the input buffer is not empty, loop as follows:

       A.  If the input buffer begins with a prefix of "../" or "./",
           then remove that prefix from the input buffer; otherwise,

       B.  if the input buffer begins with a prefix of "/./" or "/.",
           where "." is a complete path segment, then replace that
           prefix with "/" in the input buffer; otherwise,

       C.  if the input buffer begins with a prefix of "/../" or "/..",
           where ".." is a complete path segment, then replace that
           prefix with "/" in the input buffer and remove the last
           segment and its preceding "/" (if any) from the output
           buffer; otherwise,

       D.  if the input buffer consists only of "." or "..", then remove
           that from the input buffer; otherwise,

       E.  move the first path segment in the input buffer to the end of
           the output buffer, including the initial "/" character (if
           any) and any subsequent characters up to, but not including,
           the next "/" character or the end of the input buffer.

   3.  Finally, the output buffer is returned as the result of
       remove_dot_segments.
*/

        static std::string remove_dot_segments(std::string input)
        {
            std::string output;
             
            std::size_t rel = 0;
            const std::size_t buflen = input.size();
            while (rel < buflen)
            {
                char* data = &input[0]+rel;
                const std::size_t length = buflen - rel;

                if (length >= 3 && data[0] == '.' && data[1] == '.' && data[2] == '/')
                { 
                    rel += 3;
                }
                else if (length >= 2 && data[0] == '.' && data[1] == '/')
                {
                    rel += 2;
                }
                else if (length >= 3 && data[0] == '/' && data[1] == '.' && data[2] == '/')
                { 
                    rel += 2;
                    data[2] = '/';
                }
                else if (length == 2 && data[0] == '/' && data[1] == '.')
                {
                    ++rel;
                    data[1] = '/';
                }
                else if (length >= 4 && data[0] == '/' && data[1] == '.' && data[2] == '.' && data[3] == '/')
                { 
                    rel += 3;
                    data[3] = '/';
                    auto rslash = output.rfind('/');
                    if (rslash != std::string::npos)
                    {
                        output.erase(rslash);
                    }
                }
                else if (length >= 3 && data[0] == '/' && data[1] == '.' && data[2] == '.')
                { 
                    rel += 2;
                    data[2] = '/';
                    auto rslash = output.rfind('/');
                    if (rslash != std::string::npos)
                    {
                        output.erase(rslash);
                    }
                }
                else if (length == 1 && data[0] == '.')
                {
                    ++rel;
                }
                else if (length == 2 && data[0] == '.' && data[1] == '.')
                {
                    rel += 2;
                }
                else
                {
                    const auto last = data+length;
                    auto it = std::find(data+1, last, '/');
                    if (it != last)
                    {
                        output.append(data, it - data);
                        rel += (it - data);
                    }
                    else
                    {
                        output.append(data, length);
                        rel += length;
                    }
                }
            }

            //std::cout << "path: " << path << ", output: " << output << "\n";
            
            return output;
        }

        static std::string merge_paths(const uri& base, const uri& relative)
        {
            std::string result;
            
            if (!base.encoded_authority().empty() && base.encoded_path().empty()) 
            {
                result = "/";
                //result.append(relative.encoded_path().data(), relative.encoded_path().length());
            } 
            else 
            {
                const auto& base_path = base.encoded_path();
                auto last_slash = base_path.rfind('/');
                result.append(std::string(base_path.substr(0,last_slash+1)));
            }
            if (!relative.encoded_path().empty()) 
            {
                result.append(relative.encoded_path().begin(), relative.encoded_path().end());
            }
            return remove_dot_segments(std::move(result));
        }

        static bool is_alpha(char ch)
        {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); 
        }

        static bool is_digit(char ch)
        {
            return (ch >= '0' && ch <= '9'); 
        }

        static bool is_alphanum(char ch)
        {
            return is_alpha(ch) || is_digit(ch); 
        }

        static bool is_unreserved(char ch)
        {
            switch (ch)
            {
                case '_':
                case '-':
                case '!':
                case '.':
                case '~':
                case '\'':
                case '(':
                case ')':
                case '*':
                    return true;
                default:
                    return is_alphanum(ch);
            }
        }

        static bool is_punct(char ch)
        {
            switch (ch)
            {
                case ',':
                case ';':
                case ':':
                case '$':
                case '&':
                case '+':
                case '=':
                    return true;
                default:
                    return false;
            }
        }

        static bool is_reserved(char ch)
        {
            switch (ch)
            { 
                case '?':
                case '/':
                case '[':
                case ']':
                case '@':
                    return true;
                default:
                    return is_punct(ch);
            }
        }

        static bool is_hex(char ch)
        {
            switch(ch)
            {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9': 
                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                    return true;
                default:
                    return false;
            }
        }

        static bool is_pct_encoded(const char* s, std::size_t length)
        {
            return length < 3 ? false : s[0] == '%' && is_hex(s[1]) && is_hex(s[2]);
        }
        
        // sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
        static bool is_sub_delim(char c)
        {
            switch (c)
            {
                case '!':
                    return true;
                case '$':
                    return true;
                case '&':
                    return true;
                case '\'':
                    return true;
                case '(':
                    return true;
                case ')':
                    return true;
                case '*':
                    return true;
                case '+':
                    return true;
                case ',':
                    return true;
                case ';':
                    return true;
                case '=':
                    return true;
                default:
                    return false;
            }
        }

    public:

        // Any character not in the unreserved, punct or escaped categories, and not equal 
        // to the slash character ('/') or the  commercial-at character ('@'), is quoted.

        static void encode_path(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                switch (ch)
                {
                    case '/':
                    case '@':
                        encoded.push_back(sv[i]);
                        break;
                    default:
                    {
                        bool escaped = is_pct_encoded(sv.data()+i,3);
                        if (!is_unreserved(ch) && !is_punct(ch) && !escaped)
                        {
                            encoded.push_back('%');
                            if (uint8_t(ch) <= 15)
                            {
                                encoded.push_back('0');
                            }
                            jsoncons::integer_to_hex((uint8_t)ch, encoded);
                        }
                        else if (escaped)
                        {
                            encoded.push_back(ch);
                            encoded.push_back(sv[++i]);
                            encoded.push_back(sv[++i]);
                        }
                        else
                        {
                            encoded.push_back(ch);
                        }
                        break;
                    }
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                switch (ch)
                {
                    case '/':
                    case '@':
                        encoded.push_back(ch);
                        break;
                    default:
                    {
                        if (!is_unreserved(ch) && !is_punct(ch))
                        {
                            encoded.push_back('%');
                            jsoncons::integer_to_hex((uint8_t)ch, encoded);
                        }
                        else
                        {
                            encoded.push_back(ch);
                        }
                        break;
                    }
                }
            }
        }


        // Any character not in the unreserved, punct, or escaped categories is quoted.

        static void encode_userinfo(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                bool escaped = is_pct_encoded(sv.data()+i,3);
                if (!is_unreserved(ch) && !is_punct(ch) && !escaped)
                {
                    encoded.push_back('%');
                    if (uint8_t(ch) <= 15)
                    {
                        encoded.push_back('0');
                    }
                    jsoncons::integer_to_hex((uint8_t)ch, encoded);
                }
                else if (escaped)
                {
                    encoded.push_back(ch);
                    encoded.push_back(sv[++i]);
                    encoded.push_back(sv[++i]);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                if (!is_unreserved(ch) && !is_punct(ch))
                {
                    encoded.push_back('%');
                    jsoncons::integer_to_hex((uint8_t)ch, encoded);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
        }

        // The set of all legal URI characters consists of the unreserved, reserved, escaped characters.

        static void encode_illegal_characters(const jsoncons::string_view& sv, std::string& encoded)
        {
            const std::size_t length1 = sv.size() <= 2 ? 0 : sv.size() - 2;

            std::size_t i = 0;
            for (; i < length1; ++i)
            {
                char ch = sv[i];

                bool escaped = is_pct_encoded(sv.data()+i,3);
                if (!is_unreserved(ch) && !is_reserved(ch) && !escaped)
                {
                    encoded.push_back('%');
                    if (uint8_t(ch) <= 15)
                    {
                        encoded.push_back('0');
                    }
                    jsoncons::integer_to_hex((uint8_t)ch, encoded);
                }
                else if (escaped)
                {
                    encoded.push_back(ch);
                    encoded.push_back(sv[++i]);
                    encoded.push_back(sv[++i]);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
 
            const std::size_t length2 = sv.size();
            for (; i < length2; ++i)
            {
                char ch = sv[i];

                if (!is_unreserved(ch) && !is_reserved(ch))
                {
                    encoded.push_back('%');
                    jsoncons::integer_to_hex((uint8_t)ch, encoded);
                }
                else
                {
                    encoded.push_back(ch);
                }
            }
        }

        // rel_segment   = 1*( unreserved | escaped | ";" | "@" | "&" | "=" | "+" | "$" | "," )
        static bool is_rel_segment(char c, const char* s, std::size_t length)
        {
            return is_unreserved(c) || is_pct_encoded(s,length) || c == ';' || c == '@' || c == '&' || c == '=' || c == '+' || c == '$' || c == ',';
        }

        // userinfo      = *( unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" | "," )

        static bool is_userinfo(char c, const char* s, std::size_t length)
        {
            return is_unreserved(c) || is_pct_encoded(s,length) || c == ';' || c == ':' || c == '&' || c == '=' || c == '+' || c == '$' || c == ',';
        }

        static std::pair<string_view::const_iterator,bool> is_pct_encoded(string_view::const_iterator first, 
            string_view::const_iterator last)
        {
            if ((last-first) < 3)
            {
                return std::pair<string_view::const_iterator,bool>{first+1,false};
            }
            bool result = first[0] == '%' && is_hex(first[1]) && is_hex(first[2]);
            
            return result ? std::pair<string_view::const_iterator,bool>{first+3,true} : std::pair<string_view::const_iterator,bool>{first+1,false};
        }

        static std::pair<string_view::const_iterator,bool> is_pchar(string_view::iterator first, string_view::iterator last)
        {
            JSONCONS_ASSERT(first != last);
            
            const char c = *first;
            if (is_unreserved(c))
            {
                return std::pair<string_view::const_iterator,bool>{first+1,true};
            }
            auto result = is_pct_encoded(first,last);
            if (result.second)
            {
                return result;
            }
            
            return std::pair<string_view::const_iterator,bool>{first+1,is_sub_delim(c) || c == ':' || c == '@'};
        }
        
        static bool validate_fragment(string_view fragment)
        {
            if (fragment.length() == 0)
            {
                return true;
            }
            bool valid = true;
            
            auto cur = fragment.begin();
            auto last = fragment.end();
            while (valid && cur != last)
            {
                auto result = is_pchar(cur,last);
                if (!result.second && !(*cur == '?' || *cur == '/'))
                {
                    valid = false;
                }
                else
                {
                    cur = result.first;
                }
            }
            return valid;
        }       

        static bool validate_userinfo(string_view userinfo)
        {
            if (userinfo.length() == 0)
            {
                return true;
            }
            
            bool valid = true;
            auto cur = userinfo.begin();
            auto last = userinfo.end();
            while (valid && cur != last)
            {
                auto unreserved = is_unreserved(*cur);
                auto pct_encoded = is_pct_encoded(cur,last);
                auto sub_delim = is_sub_delim(*cur);
                if (!unreserved && !pct_encoded.second && !sub_delim && !(*cur == ':'))
                {
                    valid = false;
                }
                if (pct_encoded.second)
                {
                    cur = pct_encoded.first;
                }
                else
                {
                    ++cur;
                }
            }
            return valid;
        }

        static bool validate_port(string_view port)
        {
            uint16_t p;
            auto result = jsoncons::to_integer<uint16_t>(port.data(), port.length(), p);
            return static_cast<bool>(result);
        }
        
        static bool validate_host(string_view userinfo)
        {
            if (userinfo.length() == 0)
            {
                return true;
            }

            bool valid = true;
            auto cur = userinfo.begin();
            auto last = userinfo.end();
            while (valid && cur != last)
            {
                if (*cur == ' ')
                {
                    valid = false;
                }
                ++cur;
            }
            return valid;
        }

        static bool validate_scheme(string_view scheme)
        {
            if (scheme.length() == 0)
            {
                return true;
            }
        
            bool valid = isalpha(scheme[0]);
            auto cur = scheme.begin();
            auto last = scheme.end();
            while (valid && cur != last)
            {
                char c  = *cur;
                if (!(isalnum(c) || c == '+' || c == '.' || c == '-'))
                {
                    valid = false;
                }
                ++cur;
            }
            return valid;
        }
        
        friend std::ostream& operator<<(std::ostream& os, const uri& a_uri)
        {
            return os << a_uri.string();
        }
    };

} // namespace jsoncons

#endif // JSONCONS_UTILITY_URI_HPP
