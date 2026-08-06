# UTF-8 module for Lua 5.x

[![Build Status](https://img.shields.io/github/actions/workflow/status/starwing/luautf8/test.yml?branch=master)](https://github.com/starwing/luautf8/actions?query=branch%3Amaster)[![Coverage Status](https://img.shields.io/coveralls/github/starwing/luautf8)](https://coveralls.io/github/starwing/luautf8?branch=master)

This module adds UTF-8 support to Lua.

It uses data extracted from the
[Unicode Character Database](http://www.unicode.org/reports/tr44/),
and is tested on Lua 5.2.3, Lua 5.3.0, Lua 5.4.8 and LuaJIT.

`parseucd.lua` is a pure Lua script that generates `unidata.h` to support
character conversion and category checking.

It is compatible with Lua's own string module and passes all
string and pattern matching tests in the Lua test suite[2].

It also adds some useful routines for UTF-8 features, such as:
- A convenient interface to escape Unicode sequences in strings
- String insert/remove operations, since UTF-8 substring extraction may be expensive
- Unicode width calculation, useful when implementing e.g. console emulators
- A useful interface to translate between Unicode character positions and byte offsets
- Checking UTF-8 strings for validity and removing invalid byte sequences
- Converting Unicode strings to Normal Form C

Note that to avoid conflict with Lua 5.3+'s built-in library `utf8`,
this library produces a file like `lua-utf8.dll` or `lua-utf8.so`, so use
it like this:

```lua
local utf8 = require 'lua-utf8'
```

in your code :-(

[2]: http://www.lua.org/tests/5.2/


## LuaRocks Installation

`luarocks install luautf8`

It's now fully-compatible with Lua 5.3's utf8 library, so replacing this
file (and headers) with lutf8lib.c from the Lua 5.3 sources is also okay.

## Usage

Many routines are the same as Lua's string module:
- `utf8.byte`
- `utf8.char`
- `utf8.find`
- `utf8.gmatch`
- `utf8.gsub`
- `utf8.len`
- `utf8.lower`
- `utf8.match`
- `utf8.reverse`
- `utf8.sub`
- `utf8.upper`

The documentation of these functions can be found in the Lua manual[3].

[3]: https://www.lua.org/manual/5.4/manual.html#6.5


Some routines in the string module don't need Unicode support:
- `string.dump`
- `string.format`
- `string.rep`

They are NOT in the `lua-utf8` module.

Some routines are for compatibility with Lua's basic UTF-8 support library:
- `utf8.offset`
- `utf8.codepoint`
- `utf8.codes`

See the Lua manual[3] for usage.

Some routines are new, providing Unicode-specific functions:

### `utf8.escape(s)`

Escapes string `s` to UTF-8 format. Supports several escape formats:

- `%ddd` - decimal number of any length: converts Unicode code point to UTF-8 format
- `%{ddd}` - same as `%ddd` but with brackets
- `%uddd` - same as `%ddd`, 'u' stands for Unicode
- `%u{ddd}` - same as `%{ddd}`
- `%xhhh` - hexadecimal version of `%ddd`
- `%x{hhh}` - same as `%xhhh`
- `%?` - '?' stands for any other character: escapes this character

Returns the escaped UTF-8 string.

**Example:**
```lua
local u = utf8.escape
print(u"%123%u123%{123}%u{123}%xABC%x{ABC}")
print(u"%%123%?%d%%u")
```

### `utf8.charpos(s [[, i], n]) --> position, codepoint`

Converts UTF-8 character position `n` to byte offset `position`. If only `n` is given, returns the byte position where the encoding of the `n`-th character of `s` starts (counting from position 1). If both `i` and `n` are given, returns the byte position where the encoding of the `n`-th character after byte position `i` starts. A negative `n` gets characters before position `i`.

Also returns the code point at the resulting position.

This function assumes that `s` is a valid UTF-8 string.

**Example:**
```lua
local pos, code = utf8.charpos("你好world", 3)
-- pos=7 (byte position of 'w'), code=119 (code point of 'w')

local pos, code = utf8.charpos("你好world", 1, 2)
-- pos=4 (2 characters after byte 1), code=22909 (code point of '好')
```

### `utf8.next(s [, i [, n]]) --> position, codepoint`

Iterates through the UTF-8 string `s`. If only `s` is given, it can be used as an iterator:
```lua
for pos, code in utf8.next, "utf8-string" do
  -- ...
end
```

If `s` and `i` are given, returns the byte position of the next UTF-8 character in the string after byte position `i`. If `i` and `n` are given, returns the byte position of the `n`-th UTF-8 character after byte position `i`. A negative `n` gets characters before position `i`.

Also returns the code point at the resulting position.

This function assumes that `s` is a valid UTF-8 string.

**Example:**
```lua
local pos, code = utf8.next("你好", 1)
-- pos=4, code=22909 (next character after byte 1 is '好')

local pos, code = utf8.next("你好world", 1, 2)
-- pos=7, code=119 (2nd character after byte 1 is 'w')
```

### `utf8.insert(s [, n], substring) --> result`

Inserts `substring` into `s`. If `n` is given, inserts the substring before the `n`-th character of `s`; otherwise, `substring` will be appended to `s`. `n` can be negative.

Returns the resulting string.

**Example:**
```lua
local result = utf8.insert("你好", "world")
-- result = "你好world"

local result = utf8.insert("你好", 2, "!")
-- result = "你!好"
```

### `utf8.remove(s [, i [, j]]) --> result`

Deletes a substring from `s`. If neither `i` nor `j` is given, deletes the last UTF-8 character in `s`. If only `i` is given, deletes characters from position `i` to the end of `s`. If `j` is given, deletes characters from position `i` to `j` (both inclusive). `i` and `j` can be negative.

Returns the resulting string.

**Example:**
```lua
local result = utf8.remove("你好world")
-- result = "你好worl" (last character removed)

local result = utf8.remove("你好world", 2, 3)
-- result = "你rld" (characters 2-3 removed: '好w')
```

### `utf8.width(s [, i [, j [, ambiwidth [, default]]]]) --> width`

Calculates the display width of UTF-8 string `s` (or a substring from byte position `i` to `j`). The default for `i` is 1 and for `j` is `#s`. If `s` is a number, it is treated as a code point and the width of that code point is returned.

If `ambiwidth` is given, characters with ambiguous width (East Asian Ambiguous) will be treated as having width `ambiwidth` (1 or 2). The default for `ambiwidth` is 1.

If `default` is given, it will be used as the width for unprintable characters. The default for `default` is 0.

The width of fullwidth/doublewidth characters is 2, and the width of most other characters is 1.

Returns the total display width in columns.

**Example:**
```lua
utf8.width("hello")              -- 5
utf8.width("你好")                -- 4 (2 characters × 2 width each)
utf8.width("hello world", 1, 5)  -- 5 (substring "hello")
utf8.width(0x4E2D)               -- 2 (code point for '中')
```

### `utf8.widthindex(s, width [, i [, j [, ambiwidth [, default]]]]) --> idx, offset, width`

Returns the character index `idx` at a given display width `width` in string `s`, where `width` is in width units (columns). The default for `i` is 1 and for `j` is `#s`. This is the inverse operation of `utf8.width()`.

If `ambiwidth` is given, characters with ambiguous width will be treated as having width `ambiwidth` (1 or 2). The default for `ambiwidth` is 1.

If `default` is given, it will be used as the width for unprintable characters. The default for `default` is 0.

If the whole string does not fit in `width`, the `offset` and `width` will be returned. the `offset` is set to indicates the `width` is in which column in the `idx`th character in string `s`. and `width` is the current `width` of that character.

**Example:**
```lua
local idx, offset, width = utf8.widthindex("你好world", 3)
-- idx=2 (second character '好'), offset=1, width=2

local idx, offset, width = utf8.widthindex("你好world", 4)
-- idx=2, offset=2 (width `4` is the second column of '好'), width=2
```

### `utf8.widthlimit(s, limit [, i [, j [, ambiwidth [, default]]]]) --> position, remain`

Finds the byte position `position` where truncation should occur to fit within a display width `limit`.

If `limit` is positive, truncates from the front (keeps prefix) and returns the ending byte position where the width limit is reached.

If `limit` is negative, truncates from the back (keeps suffix) and returns the starting byte position where the width limit (using absolute value) is reached.

The default for `i` is 1 and for `j` is `#s`.

If `ambiwidth` is given, characters with ambiguous width will be treated as having width `ambiwidth` (1 or 2). The default for `ambiwidth` is 1.

If `default` is given, it will be used as the width for unprintable characters. The default for `default` is 0.

Returns the byte position `position` and the remaining width `remain`.  The `remain` value indicates the remaining width left unused after truncation. For a perfect fit, `remain` is 0. If the limit cannot be fully utilized, `remain` will be positive (for positive limit) or negative (for negative limit).

Notice that if even one byte can not put into `limit` space, this routine returns `i-1` for empty string (if `limit` is positive) or `j+1` (if `limit` is negative).

**Example:**
```lua
-- Truncate from front (keep prefix)
local pos, remain = utf8.widthlimit("hello world", 5)
-- pos=5, remain=0 (perfectly fits "hello")

-- Truncate from back (keep suffix)
local pos, remain = utf8.widthlimit("/path/to/file.lua", -8)
-- pos=10, remain=0 (starts at byte 10, "file.lua")

-- Truncate with remaining space
local pos, remain = utf8.widthlimit("world你好", 6)
-- pos=5, remain=1 ("world" fits in 5 columns, 1 column unused)

-- Substring truncation
local pos, remain = utf8.widthlimit("你好world", 3, 1, 11)
-- pos=4, remain=1 (first character '你' has width 2, leaving 1 column unused)
```

### `utf8.title(s) --> result`

### `utf8.fold(s) --> result`

Converts UTF-8 string `s` to title-case (for `utf8.title`), or folded case (for `utf8.fold`, used for case-insensitive comparison). If `s` is a number, it is treated as a code point and a converted code point (number) is returned.

Returns the converted string (or code point).

`utf8.lower` and `utf8.upper` have the same extension.

**Example:**

```lua
utf8.title("hello world")  -- "Hello World"
utf8.fold("Straße")        -- "strasse" (for comparison)
utf8.upper(0x61)           -- 0x41 (code point 'a' -> 'A')
```

### `utf8.ncasecmp(a, b) --> result`

Compares strings `a` and `b` without case. Returns -1 if `a < b`, 0 if `a == b`, and 1 if `a > b`.

**Example:**
```lua
utf8.ncasecmp("Hello", "hello")  -- 0
utf8.ncasecmp("abc", "DEF")      -- -1
```

### `utf8.isvalid(s) --> boolean`

Checks whether `s` is a valid UTF-8 string. Returns `true` if valid, `false` otherwise.

**Example:**
```lua
utf8.isvalid("你好")       -- true
utf8.isvalid("\xFF\xFE")  -- false (invalid UTF-8)
```

### `utf8.clean(s [, replacement]) --> result, was_valid`

Replaces any invalid UTF-8 byte sequences in `s` with the replacement string `replacement`. If no replacement string is provided, the default is "�" (REPLACEMENT CHARACTER U+FFFD). Any number of consecutive invalid bytes will be replaced by a single copy of the replacement string.

Returns the cleaned string `result` and a boolean `was_valid` indicating whether the original string was valid (`true` if no replacements were made).

**Example:**
```lua
local clean, valid = utf8.clean("你好\xFFworld")
-- clean = "你好�world", valid = false

local clean, valid = utf8.clean("你好")
-- clean = "你好", valid = true
```

### `utf8.invalidoffset(s [, i]) --> position`

Returns the byte position `position` within `s` of the first invalid UTF-8 byte sequence (1 is the first byte of the string). If `s` is a valid UTF-8 string, returns `nil`. The default for `i` is 1. The optional numeric argument `i` specifies where to start the search and can be negative.

**Example:**

```lua
local pos = utf8.invalidoffset("你好\xFFworld")
-- pos = 7 (first invalid byte)

local pos = utf8.invalidoffset("你好")
-- pos = nil (valid string)
```

### `utf8.isnfc(s) --> boolean`

Checks whether `s` is in Normal Form C.

**Normal Form C** means that whenever possible, combining marks are combined with a preceding codepoint. For example, instead of U+0041 (LATIN CAPITAL LETTER A) U+00B4 (ACUTE ACCENT), an NFC string will use U+00C1 (LATIN CAPITAL LETTER A WITH ACUTE). Also, some deprecated codepoints are converted to the recommended replacements.

Since the same sequence of characters can be represented in more than one way in Unicode, it is better to ensure strings are in Normal Form before comparing them.

Returns `true` if `s` is in NFC, `false` otherwise. An error may be raised if `s` is not a valid UTF-8 string.

**Example:**
```lua
utf8.isnfc("café")   -- true (if already NFC)
utf8.isnfc("cafe\u{0301}")  -- false (combining accent)
```

### `utf8.normalize_nfc(s) --> result, was_nfc`

Converts `s` to Normal Form C.

Returns the normalized string `result` and a boolean `was_nfc` indicating whether the original string was already in NFC (`true` if no modifications were made).

An error will be raised if `s` is not a valid UTF-8 string.

**Example:**
```lua
local normalized, was_nfc = utf8.normalize_nfc("cafe\u{0301}")
-- normalized = "café", was_nfc = false
```

### `utf8.grapheme_indices(s [, i [, j]]) --> iterator`

Returns an iterator which yields the starting byte index `from` and ending byte index `to` of each successive grapheme cluster in `s`. This range of bytes is inclusive of the endpoints, so the yielded values can be passed to `string.sub` to extract the grapheme cluster.

The default for `i` is 1 and for `j` is `#s`. If you provide `i` and `j` byte indices, then the iterator will only cover the requested byte range. `i` and `j` should fall on character boundaries, since an error will be raised if the requested byte range is not a valid UTF-8 string.

**Example:**
```lua
local count = 1
for from, to in utf8.grapheme_indices(s) do
  print("grapheme cluster "..count.." is from byte "..from.." to byte "..to)
  count = count + 1
end
```

## v0.2.0 BREAKING CHANGES

### ⚠️ API Changes

**Parameter changes for existing functions:**

- **`utf8.width()`**: 
  - Old: `utf8.width(s[, ambi_is_double[, default_width]])`
  - New: `utf8.width(s[, i[, j[, ambiwidth[, default_width]]]])`
  - **Breaking**: `ambi_is_double` (boolean) replaced with `ambiwidth` (integer 1 or 2)
  - **Breaking**: New optional byte range parameters `i, j` inserted before `ambiwidth`

- **`utf8.widthindex()`**:
  - Old: `utf8.widthindex(s, width[, ambi_is_double[, default_width]])`
  - New: `utf8.widthindex(s, width[, i[, j[, ambiwidth[, default_width]]]])`
  - **Breaking**: Same parameter changes as `utf8.width()`

**New functions:**
- `utf8.widthlimit(s, limit[, i[, j[, ambiwidth[, default]]]])` - Find truncation point within width limit

### Migration Guide

**Most common usage (unaffected):**
```lua
-- These still work:
utf8.width("你好")  -- No parameters changed
utf8.widthindex("你好", 3)  -- No parameters changed
```

**If you used `ambi_is_double` parameter:**
```lua
-- Old (v0.1.x):
utf8.width(s, true, 1)      -- ambi_is_double=true means width 2
utf8.width(s, false, 1)     -- ambi_is_double=false means width 1

-- New (v0.2.x):
utf8.width(s, 1, #s+1, 2, 1)  -- ambiwidth=2
utf8.width(s, 1, #s+1, 1, 1)  -- ambiwidth=1
```

**Parameter mapping:**
- `ambi_is_double = true` → `ambiwidth = 2`
- `ambi_is_double = false` or `nil` → `ambiwidth = 1` or omit

### Rationale

- **Consistency**: All width functions now use `ambiwidth` (integer) instead of `ambi_is_double` (boolean)
- **Flexibility**: Byte range parameters `(i, j)` enable substring width calculation
- **Clarity**: Integer `ambiwidth` is more intuitive than boolean `ambi_is_double`

### Impact Assessment

Estimated affected users: < 5 out of 22 dependent packages (most don't pass `ambi_is_double`).

## Improvements Needed

- Add Lua 5.3 spec test-suite
- More test cases
- Grapheme-compose support, and its effect on `utf8.reverse` and `utf8.width`


## License
It uses the same license as Lua: http://www.lua.org/license.html
