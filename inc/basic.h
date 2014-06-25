#ifndef __BASIC_INTERPRETER_H__
#define __BASIC_INTERPRETER_H__

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

/* Defines ------------------------------------------------------------------ */
#define LINEBUF_LEN           512
#define STACK_SIZE            512
// Variables limitations
#define MAX_VAR_COUNT         512
#define VAR_NAME_LEN          64
// Labels limitations
#define MAX_LABEL_COUNT       64
#define LABEL_NAME_LEN        64
// Tokens limitations
#define MAX_TOK_COUNT         32
#define TOK_NAME_LEN          64
// Parser limitations
#define NUM_PARSE_TREE_NODES  128

// Return types
#define rSUCCESS        0
#define rFAILURE        1
#define rEOF            2

// Debug enable (1) or disable (0)
#define DEBUG           1

// Keyword Enums
typedef enum {
  PRINT,
  VAR,
  INT8,
  UINT8
} keyword_t;

// Token Enums
typedef enum {
  KEYWORD,
  NUMBER,
  STRING,
  OPERATOR,
  COMMA,
  IDENTIFIER,
  VARIABLE,
  LABEL,
} token_type_t;

// Variable data structure
typedef struct {
  char      name[VAR_NAME_LEN];
  uint32_t  value;
} var_t;

// Token data structure
typedef struct {
  int           idx1;
  int           idx2;
  token_type_t  type;
} token_t;

// Parser Tree Node
typedef struct {
  token_t *self;
  token_t *parent;
  token_t *left_child;
  token_t *right_child;
} parser_node_t;

/* Function Prototypes ------------------------------------------------------ */
int BasicCommandLine(void);
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
int LexIsKeyword(int idx1, int idx2);
int LexIsVariable(int idx1, int idx2);
int LexIsLabel(char *id);
char *LexGetCurrentLine(void);
char *LexGetErrorMessage(void);
int LexGetCurrentLineCount(void);
int LexGetCurrentColumnCount(void);
int ParseLine(void);

#endif /* __BASIC_INTERPRETER_H__ */

