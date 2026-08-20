// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#include "StdAfx.h"
#include "MathParser.h"

#include <algorithm>
#include <numbers>
#include <string>

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
	const WCHAR* name;
	SingleArgFunction singleArgProc;
	MultiArgFunction multiArgProc;
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
	{ L"atan2", nullptr, &ATan2, 5 },              // FUNC_ATAN2
	{ L"atan", &atan, nullptr, 4 },                // FUNC_ATAN
	{ L"cos", &cos, nullptr, 3 },                  // FUNC_COS
	{ L"sin", &sin, nullptr, 3 },                  // FUNC_SIN
	{ L"tan", &tan, nullptr, 3 },                  // FUNC_TAN
	{ L"abs", &fabs, nullptr, 3 },                 // FUNC_ABS
	{ L"exp", &exp, nullptr, 3 },                  // FUNC_EXP
	{ L"ln", &log, nullptr, 2 },                   // FUNC_LN
	{ L"log", &log10, nullptr, 3 },                // FUNC_LOG
	{ L"sqrt", &sqrt, nullptr, 4 },                // FUNC_SQRT
	{ L"frac", &frac, nullptr, 4 },                // FUNC_FRAC
	{ L"trunc", &trunc, nullptr, 5 },              // FUNC_TRUNC
	{ L"floor", &floor, nullptr, 5 },              // FUNC_FLOOR
	{ L"ceil", &ceil, nullptr, 4 },                // FUNC_CEIL
	{ L"round", nullptr, &round, 5 },              // FUNC_ROUND
	{ L"asin", &asin, nullptr, 4 },                // FUNC_ASIN
	{ L"acos", &acos, nullptr, 4 },                // FUNC_ACOS
	{ L"rad", &rad, nullptr, 3 },                  // FUNC_RAD
	{ L"deg", &deg, nullptr, 3 },                  // FUNC_DEG
	{ L"sgn", &sgn, nullptr, 3 },                  // FUNC_SGN
	{ L"neg", &neg, nullptr, 3 },                  // FUNC_NEG
	{ L"min", nullptr, &Min, 3 },                  // FUNC_MIN
	{ L"max", nullptr, &Max, 3 },                  // FUNC_MAX
	{ L"clamp", nullptr, &Clamp, 5 },              // FUNC_CLAMP
	{ L"e", nullptr, nullptr, 1 },                 // FUNC_E
	{ L"pi", nullptr, nullptr, 2 }                 // FUNC_PI
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
static CharType GetCharType(const WCHAR* str, const WCHAR* end);
static BYTE GetFunctionIndex(const WCHAR* str, BYTE len);
static Operator GetOperator(const WCHAR* str, const WCHAR* end);

struct Parser
{
	Operation opStack[96];
	double numStack[64];
	char opTop;
	char valTop;
	int obrDist;

	Parser() : opStack(), numStack(), opTop(0), valTop(-1), obrDist(2) { opStack[0].type = Operator::OpeningBracket; }
};

static const WCHAR* CalcToObr(Parser& parser);
static const WCHAR* Calc(Parser& parser);

struct Lexer
{
	const WCHAR* string;
	const WCHAR* end;
	const WCHAR* name = nullptr;
	size_t nameLen = 0;

	Token token = Token::None;
	union
	{
		Operator oper;  // token == Token::Operator
		double num;  // token == Token::Number
	} value = {};

	CharType charType;

	Lexer(std::wstring_view str) :
		string(str.data()),
		end(str.data() + str.size()),
		charType(GetCharType(string, end))
	{
	}
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

MathParser::MathParser(GetValueFunc getValue, void* getValueContext) :
	m_GetValue(getValue),
	m_GetValueContext(getValueContext)
{
}

const WCHAR* MathParser::Check(std::wstring_view formula) const
{
	int brackets = 0;

	// Brackets Matching
	for (WCHAR ch : formula)
	{
		if (ch == L'(')
		{
			++brackets;
		}
		else if (ch == L')')
		{
			--brackets;
		}
	}

	return (brackets != 0) ? eBrackets : nullptr;
}

const WCHAR* MathParser::CheckedParse(std::wstring_view formula, double* result) const
{
	const WCHAR* error = Check(formula);
	if (!error)
	{
		error = Parse(formula, result);
	}
	return error;
}

const WCHAR* MathParser::Parse(std::wstring_view formula, double* result, ParseMode mode, const WCHAR** parseEnd) const
{
	static WCHAR errorBuffer[128];
	if (parseEnd) *parseEnd = formula.data();
	if (mode == ParseMode::MatchingClosingBracket)
	{
		const WCHAR* current = formula.data();
		const WCHAR* end = formula.data() + formula.size();
		while (current != end && (*current == L' ' || *current == L'\t' || *current == L'\n')) ++current;
		if (current == end || *current != L'(') return eSyntax;
	}

	if (formula.empty())
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
			if (mode == ParseMode::MatchingClosingBracket)
			{
				return eBrackets;
			}

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
				if (parseEnd) *parseEnd = lexer.string;
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

					if (mode == ParseMode::MatchingClosingBracket && parser.opTop == 0)
					{
						if ((error = CalcToObr(parser)) != nullptr) return error;
						if (parser.opTop != -1 || parser.valTop != 0) return eInternal;

						*result = parser.numStack[0];
						if (parseEnd) *parseEnd = lexer.string;
						return nullptr;
					}
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
					Operation op = {};
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
				Operation op = {};
				if (lexer.nameLen <= FUNC_MAX_LEN &&
					((op.funcIndex = GetFunctionIndex(lexer.name, (BYTE)lexer.nameLen)) != FUNC_INVALID))
				{
					switch (op.funcIndex)
					{
					case FUNC_E:
						parser.numStack[++parser.valTop] = std::numbers::e;
						break;

					case FUNC_PI:
						parser.numStack[++parser.valTop] = std::numbers::pi;
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
					if (m_GetValue && m_GetValue(lexer.name, (int)lexer.nameLen, &dblval, m_GetValueContext))
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
		const WCHAR* error = g_Functions[op.funcIndex].multiArgProc(paramcnt, &parser.numStack[parser.valTop + 1], &res);
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
		res = g_Functions[op.funcIndex].singleArgProc(right);
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

// wcstod/wcstoll happily scan past |end| if the underlying buffer isn't null-terminated
// there, which would both read out-of-view characters and fold them into the result (e.g.
// parsing "3.1" out of a view over "3.123456" would otherwise yield 3.123456). If that
// happens, |start| is re-parsed from a small null-terminated copy bounded by |end| so the
// result only reflects characters within the view.
template <typename T, typename ParseFunc>
static T ParseBounded(const WCHAR* start, const WCHAR* end, WCHAR** outEnd, ParseFunc parseFunc)
{
	T value = parseFunc(start, outEnd);
	if (*outEnd <= end) return value;

	WCHAR buffer[64];
	size_t len = std::min<size_t>(end - start, _countof(buffer) - 1);
	wmemcpy(buffer, start, len);
	buffer[len] = L'\0';

	WCHAR* boundedEnd = nullptr;
	value = parseFunc(buffer, &boundedEnd);
	*outEnd = const_cast<WCHAR*>(start) + (boundedEnd - buffer);
	return value;
}

Token GetNextToken(Lexer& lexer)
{
	while (lexer.charType == CharType::Separator)
	{
		lexer.charType = GetCharType(++lexer.string, lexer.end);
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
				lexer.charType = GetCharType(++lexer.string, lexer.end);
			}
			while (lexer.charType <= CharType::Digit);
			lexer.nameLen = lexer.string - lexer.name;
		}
		break;

	case CharType::Digit:
		{
			// wcstoll/wcstod scan for as many valid characters as they can find, which may run
			// past |lexer.end| if the underlying buffer isn't null-terminated there. The result
			// is re-parsed from a bounded copy below, so out-of-view characters can't affect
			// the resulting value.
			WCHAR* newString = nullptr;
			const bool hasNextChar = (lexer.string + 1) < lexer.end;
			if (lexer.string[0] == L'0' && hasNextChar)
			{
				bool valid = true;
				long long num = 0;
				switch (lexer.string[1])
				{
				case L'x':	// Hexadecimal
					num = ParseBounded<long long>(lexer.string, lexer.end, &newString,
						[](const WCHAR* s, WCHAR** e) { return wcstoll(s, e, 16); });
					break;

				case L'o':	// Octal
					num = ParseBounded<long long>(lexer.string + 2, lexer.end, &newString,
						[](const WCHAR* s, WCHAR** e) { return wcstoll(s, e, 8); });
					break;

				case L'b':	// Binary
					num = ParseBounded<long long>(lexer.string + 2, lexer.end, &newString,
						[](const WCHAR* s, WCHAR** e) { return wcstoll(s, e, 2); });
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
						lexer.value.num = (double)num;
						lexer.string = newString;
						lexer.charType = GetCharType(lexer.string, lexer.end);
					}
					break;
				}
			}

			// Decimal
			double num = ParseBounded<double>(lexer.string, lexer.end, &newString,
				[](const WCHAR* s, WCHAR** e) { return wcstod(s, e); });
			if (lexer.string != newString)
			{
				lexer.token = Token::Number;
				lexer.value.num = num;
				lexer.string = newString;
				lexer.charType = GetCharType(lexer.string, lexer.end);
			}
		}
		break;

	case CharType::Symbol:
		{
			Operator oper = GetOperator(lexer.string, lexer.end);
			if (oper != Operator::Invalid)
			{
				lexer.token = Token::Operator;
				lexer.value.oper = oper;
				lexer.string += ((int)oper <= (int)Operator::LogicalOR) ? 2 : 1;
				lexer.charType = GetCharType(lexer.string, lexer.end);
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
	case L',':
	case L'?':
	case L':':
	case L'=':
	case L'&':
	case L'|':
	case L'^':
		return CharType::Symbol;
	}

	if (iswdigit(ch)) return CharType::Digit;

	// Make sure this is the last character test before "Unknown".
	// This will catch all characters with graphical representation and treat them as a "Letter".
	// This includes all "alpha" characters and the following characers not defined above: _\`!#@{}[]'$";
	if (iswgraph(ch)) return CharType::Letter;

	return CharType::Unknown;
}

CharType GetCharType(const WCHAR* str, const WCHAR* end)
{
	return (str >= end) ? CharType::Final : GetCharType(*str);
}

bool MathParser::IsDelimiter(WCHAR ch) const
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

Operator GetOperator(const WCHAR* str, const WCHAR* end)
{
	const bool hasNext = (str + 1) < end;

	switch (str[0])
	{
	case L'(':
		return Operator::OpeningBracket;

	case L'+':
		return Operator::Addition;

	case L'-':
		return Operator::Subtraction;

	case L'*':
		return (hasNext && str[1] == L'*') ? Operator::Power : Operator::Multiplication;

	case L'/':
		return Operator::Division;

	case L'%':
		return Operator::Modulo;

	case L'^':
		return Operator::BitwiseXOR;

	case L'~':
		return Operator::BitwiseNOT;

	case L'&':
		return (hasNext && str[1] == L'&') ? Operator::LogicalAND : Operator::BitwiseAND;

	case L'|':
		return (hasNext && str[1] == L'|') ? Operator::LogicalOR : Operator::BitwiseOR;

	case L'=':
		return Operator::Equal;

	case L'>':
		return (hasNext && str[1] == L'>') ? Operator::ShiftRight : (hasNext && str[1] == L'=') ? Operator::GreatorOrEqual : Operator::Greater;

	case L'<':
		return (hasNext && str[1] == L'>') ? Operator::NotEqual : (hasNext && str[1] == L'<') ? Operator::ShiftLeft : (hasNext && str[1] == L'=') ? Operator::LessOrEqual : Operator::Less;

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
	return (deg / 180.0) * std::numbers::pi;
}

static double deg(double rad)
{
	return rad * (180.0 / std::numbers::pi);
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

