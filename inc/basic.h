#ifndef __BASIC_INTERPRETER_H__
#define __BASIC_INTERPRETER_H__

/* Includes ----------------------------------------------------------------- */
#include <stdint.h>

/* Defines ------------------------------------------------------------------ */
#define LINEBUF_LEN           512 // Bytes
#define STACK_SIZE            512 // Bytes
// Variables limitations
#define MAX_VAR_COUNT         512
#define VAR_NAME_LEN          64
// Labels limitations
#define MAX_LABEL_COUNT       64
#define LABEL_NAME_LEN        64
// Tokens limitations
#define MAX_TOK_COUNT         128 
// Parser limitations
#define NUM_PARSE_TREE_NODES  128
// Console definitions
#define CONSOLEBUF_LEN        LINEBUF_LEN

// Sizes (in bytes) for Variable Types
#define SIZEOF_PTR            4
#define SIZEOF_CHAR           SIZEOF_UINT8
#define SIZEOF_INT8           1
#define SIZEOF_UINT8          1
#define SIZEOF_INT16          2
#define SIZEOF_UINT16         2
#define SIZEOF_INT32          4
#define SIZEOF_UINT32         4
#define SIZEOF_CHARPTR        SIZEOF_PTR
#define SIZEOF_INT8PTR        SIZEOF_PTR
#define SIZEOF_UINT8PTR       SIZEOF_PTR
#define SIZEOF_INT16PTR       SIZEOF_PTR
#define SIZEOF_UINT16PTR      SIZEOF_PTR
#define SIZEOF_INT32PTR       SIZEOF_PTR
#define SIZEOF_UINT32PTR      SIZEOF_PTR
#define NUM_DATA_TYPES        7

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
  CHAR,
  INT8,
  UINT8,
  INT16,
  UINT16,
  INT32,
  UINT32,
  CHARPTR,
  INT8PTR,
  UINT8PTR,
  INT16PTR,
  UINT16PTR,
  INT32PTR,
  UINT32PTR,
  IF,
  THEN,
  ELSE,
  END,
  // Debug keywords
  MEMPEEK
} keyword_t;

// Token Enums
typedef enum {
  KEYWORD,
  NUMBER,
  STRING,
  OPERATOR,
  PLUS,
  MINUS,
  EQUALS,
  COMMA,
  OPEN_SQUARE_BRACKET,
  CLOSED_SQUARE_BRACKET,
  IDENTIFIER,
  VARIABLE,
  LABEL,
} token_type_t;

typedef enum {
  VAR_CHAR = 0,
  VAR_INT8,
  VAR_UINT8,
  VAR_INT16,
  VAR_UINT16,
  VAR_INT32,
  VAR_UINT32,
  VAR_CHARPTR,
  VAR_INT8PTR,
  VAR_UINT8PTR,
  VAR_INT16PTR,
  VAR_UINT16PTR,
  VAR_INT32PTR,
  VAR_UINT32PTR
} var_type_t;

// Variable data structure
typedef struct {
  char          name[VAR_NAME_LEN];
  uint16_t      addr;
  uint16_t      size_in_bytes;
  uint16_t      len;
  int           var_type;
  int           sub_var_type;
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

