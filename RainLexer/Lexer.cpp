/*
  Copyright (C) 2010-2012 Birunthan Mohanathas <http://poiru.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"
#include "Lexer.h"

static const char styleSubable[] = { 0 };

namespace RainLexer {

bool IsReserved(int ch) {
    if (IsAlphaNumeric(ch))
    {
        return false;
    }

    return (ch == '<' || ch == '>' || ch == ':' ||
        ch == '"' || ch == '/' || ch == '\\' ||
        ch == '|' || ch == '?' || ch == '*');
}

ILexer5* RainLexer::LexerFactory()
{
    return new RainLexer(nullptr, 0U);
}

//
// ILexer
//

void SCI_METHOD RainLexer::Release() {
    delete this;
}

int SCI_METHOD RainLexer::Version() const {
    return lvRelease5;
}

const char* SCI_METHOD RainLexer::PropertyNames() {
    return "";
}

int SCI_METHOD RainLexer::PropertyType(const char* /*name*/) {
    return SC_TYPE_BOOLEAN;
}

const char* SCI_METHOD RainLexer::DescribeProperty(const char* /*name*/) {
    return "";
}

Sci_Position SCI_METHOD RainLexer::PropertySet(const char* /*key*/, const char* /*val*/) {
    return -1;
}

const char* SCI_METHOD RainLexer::DescribeWordListSets() {
    return "";
}

Sci_Position SCI_METHOD RainLexer::WordListSet(int n, const char* wl) {
    if (n < _countof(m_WordLists))
    {
        WordList wlNew(false);
        wlNew.Set(wl);
        if (m_WordLists[n] != wlNew)
        {
            m_WordLists[n].Set(wl);
            return 0;
        }
    }
    return -1;
}

void SCI_METHOD RainLexer::Lex(Sci_PositionU startPos, Sci_Position length, int /*initStyle*/, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);

    char buffer[128];
    const WordList& keywords = m_WordLists[0];
    const WordList& numwords = m_WordLists[1];
    const WordList& optwords = m_WordLists[2];
    const WordList& options = m_WordLists[3];
    const WordList& bangs = m_WordLists[4];
    const WordList& variables = m_WordLists[5];
    const WordList& depKeywords = m_WordLists[6];
    const WordList& depOtions = m_WordLists[7];
    const WordList& depBangs = m_WordLists[8];

    length += startPos;
    styler.StartAt(startPos);
    styler.StartSegment(startPos);

    auto state = TextState::TS_DEFAULT;
    int count = 0;
    int digits = 0;

    int skipRainmeterBang = 0;
    int beginValueIdx = 0; // For cases like PlayerName=[ParentMeasure]

    bool isNested = false;
    auto nestVarIdx = startPos; // For cases like [#myVar#10]

    bool onlyDigits = true;
    bool isPipeOpt = false;
    int countParentheses = 0;

    bool isSubsOpt = false;
    bool isFormatOpt = false;
    bool isNotNumValOpt = false;
    bool isExtOption = false;

    for (auto i = startPos; i < static_cast<Sci_PositionU>(length); ++i)
    {
        bool isEOF = (i == static_cast<Sci_PositionU>(length) - 1);

        // Make ch 0 if at EOF.
        char ch = isEOF ? '\0' : styler.SafeGetCharAt(i, '\0');

        // Amount of EOL chars is 2 (\r\n) with the Windows format and 1 (\n) with Unix format.
        int chEOL = (styler[i] == '\n' && styler[i - 1] == '\r') ? 2 : (isEOF ? 0 : 1);

        switch (state)
        {
        case TextState::TS_DEFAULT:
            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '[':
                state = TextState::TS_SECTION;
                break;

            case ';':
                state = TextState::TS_COMMENT;
                break;

            case '\t':
            case ' ':
                break;

            default:
                if (IsUpperOrLowerCase(ch) || ch == '@')
                {
                    count = 0;
                    digits = 0;
                    buffer[count++] = MakeLowerCase(ch);

                    isExtOption = false;
                    isSubsOpt = false;
                    isFormatOpt = false;
                    isNotNumValOpt = false;
                    isPipeOpt = false;

                    state = TextState::TS_KEYWORD;
                }
                else if (IsADigit(ch))
                {
                    state = TextState::TS_NOT_KEYWORD;
                }
                else
                {
                    state = TextState::TS_LINEEND;
                }
            }
            break;

        case TextState::TS_COMMENT:
            // Style as comment when EOL (or EOF) is reached
            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i - chEOL, TC_COMMENT);
                styler.ColourTo(i, TC_DEFAULT);
                break;

            default:
                styler.ColourTo(i, TC_COMMENT);
            }
            break;

        case TextState::TS_SECTION:
            // Style as section when EOL (or EOF) is reached unless section name has a space
            switch (ch)
            {
            case '\0':
                if (styler.SafeGetCharAt(i, '\0') == ']')
                {
                    styler.ColourTo(i, TC_SECTION);
                }
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case ']':
                styler.ColourTo(i, TC_SECTION);
                state = TextState::TS_LINEEND;
                break;

            default:
                break;
            }
            break;

        case TextState::TS_KEYWORD:
            // Read max. 32 chars into buffer until the equals sign (or EOF/EOL) is met.
            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '=':
                // Ignore trailing whitespace
                while (count > 0 && IsASpaceOrTab(buffer[count - 1]))
                {
                    --count;
                }

                buffer[count] = '\0';

                isPipeOpt = pipeOpt.find(buffer) != pipeOpt.end();
                countParentheses = 0;
                onlyDigits = true;

                if (keywords.InList(buffer) || strncmp(buffer, "@include", 8) == 0)
                {
                    isSubsOpt = strcmp(buffer, "substitute") == 0;
                    isFormatOpt = formatOpt.find(buffer) != formatOpt.end();
                    isNotNumValOpt = nonNumValOpt.find(buffer) != nonNumValOpt.end();

                    state = TextState::TS_VALUE;
                    styler.ColourTo(i - 1, TC_KEYWORD);
                    styler.ColourTo(i, TC_EQUALS);
                }
                else if (optwords.InList(buffer))
                {
                    state = TextState::TS_OPTION;
                    isExtOption = extKeyWords.find(buffer) != extKeyWords.end();

                    if (depKeywords.InList(buffer))
                    {
                        styler.ColourTo(i - 1, TC_DEP_KEYWORD);
                    }
                    else
                    {
                        styler.ColourTo(i - 1, TC_KEYWORD);
                    }

                    buffer[count++] = '=';
                    beginValueIdx = count;
                    styler.ColourTo(i, TC_EQUALS);

                    // Ignore leading whitepsace
                    while (IsASpaceOrTab(styler.SafeGetCharAt(i + 1, '\0')))
                    {
                        ++i;
                    }
                }
                else if (digits > 0)
                {
                    // For cases with number in middle word or defined number at end, like UseD2D
                    if (depKeywords.InList(buffer))
                    {
                        isNotNumValOpt = nonNumValOpt.find(buffer) != nonNumValOpt.end();
                        state = TextState::TS_VALUE;
                        styler.ColourTo(i - 1, TC_DEP_KEYWORD);
                        styler.ColourTo(i, TC_EQUALS);
                        break;
                    }

                    // Try removing chars from the end to check for words like ScaleN
                    count -= digits;
                    buffer[count] = '\0';
                    digits = 0;

                    isExtOption = extKeyWords.find(buffer) != extKeyWords.end();
                    isPipeOpt = pipeOpt.find(buffer) != pipeOpt.end();
                    state = isExtOption ? TextState::TS_OPTION : TextState::TS_VALUE;

                    // Special case for option Command from iTunes plugin, and similar Command1, Command2, ... options from InputText plugin
                    if (numwords.InList(buffer))
                    {
                        isNotNumValOpt = nonNumValOpt.find(buffer) != nonNumValOpt.end();
                        if (depKeywords.InList(buffer) && strcmp(buffer, "command") != 0)
                        {
                            styler.ColourTo(i - 1, TC_DEP_KEYWORD);
                        }
                        else
                        {
                            styler.ColourTo(i - 1, TC_KEYWORD);
                        }

                        if (isExtOption)
                        {
                            buffer[count++] = '=';
                            beginValueIdx = count;
                            while (IsASpaceOrTab(styler.SafeGetCharAt(i + 1, '\0')))
                            {
                                ++i;
                            }
                        }
                    }
                    else
                    {
                        isPipeOpt = true;
                        styler.ColourTo(i - 1, TC_DEFAULT);
                    }
                    styler.ColourTo(i, TC_EQUALS);
                }
                else
                {
                    if (depKeywords.InList(buffer))
                    {
                        styler.ColourTo(i - 1, TC_DEP_KEYWORD);
                        isNotNumValOpt = nonNumValOpt.find(buffer) != nonNumValOpt.end();
                    }
                    else
                    {
                        styler.ColourTo(i - 1, TC_DEFAULT);
                        isPipeOpt = true;
                    }

                    styler.ColourTo(i, TC_EQUALS);
                    state = TextState::TS_VALUE;
                }
                break;

            default:
                if (count < _countof(buffer))
                {
                    if (isdigit(ch) > 0)
                    {
                        ++digits;
                    }
                    buffer[count++] = MakeLowerCase(ch);
                }
                else
                {
                    state = TextState::TS_LINEEND;
                }
            }
            break;

        case TextState::TS_OPTION:
            // Read value into buffer and check if it's valid for cases like StringAlign=RIGHT
            switch (ch)
            {
            case '#':
                count = 0;
                styler.ColourTo(i - 1, TC_DEFAULT);
                state = TextState::TS_VARIABLE;
                break;

            case '\0':
                // Read the last character if at EOF
                if (isEOF)
                {
                    buffer[count++] = MakeLowerCase(styler.SafeGetCharAt(i++, '\0'));
                }
            case '\t':
            case ' ':
                if (isExtOption)
                {
                    isPipeOpt = true;
                }
                else if (!isEOF)
                {
                    if (count < _countof(buffer))
                    {
                        buffer[count++] = MakeLowerCase(ch);
                    }
                    else
                    {
                        state = TextState::TS_LINEEND;
                    }
                    break;
                }
            case '\r':
            case '\n':
                if (!isExtOption || isEOF)
                {
                    while (IsASpaceOrTab(buffer[count - 1]))
                    {
                        --count;
                    }
                }

                buffer[count] = '\0';

                state = isExtOption ? TextState::TS_VALUE : TextState::TS_DEFAULT;

                if (options.InList(buffer) || extOptions.find(buffer) != extOptions.end())
                {
                    styler.ColourTo(i - chEOL, TC_VALID);
                }
                else if (depOtions.InList(buffer))
                {
                    styler.ColourTo(i - chEOL, TC_DEP_VALID);
                }
                else if (buffer[beginValueIdx] == '[' && buffer[count - 1] == ']')
                {
                    styler.ColourTo(i - chEOL, TC_DEFAULT);
                }
                else
                {
                    styler.ColourTo(i - chEOL, TC_INVALID);
                    state = TextState::TS_LINEEND;
                }
                styler.ColourTo(i, TC_DEFAULT);
                beginValueIdx = 0;
                count = 0;
                break;

            case '[':
                if (styler.SafeGetCharAt(i + 1, '\0') == '#')
                {
                    isNested = true;
                    count = 0;
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    nestVarIdx = i++;
                    state = TextState::TS_VARIABLE;
                    break;
                }

            default:
                if (count < _countof(buffer))
                {
                    buffer[count++] = MakeLowerCase(ch);
                }
                else
                {
                    state = TextState::TS_LINEEND;
                }
            }
            break;

        case TextState::TS_NOT_KEYWORD:
            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '=':
                onlyDigits = true;
                state = TextState::TS_VALUE;
                styler.ColourTo(i - 1, TC_DEFAULT);
                styler.ColourTo(i, TC_EQUALS);
            default:
                break;
            }
            break;

        case TextState::TS_VALUE:
            // Read values to highlight variables and bangs
            isNested = false;

            switch (ch)
            {
            case '#':
                onlyDigits = !isNotNumValOpt;
                if (isFormatOpt && styler.SafeGetCharAt(i - 1, '\0') == '%')
                {
                    styler.ColourTo(i, TC_DEFAULT);
                }
                else
                {
                    count = 0;
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    state = TextState::TS_VARIABLE;
                }
                break;

            case '[':
                onlyDigits = !isNotNumValOpt;
                if (styler.SafeGetCharAt(i + 1, '\0') == '#')
                {
                    isNested = true;
                    count = 0;
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    nestVarIdx = i++;
                    state = TextState::TS_VARIABLE;
                }
                else if (styler.SafeGetCharAt(i + 1, '\0') == '\\')
                {
                    count = 1;
                    if (MakeLowerCase(styler.SafeGetCharAt(i + 2, '\0')) == 'x')
                    {
                        buffer[0] = '0';
                    }
                    else
                    {
                        buffer[0] = ' ';
                    }

                    styler.ColourTo(i - 1, TC_DEFAULT);
                    state = TextState::TS_CHAR_VARIABLE;
                    ++i;
                }
                break;

            case '!':
                if (!IsUpperOrLowerCase(styler.SafeGetCharAt(i + 1, '\0')) || isFormatOpt)
                {
                    styler.ColourTo(i, TC_DEFAULT);
                }
                else
                {
                    count = 0;
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    state = TextState::TS_BANG;
                }
                break;

            case '\0':
                if (isEOF)
                {
                    if (onlyDigits && !isNotNumValOpt && IsADigit(styler.SafeGetCharAt(i, '\0')))
                    {
                        styler.ColourTo(i, TC_DIGITS);
                    }
                    else if (styler.SafeGetCharAt(i, '\0') == '|' && isPipeOpt && countParentheses == 0)
                    {
                        styler.ColourTo(i, TC_PIPE);
                    }
                }
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '|':
                onlyDigits = !isNotNumValOpt;
                if (isPipeOpt && countParentheses == 0)
                {
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    styler.ColourTo(i, TC_PIPE);
                }
                break;

            case '(':
                ++countParentheses;
                onlyDigits = !isNotNumValOpt;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case ')':
                --countParentheses;
            default:
                if (isNotNumValOpt)
                {
                    styler.ColourTo(i, TC_DEFAULT);
                    break;
                }

                char prevCh = styler.SafeGetCharAt(i - 1, '\0');
                if (onlyDigits && IsADigit(ch) && !IsUpperOrLowerCase(prevCh) && IsASCII(prevCh))
                {
                    styler.ColourTo(i - 1, TC_DEFAULT);
                    styler.ColourTo(i, TC_DIGITS);
                    state = TextState::TS_DIGITS;
                    break;
                }

                if (IsUpperOrLowerCase(ch))
                {
                    onlyDigits = false;
                }
                else if (IsASpaceOrTab(ch) || IsReserved(ch) || ch == ']' ||
                    ch == '=' || ch == '%' || ch == '$' || ch == '(')
                {
                    onlyDigits = true;
                }

                styler.ColourTo(i, TC_DEFAULT);
                break;
            }
            break;

        case TextState::TS_DIGITS:
            // Highlight digits
            switch (ch)
            {
            case '\0':
                if (isEOF)
                {
                    if (IsADigit(styler.SafeGetCharAt(i, '\0')))
                    {
                        styler.ColourTo(i, TC_DIGITS);
                    }
                    else if (styler.SafeGetCharAt(i, '\0') == '|' && isPipeOpt && countParentheses == 0)
                    {
                        styler.ColourTo(i, TC_PIPE);
                    }
                }
            case '\r':
            case '\n':
                onlyDigits = false;
                styler.ColourTo(i - 1, TC_DIGITS);
                styler.ColourTo(i, TC_DEFAULT);
                state = TextState::TS_DEFAULT;
                break;

            case '#':
            case '[':
            case '|':
            case '(':
            case ')':
                state = TextState::TS_VALUE;
                styler.ColourTo(i - 1, TC_DIGITS);
                --i;
                break;

            case '.':
                if (!IsADigit(styler.SafeGetCharAt(i + 1, '\0')))
                {
                    styler.ColourTo(i - 1, TC_DIGITS);
                    styler.ColourTo(i, TC_DEFAULT);
                    state = TextState::TS_VALUE;
                }
                break;
                
            default:
                if (IsUpperOrLowerCase(ch) || !IsASCII(ch))
                {
                    onlyDigits = false;
                }
                else if (IsADigit(ch))
                {
                    styler.ColourTo(i, TC_DIGITS);
                    break;
                }

                styler.ColourTo(i - 1, TC_DIGITS);
                state = TextState::TS_VALUE;
                break;
            }
            break;

        case TextState::TS_BANG:
            // Highlight bangs
            switch (ch)
            {
            case '\0':
                if (isEOF)
                {
                    buffer[count++] = MakeLowerCase(styler.SafeGetCharAt(i++, '\0'));
                }
            case '\r':
            case '\n':
            case '\t':
            case ' ':
            case '[':
            case ']':
                buffer[count] = '\0';
                count = 0;

                // Skip rainmeter before comparing the bang
                skipRainmeterBang = (strncmp(buffer, "rainmeter", 9) == 0) ? 9 : 0;
                if (bangs.InList(&buffer[skipRainmeterBang]))
                {
                    styler.ColourTo(i - chEOL, TC_BANG);
                }
                else if (depBangs.InList(&buffer[skipRainmeterBang]))
                {
                    styler.ColourTo(i - chEOL, TC_DEP_BANG);
                }

                if (setterBangWords.find(&buffer[skipRainmeterBang]) != setterBangWords.end())
                {
                    isPipeOpt = true;
                    isNotNumValOpt = false;
                }
                else
                {
                    isPipeOpt = false;
                    isNotNumValOpt = true;
                }

                state = (ch == '\r' || ch == '\n') ? TextState::TS_DEFAULT : TextState::TS_VALUE;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '#':
                count = 0;
                styler.ColourTo(i - 1, TC_DEFAULT);
                state = TextState::TS_VARIABLE;
                break;

            default:
                if (count < _countof(buffer))
                {
                    buffer[count++] = MakeLowerCase(ch);
                }
                else
                {
                    state = TextState::TS_VALUE;
                }
            }
            break;

        case TextState::TS_VARIABLE:
            // Highlight variables
            if (isEOF)
            {
                if (styler.SafeGetCharAt(i, '\0') == '#')
                {
                    ch = '#';
                }
                else if (styler.SafeGetCharAt(i, '\0') == ']')
                {
                    ch = ']';
                }
            }

            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case '#':
                if (isNested)
                {
                    if (styler.SafeGetCharAt(i - 1, '\0') == '[')
                    {
                        --i;
                        state = TextState::TS_VALUE;
                        break;
                    }
                    styler.ColourTo(nestVarIdx, TC_DEFAULT);
                    isNested = false;
                }
            case ']':
                if (!isNested && ch == ']')
                {
                    state = TextState::TS_VALUE;
                    break;
                }

                if (count > 0)
                {
                    buffer[count] = '\0';

                    if (variables.InList(buffer))
                    {
                        styler.ColourTo(i, TC_INTVARIABLE);
                    }
                    else
                    {
                        if (buffer[0] == '*' && buffer[count - 1] == '*')
                        {
                            // Escaped variable, don't highlight
                            styler.ColourTo(i, TC_DEFAULT);
                        }
                        else
                        {
                            styler.ColourTo(i, TC_EXTVARIABLE);
                        }
                    }

                    count = 0;
                }

                state = isEOF ? TextState::TS_DEFAULT : TextState::TS_VALUE;
                break;

            case '[':
                --i;
            case ' ':
                state = TextState::TS_VALUE;
                break;

            case '\'':
            case '"':
                if (isSubsOpt)
                {
                    state = TextState::TS_VALUE;
                    break;
                }
            default:
                if (count < _countof(buffer))
                {
                    buffer[count++] = MakeLowerCase(ch);
                }
                else
                {
                    state = TextState::TS_VALUE;
                }
            }
            break;

        case TextState::TS_CHAR_VARIABLE:
            // Highlight variables
            if (isEOF && styler.SafeGetCharAt(i, '\0') == ']')
            {
                ch = ']';
            }

            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
                styler.ColourTo(i, TC_DEFAULT);
                break;

            case ']':
                if (count > 2 || (count == 2 && buffer[0] == ' '))
                {
                    buffer[count] = '\0';

                    std::string charNumber = buffer;
                    if (std::stol(charNumber, nullptr, 0) < 0xFFFF)
                    {
                        styler.ColourTo(i, TC_CHAR_VARIABLE);
                    }
                    else
                    {
                        styler.ColourTo(i, TC_DEFAULT);
                    }
                    count = 0;
                }

                state = isEOF ? TextState::TS_DEFAULT : TextState::TS_VALUE;
                break;

            case '[':
                --i;
                state = TextState::TS_VALUE;
                break;

            default:
                if (count == 1 && buffer[0] == '0')
                {
                    buffer[count++] = 'x';
                }
                else if (count < 6 && IsADigit(ch, buffer[1] == 'x' ? 16 : 10) && count < _countof(buffer))
                {
                    buffer[count++] = MakeLowerCase(ch);
                }
                else
                {
                    state = TextState::TS_VALUE;
                }
            }
            break;

        default:
        case TextState::TS_LINEEND:
            // Apply default style when EOL (or EOF) is reached
            switch (ch)
            {
            case '\0':
            case '\r':
            case '\n':
                state = TextState::TS_DEFAULT;
            default:
                styler.ColourTo(i, TC_DEFAULT);
            }
            break;
        }
    }

    styler.Flush();
}

void SCI_METHOD RainLexer::Fold(Sci_PositionU startPos, Sci_Position length, int /*initStyle*/, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);

    length += startPos;
    int line = styler.GetLine(startPos);

    for (auto i = startPos; i < static_cast<Sci_PositionU>(length); ++i)
    {
        if ((styler[i] == '\n') || (i == length - 1))
        {
            int level = (styler.StyleAt(i - 2) == static_cast<int>(TextState::TS_SECTION))
                ? SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG
                : SC_FOLDLEVELBASE + 1;

            if (level != styler.LevelAt(line))
            {
                styler.SetLevel(line, level);
            }

            ++line;
        }
    }

    styler.Flush();
}

void* SCI_METHOD RainLexer::PrivateCall(int /*operation*/, void* /*pointer*/) {
    return nullptr;
}

int SCI_METHOD RainLexer::LineEndTypesSupported() {
    return SC_LINE_END_TYPE_DEFAULT;
}

int SCI_METHOD RainLexer::AllocateSubStyles(int /*styleBase*/, int /*numberStyles*/) {
    return -1;
}

int SCI_METHOD RainLexer::SubStylesStart(int /*styleBase*/) {
    return -1;
}

int SCI_METHOD RainLexer::SubStylesLength(int /*styleBase*/) {
    return 0;
}

int SCI_METHOD RainLexer::StyleFromSubStyle(int subStyle) {
    return subStyle;
}

int SCI_METHOD RainLexer::PrimaryStyleFromStyle(int style) {
    return style;
}

void SCI_METHOD RainLexer::FreeSubStyles() {
}

void SCI_METHOD RainLexer::SetIdentifiers(int /*style*/, const char* /*identifiers*/) {
}

int SCI_METHOD RainLexer::DistanceToSecondaryStyles() {
    return 0;
}

const char* SCI_METHOD RainLexer::GetSubStyleBases() {
    return styleSubable;
}

int SCI_METHOD RainLexer::NamedStyles() {
    return static_cast<int>(nClasses);
}

const char* SCI_METHOD RainLexer::NameOfStyle(int style) {
    return (style < NamedStyles()) ? lexClasses[style].name : "";
}

const char* SCI_METHOD RainLexer::TagsOfStyle(int style) {
    return (style < NamedStyles()) ? lexClasses[style].tags : "";
}

const char* SCI_METHOD RainLexer::DescriptionOfStyle(int style) {
    return (style < NamedStyles()) ? lexClasses[style].description : "";
}

const char* SCI_METHOD RainLexer::GetName() {
    return "Rainmeter";
}

int SCI_METHOD RainLexer::GetIdentifier() {
    return SCLEX_AUTOMATIC;
}

const char* SCI_METHOD RainLexer::PropertyGet(const char* /*key*/) {
    return "";
}

//
// Scintilla exports
//

int SCI_METHOD GetLexerCount()
{
    return 1;
}

void SCI_METHOD GetLexerName(unsigned int /*index*/, char* name, int buflength)
{
    strncpy(name, "Rainmeter", buflength);
    name[buflength - 1] = '\0';
}

void SCI_METHOD GetLexerStatusText(unsigned int /*index*/, WCHAR* desc, int buflength)
{
    wcsncpy(desc, L"Rainmeter skin file", buflength);
    desc[buflength - 1] = L'\0';
}

LexerFactoryFunction SCI_METHOD GetLexerFactory(unsigned int index)
{
    return (index == 0) ? RainLexer::LexerFactory : nullptr;
}

}	// namespace RainLexer