# RainLexer 5.1.0 changes

Date: October 24, 2026

## Overview

Compared RainLexer's keyword lists and lexer rules with the local Rainmeter source, added missing supported options and bangs, corrected accepted values, and built release 5.1.0.

## Keyword lists

Updated `Config/Languages.xml`. Entries below use the lowercase spelling stored in the keyword lists.

### Section 0: ordinary options

Added:

```text
bothuseexiforientation
captureoverlaycolor
captureoverlaycursorname
maskcolormatrix1
maskcolormatrix2
maskcolormatrix3
maskcolormatrix4
maskcolormatrix5
maskgreyscale
maskimagealpha
maskimagecrop
maskimagetint
maskuseexiforientation
onaudioendaction
onaudiostartaction
primaryuseexiforientation
readonly
secondaryuseexiforientation
syntaxversion
uitheme
```

These cover Histogram image EXIF orientation, Mouse capture overlays, Image meter mask processing, AudioLevel actions, TextEdit read-only mode, skin syntax selection, and the Rainmeter UI theme setting.

Moved `placeholderstringstyle` from section 0 to section 2 so its value is validated.

### Section 2: options with validated values

Added `placeholderstringstyle`.

### Section 3: accepted values

Added:

```text
alias=custom
outputtype=utf-16
outputtype=utf-8
placeholderstringstyle=bold
placeholderstringstyle=bolditalic
placeholderstringstyle=italic
placeholderstringstyle=normal
state=maximize
state=minimize
sysinfotype=domainworkgroup
traymeter=none
```

Removed `state=maximized` and `state=minimized`, replacing them with the spellings accepted by the current RunCommand reader: `state=maximize` and `state=minimize`.

The hyphenated output encodings supplement the existing `utf16` and `utf8` spellings. `sysinfotype=domainworkgroup` is an accepted alternate spelling of `domain_workgroup`.

### Section 4: bangs

Added the following registered commands. The keyword list omits the leading `!` used in skin files.

```text
actiontimer:execute
actiontimer:stop
audio:mute
audio:setoutputindex
audio:setvolume
audio:togglemute
audio:unmute
forceenablevisiblemode
loop:reset
mouse:startcapture
mouse:stopcapture
recyclemanager:emptybin
recyclemanager:emptybinsilent
recyclemanager:openbin
restart
runcommand:close
runcommand:kill
runcommand:run
sendkey
webparser:clearresults
webparser:refetch
windowmessage:send
```

### Section 8: deprecated bangs

Added:

- `changezpos`: retained by Rainmeter for backward compatibility.
- `lsboxhook`: explicitly deprecated; its handler performs no action.

The edited keyword lists were sorted and deduplicated. Sections 1, 5, 6, and 7 were unchanged. The unimplemented UsageMonitor aliases `netdown` and `netup` were not added.

## Lexer rules

Updated `RainLexer/Lexer.h` by adding these entries to `nonNumValOpt`:

```text
captureoverlaycursorname
onaudioendaction
onaudiostartaction
```

These options contain a cursor name or actions, so their values use the lexer's nonnumeric value handling.

## Version

Updated `RainLexer/Version.h` from 5.0.0 to 5.1.0:

```cpp
#define RAINLEXER_VERSION_RC 5,1,0,0
#define RAINLEXER_VERSION_STRING "5.1.0.0"
#define RAINLEXER_TITLE L"RainLexer 5.1.0"
```

The initial release commit used `RainLexer 5.1.0.0` for the display title. The build script generated `RainLexer 5.1.0`, and that title correction was committed afterward.

## Build configuration

Updated `RainLexer/RainLexer.vcxproj` to use platform toolset `v145` instead of `v143`. The shared setting applies to Debug and Release builds for Win32 and x64.

The initial build failed because the requested v143 toolset was unavailable. A retry with a command-line v145 override encountered a program database manager mismatch in the sandbox. After the project file was fixed, the full release script succeeded outside the sandbox without a toolset override:

```powershell
$env:CI = 'true'
./Build/Build.ps1 full 5.1.0
```

The build regenerated the Default and Zenburn configurations under `Config/Generated`, compiled both release DLLs, and packaged the installer. Generated files and build outputs are ignored by Git.

## Validation and outputs

- XML parsing and keyword duplicate checks passed.
- Verified the PlaceholderStringStyle classification and corrected RunCommand values.
- All 21 lexer test cases passed, including the harness's incremental styling checks.
- Win32 and x64 release builds succeeded.
- NSIS installer generation succeeded.

| Output | Embedded file version | Size in bytes |
| --- | --- | ---: |
| `x32-Release/RainLexer.dll` | 5.1.0.0 | 182272 |
| `x64-Release/RainLexer.dll` | 5.1.0.0 | 211968 |
| `Build/RainLexer-5.1.0.exe` | 5.1.0 | 236498 |

## Git history and release tag

The following commits were pushed to `origin/master` in `rainmeter/rainmeter`:

| Commit | Description |
| --- | --- |
| `78d61479` | RainLexer: Update keywords and release version 5.1.0 |
| `59a72ed5` | RainLexer: Use Visual Studio v145 toolset |
| `303a73fc` | RainLexer: Match release title to build-generated version |

Created and pushed the annotated tag `rainlexer-v5.1.0` at commit `78d61479`. The tag remains on that original release commit; it does not include the subsequent toolset and display-title fixes. The completed build includes those fixes, which are on `master` at `303a73fc`.
