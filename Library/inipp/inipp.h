/*
MIT License

Copyright (c) 2017-2020 Matthias C. M. Troffaes

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <vector>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace inipp {

namespace detail {

// trim functions based on http://stackoverflow.com/a/217605

template <class CharT>
inline void ltrim(std::basic_string<CharT> & s, const std::locale & loc) {
	s.erase(s.begin(),
                std::find_if(s.begin(), s.end(),
                             [&loc](CharT ch) { return !std::isspace(ch, loc); }));
}

template <class CharT>
inline void rtrim(std::basic_string<CharT> & s, const std::locale & loc) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
                             [&loc](CharT ch) { return !std::isspace(ch, loc); }).base(),
                s.end());
}

template <class CharT, class UnaryPredicate>
inline void rtrim2(std::basic_string<CharT>& s, UnaryPredicate pred) {
	s.erase(std::find_if(s.begin(), s.end(), pred), s.end());
}

// string replacement function based on http://stackoverflow.com/a/3418285

template <class CharT>
inline bool replace(std::basic_string<CharT> & str, const std::basic_string<CharT> & from, const std::basic_string<CharT> & to) {
	auto changed = false;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::basic_string<CharT>::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
		changed = true;
	}
	return changed;
}

} // namespace detail

template <typename CharT, typename T>
inline bool extract(const std::basic_string<CharT> & value, T & dst) {
	CharT c;
	std::basic_istringstream<CharT> is{ value };
	T result;
	if ((is >> std::boolalpha >> result) && !(is >> c)) {
		dst = result;
		return true;
	}
	else {
		return false;
	}
}

template <typename CharT>
inline bool extract(const std::basic_string<CharT> & value, std::basic_string<CharT> & dst) {
	dst = value;
	return true;
}

template <typename CharT, typename T>
inline bool get_value(const std::map<std::basic_string<CharT>, std::basic_string<CharT>> & sec, const std::basic_string<CharT> & key, T & dst) {
	const auto it = sec.find(key);
	if (it == sec.end()) return false;
	return extract(it->second, dst);
}

template <typename CharT, typename T>
inline bool get_value(const std::map<std::basic_string<CharT>, std::basic_string<CharT>>& sec, const CharT* key, T& dst) {
	return get_value(sec, std::basic_string<CharT>(key), dst);
}

template<class CharT>
class Format
{
public:
	// used for generating
	const CharT char_section_start;
	const CharT char_section_end;
	const CharT char_assign;
	const CharT char_comment;

	// used for parsing
	virtual bool is_section_start(CharT ch) const { return ch == char_section_start; }
	virtual bool is_section_end(CharT ch) const { return ch == char_section_end; }
	virtual bool is_assign(CharT ch) const { return ch == char_assign; }
	virtual bool is_comment(CharT ch) const { return ch == char_comment; }

	// used for interpolation
	const CharT char_interpol;
	const CharT char_interpol_start;
	const CharT char_interpol_sep;
	const CharT char_interpol_end;

	Format(CharT section_start, CharT section_end, CharT assign, CharT comment, CharT interpol, CharT interpol_start, CharT interpol_sep, CharT interpol_end)
		: char_section_start(section_start)
		, char_section_end(section_end)
		, char_assign(assign)
		, char_comment(comment)
		, char_interpol(interpol)
		, char_interpol_start(interpol_start)
		, char_interpol_sep(interpol_sep)
		, char_interpol_end(interpol_end) {}

	Format() : Format('[', ']', '=', ';', '$', '{', ':', '}') {}

	const std::basic_string<CharT> local_symbol(const std::basic_string<CharT>& name) const {
		return char_interpol + (char_interpol_start + name + char_interpol_end);
	}

	const std::basic_string<CharT> global_symbol(const std::basic_string<CharT>& sec_name, const std::basic_string<CharT>& name) const {
		return local_symbol(sec_name + char_interpol_sep + name);
	}
};

template<class CharT>
class Ini
{
public:
	using String = std::basic_string<CharT>;
	using Section = std::map<String, String>;
	using Sections = std::map<String, Section>;

	Sections sections;
	std::list<String> errors;
	std::shared_ptr<Format<CharT>> format;

	static const int max_interpolation_depth = 10;

	Ini() : format(std::make_shared<Format<CharT>>()) {};
	Ini(std::shared_ptr<Format<CharT>> fmt) : format(fmt) {};

	void generate(std::basic_ostream<CharT>& os) const {
		for (auto const & sec : sections) {
			os << format->char_section_start << sec.first << format->char_section_end << std::endl;
			for (auto const & val : sec.second) {
				os << val.first << format->char_assign << val.second << std::endl;
			}
			os << std::endl;
		}
	}

	void parse(std::basic_istream<CharT> & is) {
		String line;
		String section;
		const std::locale loc{"C"};
		while (std::getline(is, line)) {
			detail::ltrim(line, loc);
			detail::rtrim(line, loc);
			const auto length = line.length();
			if (length > 0) {
				const auto pos = std::find_if(line.begin(), line.end(), [this](CharT ch) { return format->is_assign(ch); });
				const auto & front = line.front();
				if (format->is_comment(front)) {
				}
				else if (format->is_section_start(front)) {
					if (format->is_section_end(line.back()))
						section = line.substr(1, length - 2);
					else
						errors.push_back(line);
				}
				else if (pos != line.begin() && pos != line.end()) {
					String variable(line.begin(), pos);
					String value(pos + 1, line.end());
					detail::rtrim(variable, loc);
					detail::ltrim(value, loc);
					auto & sec = sections[section];
					if (sec.find(variable) == sec.end())
						sec.emplace(variable, value);
					else
						errors.push_back(line);
				}
				else {
					errors.push_back(line);
				}
			}
		}
	}

	void interpolate() {
		int global_iteration = 0;
		auto changed = false;
		// replace each "${variable}" by "${section:variable}"
		for (auto & sec : sections)
			replace_symbols(local_symbols(sec.first, sec.second), sec.second);
		// replace each "${section:variable}" by its value
		do {
			changed = false;
			const auto syms = global_symbols();
			for (auto & sec : sections)
				changed |= replace_symbols(syms, sec.second);
		} while (changed && (max_interpolation_depth > global_iteration++));
	}

	void default_section(const Section & sec) {
		for (auto & sec2 : sections)
			for (const auto & val : sec)
				sec2.second.insert(val);
	}

	void strip_trailing_comments() {
		const std::locale loc{ "C" };
		for (auto & sec : sections)
			for (auto & val : sec.second) {
				detail::rtrim2(val.second, [this](CharT ch) { return format->is_comment(ch); });
				detail::rtrim(val.second, loc);
			}
	}

	void clear() {
		sections.clear();
		errors.clear();
	}

private:
	using Symbols = std::vector<std::pair<String, String>>;

	const Symbols local_symbols(const String & sec_name, const Section & sec) const {
		Symbols result;
		for (const auto & val : sec)
			result.emplace_back(format->local_symbol(val.first), format->global_symbol(sec_name, val.first));
		return result;
	}

	const Symbols global_symbols() const {
		Symbols result;
		for (const auto & sec : sections)
			for (const auto & val : sec.second)
				result.emplace_back(format->global_symbol(sec.first, val.first), val.second);
		return result;
	}

	bool replace_symbols(const Symbols & syms, Section & sec) const {
		auto changed = false;
		for (auto & sym : syms)
			for (auto & val : sec)
				changed |= detail::replace(val.second, sym.first, sym.second);
		return changed;
	}
};

} // namespace inipp
