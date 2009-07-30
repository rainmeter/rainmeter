#pragma once

/*
    Universal lexical analiser by hq_software
*/

#ifndef __LEXER_HPP__
#define __LEXER_HPP__

#include "pack.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uchar;

typedef enum {
    CH_LETTER = 0x01, CH_DIGIT = 0x02, CH_SEPARAT = 0x04,
    CH_SYMBOL = 0x08, CH_QUOTE = 0x10,
    CH_UNKNOWN= 0x7E, CH_FINAL = 0x7F
} hqCharType;

typedef enum {
    TOK_ERROR, TOK_NONE, TOK_FINAL, TOK_INT, TOK_FLOAT, TOK_SYMBOL,
    TOK_NAME, TOK_STRING
} hqTokenType;

#pragma pack(push,1)

typedef struct {
    char Sym[4];
    char Len;
    char Index;
    char More;
} SymbolRec;

typedef struct {
    // input params
    const char *SS;
    hqCharType *CharTypeTable;
    SymbolRec **SymTable;
    int		NoIntegers;
    int		cssn;	// Comment Start Symbol Number. -1 if none
    char       *ComEnd;	// End of comment
    // output params
    const char *Name;
    size_t		NameLen;
    double	ExtValue;
    int		IntValue;
    hqTokenType PrevTokenType;
    hqCharType	CharType;
} hqLexer;

#pragma pack(pop)

/* Main "API" */

int Lexer_SetParseString( hqLexer *lexer, const char *str );
hqTokenType Lexer_GetNextToken( hqLexer *lexer );
const char* Lexer_GetCurrentPos( hqLexer *lexer );

/* Misc */

void UpcaseWin1251Str( char *Str );
void InitCharTypeTable( hqCharType *CharTypeTable, int CharTypes );

void TypeTableAddChars( hqCharType *CharTypeTable, char *Symbols,
			hqCharType CharType );

void PrepareSymTable( SymbolRec **SymTable, char *symbols );

//int IsEngWin1251RusName( char *Str );

extern char const Win1251UpcaseTbl[];

#ifdef __cplusplus
}
#endif

#endif /* __LEXER_HPP__ */
