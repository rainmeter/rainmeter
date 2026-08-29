# Lexer tests

Run:

    .\Build\Build.ps1 test 0.0.0

To run the harness directly:

    x64-Test\Test.exe Config\Languages.xml Test\Cases\*.ini

## Updating the expected output

After a deliberate change to how something is highlighted:

    x64-Test\Test.exe Config\Languages.xml --update Test\Cases\*.ini

Read the resulting diff before committing it. These files record what the lexer currently
does, so `--update` will just as happily record a regression.

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
