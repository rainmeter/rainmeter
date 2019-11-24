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

#ifndef RAINLEXER_LEXER_H_
#define RAINLEXER_LEXER_H_

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "LexerModule.h"
#include "CharacterSet.h"

using namespace Scintilla;

namespace RainLexer {

class RainLexer final : public ILexer4
{
protected:
    const LexicalClass* lexClasses;
    size_t nClasses;
public:
    RainLexer(const LexicalClass* lexClasses_ = nullptr, size_t nClasses_ = 0) :
        lexClasses(lexClasses_), nClasses(nClasses_) {};
    virtual ~RainLexer() = default;

    static ILexer4* LexerFactory();

    // ILexer
    void SCI_METHOD Release() override;
    int SCI_METHOD Version() const override;
    const char* SCI_METHOD PropertyNames() override;
    int SCI_METHOD PropertyType(const char* name) override;
    const char* SCI_METHOD DescribeProperty(const char* name) override;
    Sci_Position SCI_METHOD PropertySet(const char* key, const char* val) override;
    const char* SCI_METHOD DescribeWordListSets() override;
    Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument* pAccess) override;
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument* pAccess) override;
    void* SCI_METHOD PrivateCall(int operation, void* pointer) override;
    int SCI_METHOD LineEndTypesSupported() override;
    int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override;
    int SCI_METHOD SubStylesStart(int styleBase) override;
    int SCI_METHOD SubStylesLength(int styleBase) override;
    int SCI_METHOD StyleFromSubStyle(int subStyle) override;
    int SCI_METHOD PrimaryStyleFromStyle(int style) override;
    void SCI_METHOD FreeSubStyles() override;
    void SCI_METHOD SetIdentifiers(int style, const char* identifiers) override;
    int SCI_METHOD DistanceToSecondaryStyles() override;
    const char* SCI_METHOD GetSubStyleBases() override;
    int SCI_METHOD NamedStyles() override;
    const char* SCI_METHOD NameOfStyle(int style) override;
    const char* SCI_METHOD TagsOfStyle(int style) override;
    const char* SCI_METHOD DescriptionOfStyle(int style) override;

private:
    enum TextState
    {
        TS_DEFAULT,
        TS_COMMENT,
        TS_SECTION,
        TS_KEYWORD,
        TS_OPTION,
        TS_VALUE,
        TS_BANG,
        TS_VARIABLE,
        TS_LINEEND,
        TS_DIGITS
    };

    enum TextColor
    {
        TC_DEFAULT,
        TC_COMMENT,
        TC_SECTION,
        TC_KEYWORD,
        TC_EQUALS,
        TC_INVALID,
        TC_VALID,
        TC_BANG,
        TC_INTVARIABLE,
        TC_EXTVARIABLE,
        TC_DIGITS
    };
    WordList m_WordLists[6];
};

}// namespace RainLexer

#endif
