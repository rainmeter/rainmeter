// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Map.h"

// IniFile keeps the Win32 private-profile behavior that skins depend on, with these deliberate
// differences:
// - A UTF-8 BOM or strictly valid UTF-8 content is read and written as UTF-8. The profile APIs
//   instead treat UTF-8 as ANSI, so a BOM spoils the first line and writes can mix in ACP bytes.
// - Lines are never truncated. The profile APIs silently cut very long lines at a position that
//   depends on the file contents.
// - Paths are used as given instead of being resolved against the Windows directory or through
//   IniFileMapping.
// - UTF-16LE without a BOM is detected only when NUL bytes appear where UTF-16 puts them. The
//   byte-frequency heuristic in IsTextUnicode is deliberately excluded because it accepts some
//   plain ASCII files too.
// - Section reads drop lines without '=' and always strip one matching pair of quotes from values.
//   This follows GetPrivateProfileString; GetPrivateProfileSection disagrees on both behaviors.
// - DecodedText::Parse reports every duplicate section and key. The higher-level lookup helpers
//   still use only the first match, like the profile APIs.
//
// The compatible behavior includes:
// - UTF-16LE is recognized with or without a BOM; UTF-16BE and odd-sized UTF-16LE input are
//   rejected. BOMless input that is neither UTF-16LE nor strictly valid UTF-8 uses the process
//   ANSI codepage.
// - CR, LF and CRLF end lines.
// - Names and values are trimmed only while their characters are at or below U+0020.
// - A section starts at the first non-whitespace '[' and ends at the first ']' or the end of the
//   line.
// - A key/value line splits at its first '='.
// - Only ';' as the first non-whitespace character starts a comment. '#' and trailing ';' are
//   ordinary text.
// - Keys before the first section are unreachable.
// - Values lose one matching pair of surrounding single or double quotes after trimming.
// - Lookups are case-insensitive and use the first matching section and key. Enumeration keeps
//   file order and original spelling.
// - Writing preserves an existing file's ANSI, UTF-8 or UTF-16LE encoding. A missing file is
//   created as ANSI without a BOM.
// - Writes update only the first match, preserve untouched text, append new keys and sections, and
//   emit changed lines with CRLF.
// - Replacing a value keeps the text before '=' and replaces everything after it.
// - Deleting a key removes its line; deleting a section removes its header and contents.
//
// Decoding a file and walking it are separate steps, so that a caller needing more than one pass
// -- ConfigParser needs the section names before it reads any keys -- pays for the decoding once.
// Nothing is stored and no string is copied: the views handed to the callbacks point into the
// decoded text and are only valid for the duration of the call.
namespace IniFile {

using OrderedSection = std::vector<std::pair<std::wstring, std::wstring>>;

class Section
{
public:
	std::optional<std::wstring_view> GetKey(std::wstring_view key) const;
	std::wstring GetKey(std::wstring_view key, std::wstring_view defaultValue) const;
	int GetIntKey(std::wstring_view key, int defaultValue) const;
	bool IsEmpty() const { return m_Values.empty(); }

private:
	StringMap<std::wstring> m_Values;

	friend Section ReadSection(const std::wstring& path, std::wstring_view section);
};

enum class Encoding
{
	ANSI,
	UTF8,
	UTF16
};

// Reading and writing use these so both paths recognize the same malformed headers and keys.
std::wstring_view Trim(std::wstring_view str);
bool ParseSection(std::wstring_view line, std::wstring_view& name);
bool ParseKeyValue(std::wstring_view line, std::wstring_view& key, std::wstring_view& value);

class DecodedText
{
public:
	static DecodedText FromMemory(const BYTE* data, size_t size);

	DecodedText(DecodedText&&) = default;
	DecodedText& operator=(DecodedText&&) = default;

	bool IsEmpty() const { return m_Length == 0; }
	std::wstring_view GetText() const { return m_Length == 0 ? std::wstring_view() : std::wstring_view(m_Text.get(), m_Length); }
	Encoding GetEncoding() const { return m_Encoding; }
	bool HasBom() const { return m_HasBom; }

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

		const std::wstring_view text = GetText();

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

			std::wstring_view section;
			if (ParseSection(line, section))
			{
				inSection = true;
				onSection(section);
				continue;
			}

			if constexpr (wantsKeyValues)
			{
				// Keys before the first section header are unreachable through every profile API.
				// Note that an explicit "[]" header does open a usable section with an empty name.
				if (!inSection) continue;

				std::wstring_view key;
				std::wstring_view value;
				if (ParseKeyValue(line, key, value)) onKeyValue(key, StripQuotes(value));
			}
		}
	}

private:
	DecodedText(std::unique_ptr<WCHAR[]> text, size_t length, Encoding encoding, bool hasBom) :
		m_Text(std::move(text)),
		m_Length(length),
		m_Encoding(encoding),
		m_HasBom(hasBom)
	{
	}

	static std::wstring_view StripQuotes(std::wstring_view value);

	std::unique_ptr<WCHAR[]> m_Text;
	size_t m_Length;
	Encoding m_Encoding;
	bool m_HasBom;
};

// Returns nothing if the file cannot be opened, or if it has content that no encoding this
// understands can decode, so that the caller can report it instead of reading an empty file.
std::optional<DecodedText> ReadFileText(const std::wstring& path);

// Reads the first matching section, stores its keys in uppercase and keeps the first duplicate key.
Section ReadSection(const std::wstring& path, std::wstring_view section);

// Reads every key and value from the first matching section in file order, including duplicate keys.
OrderedSection ReadSectionInOrder(const std::wstring& path, std::wstring_view section);

// Reads the first case-insensitive occurrence of each section name in file order.
std::vector<std::wstring> ReadSectionNames(const std::wstring& path);

// Reads the first matching key from the first matching section. Returns nothing if the file cannot
// be read or the section or key is missing.
std::optional<std::wstring> ReadKey(const std::wstring& path, std::wstring_view section, std::wstring_view key);

// Returns defaultValue if the file cannot be read or the section or key is missing. An empty value
// in the file is returned as an empty string.
std::wstring ReadKey(const std::wstring& path, std::wstring_view section, std::wstring_view key, std::wstring_view defaultValue);

// Returns defaultValue if the file cannot be read or the section or key is missing. An empty or
// nonnumeric value in the file returns zero.
int ReadIntKey(const std::wstring& path, std::wstring_view section, std::wstring_view key, int defaultValue);

// Edits .ini files with the compatibility behavior summarized above.
//
// Changes stay in memory until Save is called. Destroying a Writer does not save it, because a
// destructor could not report a write failure to the caller.
class Writer
{
public:
	explicit Writer(const std::wstring& path);
	~Writer();

	Writer(const Writer&) = delete;
	Writer& operator=(const Writer&) = delete;

	bool IsValid() const;
	void WriteKey(std::wstring_view section, std::wstring_view key, std::wstring_view value);
	void DeleteKey(std::wstring_view section, std::wstring_view key);
	void DeleteSection(std::wstring_view section);
	void WriteSection(std::wstring_view section, const std::vector<std::wstring>& lines);
	bool Save();

private:
	struct Impl;
	std::unique_ptr<Impl> m_Impl;
};

bool WriteKey(const std::wstring& path, std::wstring_view section, std::wstring_view key, std::wstring_view value);
bool DeleteKey(const std::wstring& path, std::wstring_view section, std::wstring_view key);
bool DeleteSection(const std::wstring& path, std::wstring_view section);
bool WriteSection(const std::wstring& path, std::wstring_view section, const std::vector<std::wstring>& lines);

}  // namespace IniFile
