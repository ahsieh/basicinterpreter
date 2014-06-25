
/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic.h"

/* Defines ------------------------------------------------------------------ */
#if DEBUG > 0
char debug_buf[128];
#define DEBUG_PRINTF(fmt, ...) \
  do { sprintf(debug_buf, fmt, ##__VA_ARGS__); puts(debug_buf); } while (0)
#else
#define DEBUG_PRINTF(...)
#endif
/* Local Variables ---------------------------------------------------------- */
// Lexer buffer
static uint32_t linebuf_idx = 0;
static char linebuf[LINEBUF_LEN];
// Tokens
static token_t tokens[MAX_TOK_COUNT];
static uint32_t tokp = 0;
// Line and column trackers
static uint32_t line_count = 0, col_count = 0;
// Error message
static char error_message[32];
// Parse tree
static parser_node_t parse_tree[NUM_PARSE_TREE_NODES];
/*
// Run-Time Stack and Stack Pointer
static uint32_t stack[STACK_SIZE], sp = 0;
// Variables
static var_t vars[MAX_VAR_COUNT];
*/

/* Constants ---------------------------------------------------------------- */
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

/* Private Function Prototypes ---------------------------------------------- */
static int LexAnalyzeLine(void);
static keyword_t ParseGetKeyword(uint32_t kptr);
static int StatementPrint(uint32_t curr_tok);
#if DEBUG >= 1
static void debug_print_type(token_type_t type);
#else
#define debug_print_type(t)
#endif

/* Function Definitions ----------------------------------------------------- */
/**
 *  @brief  Interpret incoming lines entered by user.
 */
int BasicCommandLine(void)
{
  tokp = 0;
  if (fgets(linebuf, LINEBUF_LEN, stdin)) {
    // LexAnalyzeLine the line
    if (LexAnalyzeLine() == rFAILURE) {
puts("Lex failed");
      return rFAILURE;
    }

    if (ParseLine() == rFAILURE) {
puts("Parse failed");
      return rFAILURE;
    }
    memset(linebuf, 0, LINEBUF_LEN);
  } else {
puts("ummmmm");
    return rFAILURE;
  }

  return rSUCCESS;
}

/**
 *  @brief  Interpret the given file.
 *  @param  f File pointer to program
 */
int BasicInterpret(FILE *f)
{
  char ch;
  line_count = 1;

  do {
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
      col_count = LINEBUF_LEN;
      sprintf(error_message, "Line length exceed");
      return rFAILURE;
    }

    //
    DEBUG_PRINTF("Line #%d: %s", line_count, linebuf);

    // LexAnalyzeLine the line
    if (LexAnalyzeLine() == rFAILURE) {
      return rFAILURE;
    }

    if (ParseLine() == rFAILURE) {
      return rFAILURE;
    }

    // Reset line buffer
    linebuf_idx = 0;
    memset(linebuf, 0, LINEBUF_LEN);

    // Update the line count
    line_count++;

  } while (ch != EOF);

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
  char ch, ch1, ch2;

  *idx1 = linebuf_idx;
  ch = linebuf[linebuf_idx];
  ch1 = linebuf[++linebuf_idx];
  ch2 = linebuf[linebuf_idx + 1];
  if (ch == '0' && (ch1 == 'x' || ch1 == 'X')) {
    // Hex
    if (LexIsHexDigit(ch2)) {
      do {
        ch = linebuf[++linebuf_idx];
      } while (LexIsHexDigit(ch));
    } else {
      return 0;
    }
  } else if (ch == '0' && (ch1 == 'b' || ch1 == 'B')) {
    // Binary
    if (LexIsBinDigit(ch2)) {
      do {
        ch = linebuf[++linebuf_idx];
      } while (LexIsBinDigit(ch));
    } else {
      return 0;
    }
  } else if (LexIsDigit(ch)) {
    // Decimal
    while (LexIsDigit(ch)) {
      ch = linebuf[++linebuf_idx];
    }
  } else {
    return 0;
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

char *LexGetCurrentLine(void)
{
  return linebuf;
}

char *LexGetErrorMessage(void)
{
  return error_message;
}

int LexGetCurrentLineCount(void)
{
  return line_count;
}

int LexGetCurrentColumnCount(void)
{
  return col_count;
}

int ParseLine(void)
{
#if 0
  puts("*** Parsing Line ***");
  printf("Line #: %d\n", line_count);
  printf("Num tokens: %d\n", tokp);
  puts(linebuf);
#endif

  int result = rSUCCESS;
  uint32_t curr_tok = 0;

  if (tokens[curr_tok].type == KEYWORD) {
    //
    switch (ParseGetKeyword(curr_tok++)) {
      case PRINT:
        result = StatementPrint(curr_tok);
        break;
      default:
        break;
    }
  } else {
    // TODO Update error message
    //return rFAILURE;
  }

  return result;
}

/* Local Function Definitions ----------------------------------------------- */
static int LexAnalyzeLine(void)
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
      DEBUG_PRINTF("Comment: %s", linebuf + linebuf_idx + 1);
      break;
    } else if (LexIsDigit(ch) && LexIsInteger(&idx1, &idx2)) {
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
          col_count = linebuf_idx;
          sprintf(error_message, "Invalid string");
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
      col_count = linebuf_idx;
      sprintf(error_message, "Invalid token");
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

static keyword_t ParseGetKeyword(uint32_t kptr)
{
  int i;
  for (i = 0; keywords[i]; i++) {
    if (strcmp(tokens[kptr].name, keywords[i]) == 0) {
      break;
    }
  }

  return i;
}

static int StatementPrint(uint32_t curr_tok)
{
  // PRINT <> | <print_obj> [ '+' <print_obj]*
  // print_obj := STRING | VARIABLE
  if (curr_tok < tokp) {
    while (curr_tok < tokp) {
      if (tokens[curr_tok].type == STRING || 
          tokens[curr_tok].type == VARIABLE) {
        printf("%s", tokens[curr_tok++].name);
        if (curr_tok < tokp && strcmp(tokens[curr_tok].name, "+") == 0) {
          if (curr_tok + 1 < tokp) {
            curr_tok++;
          } else {
            sprintf(error_message, "Invalid syntax: Missing token.");
            puts("");
            return rFAILURE;
          }
        } else if (curr_tok == tokp) {
          break;
        } else {
          sprintf(error_message, "Invalid syntax: '+' missing?");
          puts("");
          return rFAILURE;
        }
      } else {
        sprintf(error_message, "Invalid syntax: Bad token.");
        puts("");
        return rFAILURE;
      }
    }
  }
  puts("");
  return rSUCCESS;
}

#if DEBUG >= 1
static void debug_print_type(token_type_t type)
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
