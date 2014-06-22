#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Defines -----
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

// Variables -----
// Lexer buffer
uint32_t linebuf_idx = 0;
char linebuf[LINEBUF_LEN];
// Tokens
token_t tokens[MAX_TOK_COUNT];
uint32_t tokp = 0;
// Run-Time Stack and Stack Pointer
uint32_t stack[STACK_SIZE], sp = 0;
// Variables
var_t vars[MAX_VAR_COUNT];
// Line and column trackers
uint32_t line_count = 0, col_count = 0;

// Constants -----
const char *keywords[] = {
  "PRINT",
  "LET",
  "IF",
  "THEN",
  "ELSE",
  "END"
};

const char *single_char_operators[] = {
  "(", ")", "=", "+", "-", "*", "/", ":",
  "&", "|", "!"
};

const char *double_char_operators[] = {
  "==", "&&", "||"
};


// Function Prototypes -----
int LexAnalyzeLine(FILE *f);
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

// Local function prototypes -----
static int LexInterpretLine(void);

#if DEBUG >= 1
void debug_print_type(token_type_t type);
#endif

// Start of main -----
int main(int argc, char *argv[])
{
  FILE *fp;

  // Make sure we are using the executable correctly.
  if (argc != 2) {
    puts("Usage: ./basic [filename]");
    return EXIT_FAILURE;
  }

  // Open the provided file.
  fp = fopen(argv[1], "r");
  if (!fp) {
    printf("Could not open file %s!\r\n", argv[1]);
    return EXIT_FAILURE;
  }

  // Let user know we are running the file now.
  printf("Running BASIC script %s\r\n", argv[1]);

  while (LexAnalyzeLine(fp) == 0);

  // Done! Close file.
  fclose(fp);
  puts("");
  puts("BASIC test program exited successfully.");
  return EXIT_SUCCESS;
}

// Function Definitions -----
int LexAnalyzeLine(FILE *f)
{
  char ch;

  // Initialize
  tokp = 0;

  // Get the entire line
  do {
    ch = fgetc(f);
    if (ch != EOF) {
      linebuf[linebuf_idx++] = ch;
    }
  } while ((ch != '\n') && (ch != EOF) && (linebuf_idx < LINEBUF_LEN));

  // 
  if (linebuf_idx == LINEBUF_LEN && ch != '\n') {
    return rFAILURE;
  }

#if (DEBUG == 1)
  // Debug
  printf("Line #%d: %s", line_count, linebuf);
#endif

  // LexInterpretLine the line
  if (LexInterpretLine() == rFAILURE) {
    puts("Parse error!");
    return rFAILURE;
  }

  // Reset line buffer
  linebuf_idx = 0;
  memset(linebuf, 0, LINEBUF_LEN);

  // Update the line count
  line_count++;

  // Return
  if (ch == EOF) {
    return rEOF;
  }

  return rSUCCESS;
}


static int LexInterpretLine(void)
{
  char ch;
  int idx1 = 0, idx2 = 0;

  // Reset index
  linebuf_idx = 0;

  //
  while (!LexIsEndOfLine(linebuf[linebuf_idx])) {
    ch = linebuf[linebuf_idx];
    if (LexIsWhiteSpace(ch)) {
      // Ignore white spaces
      linebuf_idx++;
    } else if (LexIsInlineComment(ch)) {
      // Ignore comments
#if DEBUG >= 1
      printf("Comment: %s\r\n", linebuf + linebuf_idx + 1);
#endif
      break;
    } else if (LexIsDigit(ch)) {
      // Check number format (dec, hex, etc.)
      LexIsInteger(&idx1, &idx2);
      // Save token name and type as NUMBER
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = NUMBER;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (LexIsDoubleQuote(ch)) {
      // Check for string literals
      idx1 = linebuf_idx + 1;
      do {
        // Make sure string is properly terminated
        ch = linebuf[linebuf_idx++];
        if (LexIsEndOfLine(ch)) {
          puts("STRING FAILURE");
          return rFAILURE;
        }
      } while (linebuf[linebuf_idx] != '"');
      idx2 = linebuf_idx++;

      // Save token name and type as STRING
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = STRING;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (LexIsOperator(ch)) {
      // Operators
      // Check two-character operators first
      int o;
      char buf[3];
      for (o = 0; double_char_operators[o]; o++) {
        memcpy(buf, linebuf + linebuf_idx, 2);
        buf[2] = 0;
        if (strcmp(buf, double_char_operators[o]) == 0) {
          strcpy(tokens[tokp].name, buf);
          tokens[tokp++].type = OPERATOR;
          linebuf_idx += 2;
          o = -1;
          break;
        }
      }

      if (o > 0) {
        tokens[tokp].name[0] = ch;
        tokens[tokp].name[1] = '\0';
        tokens[tokp++].type = OPERATOR;
        linebuf_idx++;
      }
    } else if (LexIsAlpha(ch)) {
      idx1 = linebuf_idx;
      while (LexIsAlpha(ch) || LexIsDigit(ch)) {
        ch = linebuf[++linebuf_idx];
        if (LexIsEndOfLine(ch) || LexIsWhiteSpace(ch)) {
          break;
        }
      }
      idx2 = linebuf_idx;

      // Save token name 
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].name[idx2 - idx1] = '\0';

      // Determine if they are keywords, labels, or variables
      if (LexIsKeyword(tokens[tokp].name)) {
        // Keyword
        tokens[tokp].type = KEYWORD;
      } else {
        // Check if we have a label
        if (linebuf[linebuf_idx] == ':') {
          // Label
          tokens[tokp].type = LABEL;
          linebuf_idx++;
        } else {
          // Variable
          tokens[tokp].type = VARIABLE;
        }
      }

      tokp++;
    } else if (LexIsEOF(ch)) {
      return rSUCCESS;
    } else {
      // Error: unrecognized token
      return rFAILURE;
    }
  }

#if (DEBUG == 1)
  int i;
  for (i = 0; i < tokp; i++) {
    printf("Token %d: %s, type:", i, tokens[i].name);
    debug_print_type(tokens[i].type);
    puts("");
  }
  puts("");
#endif

  return rSUCCESS;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a integer (signed or unsigned).
 *  @param  idx1  Start index in linebuf (inclusive)
 *  @param  idx2  End index in linebuf (exclusive)
 */
int LexIsInteger(int *idx1, int *idx2)
{
  char ch;

  ch = linebuf[linebuf_idx];
  if (!LexIsDigit(ch)) {
    return 0;
  }

  *idx1 = linebuf_idx;
  ch = linebuf[++linebuf_idx];
  if (ch == 'x' || ch == 'X') {
    // Hex
    do {
      ch = linebuf[++linebuf_idx];
    } while (LexIsHexDigit(ch));
  } else if (ch == 'b' || ch == 'B') {
    // Binary
    do {
      ch = linebuf[++linebuf_idx];
    } while (LexIsBinDigit(ch));
  } else {
    while (LexIsDigit(ch)) {
      ch = linebuf[++linebuf_idx];
    }
  } 
  *idx2 = linebuf_idx;

  return 1;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a EOF character or not.
 *  @param  c Character of interest.
 */
int LexIsEOF(char c)
{
  return (c == '\0' ? 1 : 0);
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a newline character or not.
 *  @param  c Character of interest.
 */
int LexIsEndOfLine(char c)
{
  return (c == '\n' ? 1 : 0);
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a white space character or not.
 *  @param  c Character of interest.
 */
int LexIsWhiteSpace(char c)
{
  if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is the start of an inline comment.
 *  @param  c Character of interest.
 */
int LexIsInlineComment(char c)
{
  if (c == '#') {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a double quote or not.
 *  @param  c Character of interest.
 */
int LexIsDoubleQuote(char c)
{
  return (c == '"' ? 1 : 0);
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is an operator or not.
 *  @param  c Character of interest.
 */
int LexIsOperator(char c)
{
  int i;
  for (i = 0; single_char_operators[i]; i++) {
    if (c == single_char_operators[i][0]) {
      return 1;
    }
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is an alphabetical character or not.
 *  @param  c Character of interest.
 */
int LexIsAlpha(char c)
{
  if ('A' <= c && c <= 'Z') {
    return 1;
  }

  if ('a' <= c && c <= 'z') {
    return 1;
  }

  if (c == '_') {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a digit or not.
 *  @param  c Character of interest.
 */
int LexIsDigit(char c)
{
  if ('0' <= c && c <= '9') {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a binary digit or not.
 *  @param  c Character of interest.
 */
int LexIsBinDigit(char c)
{
  if (c == '0' || c == '1') {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          character is a hexadecimal digit or not.
 *  @param  c Character of interest.
 */
int LexIsHexDigit(char c)
{
  if (LexIsDigit(c) || ('A' <= c && c <= 'F')
      || ('a' <= c && c <= 'f')) {
    return 1;
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          identifier is a keyword or not.
 *  @param  
 */
int LexIsKeyword(char *id)
{
  int i;
  for (i = 0; keywords[i]; i++) {
    if (strcmp(id, keywords[i]) == 0) {
      return 1;
    }
  }

  return 0;
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          identifier is a variable or not.
 *  @param  id  
 */
int LexIsVariable(char *id)
{
  return !LexIsKeyword(id);
}

/**
 *  @brief  Helps lexical analyzer determine if the given
 *          identifier is a variable or not.
 *  @param  idx1  
 */
int LexIsLabel(char *id)
{
  return 0;
}

#if DEBUG >= 1
void debug_print_type(token_type_t type)
{
  switch (type) {
    case NUMBER:
      printf("Number");
      break;
    case STRING:
      printf("String");
      break;
    case OPERATOR:
      printf("Operator");
      break;
    case IDENTIFIER:
      printf("Identifier");
      break;
    case VARIABLE:
      printf("Variable");
      break;
    case KEYWORD:
      printf("Keyword");
      break;
    case LABEL:
      printf("Label");
      break;
    default:
      break;
  }
}
#endif
