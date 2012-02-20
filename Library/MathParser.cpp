/*
  Copyright (C) 2011 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#include "StdAfx.h"
#include "MeasureCalc.h"
#include "MathParser.h"

static const double M_E = 2.7182818284590452354;
static const double M_PI = 3.14159265358979323846;

typedef double (*OneArgProc)(double arg);
typedef WCHAR* (*MultiArgProc)(int paramcnt, double* args, double* result);

enum OperationType
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
};

enum CharType
{
	CH_UNKNOWN = 0x00,
	CH_LETTER  = 0x01,
	CH_DIGIT   = 0x02,
	CH_SEPARAT = 0x04,
	CH_SYMBOL  = 0x08,
	CH_FINAL   = 0x7F
};

enum MathTokenType
{
	TOK_ERROR,
	TOK_NONE,
	TOK_FINAL,
	TOK_FLOAT,
	TOK_SYMBOL,
	TOK_NAME
};

struct Operation
{
	BYTE type;
	BYTE funcIndex;
	char prevTop;
};

struct Function
{
	WCHAR* name;
	OneArgProc proc;
	BYTE length;
};

static double neg(double x);
static double frac(double x);
static double trunc(double x);
static double sgn(double x);
static const WCHAR* round(int paramcnt, double* args, double* result);

static Function g_Functions[] =
{
	{ L"atan", &atan, 4 },
	{ L"cos", &cos, 3 },
	{ L"sin", &sin, 3 },
	{ L"tan", &tan, 3 },
	{ L"abs", &fabs, 3 },
	{ L"exp", &exp, 3 },
	{ L"ln", &log, 2 },
	{ L"log", &log10, 3 },
	{ L"sqrt", &sqrt, 4 },
	{ L"frac", &frac, 4 },
	{ L"trunc", &trunc, 5 },
	{ L"floor", &floor, 5 },
	{ L"ceil", &ceil, 4 },
	{ L"round", (OneArgProc)&round, 5 },
	{ L"asin", &asin, 4 },
	{ L"acos", &acos, 4 },
	{ L"sgn", &sgn, 4 },
	{ L"neg", &neg, 4 },
	{ L"e", NULL, 1 },
	{ L"pi", NULL, 2 }
};

static const int FUNC_MAX_LEN = 5;
static const int FUNC_ROUND = 13;
static const int FUNC_E = 18;
static const int FUNC_PI = 19;
static const BYTE FUNC_INVALID = UCHAR_MAX;

static const Operation g_BrOp = { OP_OBR, 0, 0};
static const Operation g_NegOp = { OP_FUNC_ONEARG, 17, 0 };

static const BYTE g_OpPriorities[OP_FUNC_MULTIARG + 1] =
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

static CharType GetCharType(WCHAR ch);
static BYTE GetFunctionIndex(const WCHAR* str, BYTE len);
static int FindSymbol(const WCHAR* str);

struct Parser
{
	Operation opStack[96];
	double valStack[64];
	char opTop;
	char valTop;
	int obrDist;

	Parser() : opTop(0), valTop(-1), obrDist(2) { opStack[0].type = OP_OBR; }
};

static const WCHAR* CalcToObr(Parser& parser);
static const WCHAR* Calc(Parser& parser);

struct Lexer
{
	const WCHAR* string;
	const WCHAR* name;
	size_t nameLen;
	double extValue;
	int intValue;
	MathTokenType prevToken;
	CharType charType;

	Lexer(const WCHAR* str) : string(str), name(), nameLen(), extValue(), intValue(), prevToken(TOK_NONE), charType(GetCharType(*str)) {}
};

static MathTokenType GetNextToken(Lexer& lexer);

const WCHAR* eBrackets = L"Unmatched brackets";
const WCHAR* eSyntax = L"Syntax error";
const WCHAR* eInternal = L"Internal error";
const WCHAR* eExtraOp = L"Extra operation";
const WCHAR* eInfinity = L"Division by 0";
const WCHAR* eUnknFunc = L"\"%s\" is unknown";
const WCHAR* eLogicErr = L"Logical expression error";
const WCHAR* eInvPrmCnt = L"Invalid function parameter count";

const WCHAR* MathParser::Check(const WCHAR* formula)
{
	int brackets = 0;

	// Brackets Matching
	while (*formula)
	{
		if (*formula == L'(')
		{
			++brackets;
		}
		else if (*formula == L')')
		{
			--brackets;
		}
		++formula;
	}

	return (brackets != 0) ? eBrackets : NULL;
}

const WCHAR* MathParser::CheckedParse(const WCHAR* formula, double* result)
{
	const WCHAR* error = Check(formula);
	if (!error)
	{
		error = Parse(formula, NULL, result);
	}
	return error;
}

const WCHAR* MathParser::Parse(const WCHAR* formula, CMeasureCalc* calc, double* result)
{
	static WCHAR errorBuffer[128];

	if (!*formula)
	{
		*result = 0.0;
		return NULL;
	}

	Parser parser;
	Lexer lexer(formula);

	const WCHAR* error;
	for (;;)
	{
		if ((parser.opTop == _countof(parser.opStack) - 2) ||
			(parser.valTop == _countof(parser.valStack) - 2))
		{
			return eInternal;
		}

		MathTokenType token = GetNextToken(lexer);
		--parser.obrDist;
		switch (token)
		{
		case TOK_ERROR:
			return eSyntax;

		case TOK_FINAL:
			if ((error = CalcToObr(parser)) != NULL)
			{
				return error;
			}
			else if (parser.opTop != -1 || parser.valTop != 0)
			{
				return eInternal;
			}
			else
			{
				// Done!
				*result = parser.valStack[0];
				return NULL;
			}
			break;

		case TOK_FLOAT:
			parser.valStack[++parser.valTop] = lexer.extValue;
			break;

		case TOK_SYMBOL:
			switch (lexer.intValue)
			{
			case OP_OBR:
				{
					parser.opStack[++parser.opTop] = g_BrOp;
					parser.obrDist = 2;
				}
				break;

			case OP_CBR:
				{
					if ((error = CalcToObr(parser)) != NULL) return error;
				}
				break;

			case OP_COMMA:
				{	
					if ((error = CalcToObr(parser)) != NULL) return error;
						
					if (parser.opStack[parser.opTop].type == OP_FUNC_MULTIARG)
					{
						parser.opStack[++parser.opTop] = g_BrOp;
						parser.obrDist = 2;
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
					op.type = (OperationType)lexer.intValue;
					switch (op.type)
					{
					case OP_ADD:
						if (parser.obrDist >= 1)
						{
							// Goto next token
							continue;
						}
						break;

					case OP_SUB:
						if (parser.obrDist >= 1)
						{
							parser.opStack[++parser.opTop] = g_NegOp;

							// Goto next token
							continue;
						}
						break;

					case OP_LOGIC:
					case OP_LOGIC_SEP:
						parser.obrDist = 2;
						break;
					}

					while (g_OpPriorities[op.type] <= g_OpPriorities[parser.opStack[parser.opTop].type])
					{
						if ((error = Calc(parser)) != NULL) return error;
					}
					parser.opStack[++parser.opTop] = op;
				}
				break;
			}
			break;

		case TOK_NAME:
			{
				Operation op;
				if (lexer.nameLen <= FUNC_MAX_LEN &&
					((op.funcIndex = GetFunctionIndex(lexer.name, lexer.nameLen)) != FUNC_INVALID))
				{
					switch (op.funcIndex)
					{
					case FUNC_E:
						parser.valStack[++parser.valTop] = M_E;
						break;

					case FUNC_PI:
						parser.valStack[++parser.valTop] = M_PI;
						break;

					case FUNC_ROUND:
						op.type = OP_FUNC_MULTIARG;
						op.prevTop = parser.valTop;
						parser.opStack[++parser.opTop] = op;
						break;

					default:	// Internal function
						op.type = OP_FUNC_ONEARG;
						parser.opStack[++parser.opTop] = op;
						break;
					}
				}
				else
				{
					double dblval;
					if (calc && calc->GetMeasureValue(lexer.name, lexer.nameLen, &dblval))
					{
						parser.valStack[++parser.valTop] = dblval;
						break;
					}

					std::wstring name(lexer.name, lexer.nameLen);
					_snwprintf_s(errorBuffer, _TRUNCATE, eUnknFunc, name.c_str());
					return errorBuffer;
				}
				break;
			}

		default:
			return eSyntax;
		}
	}
}

static const WCHAR* Calc(Parser& parser)
{
	double res;
	Operation op = parser.opStack[parser.opTop--];

	// Multi-argument function
	if (op.type == OP_LOGIC)
	{
		return NULL; 
	}
	else if (op.type == OP_FUNC_MULTIARG)
	{
		int paramcnt = parser.valTop - op.prevTop;

		parser.valTop = op.prevTop;
		WCHAR* error = (*(MultiArgProc)g_Functions[op.funcIndex].proc)(paramcnt, &parser.valStack[parser.valTop + 1], &res);
		if (error) return error;

		parser.valStack[++parser.valTop] = res;
		return NULL;
	}
	else if (parser.valTop < 0)
	{
		return eExtraOp;
	}

	// Right arg
	double right = parser.valStack[parser.valTop--];

	// One arg operations
	if (op.type == OP_NOT)
	{
		res = (double)(~((long long)right));
	}
	else if (op.type == OP_FUNC_ONEARG)
	{
		res = (*(OneArgProc)g_Functions[op.funcIndex].proc)(right);
	}
	else
	{
		if (parser.valTop < 0)
		{
			return eExtraOp;
		}

		// Left arg
		double left = parser.valStack[parser.valTop--];
		switch (op.type)
		{
		case OP_SHL:
			res = (double)((long long)left << (long long)right);
			break;

		case OP_SHR:
			res = (double)((long long)left >> (long long)right);
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
			res = (double)((long long)left ^ (long long)right);
			break;

		case OP_AND:
			res = (double)((long long)left & (long long)right);
			break;

		case OP_OR:
			res = (double)((long long)left | (long long)right);
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
				// Needs three arguments
				if (parser.opTop < 0 || parser.opStack[parser.opTop--].type != OP_LOGIC)
				{
					return eLogicErr;
				}
				res = parser.valStack[parser.valTop--] ? left : right;
			}
			break;

		default:
			return eInternal;
		}
	}

	parser.valStack[++parser.valTop] = res;
	return NULL;
}

static const WCHAR* CalcToObr(Parser& parser)
{
	while (parser.opStack[parser.opTop].type != OP_OBR)
	{
		const WCHAR* error = Calc(parser);
		if (error) return error;
	}
	--parser.opTop;
	return NULL;
}

MathTokenType GetNextToken(Lexer& lexer)
{
	MathTokenType result = TOK_ERROR;

	while (lexer.charType == CH_SEPARAT)
	{
		lexer.charType = GetCharType(*++lexer.string);
	}
	
	switch (lexer.charType)
	{
	case CH_FINAL:
		{
			result = TOK_FINAL;
		}
		break;

	case CH_LETTER:
		{
			lexer.name = lexer.string;
			do
			{
				lexer.charType = GetCharType(*++lexer.string);
			}
			while (lexer.charType <= CH_DIGIT);

			lexer.nameLen = lexer.string - lexer.name;
			result = TOK_NAME;
		}
		break;

	case CH_DIGIT:
		{
			WCHAR* newString;
			if (lexer.string[0] == L'0')
			{
				bool valid = true;
				switch (lexer.string[1])
				{
				case L'x':	// Hexadecimal
					lexer.intValue = wcstol(lexer.string, &newString, 16);
					break;

				case L'o':	// Octal
					lexer.intValue = wcstol(lexer.string + 2, &newString, 8);
					break;

				case L'b':	// Binary
					lexer.intValue = wcstol(lexer.string + 2, &newString, 2);
					break;

				default:
					valid = false;
					break;
				}

				if (valid)
				{
					if (lexer.string != newString)
					{
						lexer.string = newString;
						lexer.charType = GetCharType(*lexer.string);
						lexer.extValue = lexer.intValue;
						result = TOK_FLOAT;
					}
					break;
				}
			}

			// Decimal
			lexer.extValue = wcstod(lexer.string, &newString);
			if (lexer.string != newString)
			{
				lexer.string = newString;
				lexer.charType = GetCharType(*lexer.string);
				result = TOK_FLOAT;
			}
		}
		break;

	case CH_SYMBOL:
		{
			int sym = FindSymbol(lexer.string);
			if (sym >= 0)
			{
				lexer.string += (sym <= OP_LOGIC_OR) ? 2 : 1;

				lexer.charType = GetCharType(*lexer.string);
				lexer.intValue = sym;
				result = TOK_SYMBOL;
			}
		}
		break;

	default:
		break;
	}
	return lexer.prevToken = result;
}

CharType GetCharType(WCHAR ch)
{
	switch (ch)
	{
	case L'\0':
		return CH_FINAL;

	case L' ':
	case L'\t':
	case L'\n':
		return CH_SEPARAT;

	case L'_':
		return CH_LETTER;

	case L'+':
	case L'-':
	case L'/':
	case L'*':
	case L'~':
	case L'(':
	case L')':
	case L'<':
	case L'>':
	case L'%':
	case L'$':
	case L',':
	case L'?':
	case L':':
	case L'=':
	case L'&':
	case L'|':
		return CH_SYMBOL;
	}

	if (iswalpha(ch)) return CH_LETTER;
	if (iswdigit(ch)) return CH_DIGIT;

	return CH_UNKNOWN;
}

bool MathParser::IsDelimiter(WCHAR ch)
{
	CharType type = GetCharType(ch);
	return type == CH_SYMBOL || type == CH_SEPARAT;
}

BYTE GetFunctionIndex(const WCHAR* str, BYTE len)
{
	const int funcCount = sizeof(g_Functions) / sizeof(Function);
	for (int i = 0; i < funcCount; ++i)
	{
		if (g_Functions[i].length == len &&
			_wcsnicmp(str, g_Functions[i].name, len) == 0)
		{
			return i;
		}
	}

	return FUNC_INVALID;
}

int FindSymbol(const WCHAR* str)
{
	switch (str[0])
	{
	case L'(':	return OP_OBR;
	case L'+':	return OP_ADD;
	case L'-':	return OP_SUB;
	case L'*':	return (str[1] == L'*') ? OP_POW : OP_MUL;
	case L'/':	return OP_DIV;
	case L'%':	return OP_MOD;
	case L'$':	return OP_UNK;
	case L'^':	return OP_XOR;
	case L'~':	return OP_NOT;
	case L'&':	return (str[1] == L'&') ? OP_LOGIC_AND : OP_AND;
	case L'|':	return (str[1] == L'|') ? OP_LOGIC_OR : OP_OR;
	case L'=':	return OP_EQU;
	case L'>':	return (str[1] == L'>') ? OP_SHR : (str[1] == L'=') ? OP_LOGIC_GEQ : OP_GREATER;
	case L'<':	return (str[1] == L'>') ? OP_LOGIC_NEQ : (str[1] == L'<') ? OP_SHL : (str[1] == L'=') ? OP_LOGIC_LEQ : OP_SMALLER;
	case L'?':	return OP_LOGIC;
	case L':':	return OP_LOGIC_SEP;
	case L')':	return OP_CBR;
	case L',':	return OP_COMMA;
	}

	return -1;
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
static const WCHAR* round(int paramcnt, double* args, double* result)
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
