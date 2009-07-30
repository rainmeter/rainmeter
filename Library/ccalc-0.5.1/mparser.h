#pragma once

/*
    Math parser for CCalc library.
*/
#include "strmap.h"
#include "lexer.h"
#include "pack.h"

//#define RUSSIAN 1

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_SIZE 32

extern const double DblErR;
extern const double DblNiN;

typedef enum {
    // Binary
    OP_SHL, OP_SHR, OP_POW,
    OP_LOGIC_NEQ, OP_LOGIC_GEQ, OP_LOGIC_LEQ,
    OP_LOGIC_AND, OP_LOGIC_OR, // Logical
    OP_COMSTART, OP_ASSIGN, // For internal needs
    OP_OBR, // Special
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_UNK, // Arithmetic
    OP_XOR, OP_NOT, OP_AND, OP_OR, // Bitwise
    OP_EQU, OP_GREATER, OP_LESS,
    OP_LOGIC, OP_LOGIC_SEP, OP_CBR, OP_COMMA, // Logical
    OP_FORMULAEND, // For script
    OP_FUNC_ONEARG, OP_FUNC_MULTIARG // Special
} OperType_t;

typedef struct {
    char       *str;
    double	value;
} Parameter;

typedef double (*OneArgFunc) ( double arg );
typedef char* (*MultiArgFunc) ( int paramcnt, double *args, 
				 hqStrMap *strparams, double *result );
typedef int (*PrmSrchFunc) ( const char *str, int len, double *value,
			     void *param );

#pragma pack(push,1)

typedef struct {
    OperType_t	OperType;
    void       *Func;
    char	PrevValTop;
    hqStrMap   *StrParams;
} Operation;

typedef struct {
    /* public */
    hqStrMap   *Parameters;	// List of numeric veriables
    hqStrMap   *ExtFunctions;   // List of multi-argument external functions
    PrmSrchFunc MoreParams;	// Function for calculating unhandled parameters
    void       *ParamFuncParam; // Parameter given to this function
    /* private */
    Operation	OpStack[MAX_STACK_SIZE];
    double	ValStack[MAX_STACK_SIZE];
    int		OpTop, ValTop;
    int		ObrDist;
    hqLexer     Lexer;
    int		script;
    hqStrMap   *VarParams;
} hqMathParser;

#pragma pack(pop)

/* API */

hqMathParser* MathParser_Create( char *MoreLetters );
void MathParser_Destroy( hqMathParser* parser );
char* MathParser_Parse( hqMathParser* parser, const char *Formula, double *result );

#ifdef __cplusplus
}
#endif