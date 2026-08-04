// Copyright 2013-2026 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_PRETTY_PRINT_HPP
#define JSONCONS_PRETTY_PRINT_HPP

#include <cstring>
#include <ostream>
#include <typeinfo>

#include <jsoncons/json_encoder.hpp>
#include <jsoncons/json_error.hpp>
#include <jsoncons/json_options.hpp>

namespace jsoncons {

template <typename Json>
class json_printable
{
public:
    using char_type = typename Json::char_type;

    json_printable(const Json& j, indenting indent)
       : j_(&j), indenting_(indent)
    {
    }

    json_printable(const Json& j,
                   const basic_json_encode_options<char_type>& options,
                   indenting indent)
       : j_(&j), options_(options), indenting_(indent)
    {
    }

    void dump(std::basic_ostream<char_type>& os) const
    {
        j_->dump(os, options_, indenting_);
    }

    friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const json_printable<Json>& pr)
    {
        pr.dump(os);
        return os;
    }

    const Json *j_;
    basic_json_encode_options<char_type> options_;
    indenting indenting_;
private:
    json_printable();
};

template <typename Json>
json_printable<Json> print(const Json& j)
{
    return json_printable<Json>(j, indenting::no_indent);
}

template <typename Json>
json_printable<Json> print(const Json& j,
                           const basic_json_encode_options<typename Json::char_type>& options)
{
    return json_printable<Json>(j, options, indenting::no_indent);
}

template <typename Json>
json_printable<Json> pretty_print(const Json& j)
{
    return json_printable<Json>(j, indenting::indent);
}

template <typename Json>
json_printable<Json> pretty_print(const Json& j,
                                  const basic_json_encode_options<typename Json::char_type>& options)
{
    return json_printable<Json>(j, options, indenting::indent);
}

} // namespace jsoncons

#endif // JSONCONS_PRETTY_PRINT_HPP
