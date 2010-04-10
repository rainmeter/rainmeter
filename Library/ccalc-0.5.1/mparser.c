#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mparser.h"

//#define MY_DEBUG 1

#ifndef M_E
# define M_E		2.7182818284590452354
#endif
#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif

const double DblErR = -1.68736462823243E308;
const double DblNiN = -1.68376462823243E308;

#if (defined(__WIN32) || defined(__WIN32__)) && defined(RUSSIAN)

char eBrackets [] = "#Ошибка со скобками!";
char eSyntax   [] = "#Синтиксическая ошибка!";
char eInternal [] = "#Внутренняя ошибка!";
char eExtraOp  [] = "#Лишняя операция!";
char eInfinity [] = "#Где-то бесконечность!";
char eInvArg   [] = "#Неверный аргумент!";
char eUnknFunc [] = "# %s - Неизвестная функция/перменная!";
char eExtrnFunc[] = "#Ошибка внешней функции!";
char eLogicErr [] = "#Ошибка в логическом выражении!";
char eCalcErr  [] = "#Ошибка при вычислениях!";
char eUnexpEnd [] = "#Неожиданный конец скрипта!";
char eExpVarRet[] = "#Ожидается переменная или return!";
char eExpAssign[] = "#Ожидается присваивание!";
char eValSizErr[] = "#Значение слишком большое для операции!";
char eInvPrmCnt[] = "#Неверное количество параметров при вызове функции!"

#else 

char eBrackets [] = "#Brackets not match!";
char eSyntax   [] = "#Syntax error!";
char eInternal [] = "#Internal error!";
char eExtraOp  [] = "#Extra operation!";
char eInfinity [] = "#Infinity somewhere!";
char eInvArg   [] = "#Invalid argument!";
char eUnknFunc [] = "# %s - Unknown function/variable!";
char eExtrnFunc[] = "#External function error!";
char eLogicErr [] = "#Logical expression error!";
char eCalcErr  [] = "#Calculation error!";
char eUnexpEnd [] = "#Unexpected end of script!";
char eExpVarRet[] = "#Variable name or return expected!";
char eExpAssign[] = "#Assignment expected!";
char eValSizErr[] = "#Value too big for operation!";
char eInvPrmCnt[] = "#Invalid parameters count for function call!";

#endif /* __WIN32__ */

static double _neg_(double);
static double _frac_(double);
static double _trunc_(double);
static double _sgn_(double);
static double _neg_(double);
static double _floor_(double);
static double _ceil_(double);
static char* _round_( int paramcnt, double *args, hqStrMap *strparams, double *result );

const Operation BrOp = { OP_OBR };
const Operation NegOp = { OP_FUNC_ONEARG, (void*)&_neg_, 0, NULL };

const char OpPriorities[OP_FUNC_MULTIARG+1] = {
    5, 5, 5,    2, 2, 2, 2, 2,    -1, -1,   0,
    3, 3,    4, 4, 4, 4,
    5, 5, 5, 5,    2, 2, 2,   1, 2, 0, 2,
    -1, 6, 6 };

char MathSymbols[] =
    "\033\002" "<<" ">>" "**" "<>" ">=" "<=" "&&" "||" "/*" ":="
    "\033\001" "(+-*/%$^~&|=><?:),;";

char StdSymbols[] = "+-/*^~()<>%$,?:=&|;";

hqCharType MathCharTypeTable[256];
int initializations_performed = 0;

char func_names[] =
    "ATAN\000COS\000SIN\000TAN\000ABS\000"
    "EXP\000LN\000LOG\000SQRT\000FRAC\000"
    "TRUNC\000FLOOR\000CEIL\000ROUND\000ASIN\000"
    "ACOS\000SGN\000NEG\000E\000PI\000";

/* Indexes of some functions in func_names[] array */
#define FUNC_ROUND	13
#define FUNC_E		18
#define FUNC_PI		19

static char* CalcToObr( hqMathParser *parser );
static char* Calc( hqMathParser *parser );
static char* MathParser_ParseScript( hqMathParser* parser, double *result );
static char* MathParser_ParseFormula( hqMathParser* parser, double *result );

double (*func_addresses[]) () = {
    &atan, &cos, &sin, &tan, &fabs,
    &exp, &log, &log10, &sqrt, &_frac_,
    &_trunc_, &_floor_, _ceil_, (double(*)(double)) &_round_, &asin,
    &acos, &_sgn_, &_neg_, NULL, NULL };

hqStrMap *IntFunctions;

SymbolRec *MathSymTable[256];

char errbuf[256];
            
// hqMathParser implementation

hqMathParser* MathParser_Create( char *MoreLetters )
{
    hqMathParser *parser = calloc( 1, sizeof(hqMathParser) );
    if (!initializations_performed) {
	// init character tables
	InitCharTypeTable( MathCharTypeTable,
			   CH_LETTER | CH_DIGIT | CH_SEPARAT | CH_QUOTE );
	TypeTableAddChars( MathCharTypeTable, StdSymbols, CH_SYMBOL );
	if (MoreLetters)
	    TypeTableAddChars( MathCharTypeTable, MoreLetters, CH_LETTER );
	// init function maps
	PrepareSymTable( MathSymTable, MathSymbols );
	IntFunctions = Strmap_CreateFromChain( sizeof(void*),
						(char*)func_names,
						func_addresses );
	initializations_performed = 1;
    }
    parser->Lexer.NoIntegers = 1;
    parser->Lexer.SymTable = MathSymTable;
    parser->Lexer.CharTypeTable = MathCharTypeTable;
    parser->Lexer.cssn = 8;
    parser->Lexer.ComEnd = "*/";
    return parser;
}

void MathParser_Destroy( hqMathParser* parser )
{
    free( parser );
}

static char* PrepareFormula( hqMathParser* parser )
{
    int BrCnt = 0;
    const char *SS = parser->Lexer.SS;

    // Brackets Matching
    while ( (!parser->script && *SS) || (parser->script && *SS != ';') ) {
	if (*SS=='(')
	    ++BrCnt;
	else if (*SS==')' && --BrCnt<0)
	    goto brkerr;
	++SS;
    }
    if (BrCnt != 0)
brkerr:	return eBrackets;

    parser->OpTop = 0;
    parser->ValTop = -1;
    parser->OpStack[0].OperType = OP_OBR;
    parser->ObrDist = 2;
    return NULL;
}

char* MathParser_Parse( hqMathParser* parser, const char *Formula, double *result )
{
    if (!Formula || !*Formula) {
	*result = 0.0;
	return NULL;
    }

    parser->script = *Formula == '#' && *(Formula+1) == '!'
		     && MathCharTypeTable[ (uchar)*(Formula+2) ] == CH_SEPARAT;

    if ( parser->script )
	Formula += 3;
	
    Lexer_SetParseString( &parser->Lexer, Formula );

    return ( parser->script )
	?
	MathParser_ParseScript( parser, result )
	:
	MathParser_ParseFormula( parser, result );
}

static char* MathParser_ParseFormula( hqMathParser* parser, double *result )
{
    char *ErrorMsg;
    hqTokenType ToTi;

    if ( (ErrorMsg = PrepareFormula( parser )) != NULL )
	return ErrorMsg;

    ToTi = Lexer_GetNextToken( &parser->Lexer );
    for (;;) {
	--parser->ObrDist;
	switch (ToTi) {
	    case TOK_ERROR:
		return eSyntax;
	    case TOK_FINAL:
formulaend:	if ( (ErrorMsg = CalcToObr( parser )) != NULL )
		    return ErrorMsg;
		goto getout;
	    case TOK_FLOAT:
		parser->ValStack[++parser->ValTop] = parser->Lexer.ExtValue;
		break;
	    case TOK_SYMBOL:
		switch ( parser->Lexer.IntValue ) {
		    case OP_OBR:	// (
			parser->OpStack[++parser->OpTop] = BrOp;
			parser->ObrDist = 2;
			break;
		    case OP_CBR:	// )
			if ( (ErrorMsg = CalcToObr( parser )) != NULL )
			    return ErrorMsg;
			break;
		    case OP_COMMA: {	// ,
			Operation *pOp;
			if ( (ErrorMsg = CalcToObr( parser )) != NULL )
			    return ErrorMsg;
			if ( (pOp = &parser->OpStack[parser->OpTop])->OperType
			     == OP_FUNC_MULTIARG ) {
			    parser->OpStack[++parser->OpTop] = BrOp;
			    parser->ObrDist = 2;
			} else
			    return eSyntax;
			break;
		    }
		    default: {
			Operation Op;
			Op.OperType = (OperType_t) parser->Lexer.IntValue;
			switch (Op.OperType) {
			    case OP_FORMULAEND:
				if (parser->script)
				    goto formulaend;
				else
				    return eSyntax;
			    case OP_ADD:
				if (parser->ObrDist >= 1)
				    goto next_tok;
				break;
			    case OP_SUB:
				if (parser->ObrDist >= 1) {
				    parser->OpStack[++parser->OpTop] = NegOp;
				    goto next_tok;
				}
				break;
			    case OP_LOGIC:
			    case OP_LOGIC_SEP:
				parser->ObrDist = 2;
			    default:
				break;
			}
			while ( OpPriorities[ Op.OperType ]
				<= 
				OpPriorities[ parser->OpStack[parser->OpTop].OperType ] ) {
			    if ( (ErrorMsg = Calc( parser )) != NULL )
				return ErrorMsg;
			}
			parser->OpStack[++parser->OpTop] = Op;
			break;
		    }
		}
		break;
	    case TOK_NAME: {
		Operation Op;
		double *value, dblval;
		void **func;
		int funcnum, /*i,*/ namelen = parser->Lexer.NameLen;
		
//		const char *SS = parser->Lexer.Name;
//		for (i = namelen; i>0; --i)
//		    *SS++ = Win1251UpcaseTbl[ (int) (uchar) *SS ];
		
		funcnum = StrMap_LenIndexOf( IntFunctions,
					parser->Lexer.Name,
					parser->Lexer.NameLen,
					(void**) &func );
		if ( funcnum >= 0 ) {
		    Op.Func = *func;
		    switch ( funcnum ) {
			case FUNC_E:
			    parser->ValStack[++parser->ValTop] = M_E;
			    break;
			case FUNC_PI:
			    parser->ValStack[++parser->ValTop] = M_PI;
			    break;
			case FUNC_ROUND:
			    Op.OperType = OP_FUNC_MULTIARG;
			    Op.PrevValTop = parser->ValTop;
			    Op.StrParams = NULL;
			    parser->OpStack[++parser->OpTop] = Op;
			    break;
			default:// Internal function
			    Op.OperType = OP_FUNC_ONEARG;
			    parser->OpStack[++parser->OpTop] = Op;
		    }
		} else if (parser->Parameters
			   &&
			   StrMap_LenIndexOf( parser->Parameters,
			                      parser->Lexer.Name,
			            	      parser->Lexer.NameLen,
					      (void**) &value )	>= 0
			  ) {
		    if (*value==DblErR) {
			return eInternal;
		    } else
			parser->ValStack[++parser->ValTop] = *value;
		} else if (parser->ExtFunctions
			   &&
			   StrMap_LenIndexOf( parser->ExtFunctions,
			                      parser->Lexer.Name,
			                      parser->Lexer.NameLen,
					      (void**) &func ) >= 0
			  ) {
		    Op.Func = *func;
		    Op.OperType = OP_FUNC_MULTIARG;
		    Op.PrevValTop = parser->ValTop;
		    Op.StrParams = NULL;
		    parser->OpStack[++parser->OpTop] = Op;
		} else if (parser->VarParams
			   &&
			   StrMap_LenIndexOf( parser->VarParams,
			                      parser->Lexer.Name,
			            	      parser->Lexer.NameLen,
					      (void**) &value )	>= 0
			  ) {
		    if (*value==DblErR) {
			return eInternal;
		    } else
			parser->ValStack[++parser->ValTop] = *value;
		} else if (parser->MoreParams
			   &&
			   (*parser->MoreParams)( parser->Lexer.Name,
						  parser->Lexer.NameLen,
						  &dblval,
						  parser->ParamFuncParam )
			  ) {
		    parser->ValStack[++parser->ValTop] = dblval;
		} else {
		    char buf[256];
		    strncpy( buf, parser->Lexer.Name, parser->Lexer.NameLen );
		    buf[ parser->Lexer.NameLen ] = '\0';
		    sprintf( errbuf, eUnknFunc, buf );
		    return errbuf;
		}
		break;
	    }
	    case TOK_STRING: {
		Operation *pOp;
		if (parser->OpTop < 1)
		    return eSyntax;
		if ( (pOp = &parser->OpStack[parser->OpTop-1])->OperType
		     == OP_FUNC_MULTIARG ) {
		    if (!pOp->StrParams)
			pOp->StrParams = Strmap_Create( 0, 0 );
		    StrMap_AddStrLen( pOp->StrParams,
				      parser->Lexer.Name,
				      parser->Lexer.NameLen, NULL );
		    parser->ValStack[++parser->ValTop] = DblNiN;
		} else
		    return eSyntax;
		break;
	    }
	    default:
		return eSyntax;
	}
next_tok:
    ToTi = Lexer_GetNextToken( &parser->Lexer );
    } // forever

getout:
    if (parser->OpTop != -1 || parser->ValTop != 0)
	return eInternal;
	
    *result = parser->ValStack[0];
    return NULL;
}

static char* MathParser_ParseScript( hqMathParser* parser, double *result )
{
    char *ErrorMsg = NULL;
    hqTokenType ToTi;
    int expectvar = 1, was_return = 0;
    const char *varname = NULL;
    int varnamelen = 0;

    parser->VarParams = Strmap_Create( sizeof(double), 0 );

    ToTi = Lexer_GetNextToken( &parser->Lexer );
    for (;;) {
	switch (ToTi) {
	    case TOK_FINAL:
		ErrorMsg = eUnexpEnd;
		goto getout;
	    case TOK_NAME: {
		if (!expectvar) {
		    ErrorMsg = eExpVarRet;
		    goto getout;
		} else {
//		    int  i;
//		    const char *SS = parser->Lexer.Name;

		    varnamelen = parser->Lexer.NameLen;

//		    for (i = varnamelen; i>0; --i)
//			*SS++ = Win1251UpcaseTbl[ (int) (uchar) *SS ];
		}
		varname = parser->Lexer.Name;

		was_return = strncmp( varname, "RETURN", varnamelen ) == 0;
		if ( was_return ) {
		    ErrorMsg = MathParser_ParseFormula( parser, result );
		    goto getout;
		}
		expectvar = 0;
	    	break;
	    }
	    case TOK_SYMBOL: {
		double *value;

		if ( parser->Lexer.IntValue != OP_ASSIGN || expectvar ) {
		    ErrorMsg = eExpAssign;
		    goto getout;
		}
		ErrorMsg = MathParser_ParseFormula( parser, result );
		if (ErrorMsg)
		  goto getout;

		if ( StrMap_LenIndexOf( parser->VarParams,
					varname, varnamelen,
					(void**) &value ) >= 0 )
		    *value = *result;
		else
		    StrMap_AddStrLen( parser->VarParams, varname, varnamelen,
				      result );
		expectvar = 1;
		break;
		}
	    default:
		ErrorMsg = eSyntax;
		goto getout;
	}
    ToTi = Lexer_GetNextToken( &parser->Lexer );
    } // forever

getout:
    StrMap_Destroy( parser->VarParams );
    parser->VarParams = NULL;
    return ErrorMsg;
}

static char* Calc( hqMathParser *parser )
{
    double Res, ValR;
    Operation Op = parser->OpStack[parser->OpTop--];

    // multi-argument external or internal fucntion
    if ( Op.OperType == OP_FUNC_MULTIARG ) {
	int paramcnt = parser->ValTop - Op.PrevValTop;
	char *ErrorMsg;
#ifdef MY_DEBUG
	printf( "ValTop = %d, OpTop = %d, PrevValTop = %d\n",
		parser->ValTop, parser->OpTop, Op.PrevValTop );
#endif
	parser->ValTop = Op.PrevValTop;
	ErrorMsg = (*(MultiArgFunc)Op.Func)( paramcnt,
					&parser->ValStack[parser->ValTop+1],
					Op.StrParams, &Res );
	if (ErrorMsg)
	    return ErrorMsg;
	if (Op.StrParams)
	    StrMap_Destroy( Op.StrParams );
	parser->ValStack[++parser->ValTop] = Res;
#ifdef MY_DEBUG
	printf("ValTop = %d, OpTop = %d\n", parser->ValTop, parser->OpTop );
#endif
	return NULL;
    }

    if (Op.OperType==OP_LOGIC)
	return NULL; 

    // get right arg
    if (parser->ValTop<0)
	return eExtraOp;
    ValR = parser->ValStack[parser->ValTop--];

    // one arg operations
    if (Op.OperType==OP_NOT) {
	if (ValR >= INT_MIN && ValR <= INT_MAX)
	    Res = ~((int) ValR);
	else
	    return eValSizErr;
    } else if (Op.OperType==OP_FUNC_ONEARG) {
	Res = (*(OneArgFunc)Op.Func)( ValR );
    } else {
	// get left arg
	double ValL;
	if (parser->ValTop<0)
	    return eExtraOp;
	ValL = parser->ValStack[parser->ValTop--];
	switch (Op.OperType) {
	    // Binary
	    case OP_SHL:
		if (ValL >= INT_MIN && ValL <= INT_MAX && ValR >= INT_MIN && ValR <= INT_MAX)
		    Res = (int) ValL << (int) ValR;
		else
		    return eValSizErr;
		break;
	    case OP_SHR:
		if (ValL >= INT_MIN && ValL <= INT_MAX && ValR >= INT_MIN && ValR <= INT_MAX)
		    Res = (int) ValL >> (int) ValR;
		else
		    return eValSizErr;
		break;
	    case OP_POW:
		Res = pow( ValL, ValR );	break;
	    // Logical
	    case OP_LOGIC_NEQ:
		Res = ValL != ValR;		break;
	    case OP_LOGIC_GEQ:
		Res = ValL >= ValR;		break;
	    case OP_LOGIC_LEQ:
		Res = ValL <= ValR;		break;
	    case OP_LOGIC_AND:
		Res = ValL && ValR;	break;
	    case OP_LOGIC_OR:
		Res = ValL || ValR;	break;
	    // Arithmetic
	    case OP_ADD:
		Res = ValL + ValR;		break;
	    case OP_SUB:
		Res = ValL - ValR;		break;
	    case OP_MUL:
		Res = ValL * ValR;		break;
	    case OP_DIV:
		if (ValR == 0.0)
		    return eInfinity;
		Res = ValL / ValR;
		break;
	    case OP_MOD:
		Res = fmod(ValL, ValR);	break;
	    case OP_UNK:
		if (ValL<=0)
		    Res = 0.0;
		else if (ValR==0.0)
		    return eInfinity;
		else
		    Res = ceil(ValL / ValR);
		break;
	    // Bitwise
	    case OP_XOR:
		if (ValL >= INT_MIN && ValL <= INT_MAX && ValR >= INT_MIN && ValR <= INT_MAX)
			Res = (int) ValL ^ (int) ValR;
		else
			return eValSizErr;
		break;
	    case OP_AND:
		if (ValL >= INT_MIN && ValL <= INT_MAX && ValR >= INT_MIN && ValR <= INT_MAX)
			Res = (int) ValL & (int) ValR;
		else
			return eValSizErr;
		break;
	    case OP_OR:
		if (ValL >= INT_MIN && ValL <= INT_MAX && ValR >= INT_MIN && ValR <= INT_MAX)
			Res = (int) ValL | (int) ValR;
		else
			return eValSizErr;
		break;
	    // Logical
	    case OP_EQU:
		Res = ValL == ValR;		break;
	    case OP_GREATER:
		Res = ValL > ValR;		break;
	    case OP_LESS:
		Res = ValL < ValR;		break;
	    case OP_LOGIC_SEP: {
		// needs three arguments
		double ValLL;
		if (parser->OpTop < 0
		    || parser->OpStack[parser->OpTop--].OperType != OP_LOGIC)
		    return eLogicErr;
		ValLL = parser->ValStack[ parser->ValTop-- ];
		Res = ValLL ? ValL : ValR;
		break;
	    }
	    default:
		return eInternal;
	}
    }
    parser->ValStack[++parser->ValTop] = Res;
    return NULL;
}

static char* CalcToObr( hqMathParser *parser )
{
    while ( parser->OpStack[parser->OpTop].OperType != OP_OBR ) {
	char *ErrorMsg;
	if ( (ErrorMsg = Calc( parser )) != NULL )
	    return ErrorMsg;
    }
    --parser->OpTop;
    return NULL;
}

/* misc functions */

static double _frac_( double x )
{
    double y;
    return modf(x, &y);
}

static double _trunc_( double x )
{
    return (x >= 0.0) ? floor(x) : ceil(x);
}

static double _sgn_( double x )
{
    return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

static double _neg_( double x )
{
    return -x;
}

static double _floor_( double x )
{
    return floor(x);
}

static double _ceil_( double x )
{
    return ceil(x);
}

/* "Advanced" round function; second argument - sharpness */
static char* _round_( int paramcnt, double *args, hqStrMap *strparams, double *result )
{
    int i, sharpness;
    double x, coef;

    if (paramcnt == 1)
	sharpness = 0;
    else if (paramcnt == 2)
	sharpness = (int) args[1];
    else
	return eInvPrmCnt;

    x = args[0];
    if (sharpness < 0) {
	coef = 0.1;
        sharpness = -sharpness;
    } else
	coef = 10;

    for (i = 0; i < sharpness; i++)
	x *= coef;

    x = (x + ( (x >= 0) ? 0.5 : -0.5 ) );
    if (x >= 0.0)
	x = floor(x);
    else
	x = ceil(x);

    for (i = 0; i < sharpness; i++)
	x /= coef;

    *result = x;

    return NULL;
}
