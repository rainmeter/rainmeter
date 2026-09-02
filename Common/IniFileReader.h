// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// A read-only .ini reader that reproduces the observable behavior of GetPrivateProfileString and
// its relatives, including the ugly parts, because skins depend on them.
//
// Docs/ProfileApiBehavior.md is the specification and the evidence behind every rule here.
//
// Decoding a file and walking it are separate steps, so that a caller needing more than one pass
// -- ConfigParser needs the section names before it reads any keys -- pays for the decoding once.
// Nothing is stored and no string is copied: the views handed to the callbacks point into the
// decoded text and are only valid for the duration of the call.
//
// Where it deliberately differs from the profile API:
// - Lines are never truncated. The profile API silently cuts very long ones at a position that
//   depends on the contents of the file.
// - The path is used as given, instead of being resolved against the Windows directory.
// - Nothing is looked up by name, so first-match-wins for duplicate sections and keys is the
//   caller's to apply: the callbacks see every occurrence.
// - A line with no '=' is dropped and a value always has its quotes stripped, which is what
//   GetPrivateProfileString does. GetPrivateProfileSection disagrees on both, and nothing needs
//   the value that way.
namespace IniFileReader {

class DecodedText
{
public:
	static DecodedText FromMemory(const BYTE* data, size_t size);

	DecodedText(DecodedText&&) = default;
	DecodedText& operator=(DecodedText&&) = default;

	bool IsEmpty() const { return m_Length == 0; }

	// Calls |onSection| for each "[Section]" line and |onKeyValue| for each "key=value" line of
	// the section that opened last, in file order. Walking the text again costs another scan of
	// it and nothing else, since the decoding already happened.
	//
	// Every occurrence of a section is reported, the way GetPrivateProfileSectionNames lists a
	// name once per occurrence. Only the first one is readable through the profile API, so a
	// caller that wants to match it has to ignore the entries under the later ones.
	template<typename SectionFunc, typename KeyValueFunc = std::nullptr_t>
	void Parse(SectionFunc&& onSection, KeyValueFunc&& onKeyValue = nullptr) const
	{
		constexpr bool wantsKeyValues = !std::is_null_pointer_v<std::decay_t<KeyValueFunc>>;

		const std::wstring_view text(m_Text.get(), m_Length);

		bool inSection = false;
		size_t pos = 0;
		while (pos < text.length())
		{
			size_t lineEnd = text.find_first_of(L"\r\n", pos);
			if (lineEnd == std::wstring_view::npos) lineEnd = text.length();

			std::wstring_view line = Trim(text.substr(pos, lineEnd - pos));

			// CR, LF and CRLF all end a line and can be mixed freely within one file. 0x1A is not
			// an end of file marker, and U+2028 and U+0085 are not line breaks.
			pos = lineEnd + 1;
			if (pos < text.length() && text[lineEnd] == L'\r' && text[pos] == L'\n') ++pos;

			// A ';' comment line is dropped completely, and '#' is not a comment character at all.
			if (line.empty() || line[0] == L';') continue;

			if (line[0] == L'[')
			{
				// The name runs to the first ']' or to the end of the line, and everything after
				// the ']' is discarded, which is why a trailing comment works on a header line.
				std::wstring_view name = line.substr(1);
				const size_t close = name.find(L']');
				if (close != std::wstring_view::npos) name = name.substr(0, close);

				inSection = true;
				onSection(Trim(name));
				continue;
			}

			if constexpr (wantsKeyValues)
			{
				// Keys before the first section header are unreachable through every profile API.
				// Note that an explicit "[]" header does open a usable section with an empty name.
				if (!inSection) continue;

				const size_t equals = line.find(L'=');
				if (equals == std::wstring_view::npos) continue;

				// The split is at the first '=', so "Multi=a=b=c" has the value "a=b=c".
				onKeyValue(Trim(line.substr(0, equals)), StripQuotes(Trim(line.substr(equals + 1))));
			}
		}
	}

private:
	DecodedText(std::unique_ptr<WCHAR[]> text, size_t length) : m_Text(std::move(text)), m_Length(length) {}

	static std::wstring_view Trim(std::wstring_view str);
	static std::wstring_view StripQuotes(std::wstring_view value);

	std::unique_ptr<WCHAR[]> m_Text;
	size_t m_Length;
};

std::optional<DecodedText> DecodeFile(const std::wstring& path);

}  // namespace IniFileReader
