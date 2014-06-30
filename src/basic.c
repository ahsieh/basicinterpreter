
/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic.h"

/* Defines ------------------------------------------------------------------ */
#if DEBUG > 0
char debug_buf[128];
#define DEBUG_PRINTF(fmt, ...) \
  do { sprintf(debug_buf, fmt, ##__VA_ARGS__); puts(debug_buf); } while (0);
#define DEBUG_PRINTBUF()  puts(debug_buf);
#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINTBUF()
#endif

#define THROW_ERROR(msg, col_num) \
  do { strcpy(error_message, msg); col_count = col_num; } while (0);

#define CONSOLE_PRINTF(fmt, ...) \
  do { printf(fmt, ##__VA_ARGS__); } while (0);

#define CONSOLE_ADD_STRING(str) \
  do { \
    strcpy(consolebuf + consolebuf_idx, str); \
    consolebuf_idx += strlen(str); \
  } while (0);

#define CONSOLE_ADD_STRING_TOK(s) \
  do { \
    memcpy(consolebuf + consolebuf_idx, linebuf + s.idx1, s.idx2 - s.idx1); \
    consolebuf_idx += s.idx2 - s.idx1; \
  } while (0);

#define CONSOLE_ADD_UNSIGNED_TOK(v) \
  do { \
    consolebuf_idx += sprintf(consolebuf + consolebuf_idx, "%u", (v)); \
  } while (0);
      
#define CONSOLE_ADD_SIGNED_TOK(v) \
  do { \
    consolebuf_idx += sprintf(consolebuf + consolebuf_idx, "%d", (v)); \
  } while (0);
      
#define CONSOLE_ADD_CHAR_TOK(v) \
  do { \
    consolebuf_idx += sprintf(consolebuf + consolebuf_idx, "%c", (v)); \
  } while (0);
      
#define CONSOLE_PRINTBUF() \
  do { puts(consolebuf); } while (0);

/* Local Variables ---------------------------------------------------------- */
// Lexer buffer
static char linebuf[LINEBUF_LEN];
static uint32_t linebuf_idx = 0;
// Tokens
static token_t tokens[MAX_TOK_COUNT];
static uint32_t tokp = 0;
// Line and column trackers
static uint32_t line_count = 0, col_count = 0;
// Error message
static char error_message[32];
// Run-Time Stack and Stack Pointer
static uint8_t stack[STACK_SIZE];
static uint32_t sp = 0;
// Run-time Heap (TODO?)
// ...
// Variables Tracker and Pointer
static var_t var_list[MAX_VAR_COUNT];
static uint32_t varp = 0;
static uint32_t var_val = 0;
// Parse tree
static parser_node_t parse_tree[NUM_PARSE_TREE_NODES];
// Console output
static int consolebuf_idx = 0;
static char consolebuf[CONSOLEBUF_LEN];

/* Constants ---------------------------------------------------------------- */
const char *keywords[] = {
  "PRINT",
  "VAR",
  "CHAR",
  "INT8",
  "UINT8",
  "INT16",
  "UINT16",
  "INT32",
  "UINT32",
  "CHARPTR",
  "INT8PTR",
  "UINT8PTR",
  "INT16PTR",
  "UINT16PTR",
  "INT32PTR",
  "UINT32PTR",
  "IF",
  "THEN",
  "ELSE",
  "END",
  // Debug keywords
  "MEMPEEK",
  ""
};

const int var_type_sizes[] = {
  SIZEOF_CHAR,
  SIZEOF_INT8,
  SIZEOF_UINT8,
  SIZEOF_INT16,
  SIZEOF_UINT16,
  SIZEOF_INT32,
  SIZEOF_UINT32,
  SIZEOF_INT8PTR,
  SIZEOF_UINT8PTR,
  SIZEOF_INT16PTR,
  SIZEOF_UINT16PTR,
  SIZEOF_INT32PTR,
  SIZEOF_UINT32PTR
};

const char single_char_operators[] = "()[]=+-*/:&|!,";
const char double_char_operators[] = "==&&||";

/* Private Function Prototypes ---------------------------------------------- */
static int LexAnalyzeLine(void);
static int ParseGetKeyword(uint32_t token_idx);
static int ParseTokToNumber(uint32_t token_idx);
static int StatementPrint(uint32_t curr_tok);
static int StatementVar(uint32_t curr_tok);
static int StatementAssignment(uint32_t curr_tok);
static int StatementExpression(uint32_t curr_tok);
static int StatementMempeek(uint32_t curr_tok);
static int VarIsList(uint32_t *curr_tok);
static int VarIsDeclaration(uint32_t *curr_tok);
static int VarIsType(uint32_t *curr_tok, int *type);
static int VarLocation(uint32_t curr_tok);
static int ExprIsTerm(uint32_t *curr_tok);
static int ExprIsFactor(uint32_t *curr_tok);
static int32_t ConvertHexNumber(int idx1, int idx2);
static int32_t ConvertBinNumber(int idx1, int idx2);
static int32_t ConvertDecNumber(int idx1, int idx2);
static int32_t GetHexValue(char ch);

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
  CONSOLE_PRINTF(">> ");
  if (fgets(linebuf, LINEBUF_LEN, stdin)) {
    // LexAnalyzeLine the line
    if (LexAnalyzeLine() == rFAILURE) {
      return rFAILURE;
    }

    if (ParseLine() == rFAILURE) {
      return rFAILURE;
    }
    memset(linebuf, 0, LINEBUF_LEN);
  } else {
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
      THROW_ERROR("Line length exceeded", LINEBUF_LEN);
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
  char ch, ch1;

  *idx1 = linebuf_idx;
  ch = linebuf[linebuf_idx];
  ch1 = linebuf[linebuf_idx + 1];
  if (ch == '0' && (ch1 == 'x' || ch1 == 'X')) {
    // Hex
    linebuf_idx += 2;
    ch = linebuf[linebuf_idx];
    if (LexIsHexDigit(ch)) {
      do {
        ch = linebuf[++linebuf_idx];
      } while (LexIsHexDigit(ch));
    } else {
      return 0;
    }
  } else if (ch == '0' && (ch1 == 'b' || ch1 == 'B')) {
    // Binary
    linebuf_idx += 2;
    ch = linebuf[linebuf_idx];
    if (LexIsBinDigit(ch)) {
      do {
        ch = linebuf[++linebuf_idx];
      } while (LexIsBinDigit(ch));
    } else {
      return 0;
    }
  } else if (LexIsDigit(ch)) {
    // Decimal
    do {
      ch = linebuf[++linebuf_idx];
    } while (LexIsDigit(ch));
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
  for (i = 0; i < strlen(single_char_operators); i++) {
    if (c == single_char_operators[i]) {
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
int LexIsKeyword(int idx1, int idx2)
{
  int i;
  for (i = 0; keywords[i]; i++) {
    if (memcmp(linebuf + idx1, keywords[i], idx2 - idx1) == 0) {
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
int LexIsVariable(int idx1, int idx2)
{
  return !LexIsKeyword(idx1, idx2);
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

  if (tokp == 0) {
    // No need to do anything, we accept these.
    DEBUG_PRINTF("Empty line");
  } else if (tokens[curr_tok].type == KEYWORD) {
    //
    switch (ParseGetKeyword(curr_tok++)) {
      case PRINT:
        result = StatementPrint(curr_tok);
        break;
      case VAR:
        result = StatementVar(curr_tok);
        break;

      case MEMPEEK:
        result = StatementMempeek(curr_tok);
      default:
        break;
    }
  } else {
    result = StatementExpression(curr_tok);
/*
    if ((result = StatementAssignment(curr_tok)) == rSUCCESS) {
      
    }
    // TODO
    // 
    else if ((result = StatementExpression(curr_tok)) == rSUCCESS) {
        
    }
*/
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
      tokens[tokp].idx1 = idx1;
      tokens[tokp].idx2 = idx2;
      tokens[tokp].type = NUMBER;
      tokp++;
    } else if (LexIsDoubleQuote(ch)) {
      // TODO Change where we store STRING literals.
      // Check for string literals
      idx1 = linebuf_idx + 1;
      do {
        // Make sure string is properly terminated
        ch = linebuf[linebuf_idx++];
        if (LexIsEndOfLine(ch)) {
          THROW_ERROR("Invalid string", linebuf_idx);
          return rFAILURE;
        }
      } while (linebuf[linebuf_idx] != '"');
      idx2 = linebuf_idx++;

      // Save token name and type as STRING
      tokens[tokp].idx1 = idx1;
      tokens[tokp].idx2 = idx2;
      tokens[tokp].type = STRING;
      tokp++;
    } else if (LexIsOperator(ch)) {
      // Operators
      // Check two-character operators first
      int o;
      idx1 = linebuf_idx;
      for (o = 0; o < strlen(double_char_operators); o += 2) {
        if (ch == double_char_operators[o] &&
                  linebuf[idx1 + 1] == double_char_operators[o + 1]) {
          tokens[tokp].idx1 = idx1;
          tokens[tokp].idx2 = idx1 + 2;
          tokens[tokp++].type = OPERATOR;
          linebuf_idx += 2;
          o = -1;
          break;
        }
      }

      if (o > 0) {
        tokens[tokp].idx1 = idx1;
        tokens[tokp].idx2 = idx1 + 1;
        switch (ch) {
          case '+':
            tokens[tokp].type = PLUS;
            break;
          case '-':
            tokens[tokp].type = MINUS;
            break;
          case '=':
            tokens[tokp].type = EQUALS;
            break;
          case ',':
            tokens[tokp].type = COMMA;
            break;
          case '[':
            tokens[tokp].type = OPEN_SQUARE_BRACKET;
            break;
          case ']':
            tokens[tokp].type = CLOSED_SQUARE_BRACKET;
            break;
          default:
            tokens[tokp].type = OPERATOR;
            break;
        }
        tokp++;
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
      tokens[tokp].idx1 = idx1;
      tokens[tokp].idx2 = idx2;

      // Determine if they are keywords, labels, or variables
      if (LexIsKeyword(tokens[tokp].idx1, tokens[tokp].idx2)) {
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
      THROW_ERROR("Invalid token", linebuf_idx);
      return rFAILURE;
    }
  }

#if (DEBUG == 1)
  int i;
  char buf[512];
  for (i = 0; i < tokp; i++) {
    memset(buf, 0, 512);
    memcpy(buf, linebuf + tokens[i].idx1, tokens[i].idx2 - tokens[i].idx1);
    printf("Token %d: %s, type:", i, buf);
    debug_print_type(tokens[i].type);
    puts("");
  }
  puts("");
#endif

  return rSUCCESS;
}

static int ParseGetKeyword(uint32_t token_idx)
{
  int i;
  int idx1 = tokens[token_idx].idx1, idx2 = tokens[token_idx].idx2;
  for (i = 0; keywords[i]; i++) {
    if (memcmp(linebuf + idx1, keywords[i], idx2 - idx1) == 0) {
      break;
    }
  }

  if (keywords[i] == NULL) {
    return -1;
  }

  return i;
}

// TODO
static int ParseTokToNumber(uint32_t token_idx)
{
  char ch1, ch2;
  int idx1 = tokens[token_idx].idx1, idx2 = tokens[token_idx].idx2;
  int32_t result;

  ch1 = linebuf[idx1];
  ch2 = linebuf[idx1 + 1];
  if ((ch1 == '0') && ((ch2 == 'x') || (ch2 == 'X'))) {
    result = ConvertHexNumber(idx1 + 2, idx2);
  } else if (ch1 == '0' && ((ch2 == 'b') || (ch2 == 'B'))) {
    result = ConvertBinNumber(idx1 + 2, idx2);
  } else {
    result = ConvertDecNumber(idx1, idx2);
  }

  return result;
}

static int StatementPrint(uint32_t curr_tok)
{
  // PRINT Syntax
  // PRINT      :== 'PRINT' [PRINT_OBJ] { '+' PRINT_OBJ }*
  // PRINT_OBJ  :== STRING | VARIABLE
  int i, j, size, vloc, var_type;
  uint32_t vval, ctok, offs;
  consolebuf_idx = 0;
  memset(consolebuf, 0, CONSOLEBUF_LEN);

  if (curr_tok < tokp) {
    while (curr_tok < tokp) {
      ctok = curr_tok;
      if (tokens[curr_tok].type == STRING ||
            (VarIsDeclaration(&ctok) == rSUCCESS)) {
        if (tokens[curr_tok].type == STRING) {
          CONSOLE_ADD_STRING_TOK(tokens[curr_tok]);
          curr_tok++;
        } else {
          if ((vloc = VarLocation(curr_tok)) >= 0) {
            if (var_list[vloc].var_type <= VAR_UINT32) {
              size = var_type_sizes[var_list[vloc].var_type];
              i = 0;
              vval = 0;
              while (size--) {
                vval |= (stack[var_list[vloc].addr + i] & 0xFF) << (i << 3);
                i++;
              }
              var_type = var_list[vloc].var_type;
            } else {
              if (ctok - curr_tok > 1) {
                offs = ParseTokToNumber(curr_tok + 2);
                if (var_list[vloc].sub_var_type <= VAR_UINT32) {
                  size = var_type_sizes[var_list[vloc].sub_var_type];
                  i = 0;
                  j = size;
                  vval = 0;
                  while (j--) {
                    vval |= (stack[var_list[vloc].addr +
                            (offs * size) + i] & 0xFF) << (i << 3);
                    i++;
                  }
                  var_type = var_list[vloc].sub_var_type;
                } else {
                  // TODO
                  return rFAILURE;
                }
              } else {
                vval = var_list[vloc].addr;
                var_type = VAR_UINT16;
              }
            }

            switch (var_type) {
              case VAR_CHAR:
                CONSOLE_ADD_CHAR_TOK((char)vval);
                break;
              case VAR_INT8:
                CONSOLE_ADD_SIGNED_TOK((int8_t)vval);
                break;
              case VAR_UINT8:
                CONSOLE_ADD_UNSIGNED_TOK((uint8_t)vval);
                break;
              case VAR_INT16:
                CONSOLE_ADD_SIGNED_TOK((int16_t)vval);
                break;
              case VAR_UINT16:
                CONSOLE_ADD_UNSIGNED_TOK((uint16_t)vval);
                break;
              case VAR_INT32:
                CONSOLE_ADD_SIGNED_TOK((int16_t)vval);
                break;
              case VAR_UINT32:
                CONSOLE_ADD_UNSIGNED_TOK((uint32_t)vval);
                break;
            }

#if 0
            i = var_list[vloc].size_in_bytes;
            vval = 0;
            
            switch (var_list[vloc].var_type) {
              case VAR_INT8:
                do {
                  vval = stack[var_list[vloc].addr];
                  CONSOLE_ADD_SIGNED_TOK((int8_t)vval);
                  if (i > 1) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i--;
                } while (i);
                break;
              case VAR_UINT8:
                do {
                  vval = stack[var_list[vloc].addr];
                  CONSOLE_ADD_UNSIGNED_TOK((uint8_t)vval);
                  if (i > 1) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i--;
                } while (i);
                break;
              case VAR_INT16:
                do {
                  vval = stack[var_list[vloc].addr];
                  vval |= ((uint16_t)stack[var_list[vloc].addr + 1]) << 8;
                  CONSOLE_ADD_SIGNED_TOK((int16_t)vval);
                  if (i > 2) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i -= 2;
                } while (i);
                break;
              case VAR_UINT16:
                do {
                  vval = stack[var_list[vloc].addr];
                  vval |= ((uint16_t)stack[var_list[vloc].addr + 1]) << 8;
                  CONSOLE_ADD_UNSIGNED_TOK((uint16_t)vval);
                  if (i > 2) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i -= 2;
                } while (i);
                break;
              case VAR_INT32:
                do {
                  vval = stack[var_list[vloc].addr];
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 1]) << 8;
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 2]) << 16;
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 3]) << 24;
                  CONSOLE_ADD_SIGNED_TOK((int32_t)vval);
                  if (i > 4) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i -= 4;
                } while (i);
                break;
              case VAR_UINT32:
                do {
                  vval = stack[var_list[vloc].addr];
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 1]) << 8;
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 2]) << 16;
                  vval |= ((uint32_t)stack[var_list[vloc].addr + 3]) << 24;
                  CONSOLE_ADD_UNSIGNED_TOK((uint32_t)vval);
                  if (i > 4) {
                    CONSOLE_ADD_STRING(", ");
                  }
                  i -= 4;
                } while (i);
                break;
              default:
                break;
            }
#endif
          } else {
            THROW_ERROR("Undefined variable.", tokens[curr_tok].idx1 + 1);
            return rFAILURE;
          }
          curr_tok = ctok;
        }

        if (curr_tok < tokp && (tokens[curr_tok].type == PLUS)) {
          if (curr_tok + 1 < tokp) {
            curr_tok++;
          } else {
            THROW_ERROR("Invalid syntax: Missing token",
                        tokens[curr_tok].idx1 + 1);
            return rFAILURE;
          }
        } else if (curr_tok == tokp) {
          break;
        } else {
          THROW_ERROR("Invalid syntax: '+' missing?",
                      tokens[curr_tok].idx1);
          return rFAILURE;
        }
      } else {
        THROW_ERROR("Invalid syntax: Bad token.",
                    tokens[curr_tok].idx1 + 1);
        return rFAILURE;
      }
    }
  }

  CONSOLE_PRINTBUF();
  return rSUCCESS;
}

static int StatementVar(uint32_t curr_tok)
{
  // VAR Syntax
  // VAR              :== 'VAR' VAR_LIST VAR_TYPE
  // VAR_LIST         :== VAR_DECLARATION {, VAR_DECLARATION}*
  // VAR_DECLARATION  :== VARIABLE [ '[' NUMBER ']' ]
  int var_type, sub_var_type;
  uint32_t temp, size_in_bytes;
  if (curr_tok < tokp) {
    // Check VAR_LIST
    if (VarIsList(&curr_tok) == rSUCCESS) {
      if (VarIsType(&curr_tok, &var_type) == rSUCCESS) {
        // Statement must end here.
        if (curr_tok < tokp) {
          THROW_ERROR("Invalid syntax", tokens[curr_tok].idx1 + 1);
          return rFAILURE;
        }

        // Once we have a valid statement, we must interpret it properly.
        // Save the variable size and type
        if (var_type < sizeof(var_type_sizes)) {
          size_in_bytes = var_type_sizes[var_type];
          if (var_type > (int)VAR_UINT32) {
            sub_var_type = var_type - NUM_DATA_TYPES;
          } else {
            sub_var_type = -1;
          }
        } else {
          THROW_ERROR("Uknown error", 0);
          return rFAILURE;
        }

        // Go through all the VARIABLE tokens and see if they
        // need to be added to the stack (if they don't exist).
        // Throw an error if they already do!
        for (curr_tok = 0; curr_tok < tokp; curr_tok++) {
          if (tokens[curr_tok].type == VARIABLE) {
            // Make sure it doesn't exist.
            if (VarLocation(curr_tok) >= 0) {
              THROW_ERROR("Variable already defined",
                          tokens[curr_tok].idx1 + 1);
              return rFAILURE;
            }

            // Add it!
            // 1. Save the name and sub_var_type
            temp = tokens[curr_tok].idx2 - tokens[curr_tok].idx1;
            memcpy(var_list[varp].name, linebuf + tokens[curr_tok].idx1, temp);
            var_list[varp].sub_var_type = sub_var_type;
            // 2. Add to the stack
            //  i) Save location on stack to idx
            var_list[varp].addr = sp;
            //  ii) Update stack based on variable size
            if (curr_tok + 3 < tokp && tokens[curr_tok + 2].type == NUMBER) {
              var_list[varp].len = ParseTokToNumber(curr_tok + 2);
              if (var_list[varp].len == 0) {
                THROW_ERROR("Array length must be non-zero",
                            tokens[curr_tok + 2].idx1 + 1);
                return rFAILURE;
              }
              if (var_list[varp].len > 1) {
                var_list[varp].sub_var_type = var_type;
                if (var_type <= (int)VAR_UINT32) {
                  var_type += NUM_DATA_TYPES;
                }
              }
            } else {
              var_list[varp].len = 1;
            }
            sp += size_in_bytes * var_list[varp].len;
            // 3. Save the var_type and size_in_bytes
            var_list[varp].var_type = var_type;
            var_list[varp].size_in_bytes = size_in_bytes;
            varp++;
          }
        }
      } else {
        return rFAILURE;
      }
    } else {
      return rFAILURE;
    }
  } else {
    return rFAILURE;
  }

  return rSUCCESS;
}

static int StatementAssignment(uint32_t curr_tok)
{
  // ASSIGNMENT Syntax
  // ASSIGNMENT :== { VAR_DECLARATION '=' } EXPRESSION
  // VAR_DECLARATION :== VARIABLE [ '[' NUMBER ']' ]
  if (curr_tok < tokp) {
    do {
      if (VarIsDeclaration(&curr_tok) == rSUCCESS) {
        if (curr_tok < tokp && tokens[curr_tok].type == EQUALS) {
          curr_tok++;
          if (curr_tok >= tokp) {
            THROW_ERROR("Missing expression", tokens[curr_tok - 1].idx2 + 1);
            return rFAILURE;
          }
        } else {
          curr_tok--;
          break;
        }
      } else {
        THROW_ERROR("Expecting type VARIABLE", tokens[curr_tok].idx1);
        return rFAILURE;
      }
    } while (tokens[curr_tok].type == VARIABLE);
  } else {
    THROW_ERROR("Missing tokens", 0);
    return rFAILURE;
  }

  // Check EXPRESSION
  DEBUG_PRINTF("curr_tok = %d\n", curr_tok);
  return rSUCCESS;
  return StatementExpression(curr_tok);
}

static int StatementExpression(uint32_t curr_tok)
{
  // EXPRESSION Syntax
  // EXPRESSION :== TERM | TERM { [+,-,&,|] TERM }
  // TERM      :== FACTOR | FACTOR { [*,/] FACTOR }
  // FACTOR    :== NUMBER | [+,-] NUMBER | VARIABLE | [+,-] VARIABLE | '(' EXPRESSION ')'
  
#if 0
  if (curr_tok < tokp) {
    // Check TERM(s)
    do {
      if (ExprIsTerm(&curr_tok) == rSUCCESS) {
        // Check [+,-] TERM
        if (curr_tok < tokp) {
          if (!(tokens[curr_tok].type == PLUS || tokens[curr_tok].type == MINUS)) {
            return rFAILURE;
          }
        }
      } else {
        return rFAILURE; 
      }
    } while (curr_tok < tokp);
  } else {
    return rFAILURE;
  }
#else
  // HACK FOR TESTING
  if (curr_tok + 2 >= tokp) {
    return rFAILURE;
  }

  if (VarIsDeclaration(&curr_tok) != rSUCCESS) {
    return rFAILURE;
  }

  int vloc, offs = 0;
  uint32_t vval;
  int size, i, j;
  token_t tok2, tok3;
  tok2 = tokens[curr_tok];
  tok3 = tokens[curr_tok + 1];
  if (tok2.type != EQUALS) {
    return rFAILURE;
  }
  if (tok3.type != NUMBER) {
    return rFAILURE;
  }

  if ((vloc = VarLocation(0)) < 0) {
    return rFAILURE;
  }

  vval = ParseTokToNumber(curr_tok + 1);
  if (var_list[vloc].var_type <= VAR_UINT32) {
    size = var_type_sizes[var_list[vloc].var_type];
    i = 0;
    while (size--) {
      stack[var_list[vloc].addr + i] = vval & 0xFF;
      i++;
      vval >>= 8;
    }
  } else {
    if (curr_tok > 1) {
      offs = ParseTokToNumber(2);
      if (var_list[vloc].sub_var_type <= VAR_UINT32) {
        size = var_type_sizes[var_list[vloc].sub_var_type];
        i = 0;
        j = size;
        while (j--) {
          stack[var_list[vloc].addr + (offs * size) + i] = vval & 0xFF;
          i++; vval >>= 8;
        }
      }
    } else {
      var_list[vloc].addr = vval;
    }
  }

/*
  switch (var_list[vloc].var_type) {
    case VAR_INT8:
    case VAR_UINT8:
      stack[var_list[vloc].addr] = vval & 0xFF;
      break;
    case VAR_INT16:
    case VAR_UINT16:
      stack[var_list[vloc].addr] = vval & 0xFF;
      stack[var_list[vloc].addr + 1] = (vval >> 8) & 0xFF;
    case VAR_INT32:
    case VAR_UINT32:
      stack[var_list[vloc].addr] = vval & 0xFF;
      stack[var_list[vloc].addr + 1] = (vval >> 8) & 0xFF;
      stack[var_list[vloc].addr + 2] = (vval >> 16) & 0xFF;
      stack[var_list[vloc].addr + 3] = (vval >> 24) & 0xFF;
    default:
      break;
  }
*/
#endif

  return rSUCCESS;
}

static int StatementMempeek(uint32_t curr_tok)
{
  // MEMPEEK Syntax
  // MEMPEEK :== 'MEMPEEK'
  int i;
  if (curr_tok < tokp) {
    THROW_ERROR("Invalid syntax; Usage: MEMPEEK", tokens[curr_tok].idx1 + 1);
    return rFAILURE;
  }

  // Show stack, var_list info.
  CONSOLE_PRINTF("Stack Size: %d\n", sp);
  for (i = 0; i < sp; i++) {
    CONSOLE_PRINTF("stack[%d] = %d\n", i, stack[i]);
  }
  CONSOLE_PRINTF("Var List Size: %d\n", varp);
  for (i = 0; i < varp; i++) {
    CONSOLE_PRINTF("var_list[%d].name = %s\n", i, var_list[i].name);
    CONSOLE_PRINTF("var_list[%d].addr = %u\n", i, var_list[i].addr);
    CONSOLE_PRINTF("var_list[%d].size = %u\n", i, var_list[i].size_in_bytes);
    CONSOLE_PRINTF("var_list[%d].len = %u\n", i, var_list[i].len);
    CONSOLE_PRINTF("var_list[%d].type = %u\n", i, var_list[i].var_type);
    CONSOLE_PRINTF("var_list[%d].subtype = %d\n", i, var_list[i].sub_var_type);
  }
  return rSUCCESS;
}

static int VarIsList(uint32_t *curr_tok)
{
  // VAR_LIST :== VAR_DECLARATION {, VAR_DECLARATION}*
  while ((*curr_tok) < tokp) {
    if (VarIsDeclaration(curr_tok) == rSUCCESS) {
      if ((*curr_tok) < tokp && tokens[*curr_tok].type == COMMA) {
        (*curr_tok)++;
        if ((*curr_tok) >= tokp) {
          THROW_ERROR("Missing VARIABLE token", tokens[*curr_tok - 1].idx2 + 1);
          return rFAILURE;
        }
      } else {
        break;
      }
    } else {
      return rFAILURE;
    }
  }

  return rSUCCESS;
}

static int VarIsDeclaration(uint32_t *curr_tok)
{
  // VAR_DECLARATION :== VARIABLE [ '[' NUMBER ']' ]
  uint32_t ctok = *curr_tok;

  if (tokens[ctok++].type == VARIABLE) {
    if (ctok < tokp && tokens[ctok].type == OPEN_SQUARE_BRACKET) {
      ctok++;
      if (ctok < tokp && tokens[ctok].type == NUMBER) {
        ctok++;
        if (ctok < tokp && tokens[ctok].type == CLOSED_SQUARE_BRACKET) {
          ctok++;
        } else {
          return rFAILURE;
        }
      } else {
        return rFAILURE;
      }
    }
  } else {
    THROW_ERROR("Invalid VARIABLE token", tokens[*curr_tok].idx1 + 1);
    return rFAILURE;
  }

  //
  *curr_tok = ctok;
  return rSUCCESS;
}

static int VarIsType(uint32_t *curr_tok, int *type)
{
  int var_type;

  if (tokens[*curr_tok].type != KEYWORD) {
    return rFAILURE;
  }

  switch ((var_type = ParseGetKeyword((*curr_tok)++))) {
    case CHAR:
      *type = (int)VAR_CHAR;
      break;
    case INT8:
      *type = (int)VAR_INT8;
      break;
    case UINT8:
      *type = (int)VAR_UINT8;
      break;
    case INT16:
      *type = (int)VAR_INT16;
      break;
    case UINT16:
      *type = (int)VAR_UINT16;
      break;
    case INT32:
      *type = (int)VAR_INT32;
      break;
    case UINT32:
      *type = (int)VAR_UINT32;
      break;
    case CHARPTR:
      *type = (int)VAR_CHARPTR;
      break;
    case INT8PTR:
      *type = (int)VAR_INT8PTR;
      break;
    case UINT8PTR:
      *type = (int)VAR_UINT8PTR;
      break;
    case INT16PTR:
      *type = (int)VAR_INT16PTR;
      break;
    case UINT16PTR:
      *type = (int)VAR_UINT16PTR;
      break;
    case INT32PTR:
      *type = (int)VAR_INT32PTR;
      break;
    case UINT32PTR:
      *type = (int)VAR_UINT32PTR;
      break;
    default:
      return rFAILURE;
  }

  return rSUCCESS;
}

static int VarLocation(uint32_t curr_tok)
{
  int v;
  for (v = 0; v < varp; v++) {
    if (memcmp(linebuf + tokens[curr_tok].idx1, var_list[v].name,
        tokens[curr_tok].idx2 - tokens[curr_tok].idx1) == 0) {
      return v;
    }
  }
 
  return -1;
}

static int ExprIsTerm(uint32_t *curr_tok)
{
  // TERM :== FACTOR | FACTOR { [*,/] FACTOR }
  if (*curr_tok < tokp) {
    do {
      if (ExprIsFactor(curr_tok) == rSUCCESS) {
        // Check [*,/] FACTOR
        if (*curr_tok < tokp) {
          
        }
      } else {
        return rFAILURE;
      }
    } while (*curr_tok < tokp);
  }
  return rSUCCESS;
}

static int ExprIsFactor(uint32_t *curr_tok)
{
  // FACTOR :== NUMBER | [+,-] NUMBER | VARIABLE | [+,-] VARIABLE | '(' EXPRESSION ')'
  uint32_t ctok = *curr_tok;

  if (tokens[ctok].type == NUMBER) {

  } else if (tokens[ctok].type == PLUS) {

  } else if (tokens[ctok].type == MINUS) {

  } 

  *curr_tok = ctok;
  return rSUCCESS;
}

static int32_t ConvertHexNumber(int idx1, int idx2)
{
  int result = 0;
  int shift_amt = 0;
  while (idx2-- != idx1) {
    result += GetHexValue(linebuf[idx2]) << shift_amt;
    shift_amt += 4;
  }
  return result;
}

static int32_t ConvertBinNumber(int idx1, int idx2)
{
  int result = 0;
  int shift_amt = 0;
  while (idx2-- != idx1) {
    result += (linebuf[idx2] - '0') << shift_amt;
    shift_amt++;
  }
  return result;
}

static int32_t ConvertDecNumber(int idx1, int idx2)
{
  int result = 0;
  int mult = 1;
  while (idx2-- != idx1) {
    result += (linebuf[idx2] - '0') * mult;
    mult *= 10;
  }

  return result;
}

static int32_t GetHexValue(char ch)
{
  if (LexIsDigit(ch)) {
    return ch - '0';
  } else {
    if ('A' <= ch && ch <= 'F') {
      return ch - 'A' + 10;
    } else {
      return ch - 'a' + 10;
    }
  }
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
    case PLUS:
      printf("Plus");
      break;
    case MINUS:
      printf("Minus");
      break;
    case EQUALS:
      printf("Equals");
      break;
    case COMMA:
      printf("Comma");
      break;
    case OPEN_SQUARE_BRACKET:
      printf("Open Square Bracket");
      break;
    case CLOSED_SQUARE_BRACKET:
      printf("Closed Square Bracket");
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
