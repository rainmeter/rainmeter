/*
  Copyright (C) 2011 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNEstring FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#include "StdAfx.h"
#include "MathParser.h"

static const int MAX_STACK_SIZE = 32;
static const double M_E = 2.7182818284590452354;
static const double M_PI = 3.14159265358979323846;

typedef double (*OneArgProc)(double arg);
typedef char* (*MultiArgProc)(int paramcnt, double* args, double* result);
typedef double (*FunctionProc)(double);

typedef enum
{
	OP_SHL,
	OP_SHR,
	OP_POW,
	OP_LOGIC_NEQ,
	OP_LOGIC_GEQ,
	OP_LOGIC_LEQ,
	OP_LOGIC_AND,
	OP_LOGIC_OR,
	OP_OBR,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_UNK,
	OP_XOR,
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_EQU,
	OP_GREATER,
	OP_SMALLER,
	OP_LOGIC,
	OP_LOGIC_SEP,
	OP_CBR,
	OP_COMMA,
	OP_FUNC_ONEARG, // Special
	OP_FUNC_MULTIARG // Special
} OperationType;

typedef struct
{
	void* Func;
	char prevvalTop;
	OperationType type;
} Operation;

typedef enum
{
	CH_LETTER  = 0x01,
	CH_DIGIT   = 0x02,
	CH_SEPARAT = 0x04,
	CH_SYMBOL  = 0x08,
	CH_UNKNOWN = 0x7E,
	CH_FINAL   = 0x7F
} CharType;

typedef enum
{
	TOK_ERROR,
	TOK_NONE,
	TOK_FINAL,
	TOK_FLOAT,
	TOK_SYMBOL,
	TOK_NAME
} MathTokenType;

typedef struct
{
	char* name;
	FunctionProc proc;
	unsigned char length;
} Function;

typedef struct
{
	const char* string;
	const char* name;
	size_t nameLen;
	double extValue;
	int intValue;
	MathTokenType PrevTokenType;
	CharType CharType;
} Lexer;

char eBrackets [] = "Unmatched brackets";
char eSyntax   [] = "Syntax error";
char eInternal [] = "Internal error";
char eExtraOp  [] = "Extra operation";
char eInfinity [] = "Infinity somewhere";
char eInvArg   [] = "Invalid argument";
char eUnknFunc [] = "\"%s\" is unknown";
char eLogicErr [] = "Logical expression error";
char eCalcErr  [] = "Calculation error";
char eValSizErr[] = "Value too big for operation";
char eInvPrmCnt[] = "Invalid function parameter count";
char g_ErrorBuffer[128];

static char* CalcToObr();
static char* Calc();
static int GetFunction(const char* str, size_t len, void** data);
int FindSymbol(const char* str);

static int Lexer_SetParseString(const char* str);
static MathTokenType Lexer_GetNextToken();

static double neg(double x);
static double frac(double x);
static double trunc(double x);
static double sgn(double x);
static char* round(int paramcnt, double* args, double* result);

static Function g_Functions[] =
{
	{ "atan", &atan, 4 },
	{ "cos", &cos, 3 },
	{ "sin", &sin, 3 },
	{ "tan", &tan, 3 },
	{ "abs", &fabs, 3 },
	{ "exp", &exp, 3 },
	{ "ln", &log, 2 },
	{ "log", &log10, 3 },
	{ "sqrt", &sqrt, 4 },
	{ "frac", &frac, 4 },
	{ "trunc", &trunc, 5 },
	{ "floor", &floor, 5 },
	{ "ceil", &ceil, 4 },
	{ "round", (FunctionProc)&round, 5 },
	{ "asin", &asin, 4 },
	{ "acos", &acos, 4 },
	{ "sgn", &sgn, 4 },
	{ "neg", &neg, 4 },
	{ "e", NULL, 1 },
	{ "pi", NULL, 2}
};

static const int MAX_FUNC_LEN = 5;

#define FUNC_ROUND	13
#define FUNC_E		18
#define FUNC_PI		19

static const Operation g_BrOp = { NULL, '0', OP_OBR };
static const Operation g_NegOp = { (void*)&neg, '0', OP_FUNC_ONEARG };

static const char g_OpPriorities[OP_FUNC_MULTIARG + 1] =
{
	5, // OP_SHL
	5, // OP_SHR
	5, // OP_POW
	2, // OP_LOGIC_NEQ
	2, // OP_LOGIC_GEQ
	2, // OP_LOGIC_LEQ
	2, // OP_LOGIC_AND
	2, // OP_LOGIC_OR
	0, // OP_OBR
	3, // OP_ADD
	3, // OP_SUB
	4, // OP_MUL
	4, // OP_DIV
	4, // OP_MOD
	4, // OP_UNK
	5, // OP_XOR
	5, // OP_NOT
	5, // OP_AND
	5, // OP_OR
	2, // OP_EQU
	2, // OP_GREATER
	2, // OP_SMALLER
	1, // OP_LOGIC
	2, // OP_LOGIC_SEP
	0, // OP_CBR
	2, // OP_COMMA
	6, // OP_FUNC_ONEARG
	6  // OP_FUNC_MULTIARG
};

static Operation g_OpStack[MAX_STACK_SIZE];
static double g_ValStack[MAX_STACK_SIZE];
static int g_OpTop = 0;
static int g_ValTop = -1;
static int g_ObrDist = 0;
static CharType g_CharTypes[256] = {(CharType)0};
static Lexer g_Lexer = {0};

void MathParser::Initialize()
{
	g_CharTypes['\0'] = CH_FINAL;

	g_CharTypes[' '] = g_CharTypes['\t'] = g_CharTypes['\n'] = CH_SEPARAT;

	g_CharTypes['_'] = CH_LETTER;
	for (int ch = 'A'; ch <= 'Z'; ++ch) g_CharTypes[ch] = CH_LETTER;
	for (int ch = 'a'; ch <= 'z'; ++ch) g_CharTypes[ch] = CH_LETTER;
	for (int ch = '0'; ch <= '9'; ++ch) g_CharTypes[ch] = CH_DIGIT;

	g_CharTypes['+'] = g_CharTypes['-'] = g_CharTypes['/'] = g_CharTypes['*'] =
		g_CharTypes['~'] = g_CharTypes['('] = g_CharTypes[')'] = g_CharTypes['<'] = 
		g_CharTypes['>'] = g_CharTypes['%'] = g_CharTypes['$'] = g_CharTypes[','] = 
		g_CharTypes['?'] = g_CharTypes[':'] = g_CharTypes['='] = g_CharTypes['&'] = 
		g_CharTypes['|'] = CH_SYMBOL;
}

char* MathParser::Check(const char* formula)
{
	int BrCnt = 0;

	// Brackets Matching
	while (*formula)
	{
		if (*formula == '(')
		{
			++BrCnt;
		}
		else if (*formula == ')' && (--BrCnt < 0))
		{
			return eBrackets;
		}
		++formula;
	}

	if (BrCnt != 0)
	{
		return eBrackets;
	}

	return NULL;
}

char* MathParser::CheckParse(const char* formula, double* result)
{
	char* ret = Check(formula);
	if (ret) return ret;

	ret = Parse(formula, NULL, result);
	if (ret) return ret;

	return NULL;
}

char* MathParser::Parse(const char* formula, ParameterSearchProc searchProc, double* result)
{
	if (!formula || !*formula)
	{
		*result = 0.0;
		return NULL;
	}

	Lexer_SetParseString(formula);

	g_OpTop = 0;
	g_ValTop = -1;
	g_OpStack[0].type = OP_OBR;
	g_ObrDist = 2;

	char* error;
	MathTokenType token = Lexer_GetNextToken();
	for (;;)
	{
		--g_ObrDist;
		switch (token)
		{
		case TOK_ERROR:
			return eSyntax;

		case TOK_FINAL:
			if ((error = CalcToObr()) != NULL)
				return error;
			goto setResult;

		case TOK_FLOAT:
			g_ValStack[++g_ValTop] = g_Lexer.extValue;
			break;

		case TOK_SYMBOL:
			switch (g_Lexer.intValue)
			{
			case OP_OBR:	// (
				{
					g_OpStack[++g_OpTop] = g_BrOp;
					g_ObrDist = 2;
				}
				break;

			case OP_CBR:	//)
				{
					if ((error = CalcToObr()) != NULL) return error;
				}
				break;

			case OP_COMMA:	// ,
				{	
					Operation* pOp;
					if ((error = CalcToObr()) != NULL) return error;
						
					if ((pOp = &g_OpStack[g_OpTop])->type == OP_FUNC_MULTIARG)
					{
						g_OpStack[++g_OpTop] = g_BrOp;
						g_ObrDist = 2;
					}
					else
					{
						return eSyntax;
					}
				}
				break;

			default:
				{
					Operation op;
					op.type = (OperationType) g_Lexer.intValue;
					switch (op.type)
					{
					case OP_ADD:
						if (g_ObrDist >= 1) goto nextToken;
						break;

					case OP_SUB:
						if (g_ObrDist >= 1)
						{
							g_OpStack[++g_OpTop] = g_NegOp;
							goto nextToken;
						}
						break;

					case OP_LOGIC:
					case OP_LOGIC_SEP:
						g_ObrDist = 2;
						break;
					}

					while (g_OpPriorities[op.type] <= g_OpPriorities[g_OpStack[g_OpTop].type])
					{
						if ((error = Calc()) != NULL) return error;
					}
					g_OpStack[++g_OpTop] = op;
				}
				break;
			}
			break;

		case TOK_NAME: 
			{
				Operation op;
				double dblval;
				void* *func = NULL;
				int funcnum, namelen = g_Lexer.nameLen;

				if (g_Lexer.nameLen <= MAX_FUNC_LEN &&
					((funcnum = GetFunction(g_Lexer.name, g_Lexer.nameLen, (void**)&op.Func)) >= 0))
				{
					switch (funcnum)
					{
					case FUNC_E:
						g_ValStack[++g_ValTop] = M_E;
						break;

					case FUNC_PI:
						g_ValStack[++g_ValTop] = M_PI;
						break;

					case FUNC_ROUND:
						op.type = OP_FUNC_MULTIARG;
						op.prevvalTop = g_ValTop;
						g_OpStack[++g_OpTop] = op;
						break;

					default:	// Internal function
						op.type = OP_FUNC_ONEARG;
						g_OpStack[++g_OpTop] = op;
						break;
					}
				}
				else
				{
					if (searchProc && (*searchProc)(g_Lexer.name, g_Lexer.nameLen, &dblval))
					{
						g_ValStack[++g_ValTop] = dblval;
						break;
					}

					char buffer[128 - _countof(eUnknFunc)];
					strncpy_s(buffer, g_Lexer.name, g_Lexer.nameLen);
					buffer[g_Lexer.nameLen] = '\0';
					_snprintf_s(g_ErrorBuffer, _TRUNCATE, eUnknFunc, buffer);
					return g_ErrorBuffer;
				}
				break;
			}

		default:
			return eSyntax;
		}

nextToken:
		token = Lexer_GetNextToken();
	}

setResult:
	if (g_OpTop != -1 || g_ValTop != 0) return eInternal;

	*result = g_ValStack[0];
	return NULL;
}

static char* Calc()
{
	double res;
	Operation op = g_OpStack[g_OpTop--];

	// Multi-argument function
	if (op.type == OP_FUNC_MULTIARG)
	{
		int paramcnt = g_ValTop - op.prevvalTop;

		g_ValTop = op.prevvalTop;
		char* error = (*(MultiArgProc)op.Func)(paramcnt, &g_ValStack[g_ValTop + 1], &res);
		if (error) return error;

		g_ValStack[++g_ValTop] = res;
		return NULL;
	}
	else if (op.type == OP_LOGIC)
	{
		return NULL; 
	}
	else if (g_ValTop < 0)
	{
		return eExtraOp;
	}

	// Right arg
	double right = g_ValStack[g_ValTop--];

	// One arg operations
	if (op.type == OP_NOT)
	{
		if (right >= INT_MIN && right <= INT_MAX)
		{
			res = ~((int)right);
		}
		else
		{
			return eValSizErr;
		}
	}
	else if (op.type == OP_FUNC_ONEARG)
	{
		res = (*(OneArgProc)op.Func)(right);
	}
	else
	{
		if (g_ValTop < 0)
		{
			return eExtraOp;
		}

		// Left arg
		double left = g_ValStack[g_ValTop--];
		switch (op.type)
		{
		case OP_SHL:
			if (left >= INT_MIN && left <= INT_MAX && right >= INT_MIN && right <= INT_MAX)
			{
				res = (int)left << (int)right;
			}
			else
			{
				return eValSizErr;
			}
			break;

		case OP_SHR:
			if (left >= INT_MIN && left <= INT_MAX && right >= INT_MIN && right <= INT_MAX)
			{
				res = (int)left >> (int)right;
			}
			else
			{
				return eValSizErr;
			}
			break;

		case OP_POW:
			res = pow(left, right);
			break;

		case OP_LOGIC_NEQ:
			res = left != right;
			break;

		case OP_LOGIC_GEQ:
			res = left >= right;
			break;

		case OP_LOGIC_LEQ:
			res = left <= right;
			break;

		case OP_LOGIC_AND:
			res = left && right;
			break;

		case OP_LOGIC_OR:
			res = left || right;
			break;

		case OP_ADD:
			res = left + right;
			break;

		case OP_SUB:
			res = left - right;
			break;

		case OP_MUL:
			res = left*  right;
			break;

		case OP_DIV:
			if (right == 0.0)
			{
				return eInfinity;
			}
			else
			{
				res = left / right;
			}
			break;

		case OP_MOD:
			res = fmod(left, right);
			break;

		case OP_UNK:
			if (left <= 0)
			{
				res = 0.0;
			}
			else if (right == 0.0)
			{
				return eInfinity;
			}
			else
			{
				res = ceil(left / right);
			}
			break;

		case OP_XOR:
			if (left >= INT_MIN && left <= INT_MAX && right >= INT_MIN && right <= INT_MAX)
			{
				res = (int)left ^ (int)right;
			}
			else
			{
				return eValSizErr;
			}
			break;

		case OP_AND:
			if (left >= INT_MIN && left <= INT_MAX && right >= INT_MIN && right <= INT_MAX)
			{
				res = (int)left & (int)right;
			}
			else
			{
				return eValSizErr;
			}
			break;

		case OP_OR:
			if (left >= INT_MIN && left <= INT_MAX && right >= INT_MIN && right <= INT_MAX)
			{
				res = (int)left | (int)right;
			}
			else
			{
				return eValSizErr;
			}
			break;

		case OP_EQU:
			res = left == right;
			break;

		case OP_GREATER:
			res = left > right;
			break;

		case OP_SMALLER:
			res = left < right;
			break;

		case OP_LOGIC_SEP:
			{
				// needs three arguments
				double ValLL;
				if (g_OpTop < 0 || g_OpStack[g_OpTop--].type != OP_LOGIC)
				{
					return eLogicErr;
				}
				ValLL = g_ValStack[g_ValTop--];
				res = ValLL ? left : right;
			}
			break;

		default:
			return eInternal;
		}
	}

	g_ValStack[++g_ValTop] = res;
	return NULL;
}

static char* CalcToObr()
{
	while (g_OpStack[g_OpTop].type != OP_OBR)
	{
		char* error = Calc();
		if (error) return error;
	}
	--g_OpTop;
	return NULL;
}

int GetFunction(const char* str, size_t len, void** data)
{
	const int funcCount = sizeof(g_Functions) / sizeof(Function);
	for (int i = 0; i < funcCount; ++i)
	{
		if (g_Functions[i].length == len &&
			_strnicmp(str, g_Functions[i].name, len) == 0)
		{
			*data = g_Functions[i].proc;
			return i;
		}
	}

	return -1;
}

int FindSymbol(const char* str)
{
	switch (str[0])
	{
	case '(':	return (int)OP_OBR;
	case '+':	return OP_ADD;
	case '-':	return OP_SUB;
	case '*':	return (str[1] == '*') ? OP_POW : OP_MUL;
	case '/':	return OP_DIV;
	case '%':	return OP_MOD;
	case '$':	return OP_UNK;
	case '^':	return OP_XOR;
	case '~':	return OP_NOT;
	case '&':	return (str[1] == '&') ? OP_LOGIC_AND : OP_AND;
	case '|':	return (str[1] == '|') ? OP_LOGIC_OR : OP_OR;
	case '=':	return OP_EQU;
	case '>':	return (str[1] == '>') ? OP_SHR : (str[1] == '=') ? OP_LOGIC_GEQ : OP_GREATER;
	case '<':	return (str[1] == '>') ? OP_LOGIC_NEQ : (str[1] == '<') ? OP_SHL : (str[1] == '=') ? OP_LOGIC_LEQ : OP_SMALLER;
	case '?':	return OP_LOGIC;
	case ':':	return OP_LOGIC_SEP;
	case ')':	return OP_CBR;
	case ',':	return OP_COMMA;
	}

	return -1;
}

// -----------------------------------------------------------------------------------------------
//  Lexer
// -----------------------------------------------------------------------------------------------

inline CharType CHARTYPEPP() { return g_CharTypes[(unsigned char)*++(g_Lexer.string)]; }
inline CharType CHARTYPE() { return g_CharTypes[(unsigned char)*g_Lexer.string]; }

int Lexer_SetParseString(const char* str)
{
	g_Lexer.PrevTokenType = TOK_NONE;
	if (!str || !*str) return 0;

	g_Lexer.string = str;
	g_Lexer.CharType = CHARTYPE();
	return 1;
}

MathTokenType Lexer_GetNextToken()
{
	MathTokenType result = TOK_ERROR;

	while (g_Lexer.CharType == CH_SEPARAT)
	{
		g_Lexer.CharType = CHARTYPEPP();
	}
	
	switch (g_Lexer.CharType)
	{
	case CH_FINAL:
		{
			result = TOK_FINAL;
		}
		break;

	case CH_LETTER:
		{
			g_Lexer.name = g_Lexer.string;
			do
			{
				g_Lexer.CharType = CHARTYPEPP();
			}
			while (g_Lexer.CharType <= CH_DIGIT);

			g_Lexer.nameLen = g_Lexer.string - g_Lexer.name;
			result = TOK_NAME;
		}
		break;

	case CH_DIGIT:
		{
			char* newString;
			if (g_Lexer.string[0] == '0')
			{
				bool valid = true;
				switch (g_Lexer.string[1])
				{
				case 'x':	// Hexadecimal
					g_Lexer.intValue = strtol(g_Lexer.string, &newString, 16);
					break;

				case 'o':	// Octal
					g_Lexer.intValue = strtol(g_Lexer.string + 2, &newString, 8);
					break;

				case 'b':	// Binary
					g_Lexer.intValue = strtol(g_Lexer.string + 2, &newString, 2);
					break;

				default:
					valid = false;
					break;
				}

				if (valid)
				{
					if (g_Lexer.string != newString)
					{
						g_Lexer.string = newString;
						g_Lexer.CharType = CHARTYPE();
						g_Lexer.extValue = g_Lexer.intValue;
						result = TOK_FLOAT;
					}
					break;
				}
			}

			// Decimal
			g_Lexer.extValue = strtod(g_Lexer.string, &newString);
			if (g_Lexer.string != newString)
			{
				g_Lexer.string = newString;
				g_Lexer.CharType = CHARTYPE();
				result = TOK_FLOAT;
			}
		}
		break;

	case CH_SYMBOL:
		{
			int sym = FindSymbol(g_Lexer.string);
			if (sym >= 0)
			{
				g_Lexer.string += (sym == OP_POW ||
					sym == OP_LOGIC_AND || sym == OP_LOGIC_OR ||
					sym == OP_SHR || sym == OP_LOGIC_GEQ ||
					sym == OP_LOGIC_NEQ || sym == OP_SHL || sym == OP_LOGIC_LEQ) ? 2 : 1;

				g_Lexer.CharType = CHARTYPE();
				g_Lexer.intValue = sym;
				result = TOK_SYMBOL;
			}
		}
		break;

	default:
		break;
	}
	return g_Lexer.PrevTokenType = result;
}

// -----------------------------------------------------------------------------------------------
//  Misc
// -----------------------------------------------------------------------------------------------

static double frac(double x)
{
	double y;
	return modf(x, &y);
}

static double trunc(double x)
{
	return (x >= 0.0) ? floor(x) : ceil(x);
}

static double sgn(double x)
{
	return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

static double neg(double x)
{
	return -x;
}

// "Advanced" round function; second argument - sharpness
static char* round(int paramcnt, double* args, double* result)
{
	int sharpness;
	if (paramcnt == 1)
	{
		sharpness = 0;
	}
	else if (paramcnt == 2)
	{
		sharpness = (int)args[1];
	}
	else
	{
		return eInvPrmCnt;
	}

	double x = args[0];
	double coef;
	if (sharpness < 0)
	{
		coef = 0.1;
		sharpness = -sharpness;
	}
	else
	{
		coef = 10;
	}

	for (int i = 0; i < sharpness; i++) x *= coef;

	x = (x + ((x >= 0) ? 0.5 : -0.5));
	x = (x >= 0.0) ? floor(x) : ceil(x);

	for (int i = 0; i < sharpness; i++) x /= coef;

	*result = x;
	return NULL;
}
