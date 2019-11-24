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

ILexer4* RainLexer::LexerFactory()
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
	return lvRelease4;
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

Sci_Position SCI_METHOD RainLexer::WordListSet(int n, const char *wl) {
	if (n < _countof(m_WordLists)) {
		WordList wlNew;
		wlNew.Set(wl);
		if (m_WordLists[n] != wlNew) {
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

	length += startPos;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	int state = TS_DEFAULT;
	char* name = nullptr;
	int count = 0;
	int digits = 0;
	for (auto i = startPos; i < static_cast<Sci_PositionU>(length); ++i)
	{
		// Make ch 0 if at EOF.
		char ch = (i == static_cast<Sci_PositionU>(length) - 1) ? '\0' : styler[i];

		// Amount of EOL chars is 2 (\r\n) with the Windows format and 1 (\n) with Unix format.
		int chEOL = (styler[i] == '\n') ? ((styler[i - 1] == '\r') ? 2 : 1): 0;

		switch(state)
		{
		case TS_DEFAULT:
			switch (ch)
			{
			case '\0':
			case '\n':
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '[':
				state = TS_SECTION;
				break;

			case ';':
				state = TS_COMMENT;
				break;

			case '\t':
			case ' ':
				break;

			default:
				if (isalpha(ch) > 0|| ch == '@')
				{
					count = 0;
					digits = 0;
					buffer[count++] = MakeLowerCase(ch);
					state = TS_KEYWORD;
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_COMMENT:
			// Style as comment when EOL (or EOF) is reached
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i - chEOL, TC_COMMENT);
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_SECTION:
			// Style as section when EOL (or EOF) is reached unless section name has a space
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i - chEOL, TC_SECTION);
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_KEYWORD:
			// Read max. 32 chars into buffer until the equals sign (or EOF/EOL) is met.
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '=':
				// Ignore trailing whitespace
				while (count > 0 && isspacechar(buffer[count - 1]))
				{
					--count;
				}

				buffer[count] = '\0';

				if (keywords.InList(buffer))
				{
					state = TS_VALUE;
				}
				else if (optwords.InList(buffer))
				{
					buffer[count++] = '=';
					state = TS_OPTION;

					// Ignore leading whitepsace
					while (isspacechar(styler[i + 1]))
					{
						++i;
					}
				}
				else if (digits > 0)
				{
					// Try removing chars from the end to check for words like ScaleN
					buffer[count - digits] = '\0';
					digits = 0;
					state = TS_VALUE;

					if (!numwords.InList(buffer))
					{
						break;
					}
				}
				else
				{
					state = TS_VALUE;
					break;
				}

				styler.ColourTo(i - 1, TC_KEYWORD);
				styler.ColourTo(i, TC_EQUALS);
				break;

			default:
				if (count < _countof(buffer))
				{
					if (isdigit(ch))
					{
						++digits;
					}
					buffer[count++] = MakeLowerCase(ch);
				}
				else
				{
					state = TS_LINEEND;
				}
			}
			break;

		case TS_OPTION:
			// Read value into buffer and check if it's valid for cases like StringAlign=RIGHT
			switch (ch)
			{
			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			case '\0':
				// Read the last character if at EOF
				buffer[count++] = MakeLowerCase(styler[i++]);

			case '\r':
				++chEOL;
			case '\n':
				while (isspacechar(buffer[count - 1]))
				{
					--count;
				}

				buffer[count] = '\0';
				count = 0;
				state = TS_DEFAULT;

				if (options.InList(buffer))
				{
					styler.ColourTo(i - chEOL, TC_VALID);
				}
				else
				{
					styler.ColourTo(i - chEOL, TC_INVALID);
				}
				styler.ColourTo(i, TC_DEFAULT);
				break;

			default:
				if (count < _countof(buffer))
				{
					buffer[count++] = MakeLowerCase(ch);
				}
				else
				{
					state = TS_LINEEND;
				}
			}
			break;

		case TS_VALUE:
			// Read values to highlight variables and bangs
			switch (ch)
			{
			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			case '!':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_BANG;
				break;

			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_BANG:
			// Highlight bangs
			switch (ch)
			{
			case '\0':
				buffer[count++] = MakeLowerCase(styler[i++]);

			case '\n':
			case ' ':
			case '[':
			case ']':
				buffer[count] = '\0';
				count = 0;
				state = (ch == '\n') ? TS_DEFAULT : TS_VALUE;

				// Skip rainmeter before comparing the bang
				if (bangs.InList(&buffer[(strncmp(buffer, "rainmeter", 9) == 0) ? 9 : 0]))
				{
					styler.ColourTo(i - chEOL, TC_BANG);
				}
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '\r':
				break;

			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			default:
				if (count < _countof(buffer))
				{
					buffer[count++] = MakeLowerCase(ch);
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_VARIABLE:
			// Highlight variables
			switch (ch)
			{
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '\0':
			case '#':
				if (count)
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
				state = TS_VALUE;
				break;

			case ' ':
			case '[':
			case ']':
				state = TS_VALUE;
				break;

			default:
				if (count < _countof(buffer))
				{
					buffer[count++] = MakeLowerCase(ch);
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_LINEEND:
			// Apply default style when EOL (or EOF) is reached
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
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

	for (auto i = startPos, isize = static_cast<Sci_PositionU>(length); i < isize; ++i)
	{
		if ((styler[i] == '\n') || (i == length - 1))
		{
			int level = (styler.StyleAt(i - 2) == TS_SECTION)
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