# How the Win32 profile APIs actually read and write .ini files

This is the observed behavior of `GetPrivateProfile*` / `WritePrivateProfile*`, derived from the
reports produced by `Tools/IniSpec`. It is the specification a replacement parser has to match,
or knowingly deviate from.

Everything here is evidence, not documentation: each statement corresponds to a named case in the
report. Where the evidence does not settle a question, it says so.

Measured on Windows 10.0.26100 (x64), with two process ANSI codepages: 1252 and 65001.

---

## 1. Encoding

Encoding detection happens once, on the whole file, before anything else. The results are stark:

| File | Result |
| --- | --- |
| ANSI / any single-byte content | Works. Bytes converted with the process ANSI codepage. |
| UTF-16LE **with** BOM | Works. Full Unicode preserved. |
| UTF-16LE **without** BOM | Works, identically. Detection is by content, not by BOM. |
| UTF-8 **without** BOM | Read as ANSI. Mojibake under CP1252; correct under ACP 65001. |
| **UTF-8 with BOM** | **Entire file unreadable. Zero sections.** |
| UTF-16BE with BOM | **Entire file unreadable. Zero sections.** |
| UTF-16BE without BOM | Catastrophically misparsed (see NUL handling below). |
| UTF-16LE with an odd byte count | **Entire file unreadable.** |

Two things deserve emphasis.

**A UTF-8 BOM makes the file completely invisible.** Not partially — a file containing nothing but
`EF BB BF [Section] CRLF Key=Value CRLF` returns zero sections and `ERROR_FILE_NOT_FOUND` on every
lookup. This is independent of the ANSI codepage; it fails under 65001 too. Since a great deal of
software writes UTF-8 with a BOM by default, this is the single most user-visible defect in the
current behavior.

**UTF-16LE needs no BOM.** The detection is content based, so a UTF-16LE file without a BOM reads
exactly like one with it.

A BOM anywhere other than offset 0 is just data. In `enc-bom-mid-file`, the line
`\xEF\xBB\xBF[Two]` does not open a section — it is an ordinary content line, and the keys below it
stay in the *previous* section.

### 1.1 The ANSI conversion

For non-UTF-16 files the conversion is exactly `MultiByteToWideChar(CP_ACP, 0, ...)` with no
`MB_ERR_INVALID_CHARS`:

- CP1252 mappings are exact, including `0x80` → U+20AC, `0x82` → U+201A, `0x91` → U+2018.
- The five bytes undefined in CP1252 (`0x81 0x8D 0x8F 0x90 0x9D`) become the matching C1 control
  characters U+0081, U+008D, U+008F, U+0090, U+009D — not U+FFFD, not dropped.
- Under ACP 65001, each invalid UTF-8 byte becomes one U+FFFD.
- `GetACP()` is the only input. `SetThreadLocale` has no effect whatsoever (`thread-locale`).

---

## 2. Lexical structure

### 2.1 Lines

CR, LF and CRLF all terminate a line, and they can be mixed freely in one file
(`mal-line-endings`). A final line with no terminator is parsed normally.

`0x1A` (SUB, the DOS EOF marker) is **not** an end-of-file marker — content after it parses fine
(`cp-sub-char`). Unicode line separators U+2028 and U+0085 are **not** line breaks
(`ws-exotic-utf16`).

### 2.2 Whitespace and the trim rule

Trimming applies to section names, key names and values: leading and trailing whitespace is
removed, interior whitespace is preserved verbatim.

**The trim predicate is `character <= 0x20`.** In `cp-control-bytes`, a value made of every byte
from `0x01` to `0x1F` plus `0x7F` came back as just `\x7F` — every byte at or below `0x20` was
trimmed away, `0x7F` was not. So space, tab, VT, FF and every other C0 control are whitespace.

Trimming is **not** Unicode aware. U+00A0, U+3000 and U+FEFF are ordinary characters and survive at
both ends of a value.

A value that is entirely whitespace becomes the empty string.

### 2.3 Section headers

A line opens a section if its first non-whitespace character is `[`. The name runs from just after
the `[` to **the first `]`, or to the end of the line if there is none**, and is then trimmed.

| Line | Section name |
| --- | --- |
| `[Name]` | `Name` |
| `  [Name]  ` | `Name` |
| `[ Name ]` | `Name` |
| `[Name] ; anything at all` | `Name` |
| `[Unclosed` | `Unclosed` |
| `[[Double]]` | `[Double` |
| `[Se]c]` | `Se` |
| `[Mid[dle]` | `Mid[dle` |
| `[]` or `[   ]` | *(empty string — a real, addressable section)* |
| `Orphan]` | *not a section; an ordinary content line* |
| `[Key]=2` | `Key` — a `[` line is always a header, whatever follows |

Everything after the closing `]` is discarded, which is why a trailing comment on a header line
works even though `;` is not otherwise a trailing comment character.

**Keys that appear before the first section header are unreachable.** They are not addressable
under the empty section name and not under the first section (`mal-keys-before-section`). Note the
contrast with an explicit `[]` header, which *does* create a usable empty-named section.

### 2.4 Key/value lines

The line is split at the **first** `=`. `Multi=a=b=c` yields key `Multi`, value `a=b=c`.

| Line | Key | Value |
| --- | --- | --- |
| `Key=Value` | `Key` | `Value` |
| `  Key  =  Value  ` | `Key` | `Value` |
| `Key=` | `Key` | *(empty)* |
| `=Value` | *(empty)* | `Value` — retrievable with an empty key name |
| `JustAKey` | — | not a key at all (see below) |

A line with no `=` is neither a key nor discarded. It shows up **in `GetPrivateProfileSection`
output verbatim**, but is excluded from key enumeration and cannot be retrieved by name. This
asymmetry is real and easy to miss.

### 2.5 Comments

**`;` starts a comment only when it is the first non-whitespace character of a line.** Such lines
are dropped completely — they do not even appear in `GetPrivateProfileSection`.

Everywhere else `;` is an ordinary character:

- `B=2 ;trailing comment` → value is `2 ;trailing comment`
- `D=;comment as value` → value is `;comment as value`
- `F;G=6` → a key literally named `F;G`

**`#` is not a comment character at all.** `#C=3` is a key named `#C`; `#bare text` is a no-`=`
line.

### 2.6 Quotes — and where the two APIs disagree

`GetPrivateProfileString` strips **one** matching pair of surrounding quotes, `"` or `'`, *after*
whitespace trimming. `GetPrivateProfileSection` does **not**.

| Value in file | `GetPrivateProfileString` | `GetPrivateProfileSection` |
| --- | --- | --- |
| `"value"` | `value` | `Plain="value"` |
| `" value "` | ` value ` | `Inner=" value "` |
| `  "value"  ` | `value` | `Padded="value"` |
| `"value` | `"value` | unchanged |
| `""` | *(empty)* | unchanged |
| `"""` | `"` | unchanged |
| `"a"b"` | `a"b` | unchanged |
| `'value'` | `value` | unchanged |
| `it's` | `it's` | unchanged |

Quotes are never stripped from section or key names.

`ConfigParser::ReadIniFile` reads through `GetPrivateProfileSection` and re-implements this
stripping by hand, which is why Rainmeter matches `GetPrivateProfileString` today.

---

## 3. Lookup and enumeration

**Everything is case-insensitive.** Enumeration returns the original spelling from the file.

**Only the first match is ever used, and duplicates are never merged.**

- Three `[Dup]` sections: `GetPrivateProfileSection("Dup")` returns only the contents of the first
  one. Keys in the second and third blocks are permanently unreachable.
- `Key` four times in a section: every lookup returns the first value.

**Enumeration and lookup disagree.** `GetPrivateProfileSectionNames` lists *every* occurrence, so a
file with three `[Dup]` sections enumerates `Dup` three times, and iterating the enumeration and
calling `GetPrivateProfileSection` for each name returns the same first block three times.

**Order is file order.** Nothing is sorted, for sections or for keys.

**Caller-supplied names are trimmed too — but only of spaces.** `" Section "` and `"Key "` match,
while `"\tSection"` does **not** (`lookup-whitespace-in-request`). This is narrower than the
`<= 0x20` rule applied to the file contents.

---

## 4. API surface details

### 4.1 Return values

Two different conventions, which is a standing source of off-by-one bugs:

- `GetPrivateProfileString` with a key returns the character count **excluding** the terminator.
- `GetPrivateProfileSectionNames`, `GetPrivateProfileSection` and `GetPrivateProfileString` with a
  `NULL` key return a multi-string, and the count **includes each string's own NUL** but excludes
  the final extra NUL. `"Section\0\0"` returns 8.

### 4.2 Buffer-too-small behavior

| API | On truncation | `ERROR_MORE_DATA` (234)? |
| --- | --- | --- |
| `GetPrivateProfileString` (key) | `ret == nSize - 1`, NUL-terminated | Yes — **also when the value fits exactly** |
| `GetPrivateProfileSection` | `ret == nSize - 2`, double-NUL terminated | **Never** |
| `GetPrivateProfileSectionNames` | `ret == nSize - 2`, double-NUL terminated | Yes |

Nothing is ever written past `nSize`; the guard region after the buffer was untouched in every
sweep.

The exact-fit quirk matters: reading `Value` into a 6-character buffer returns 5 and sets
`ERROR_MORE_DATA`, even though nothing was lost. `ret == nSize - 1` is genuinely ambiguous. For
`GetPrivateProfileSection` there is no error code at all, so `ret == nSize - 2` is the *only*
signal — which is exactly why `ConfigParser` grows its buffer on `res >= itemsSize - 2`. That
heuristic is correct.

### 4.3 Error codes

| Situation | `lastError` |
| --- | --- |
| Section or key not found (file exists) | 2 `ERROR_FILE_NOT_FOUND` |
| File does not exist | 2 `ERROR_FILE_NOT_FOUND` |
| File is empty, or only comments/blank lines | **0** — `ret` is 0 with no error |
| Path is a directory | 5 `ERROR_ACCESS_DENIED` |
| File open exclusively by someone else | 32 `ERROR_SHARING_VIOLATION` |
| Write to a read-only file | 5 `ERROR_ACCESS_DENIED`, `ret == 0` |
| Write into a non-existent directory | 3 `ERROR_PATH_NOT_FOUND`, `ret == 0` |

A missing key and a missing file are indistinguishable by error code. An empty-but-readable file
*is* distinguishable, because it reports no error at all.

### 4.4 Paths

Relative names resolve against the **Windows directory**, not the current directory — with the
process CWD at `D:\a\rainmeter\rainmeter\Build`, reading `win.ini` returned the contents of
`C:\Windows\win.ini`. Forward slashes work as separators.

### 4.5 `GetPrivateProfileInt`

Returns the default **only when the key is absent**. A key that is present but non-numeric returns
`0`, not the default.

---

## 5. Writing

### 5.1 Encoding on write

| Existing file | Result |
| --- | --- |
| Does not exist | Created as **ANSI, no BOM, CRLF**, always |
| ANSI | Stays ANSI |
| UTF-16LE (BOM or not) | Stays UTF-16LE, full Unicode written correctly |
| UTF-8 with BOM | **Corrupted** — see below |

A new file is never created as UTF-16, even when the value being written needs it. Writing
`日本語` to a fresh file produces `Cjk=???`.

**Characters not representable in the ANSI codepage become `?`**, one per UTF-16 code unit, with no
error and `ret == 1`. An emoji becomes `??`. This is silent, unrecoverable data loss.

**The UTF-8-BOM corruption path**: because the reader cannot see any section in such a file, the
writer believes the section does not exist and appends a duplicate one at the end. Then it writes
the value in ANSI. The file ends up containing both UTF-8 and CP1252 bytes, the original content
is orphaned, and the new content is in the wrong encoding. Rainmeter destroys UTF-8-BOM .ini files
today, on any save.

### 5.2 Placement and formatting

- A new key is appended to the end of its section; a new section to the end of the file.
- Overwriting a value **preserves the line's position and the whitespace before `=`**, and
  **destroys everything after `=`** — including any trailing comment. `  One   =   1   ; comment`
  becomes `  One   =new`.
- Comments and blank lines elsewhere are preserved.
- An existing section or key is matched case-insensitively and **keeps its original spelling**.
- Untouched lines keep their line endings, but everything the writer emits uses CRLF, so writing
  into an LF-only file produces a mixed-ending file.
- Appending to a file with no trailing newline correctly inserts one first.
- Only the first of duplicate sections/keys is ever updated.
- Pre-section keys cannot be updated; writing to section `""` appends a literal `[]` section.

### 5.3 Values are written verbatim

There is no escaping and no quoting. Consequences:

- Leading/trailing whitespace is written but then lost on read-back.
- `"quoted"` is written literally and read back as `quoted` — a silent round-trip change.
- **A value containing CR or LF is written raw and structurally corrupts the file.** Writing
  `a\r\nb=c` produces two lines, the second of which becomes a real key.
- A key name containing `=` produces an unrecoverable line (`Eq=Key=4` reads back as key `Eq`).
- A section name containing `]` is not round-trippable (`[Br[ack]et]` reads back as `Br[ack`).

### 5.4 Deletion

- `WritePrivateProfileString(section, key, NULL)` removes the key's whole line.
- `WritePrivateProfileString(section, NULL, NULL)` removes the header and its keys, including
  comments inside the section, but leaves the blank line that followed it.
- Deleting something that does not exist returns **1 (success)**. The return value says nothing
  about whether anything was removed; it only reports I/O failure.

### 5.5 `WritePrivateProfileSection`

Replaces the section's entire contents. Entries are written verbatim, one per line. An **empty
payload leaves an empty section header behind** — it is not the same as deleting the section.

---

## 6. Things that turned out not to exist

**There is no observable read cache.** This was the biggest open risk going in. Rewriting a file
behind the API's back — same size *and* with the original last-write time restored — still produced
the new value on the very next call. There is no stale-read behavior to reproduce, and no cache
invalidation logic the replacement needs to emulate.

**There is no DBCS ambiguity on this runner**, because the runner is SBCS. See open questions.

---

## 7. Traps for the replacement

Ranked by how likely they are to bite:

1. **Embedded NUL bytes pass straight through into results.** `[Sec\0tion]` produces a section
   whose name literally contains a NUL, which is then written into a multi-string buffer — making
   it indistinguishable from two sections named `Sec` and `tion`. Neither name can be looked up.
   The multi-string encoding is simply not injective. A UTF-16BE file with no BOM hits this on
   every single character.
2. **A no-`=` line appears in `GetPrivateProfileSection` but not in key enumeration.** Code that
   assumes every entry contains `=` will mis-slice. `ConfigParser` already guards this with its
   `valuePos != nullptr` check.
3. **An empty key name enumerates as an empty string**, which is indistinguishable from the
   multi-string terminator.
4. **`GetPrivateProfileSection` and `GetPrivateProfileString` disagree about quotes.**
5. **`ERROR_MORE_DATA` on an exact fit**, and no error at all from `GetPrivateProfileSection`.

---

## 8. Open questions

These are genuinely unresolved by the current reports, not oversights in the analysis.

**Byte-level or character-level parsing?** The runner's ANSI codepage is single-byte, so the
question of whether a DBCS lead byte can swallow a `]` or `=` cannot be answered here. The
`cp-dbcs-lead-bytes` case is in the corpus and ready; it needs a run on a machine whose system
locale is Japanese (CP932) or Chinese (CP936). The evidence available is consistent with
parse-then-convert — `[A\x81]B]` produced the name `A\x81`, cut at the first `]` — but that is not
proof.

**Where exactly are long lines truncated?** A 5000-character value survives intact. A
200000-character value read back as 3392 characters; a 100000-character value in a different file
read back as 34464. Both are silent truncations of the stored line, and parsing resumes correctly
on the following line. The cut point is position dependent and matches no obvious constant, so it
needs a bisect probe. Nothing suggests the replacement should reproduce it.

**Why does a lone U+20AC write as an empty value under ACP 65001?** Under CP1252 it writes as
`0x80` correctly. Under ACP 65001, `WritePrivateProfileString(..., L"\u20AC")` produced `Euro=`
with no bytes at all, while three-character CJK values in the same run wrote correctly. One data
point, no theory; it needs its own case.

---

## 9. What the replacement should do

Observations end here; this section is recommendation.

Match the API exactly on the lexical rules — the trim rule, the first-`=` split, first-match-wins,
case insensitivity, section header parsing, `;`-only-at-line-start comments, quote stripping.
Existing skins depend on all of it, including the ugly parts.

Deliberately deviate on:

- **Read UTF-8 with a BOM as UTF-8**, and UTF-8 without a BOM too where it is unambiguous. This
  fixes a real, common, silent failure.
- **Preserve the file's encoding on write, and never write `?`.** Promote to UTF-8 rather than lose
  characters.
- **Reject or escape CR/LF in written values** instead of corrupting the file.
- **Do not impose a line length limit.**
- **Keep NULs out of results** rather than propagating an ambiguous multi-string.

Keep the `res >= size - 2` buffer growth in `ConfigParser` until the call sites move off the Win32
signatures; it is correct against the real behavior.
