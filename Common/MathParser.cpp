/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#include "StdAfx.h"
#include "MathParser.h"

#include <string>

namespace MathParser {

static const double M_E = 2.7182818284590452354;
static const double M_PI = 3.14159265358979323846;

typedef double (*SingleArgFunction)(double arg);
typedef const WCHAR* (*MultiArgFunction)(int paramcnt, double* args, double* result);

enum class Operator : uint8_t
{
	ShiftLeft,
	ShiftRight,
	Power,
	NotEqual,
	GreatorOrEqual,
	LessOrEqual,
	LogicalAND,
	LogicalOR,
	OpeningBracket,
	Addition,
	Subtraction,
	Multiplication,
	Division,
	Modulo,
	UNK,
	BitwiseXOR,
	BitwiseNOT,
	BitwiseAND,
	BitwiseOR,
	Equal,
	Greater,
	Less,
	Conditional,
	ConditionalSeparator,
	ClosingBracket,
	Comma,
	SingleArgFunction,
	MultiArgFunction,
	Invalid // Must be last.
};

enum class CharType
{
	Unknown     = 0x00,
	Letter      = 0x01,
	Digit       = 0x02,
	Separator   = 0x04,
	Symbol      = 0x08,
	MinusSymbol = 0x10,
	Final       = 0x7F
};

enum class Token
{
	Error,
	None,
	Final,
	Operator,
	Number,
	Name
};

struct Operation
{
	Operator type;
	BYTE funcIndex;
	char prevTop;
};

struct Function
{
	WCHAR* name;
	SingleArgFunction proc;
	BYTE length;
};

static double frac(double x);
static double rad(double deg);
static double deg(double rad);
static double sgn(double x);
static double neg(double x);
static const WCHAR* Min(int paramcnt, double* args, double* result);
static const WCHAR* Max(int paramcnt, double* args, double* result);
static const WCHAR* Clamp(int paramcnt, double* args, double* result);
static const WCHAR* round(int paramcnt, double* args, double* result);
static const WCHAR* ATan2(int paramcnt, double* args, double* result);

enum {
	FUNC_ATAN2,			// note: must be before atan so it gets matched first!
	FUNC_ATAN,
	FUNC_COS,
	FUNC_SIN,
	FUNC_TAN,
	FUNC_ABS,
	FUNC_EXP,
	FUNC_LN,
	FUNC_LOG,
	FUNC_SQRT,
	FUNC_FRAC,
	FUNC_TRUNC,
	FUNC_FLOOR,
	FUNC_CEIL,
	FUNC_ROUND,
	FUNC_ASIN,
	FUNC_ACOS,
	FUNC_RAD,
	FUNC_DEG,
	FUNC_SGN,
	FUNC_NEG,
	FUNC_MIN,
	FUNC_MAX,
	FUNC_CLAMP,
	FUNC_E,
	FUNC_PI,
	// ... //
	NUM_FUNCS
};

static Function g_Functions[NUM_FUNCS] =
{
	{ L"atan2", (SingleArgFunction)&ATan2, 5 },    // FUNC_ATAN2
	{ L"atan", &atan, 4 },                         // FUNC_ATAN
	{ L"cos", &cos, 3 },                           // FUNC_COS
	{ L"sin", &sin, 3 },                           // FUNC_SIN
	{ L"tan", &tan, 3 },                           // FUNC_TAN
	{ L"abs", &fabs, 3 },                          // FUNC_ABS
	{ L"exp", &exp, 3 },                           // FUNC_EXP
	{ L"ln", &log, 2 },                            // FUNC_LN
	{ L"log", &log10, 3 },                         // FUNC_LOG
	{ L"sqrt", &sqrt, 4 },                         // FUNC_SQRT
	{ L"frac", &frac, 4 },                         // FUNC_FRAC
	{ L"trunc", &trunc, 5 },                       // FUNC_TRUNC
	{ L"floor", &floor, 5 },                       // FUNC_FLOOR
	{ L"ceil", &ceil, 4 },                         // FUNC_CEIL
	{ L"round", (SingleArgFunction)&round, 5 },    // FUNC_ROUND
	{ L"asin", &asin, 4 },                         // FUNC_ASIN
	{ L"acos", &acos, 4 },                         // FUNC_ACOS
	{ L"rad", &rad, 3 },                           // FUNC_RAD
	{ L"deg", &deg, 3 },                           // FUNC_DEG
	{ L"sgn", &sgn, 3 },                           // FUNC_SGN
	{ L"neg", &neg, 3 },                           // FUNC_NEG
	{ L"min", (SingleArgFunction)&Min, 3 },        // FUNC_MIN
	{ L"max", (SingleArgFunction)&Max, 3 },        // FUNC_MAX
	{ L"clamp", (SingleArgFunction)&Clamp, 5 },    // FUNC_CLAMP
	{ L"e", nullptr, 1 },                          // FUNC_E
	{ L"pi", nullptr, 2 }                          // FUNC_PI
};

static const int FUNC_MAX_LEN = 5;
static const BYTE FUNC_INVALID = UCHAR_MAX;

static const Operation g_BrOp = { Operator::OpeningBracket, 0, 0};
static const Operation g_NegOp = { Operator::SingleArgFunction, FUNC_NEG, 0 };

static const BYTE g_OpPriorities[(uint8_t)Operator::Invalid] =
{
	5, // Operator::ShiftLeft
	5, // Operator::ShiftRight
	5, // Operator::Power
	2, // Operator::NotEqual
	2, // Operator::GreatorOrEqual
	2, // Operator::LessOrEqual
	2, // Operator::LogicalAND
	2, // Operator::LogicalOR
	0, // Operator::OpeningBracket
	3, // Operator::Addition
	3, // Operator::Subtraction
	4, // Operator::Multiplication
	4, // Operator::Division
	4, // Operator::Modulo
	4, // Operator::UNK
	5, // Operator::BitwiseXOR
	5, // Operator::BitwiseNOT
	5, // Operator::BitwiseAND
	5, // Operator::BitwiseOR
	2, // Operator::Equal
	2, // Operator::Greater
	2, // Operator::Less
	1, // Operator::Conditional
	2, // Operator::ConditionalSeparator
	0, // Operator::ClosingBracket
	2, // Operator::Comma
	6, // Operator::SingleArgFunction
	6  // Operator::MultiArgFunction
};

static CharType GetCharType(WCHAR ch);
static BYTE GetFunctionIndex(const WCHAR* str, BYTE len);
static Operator GetOperator(const WCHAR* str);

struct Parser
{
	Operation opStack[96];
	double numStack[64];
	char opTop;
	char valTop;
	int obrDist;

	Parser() : opTop(0), valTop(-1), obrDist(2) { opStack[0].type = Operator::OpeningBracket; }
};

static const WCHAR* CalcToObr(Parser& parser);
static const WCHAR* Calc(Parser& parser);

struct Lexer
{
	const WCHAR* string;
	const WCHAR* name;
	size_t nameLen;

	Token token;
	union
	{
		Operator oper;  // token == Token::Operator
		double num;  // token == Token::Number
	} value;

	CharType charType;

	Lexer(const WCHAR* str) : string(str), name(), nameLen(), value(), token(Token::None), charType(GetCharType(*str)) {}
};

static Token GetNextToken(Lexer& lexer);

const WCHAR* eBrackets = L"Unmatched brackets";
const WCHAR* eSyntax = L"Syntax error";
const WCHAR* eInternal = L"Internal error";
const WCHAR* eExtraOp = L"Extra operation";
const WCHAR* eInfinity = L"Division by 0";
const WCHAR* eUnknFunc = L"\"%s\" is unknown";
const WCHAR* eLogicErr = L"Logical expression error";
const WCHAR* eInvPrmCnt = L"Invalid function parameter count";

const WCHAR* Check(const WCHAR* formula)
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

	return (brackets != 0) ? eBrackets : nullptr;
}

const WCHAR* CheckedParse(const WCHAR* formula, double* result)
{
	const WCHAR* error = Check(formula);
	if (!error)
	{
		error = Parse(formula, result);
	}
	return error;
}

const WCHAR* Parse(
	const WCHAR* formula, double* result, GetValueFunc getValue, void* getValueContext)
{
	static WCHAR errorBuffer[128];

	if (!*formula)
	{
		*result = 0.0;
		return nullptr;
	}

	Parser parser;
	Lexer lexer(formula);

	const WCHAR* error;
	for (;;)
	{
		if ((parser.opTop == _countof(parser.opStack) - 2) ||
			(parser.valTop == _countof(parser.numStack) - 2))
		{
			return eInternal;
		}

		Token token = GetNextToken(lexer);
		--parser.obrDist;
		switch (token)
		{
		case Token::Error:
			return eSyntax;

		case Token::Final:
			if ((error = CalcToObr(parser)) != nullptr)
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
				*result = parser.numStack[0];
				return nullptr;
			}
			break;

		case Token::Number:
			parser.numStack[++parser.valTop] = lexer.value.num;
			break;

		case Token::Operator:
			switch (lexer.value.oper)
			{
			case Operator::OpeningBracket:
				{
					parser.opStack[++parser.opTop] = g_BrOp;
					parser.obrDist = 2;
				}
				break;

			case Operator::ClosingBracket:
				{
					if ((error = CalcToObr(parser)) != nullptr) return error;
				}
				break;

			case Operator::Comma:
				{
					if ((error = CalcToObr(parser)) != nullptr) return error;
						
					if (parser.opStack[parser.opTop].type == Operator::MultiArgFunction)
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
					op.type = lexer.value.oper;
					switch (op.type)
					{
					case Operator::Addition:
						if (parser.obrDist >= 1)
						{
							// Goto next token
							continue;
						}
						break;

					case Operator::Subtraction:
						if (parser.obrDist >= 1)
						{
							parser.opStack[++parser.opTop] = g_NegOp;

							// Goto next token
							continue;
						}
						break;

					case Operator::Conditional:
					case Operator::ConditionalSeparator:
						parser.obrDist = 2;
						break;
					}

					while (g_OpPriorities[(int)op.type] <= g_OpPriorities[(int)parser.opStack[parser.opTop].type])
					{
						if ((error = Calc(parser)) != nullptr) return error;
					}
					parser.opStack[++parser.opTop] = op;
				}
				break;
			}
			break;

		case Token::Name:
			{
				Operation op;
				if (lexer.nameLen <= FUNC_MAX_LEN &&
					((op.funcIndex = GetFunctionIndex(lexer.name, (BYTE)lexer.nameLen)) != FUNC_INVALID))
				{
					switch (op.funcIndex)
					{
					case FUNC_E:
						parser.numStack[++parser.valTop] = M_E;
						break;

					case FUNC_PI:
						parser.numStack[++parser.valTop] = M_PI;
						break;

					case FUNC_ATAN2:
					case FUNC_ROUND:
					case FUNC_MIN:
					case FUNC_MAX:
					case FUNC_CLAMP:
						op.type = Operator::MultiArgFunction;
						op.prevTop = parser.valTop;
						parser.opStack[++parser.opTop] = op;
						break;

					default:	// Internal function
						op.type = Operator::SingleArgFunction;
						parser.opStack[++parser.opTop] = op;
						break;
					}
				}
				else
				{
					double dblval;
					if (getValue && getValue(lexer.name, (int)lexer.nameLen, &dblval, getValueContext))
					{
						parser.numStack[++parser.valTop] = dblval;
						break;
					}

					const std::wstring name(lexer.name, lexer.nameLen);
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
	if (op.type == Operator::Conditional)
	{
		return nullptr; 
	}
	else if (op.type == Operator::MultiArgFunction)
	{
		int paramcnt = parser.valTop - op.prevTop;

		parser.valTop = op.prevTop;
		const WCHAR* error = (*(MultiArgFunction)g_Functions[op.funcIndex].proc)(paramcnt, &parser.numStack[parser.valTop + 1], &res);
		if (error) return error;

		parser.numStack[++parser.valTop] = res;
		return nullptr;
	}
	else if (parser.valTop < 0)
	{
		return eExtraOp;
	}

	// Right arg
	double right = parser.numStack[parser.valTop--];

	// One arg operations
	if (op.type == Operator::BitwiseNOT)
	{
		res = (double)(~((long long)right));
	}
	else if (op.type == Operator::SingleArgFunction)
	{
		res = (*(SingleArgFunction)g_Functions[op.funcIndex].proc)(right);
	}
	else
	{
		if (parser.valTop < 0)
		{
			return eExtraOp;
		}

		// Left arg
		double left = parser.numStack[parser.valTop--];
		switch (op.type)
		{
		case Operator::ShiftLeft:
			res = (double)((long long)left << (long long)right);
			break;

		case Operator::ShiftRight:
			res = (double)((long long)left >> (long long)right);
			break;

		case Operator::Power:
			res = pow(left, right);
			break;

		case Operator::NotEqual:
			res = left != right;
			break;

		case Operator::GreatorOrEqual:
			res = left >= right;
			break;

		case Operator::LessOrEqual:
			res = left <= right;
			break;

		case Operator::LogicalAND:
			res = left && right;
			break;

		case Operator::LogicalOR:
			res = left || right;
			break;

		case Operator::Addition:
			res = left + right;
			break;

		case Operator::Subtraction:
			res = left - right;
			break;

		case Operator::Multiplication:
			res = left*  right;
			break;

		case Operator::Division:
			if (right == 0.0)
			{
				return eInfinity;
			}
			else
			{
				res = left / right;
			}
			break;

		case Operator::Modulo:
			res = fmod(left, right);
			break;

		case Operator::UNK:
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

		case Operator::BitwiseXOR:
			res = (double)((long long)left ^ (long long)right);
			break;

		case Operator::BitwiseAND:
			res = (double)((long long)left & (long long)right);
			break;

		case Operator::BitwiseOR:
			res = (double)((long long)left | (long long)right);
			break;

		case Operator::Equal:
			res = left == right;
			break;

		case Operator::Greater:
			res = left > right;
			break;

		case Operator::Less:
			res = left < right;
			break;

		case Operator::ConditionalSeparator:
			{
				// Needs three arguments
				if (parser.opTop < 0 || parser.opStack[parser.opTop--].type != Operator::Conditional)
				{
					return eLogicErr;
				}
				res = parser.numStack[parser.valTop--] ? left : right;
			}
			break;

		default:
			return eInternal;
		}
	}

	parser.numStack[++parser.valTop] = res;
	return nullptr;
}

static const WCHAR* CalcToObr(Parser& parser)
{
	while (parser.opStack[parser.opTop].type != Operator::OpeningBracket)
	{
		const WCHAR* error = Calc(parser);
		if (error) return error;
	}
	--parser.opTop;
	return nullptr;
}

Token GetNextToken(Lexer& lexer)
{
	while (lexer.charType == CharType::Separator)
	{
		lexer.charType = GetCharType(*++lexer.string);
	}

	if (lexer.charType == CharType::MinusSymbol)
	{
		// If the - sign follows a symbol, it is treated as a (negative) number.
		lexer.charType = CharType::Symbol;
		if (lexer.token == Token::Operator &&
			lexer.value.oper != Operator::OpeningBracket &&  // Special case for e.g. (-PI/2), (-(5)-2).
			lexer.value.oper != Operator::ClosingBracket)  // Special case for e.g. (5)-2.
		{
			lexer.charType = CharType::Digit;
		}
	}

	switch (lexer.charType)
	{
	case CharType::Final:
		{
			lexer.token = Token::Final;
		}
		break;

	case CharType::Letter:
		{
			lexer.token = Token::Name;
			lexer.name = lexer.string;
			do
			{
				lexer.charType = GetCharType(*++lexer.string);
			}
			while (lexer.charType <= CharType::Digit);
			lexer.nameLen = lexer.string - lexer.name;
		}
		break;

	case CharType::Digit:
		{
			WCHAR* newString;
			if (lexer.string[0] == L'0')
			{
				bool valid = true;
				int num = 0;
				switch (lexer.string[1])
				{
				case L'x':	// Hexadecimal
					num = wcstol(lexer.string, &newString, 16);
					break;

				case L'o':	// Octal
					num = wcstol(lexer.string + 2, &newString, 8);
					break;

				case L'b':	// Binary
					num = wcstol(lexer.string + 2, &newString, 2);
					break;

				default:
					valid = false;
					break;
				}

				if (valid)
				{
					if (lexer.string != newString)
					{
						lexer.token = Token::Number;
						lexer.value.num = num;
						lexer.string = newString;
						lexer.charType = GetCharType(*lexer.string);
					}
					break;
				}
			}

			// Decimal
			double num = wcstod(lexer.string, &newString);
			if (lexer.string != newString)
			{
				lexer.token = Token::Number;
				lexer.value.num = num;
				lexer.string = newString;
				lexer.charType = GetCharType(*lexer.string);
			}
		}
		break;

	case CharType::Symbol:
		{
			Operator oper = GetOperator(lexer.string);
			if (oper != Operator::Invalid)
			{
				lexer.token = Token::Operator;
				lexer.value.oper = oper;
				lexer.string += ((int)oper <= (int)Operator::LogicalOR) ? 2 : 1;
				lexer.charType = GetCharType(*lexer.string);
			}
		}
		break;

	default:
		lexer.token = Token::Error;
		break;
	}

	return lexer.token;
}

CharType GetCharType(WCHAR ch)
{
	switch (ch)
	{
	case L'\0':
		return CharType::Final;

	case L' ':
	case L'\t':
	case L'\n':
		return CharType::Separator;

	case L'_':
		return CharType::Letter;

	case L'-':
		return CharType::MinusSymbol;

	case L'+':
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
	case L'^':
		return CharType::Symbol;
	}

	if (iswalpha(ch)) return CharType::Letter;
	if (iswdigit(ch)) return CharType::Digit;

	return CharType::Unknown;
}

bool IsDelimiter(WCHAR ch)
{
	CharType type = GetCharType(ch);
	return type == CharType::MinusSymbol || type == CharType::Symbol || type == CharType::Separator;
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

Operator GetOperator(const WCHAR* str)
{
	switch (str[0])
	{
	case L'(':
		return Operator::OpeningBracket;

	case L'+':
		return Operator::Addition;

	case L'-':
		return Operator::Subtraction;

	case L'*':
		return (str[1] == L'*') ? Operator::Power : Operator::Multiplication;

	case L'/':
		return Operator::Division;

	case L'%':
		return Operator::Modulo;

	case L'$':
		return Operator::UNK;

	case L'^':
		return Operator::BitwiseXOR;

	case L'~':
		return Operator::BitwiseNOT;

	case L'&':
		return (str[1] == L'&') ? Operator::LogicalAND : Operator::BitwiseAND;

	case L'|':
		return (str[1] == L'|') ? Operator::LogicalOR : Operator::BitwiseOR;

	case L'=':
		return Operator::Equal;

	case L'>':
		return (str[1] == L'>') ? Operator::ShiftRight : (str[1] == L'=') ? Operator::GreatorOrEqual : Operator::Greater;

	case L'<':
		return (str[1] == L'>') ? Operator::NotEqual : (str[1] == L'<') ? Operator::ShiftLeft : (str[1] == L'=') ? Operator::LessOrEqual : Operator::Less;

	case L'?':
		return Operator::Conditional;

	case L':':
		return Operator::ConditionalSeparator;

	case L')':
		return Operator::ClosingBracket;

	case L',':
		return Operator::Comma;
	}

	return Operator::Invalid;
}

// -----------------------------------------------------------------------------------------------
//  Misc
// -----------------------------------------------------------------------------------------------

static double frac(double x)
{
	double y;
	return modf(x, &y);
}

static double rad(double deg)
{
	return (deg / 180.0) * M_PI;
}

static double deg(double rad)
{
	return rad * (180.0 / M_PI);
}

static double sgn(double x)
{
	return (x > 0.0) ? 1.0 : (x < 0.0) ? -1.0 : 0.0;
}

static double neg(double x)
{
	return -x;
}

static const WCHAR* Min(int paramcnt, double* args, double* result)
{
	if (paramcnt == 2)
	{
		const double& a = args[0];
		const double& b = args[1];

		*result = (a < b) ? a : b;
		return nullptr;
	}
	return eInvPrmCnt;
}

static const WCHAR* Max(int paramcnt, double* args, double* result)
{
	if (paramcnt == 2)
	{
		const double& a = args[0];
		const double& b = args[1];

		*result = (a > b) ? a : b;
		return nullptr;
	}
	return eInvPrmCnt;
}

static const WCHAR* Clamp(int paramcnt, double* args, double* result)
{
	if (paramcnt == 3)
	{
		const double& x = args[0];
		const double& a = args[1];
		const double& b = args[2];

		*result = (x < a) ? a : ((x > b) ? b : x);
		return nullptr;
	}
	return eInvPrmCnt;
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
	double coef = 10.0;
	if (sharpness < 0)
	{
		coef = 0.1;
		sharpness = -sharpness;
	}

	for (int i = 0; i < sharpness; i++) x *= coef;

	x = (x + ((x >= 0.0) ? 0.5 : -0.5));
	x = (x >= 0.0) ? floor(x) : ceil(x);

	for (int i = 0; i < sharpness; i++) x /= coef;

	*result = x;
	return nullptr;
}

// wrapper for standard math lib atan2
static const WCHAR* ATan2(int paramcnt, double* args, double* result)
{
	if (paramcnt == 2)
	{
		const double& y = args[0];
		const double& x = args[1];

		*result = atan2(y, x);
		return nullptr;
	}
	return eInvPrmCnt;
}

}  // namespace MathParser
