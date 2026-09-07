// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "IniFile.h"
#include "FileUtil.h"
#include "StringUtil.h"

#include <optional>
#include <vector>

namespace {

enum class DetectedEncoding
{
	ANSI,
	UTF16,
	Unreadable
};

DetectedEncoding DetectEncoding(const BYTE* data, size_t size)
{
	// A UTF-8 BOM is deliberately not recognized, so those three bytes go through the ANSI
	// codepage like any others and corrupt the first line of the file, exactly as the profile API
	// leaves it.
	if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) return DetectedEncoding::Unreadable;

	// An odd byte count leaves half a code unit dangling, and the file is rejected outright.
	if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
	{
		return (size % 2 == 0) ? DetectedEncoding::UTF16 : DetectedEncoding::Unreadable;
	}

	// UTF-16LE is detected by content too, so a file without a BOM reads exactly like one with it.
	// Both tests below need a 0x00 byte to pass, and ANSI text has none, so neither can claim an
	// ANSI file. IS_TEXT_UNICODE_STATISTICS is left out because it decides from byte frequency
	// alone and does say yes to some plain ASCII. A file wrongly taken for UTF-16 decodes to
	// garbage, which means no sections, no keys, and nothing in the log to say why.
	int flags = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
	if (size >= 2 && IsTextUnicode(data, (int)std::min<size_t>(size, INT_MAX), &flags))
	{
		return (size % 2 == 0) ? DetectedEncoding::UTF16 : DetectedEncoding::Unreadable;
	}

	return DetectedEncoding::ANSI;
}

std::unique_ptr<WCHAR[]> Decode(const BYTE* data, size_t size, size_t& textLength, IniFile::Encoding& fileEncoding, bool& hasBom)
{
	textLength = 0;
	fileEncoding = IniFile::Encoding::ANSI;
	hasBom = false;
	if (!data || size == 0) return nullptr;

	const DetectedEncoding encoding = DetectEncoding(data, size);
	if (encoding == DetectedEncoding::Unreadable) return nullptr;

	if (encoding == DetectedEncoding::UTF16)
	{
		hasBom = data[0] == 0xFF && data[1] == 0xFE;
		fileEncoding = IniFile::Encoding::UTF16;
		const size_t offset = hasBom ? 2 : 0;
		const size_t count = (size - offset) / 2;
		if (count == 0) return nullptr;

		std::unique_ptr<WCHAR[]> text(new (std::nothrow) WCHAR[count]);
		if (!text) return nullptr;

		memcpy(text.get(), data + offset, count * sizeof(WCHAR));
		textLength = count;
		return text;
	}

	// No MB_ERR_INVALID_CHARS, which is what the API does: the bytes CP1252 leaves undefined
	// become C1 controls rather than U+FFFD. No codepage can produce more characters than it is
	// given bytes, so one pass into a buffer of |size| is both safe and enough.
	std::unique_ptr<WCHAR[]> text(new (std::nothrow) WCHAR[size]);
	if (!text) return nullptr;

	const int count = MultiByteToWideChar(CP_ACP, 0, (const char*)data, (int)size, text.get(), (int)size);
	if (count <= 0) return nullptr;

	textLength = (size_t)count;
	return text;
}

struct Document
{
	std::wstring text;
	IniFile::Encoding encoding = IniFile::Encoding::ANSI;
	bool hasBom = false;
};

struct Line
{
	size_t begin;
	size_t contentEnd;
	size_t end;
};

struct Section
{
	size_t header;
	size_t end;
	size_t separator;
};

bool ContainsNull(std::wstring_view value)
{
	return value.find(L'\0') != std::wstring_view::npos;
}

std::wstring_view TrimRequest(std::wstring_view value)
{
	// The profile APIs trim only spaces from names supplied by the caller, even though names in
	// the file are trimmed of every character at or below 0x20.
	size_t begin = 0;
	while (begin < value.length() && value[begin] == L' ') ++begin;

	size_t end = value.length();
	while (end > begin && value[end - 1] == L' ') --end;

	return value.substr(begin, end - begin);
}

bool EqualsNoCase(std::wstring_view left, std::wstring_view right)
{
	if (left.empty() || right.empty()) return left.empty() && right.empty();
	if (left.length() > INT_MAX || right.length() > INT_MAX) return false;

	return CompareStringOrdinal(
		left.data(), (int)left.length(), right.data(), (int)right.length(), TRUE) == CSTR_EQUAL;
}

std::vector<Line> ParseLines(const std::wstring& text)
{
	std::vector<Line> lines;

	size_t pos = 0;
	while (pos < text.length())
	{
		size_t lineEnd = text.find_first_of(L"\r\n", pos);
		if (lineEnd == std::wstring::npos) lineEnd = text.length();

		size_t next = lineEnd;
		if (next < text.length())
		{
			++next;
			if (text[lineEnd] == L'\r' && next < text.length() && text[next] == L'\n') ++next;
		}

		lines.push_back({ pos, lineEnd, next });
		pos = next;
	}

	return lines;
}

std::wstring_view GetLine(const std::wstring& text, const Line& line)
{
	return std::wstring_view(text).substr(line.begin, line.contentEnd - line.begin);
}

std::optional<Section> FindSection(const std::wstring& text, const std::vector<Line>& lines, std::wstring_view requested)
{
	requested = TrimRequest(requested);

	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::wstring_view name;
		if (!IniFile::ParseSection(GetLine(text, lines[i]), name) || !EqualsNoCase(name, requested)) continue;

		size_t end = i + 1;
		for (; end < lines.size(); ++end)
		{
			std::wstring_view nextName;
			if (IniFile::ParseSection(GetLine(text, lines[end]), nextName)) break;
		}

		// Blank lines at the end are separators rather than section contents. Keeping them outside
		// the edit range is why deleting or replacing a section leaves those lines behind.
		size_t separator = end;
		while (separator > i + 1 && IniFile::Trim(GetLine(text, lines[separator - 1])).empty()) --separator;

		return Section{ i, end, separator };
	}

	return std::nullopt;
}

std::optional<size_t> FindKey(const std::wstring& text, const std::vector<Line>& lines, const Section& section, std::wstring_view requested)
{
	requested = TrimRequest(requested);

	for (size_t i = section.header + 1; i < section.end; ++i)
	{
		const std::wstring_view line = IniFile::Trim(GetLine(text, lines[i]));
		if (line.empty() || line[0] == L';') continue;

		std::wstring_view key;
		std::wstring_view value;
		if (IniFile::ParseKeyValue(line, key, value) && EqualsNoCase(key, requested)) return i;
	}

	return std::nullopt;
}

bool ReadBytes(const std::wstring& path, std::vector<BYTE>& bytes)
{
	HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(file, &fileSize))
	{
		CloseHandle(file);
		return false;
	}
	if (fileSize.QuadPart < 0 || (uint64_t)fileSize.QuadPart > SIZE_MAX)
	{
		CloseHandle(file);
		return false;
	}

	bytes.resize((size_t)fileSize.QuadPart);
	size_t offset = 0;
	while (offset < bytes.size())
	{
		const DWORD count = (DWORD)std::min<size_t>(bytes.size() - offset, MAXDWORD);
		DWORD read = 0;
		if (!ReadFile(file, bytes.data() + offset, count, &read, nullptr) || read != count)
		{
			CloseHandle(file);
			return false;
		}
		offset += read;
	}

	CloseHandle(file);
	return true;
}

bool LoadDocument(const std::wstring& path, Document& document)
{
	const DWORD attributes = GetFileAttributes(path.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		const DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return true;
		return false;
	}
	if (attributes & FILE_ATTRIBUTE_DIRECTORY) return false;

	std::vector<BYTE> bytes;
	if (!ReadBytes(path, bytes)) return false;

	IniFile::DecodedText decoded = IniFile::DecodedText::FromMemory(bytes.data(), bytes.size());

	// A UTF-16 BOM by itself is a valid empty UTF-16 file, not a decoding failure.
	const bool emptyUtf16 = bytes.size() == 2 && decoded.GetEncoding() == IniFile::Encoding::UTF16 && decoded.HasBom();
	if (!bytes.empty() && decoded.IsEmpty() && !emptyUtf16)
	{
		return false;
	}

	document.text.assign(decoded.GetText());
	document.encoding = decoded.GetEncoding();
	document.hasBom = decoded.HasBom();
	return true;
}

bool Encode(const Document& document, std::vector<BYTE>& bytes)
{
	if (document.encoding == IniFile::Encoding::UTF16)
	{
		const size_t bomSize = document.hasBom ? 2 : 0;
		if (document.text.length() > (SIZE_MAX - bomSize) / sizeof(WCHAR))
		{
			return false;
		}

		bytes.resize(bomSize + document.text.length() * sizeof(WCHAR));
		if (document.hasBom)
		{
			bytes[0] = 0xFF;
			bytes[1] = 0xFE;
		}
		if (!document.text.empty()) memcpy(bytes.data() + bomSize, document.text.data(), document.text.length() * sizeof(WCHAR));
		return true;
	}

	if (document.text.length() > INT_MAX)
	{
		return false;
	}

	// This is intentionally lossy. The profile writer treats every non-UTF-16 file as ANSI and
	// substitutes characters that the active codepage cannot represent.
	const int size = WideCharToMultiByte(CP_ACP, 0, document.text.data(), (int)document.text.length(), nullptr, 0, nullptr, nullptr);
	if (size == 0 && !document.text.empty()) return false;

	bytes.resize((size_t)size);
	if (size != 0 && WideCharToMultiByte(CP_ACP, 0, document.text.data(), (int)document.text.length(), (char*)bytes.data(), size, nullptr, nullptr) == 0)
	{
		return false;
	}

	return true;
}

bool WriteBytes(const std::wstring& path, const std::vector<BYTE>& bytes)
{
	HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;

	size_t offset = 0;
	while (offset < bytes.size())
	{
		const DWORD count = (DWORD)std::min<size_t>(bytes.size() - offset, MAXDWORD);
		DWORD written = 0;
		if (!WriteFile(file, bytes.data() + offset, count, &written, nullptr) || written != count)
		{
			CloseHandle(file);
			return false;
		}
		offset += written;
	}

	if (!CloseHandle(file)) return false;
	return true;
}

bool SaveDocument(const std::wstring& path, const Document& document)
{
	std::vector<BYTE> bytes;
	if (!Encode(document, bytes)) return false;
	if (!WriteBytes(path, bytes)) return false;

	return true;
}

void AppendLine(std::wstring& text, size_t offset, std::wstring_view line)
{
	std::wstring inserted;
	if (offset > 0 && text[offset - 1] != L'\r' && text[offset - 1] != L'\n') inserted = L"\r\n";
	inserted.append(line);
	inserted += L"\r\n";
	text.insert(offset, inserted);
}

void AppendSection(std::wstring& text, std::wstring_view section, const std::vector<std::wstring_view>& lines)
{
	// The writer terminates an unfinished final line, but does not add a blank line before the new
	// section.
	if (!text.empty() && text.back() != L'\r' && text.back() != L'\n') text += L"\r\n";

	text += L'[';
	text.append(section);
	text += L"]\r\n";
	for (std::wstring_view line : lines)
	{
		text.append(line);
		text += L"\r\n";
	}
}

bool Validate(std::wstring_view value)
{
	return !ContainsNull(value);
}

bool ApplyWriteKey(Document& document, std::wstring_view section, std::wstring_view key, std::wstring_view value)
{
	const std::vector<Line> lines = ParseLines(document.text);
	const std::optional<Section> foundSection = FindSection(document.text, lines, section);
	if (!foundSection)
	{
		std::wstring entry(key);
		entry += L'=';
		entry.append(value);
		AppendSection(document.text, section, { std::wstring_view(entry) });
		return true;
	}

	const std::optional<size_t> foundKey = FindKey(document.text, lines, *foundSection, key);
	if (foundKey)
	{
		const Line& line = lines[*foundKey];
		const size_t equals = GetLine(document.text, line).find(L'=');
		std::wstring replacement(value);
		replacement += L"\r\n";

		// Replacing through the old terminator discards everything after '=' and gives the edited
		// line a CRLF, even when the rest of the file uses another line ending.
		document.text.replace(line.begin + equals + 1, line.end - line.begin - equals - 1, replacement);
	}
	else
	{
		const size_t offset = foundSection->separator < lines.size() ? lines[foundSection->separator].begin : document.text.length();
		std::wstring entry(key);
		entry += L'=';
		entry.append(value);
		AppendLine(document.text, offset, entry);
	}

	return true;
}

bool ApplyDeleteKey(Document& document, std::wstring_view section, std::wstring_view key)
{
	const std::vector<Line> lines = ParseLines(document.text);
	const std::optional<Section> foundSection = FindSection(document.text, lines, section);
	if (!foundSection) return false;

	const std::optional<size_t> foundKey = FindKey(document.text, lines, *foundSection, key);
	if (!foundKey) return false;

	const Line& line = lines[*foundKey];
	document.text.erase(line.begin, line.end - line.begin);
	return true;
}

bool ApplyDeleteSection(Document& document, std::wstring_view section)
{
	const std::vector<Line> lines = ParseLines(document.text);
	const std::optional<Section> foundSection = FindSection(document.text, lines, section);
	if (!foundSection) return false;

	const size_t begin = lines[foundSection->header].begin;
	const size_t end = foundSection->separator < lines.size() ? lines[foundSection->separator].begin : document.text.length();
	document.text.erase(begin, end - begin);
	return true;
}

bool ApplyWriteSection(Document& document, std::wstring_view section, const std::vector<std::wstring>& entries)
{
	std::vector<std::wstring_view> entryViews;
	entryViews.reserve(entries.size());
	for (const std::wstring& entry : entries) entryViews.emplace_back(entry);

	const std::vector<Line> lines = ParseLines(document.text);
	const std::optional<Section> foundSection = FindSection(document.text, lines, section);
	if (!foundSection)
	{
		AppendSection(document.text, section, entryViews);
		return true;
	}

	const Line& header = lines[foundSection->header];
	const size_t end = foundSection->separator < lines.size() ? lines[foundSection->separator].begin : document.text.length();
	std::wstring replacement;
	if (header.end == header.contentEnd) replacement = L"\r\n";
	for (std::wstring_view entry : entryViews)
	{
		replacement.append(entry);
		replacement += L"\r\n";
	}
	document.text.replace(header.end, end - header.end, replacement);
	return true;
}

}  // namespace

namespace IniFile {

DecodedText DecodedText::FromMemory(const BYTE* data, size_t size)
{
	size_t length = 0;
	Encoding encoding = Encoding::ANSI;
	bool hasBom = false;
	std::unique_ptr<WCHAR[]> text = Decode(data, size, length, encoding, hasBom);
	return DecodedText(std::move(text), length, encoding, hasBom);
}

std::optional<DecodedText> ReadFileText(const std::wstring& path)
{
	size_t size = 0;
	const std::unique_ptr<BYTE[]> data = FileUtil::ReadFullFile(path, &size);
	if (!data) return std::nullopt;

	DecodedText text = DecodedText::FromMemory(data.get(), size);

	// A file with bytes in it that decodes to nothing was turned down by the encoding detection:
	// UTF-16BE, or an odd byte count. Report it rather than hand back empty text, which the caller
	// cannot tell apart from an empty file.
	if (size != 0 && text.IsEmpty()) return std::nullopt;

	return std::optional<DecodedText>(std::move(text));
}

// Whitespace is every character at or below 0x20 and nothing else, so U+00A0 and U+3000 survive.
std::wstring_view Trim(std::wstring_view str)
{
	size_t begin = 0;
	while (begin < str.length() && str[begin] <= L' ') ++begin;

	size_t end = str.length();
	while (end > begin && str[end - 1] <= L' ') --end;

	return str.substr(begin, end - begin);
}

bool ParseSection(std::wstring_view line, std::wstring_view& name)
{
	line = Trim(line);
	if (line.empty() || line[0] != L'[') return false;

	name = line.substr(1);
	const size_t close = name.find(L']');
	if (close != std::wstring_view::npos) name = name.substr(0, close);
	name = Trim(name);
	return true;
}

bool ParseKeyValue(std::wstring_view line, std::wstring_view& key, std::wstring_view& value)
{
	line = Trim(line);
	const size_t equals = line.find(L'=');
	if (equals == std::wstring_view::npos) return false;

	key = Trim(line.substr(0, equals));
	value = Trim(line.substr(equals + 1));
	return true;
}

// One surrounding pair of double or single quotes, removed from every value.
std::wstring_view DecodedText::StripQuotes(std::wstring_view value)
{
	return StringUtil::StripLeadingAndTrailingQuotes(value, true);
}

struct Writer::Impl
{
	std::wstring path;
	Document document;
	bool valid = false;
	bool changed = false;
};

Writer::Writer(const std::wstring& path) : m_Impl(std::make_unique<Impl>())
{
	m_Impl->path = path;
	m_Impl->valid = LoadDocument(path, m_Impl->document);
}

Writer::~Writer() = default;

bool Writer::IsValid() const
{
	return m_Impl->valid;
}

void Writer::WriteKey(std::wstring_view section, std::wstring_view key, std::wstring_view value)
{
	if (!m_Impl->valid) return;
	if (!Validate(section) || !Validate(key) || !Validate(value))
	{
		m_Impl->valid = false;
		return;
	}

	if (ApplyWriteKey(m_Impl->document, section, key, value)) m_Impl->changed = true;
}

void Writer::DeleteKey(std::wstring_view section, std::wstring_view key)
{
	if (!m_Impl->valid) return;
	if (!Validate(section) || !Validate(key))
	{
		m_Impl->valid = false;
		return;
	}

	if (ApplyDeleteKey(m_Impl->document, section, key)) m_Impl->changed = true;
}

void Writer::DeleteSection(std::wstring_view section)
{
	if (!m_Impl->valid) return;
	if (!Validate(section))
	{
		m_Impl->valid = false;
		return;
	}

	if (ApplyDeleteSection(m_Impl->document, section)) m_Impl->changed = true;
}

void Writer::WriteSection(std::wstring_view section, const std::vector<std::wstring>& entries)
{
	if (!m_Impl->valid) return;
	if (!Validate(section))
	{
		m_Impl->valid = false;
		return;
	}

	for (const std::wstring& entry : entries)
	{
		if (!Validate(entry))
		{
			m_Impl->valid = false;
			return;
		}
	}

	if (ApplyWriteSection(m_Impl->document, section, entries)) m_Impl->changed = true;
}

bool Writer::Save()
{
	if (!m_Impl->valid) return false;
	if (!m_Impl->changed) return true;
	if (!SaveDocument(m_Impl->path, m_Impl->document)) return false;

	m_Impl->changed = false;
	return true;
}

bool WriteKey(const std::wstring& path, std::wstring_view section, std::wstring_view key, std::wstring_view value)
{
	Writer ini(path);
	ini.WriteKey(section, key, value);
	return ini.Save();
}

bool DeleteKey(const std::wstring& path, std::wstring_view section, std::wstring_view key)
{
	Writer ini(path);
	ini.DeleteKey(section, key);
	return ini.Save();
}

bool DeleteSection(const std::wstring& path, std::wstring_view section)
{
	Writer ini(path);
	ini.DeleteSection(section);
	return ini.Save();
}

bool WriteSection(const std::wstring& path, std::wstring_view section, const std::vector<std::wstring>& entries)
{
	Writer ini(path);
	ini.WriteSection(section, entries);
	return ini.Save();
}

}  // namespace IniFile
