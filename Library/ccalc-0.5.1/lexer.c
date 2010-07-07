/*
    Universal lexical analiser implementation
*/
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "lexer.h"

//#define MY_DEBUG 1

#ifdef MY_DEBUG
#include <stdio.h>
#endif
/* Uppercase translation table for the Win1251 charset */
const char Win1251UpcaseTbl[] =
    "\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
    "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
    " !\042#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\177"
    "\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
    "\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
    "\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
    "\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277"
    "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
    "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
    "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
    "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337";

char Win1251RusLetters[] =
    "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
    "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
    "\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
    "\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";


hqCharType Win1251NameTbl[256];
int win1251nametbl_initialized = 0;

int FindSymbol( SymbolRec **SymTable, const char *str, int *nchars );

// initializations

void InitCharTypeTable( hqCharType *CharTypeTable, int CharTypes )
{
    int ch;
#ifdef MY_DEBUG
    printf( "CharTypeTable = 0x%X; CharTypes = %d\n", (unsigned)CharTypeTable,
	    CharTypes );
#endif
    memset( CharTypeTable, CH_UNKNOWN, 256 * sizeof(hqCharType) );

    CharTypeTable[0] = CH_FINAL;

    if (CharTypes & CH_SEPARAT) {
	CharTypeTable[' '] = CH_SEPARAT;
	CharTypeTable[9]  = CH_SEPARAT;
	CharTypeTable[13]  = CH_SEPARAT;
	CharTypeTable[10]  = CH_SEPARAT;
    }

    if (CharTypes & CH_QUOTE) {
	CharTypeTable['\'']  = CH_QUOTE;
    }

    if (CharTypes & CH_LETTER) {
	for (ch='A'; ch<='Z'; ch++)
	    CharTypeTable[ch] = CH_LETTER;
	for (ch='a'; ch<='z'; ch++)
	    CharTypeTable[ch] = CH_LETTER;
	CharTypeTable['_'] = CH_LETTER;
    }

    if (CharTypes & CH_DIGIT) {
	for (ch='0'; ch<='9'; ch++)
	    CharTypeTable[ch] = CH_DIGIT;
    }
}

void TypeTableAddChars( hqCharType *CharTypeTable, char *Symbols,
			hqCharType CharType )
{
    while (*Symbols)
	CharTypeTable[ (uchar) *Symbols++] = CharType;
}

void UpcaseWin1251Str( char *Str )
{
    while (( *Str = Win1251UpcaseTbl[ (uchar) *Str ] ))
	++Str;
}


// hqLexer implementation

#define CHARTYPEPP lexer->CharTypeTable[ (uchar) *++(lexer->SS) ]
#define CHARTYPE lexer->CharTypeTable[ (uchar) *lexer->SS ]

int Lexer_SetParseString( hqLexer *lexer, const char *str )
{
    lexer->PrevTokenType = TOK_NONE;
    if ( !str || !*str )
	return 0;

    lexer->SS = str;
    lexer->CharType = CHARTYPE;
    return 1;
}

hqTokenType Lexer_GetNextToken( hqLexer *lexer )
{
    hqTokenType result = TOK_ERROR;

next_token:

    while ( lexer->CharType == CH_SEPARAT )
	lexer->CharType = CHARTYPEPP;
	
    switch ( lexer->CharType ) {
	case CH_FINAL:
	    result = TOK_FINAL;
	    break;
	case CH_LETTER:
	    lexer->Name = lexer->SS;
	    do {
		lexer->CharType = CHARTYPEPP;
	    } while (lexer->CharType <= CH_DIGIT);
	    lexer->NameLen = lexer->SS - lexer->Name;
	    result = TOK_NAME;
	    break;
	case CH_DIGIT: {
	    char *NewSS, ch = *lexer->SS, nch = *(lexer->SS+1);
	    int intreaded = 0;
	    // Readind hex number
	    if ( ch == '0' && nch == 'x' ) {
	    	lexer->IntValue = strtol( lexer->SS, &NewSS, 16 );
                intreaded = 1;
	    }
	    // Readind oct number
		if ( ch == '0' && nch == 'o') { // original version: if ( ch == '0' && nch >= '0' && nch <='9')
	    	lexer->IntValue = strtol( lexer->SS+2, &NewSS, 8 );
                intreaded = 1;
	    }

		// Readind bin number
		if ( ch == '0' && nch == 'b') { // original version: if ( ch == '0' && nch >= '0' && nch <='9')
	    	lexer->IntValue = strtol( lexer->SS+2, &NewSS, 2 );
                intreaded = 1;
	    }

	    if (intreaded == 1) {
                if ( lexer->SS != NewSS ) {
		    lexer->SS = NewSS;
		    if (lexer->NoIntegers) {
			lexer->ExtValue = lexer->IntValue;
			result = TOK_FLOAT;
		    } else
			result = TOK_INT;
		    lexer->CharType = CHARTYPE;
                }
                break;
	    }

            // Readind dec number
	    lexer->ExtValue = strtod( lexer->SS, &NewSS );
	    if ( lexer->SS != NewSS ) {;
		lexer->SS = NewSS;
		if ( !lexer->NoIntegers
		    && lexer->ExtValue<=INT_MAX
		    && lexer->ExtValue>=INT_MAX
		    && (double)( lexer->IntValue = (uchar) lexer->ExtValue )
			== lexer->ExtValue ) {
		    result = TOK_INT;
		} else
		    result = TOK_FLOAT;
		lexer->CharType = CHARTYPE;
	    }
	    break;
	}
	case CH_SYMBOL: {
	    int nchars;
	    int i = FindSymbol( lexer->SymTable, lexer->SS, &nchars );
	    if (i >= 0) {
		lexer->SS += nchars;
		if (i == lexer->cssn) {
		    char comend = *lexer->ComEnd;
		    char comendpp = *(lexer->ComEnd+1);
		    while ( *lexer->SS ) {
			if ( *lexer->SS == comend
			     && 
			     ( comendpp == '\0' || *(lexer->SS+1) == comendpp )
			   ) {
			    ++lexer->SS;
			    if (comendpp != '\0')
				++lexer->SS;
			    lexer->CharType = CHARTYPE;
			    goto next_token;
			}
			++lexer->SS;
		    }
		    break;
		}
		lexer->CharType = CHARTYPE;
		lexer->IntValue = i;
		result = TOK_SYMBOL;
	    }
	    break;
	}
	case CH_QUOTE:
	    lexer->Name = ++(lexer->SS);
	    while ( lexer->CharTypeTable[ (uchar)*lexer->SS ] != CH_QUOTE
		    && *(lexer->SS) != '\0' )
		++lexer->SS;
	    if ( CHARTYPE == CH_QUOTE ) {
		lexer->NameLen = lexer->SS - lexer->Name;
		++lexer->SS;
		lexer->CharType = CHARTYPE;
		result = TOK_STRING;
	    }
	    break;
	default:
	    break;
    }
    return lexer->PrevTokenType = result;
}

const char* Lexer_GetCurrentPos( hqLexer *lexer )
{
    return lexer->SS;
}

// misc functions

void PrepareSymTable( SymbolRec **SymTable, char *symbols )
{
    int i = 0, nchars = 1;
    memset( SymTable, 0, 256 * sizeof(void*) );
    while (*symbols) {
	if (*symbols=='\033') {
	    nchars = *++symbols;
	    ++symbols;
	} else {
	    SymbolRec **RecList = SymTable + *symbols;
	    SymbolRec *Rec = *RecList;
	    int count = 0;
	    while ( Rec ) {
		++count;
		if ( Rec->More )
		    ++Rec;
		else
		    break;
	    }
	    if ( Rec ) {
		*RecList = (SymbolRec*)
			realloc( *RecList, (count+1)*sizeof(SymbolRec) );
		Rec = *RecList + count;
		(Rec-1)->More = 1;
	    } else {
		*RecList = (SymbolRec*) malloc( sizeof(SymbolRec) );
		Rec = *RecList;
	    }
	    strncpy( Rec->Sym, symbols, 4 );
	    Rec->Len = (char) nchars;
	    Rec->Index = (char) i;
	    Rec->More = 0;
	    symbols += nchars;
	    ++i;
	}
    }
}

int FindSymbol( SymbolRec **SymTable, const char *str, int *nchars )
{
    SymbolRec *Rec = SymTable[ (int)*str ];
    while ( Rec ) {
	if ( (Rec->Len == 1 && Rec->Sym[0] == str[0])
	     ||
	     (Rec->Len == 2 && Rec->Sym[0] == str[0] && Rec->Sym[1] == str[1])
	     ||
	     (Rec->Len == 3 && Rec->Sym[0] == str[0] && Rec->Sym[1] == str[1]
	      && Rec->Sym[2] == str[2])
	   ) {
	    *nchars = Rec->Len;
	    return Rec->Index;
	}
	Rec = ( Rec->More ) ? Rec + 1 : NULL;
    }
    return -1;
}

