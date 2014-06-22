#ifndef __BASIC_INTERPRETER_H__
#define __BASIC_INTERPRETER_H__

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

/* Defines ------------------------------------------------------------------ */
#define LINEBUF_LEN     512
#define STACK_SIZE      512
// Variables limitations
#define MAX_VAR_COUNT   512
#define VAR_NAME_LEN    64
// Labels limitations
#define MAX_LABEL_COUNT 64
#define LABEL_NAME_LEN  64
// Tokens limitations
#define MAX_TOK_COUNT   32
#define TOK_NAME_LEN    64

// Return types
#define rSUCCESS        0
#define rFAILURE        1
#define rEOF            2

// Debug enable (1) or disable (0)
#define DEBUG           1

// Keyword Enums
typedef enum {
  PRINT,
} keyword_t;

// Token Enums
typedef enum {
  NUMBER,
  STRING,
  OPERATOR,
  IDENTIFIER,
  VARIABLE,
  KEYWORD,
  LABEL
} token_type_t;

// Variable data structure
typedef struct {
  char      name[VAR_NAME_LEN];
  uint32_t  value;
} var_t;

// Token data structure
typedef struct {
  char          name[TOK_NAME_LEN];
  token_type_t  type;
} token_t;

/* Function Prototypes ------------------------------------------------------ */
int BasicInterpret(FILE *f);
int LexIsEOF(char c);
int LexIsEndOfLine(char c);
int LexIsWhiteSpace(char c);
int LexIsInlineComment(char c);
int LexIsDoubleQuote(char c);
int LexIsOperator(char c);
int LexIsAlpha(char c);
int LexIsInteger(int *idx1, int *idx2);
int LexIsDigit(char c);
int LexIsBinDigit(char c);
int LexIsHexDigit(char c);
int LexIsKeyword(char *id);
int LexIsVariable(char *id);
int LexIsLabel(char *id);
char *LexCurrentLine(void);
char *LexErrorMessage(void);
int LexCurrentLineCount(void);
int LexCurrentColumnCount(void);

#endif /* __BASIC_INTERPRETER_H__ */

