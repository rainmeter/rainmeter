# Lexer tests

Each `Cases/*.ini` file is lexed and folded, and the result is compared against the
`.styled` file beside it. The keyword lists come from `Config/Default/RainLexer.xml`, the
same configuration the plugin installs, so a case also fails if a keyword is removed from
under it.

## Running

    .\Build\Build.ps1 test 0.0.0

`full` runs the tests too, before building anything. The version argument is only there
because `Build.ps1` requires one; the tests do not use it.

To run the harness directly:

    x64-Test\Test.exe Config\Default\RainLexer.xml Test\Cases\*.ini

## Updating the expected output

After a deliberate change to how something is highlighted:

    x64-Test\Test.exe Config\Default\RainLexer.xml --update Test\Cases\*.ini

Read the resulting diff before committing it. These files record what the lexer currently
does, so `--update` will just as happily record a regression.

## The `.styled` format

Text is reproduced with each run of characters prefixed by its style number, `{0}` for
the default style, `{2}` for a section name and so on. The numbers are the `TextColor`
enumerators in `RainLexer/Lexer.h`. Line endings are written out as `\r` and `\n`, and
every line restarts with an explicit style so that a change on one line does not shift
the markers on the lines after it. A literal `{` in the source appears as `{{`.

Two sections follow the text:

- `--- folds ---` lists the fold level of each line, and marks the lines Scintilla will
  draw a fold header on.
- `--- out of range ---` counts styles and fold levels the lexer wrote outside the
  document. Scintilla discards these silently, so without counting them a lexer reaching
  past the end of the buffer would leave no trace. Both should be zero.

## What the harness checks beyond the expected output

Every case is also lexed a second time in chunks of 1, 7, 64 and 997 bytes, resuming from
the start of the line that styling stopped inside, which is how Scintilla styles a
document it only needs part of. The result has to be identical to lexing the whole
document in one go. Scintilla chooses those boundaries, not the lexer, so any difference
is a bug regardless of what the expected output says.

Asserts are left enabled in the test build. Scintilla's `LexAccessor::ColourTo` checks
its own preconditions with them, and those have caught real faults that were invisible in
the styled output.

## Adding a case

Drop a `.ini` file in `Cases` and run with `--update` to create its `.styled` file, then
check the output says what you expect. `Cases/.gitattributes` turns off line ending
normalisation for the directory, so a case can use CRLF or LF deliberately and keep it.
