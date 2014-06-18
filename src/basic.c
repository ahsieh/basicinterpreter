#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Defines -----
#define LINEBUF_LEN     512
#define STACK_SIZE      512
#define MAX_VAR_COUNT   512
#define VAR_NAME_LEN    64
#define TOK_NAME_LEN    64
#define MAX_TOK_COUNT   32

#define rSUCCESS        0
#define rFAILURE        1
#define rEOF            2

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
  ""
};

const char *single_char_operators[] = {
  "(", ")", "=", "+", "-", "*", "/", ":",
  "&", "|"
};

const char *double_char_operators[] = {
  "==", "&&", "||"
};


// Function Prototypes -----
int lexline(FILE *f);
int interpret(void);
int iseof(char c);
int isendofline(char c);
int iswhitespace(char c);
int isinlinecomment(char c);
int isdoublequote(char c);
int lexisoperator(char c);
int lexisalpha(char c);
int lexisdigit(char c);
int lexisbindigit(char c);
int lexishexdigit(char c);

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

  while (lexline(fp) == 0);

  // Done! Close file.
  fclose(fp);
  puts("");
  puts("BASIC test program exited successfully.");
  return EXIT_SUCCESS;
}

// Function Definitions -----
int lexline(FILE *f)
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

  // Interpret the line
  if (interpret() == rFAILURE) {
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


int interpret(void)
{
  char ch;
  int idx1 = 0, idx2 = 0;

  // Reset index
  linebuf_idx = 0;

  //
  while (!isendofline(linebuf[linebuf_idx])) {
    ch = linebuf[linebuf_idx];
    if (iswhitespace(ch)) {
      // Ignore white spaces
      linebuf_idx++;
    } else if (isinlinecomment(ch)) {
      // Ignore comments
#if DEBUG >= 1
      printf("Comment: %s\r\n", linebuf + linebuf_idx + 1);
#endif
      break;
    } else if (lexisdigit(ch)) {
      // Check number format (dec, hex, etc.)
      idx1 = linebuf_idx;
      ch = linebuf[++linebuf_idx];
      if (ch == 'x' || ch == 'X') {
        // Hex
        do {
          ch = linebuf[++linebuf_idx];
        } while (lexishexdigit(ch));
      } else if (ch == 'b' || ch == 'B') {
        // Binary
        do {
          ch = linebuf[++linebuf_idx];
        } while (lexisbindigit(ch));
      } else {
        while (lexisdigit(ch)) {
          ch = linebuf[++linebuf_idx];
        }
      } 
      idx2 = linebuf_idx;

      // Save token name and type as NUMBER
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = NUMBER;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (isdoublequote(ch)) {
      // Check for string literals
      idx1 = linebuf_idx + 1;
      do {
        // Make sure string is properly terminated
        ch = linebuf[linebuf_idx++];
        if (isendofline(ch)) {
          puts("STRING FAILURE");
          return rFAILURE;
        }
      } while (linebuf[linebuf_idx] != '"');
      idx2 = linebuf_idx++;

      // Save token name and type as STRING
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = STRING;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (lexisoperator(ch)) {
      // Operators
      idx1 = linebuf_idx;
      do {
        ch = linebuf[++linebuf_idx];
      } while (lexisoperator(ch));
      idx2 = linebuf_idx;
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = OPERATOR;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (lexisalpha(ch)) {
      // Identifiers
      idx1 = linebuf_idx;
      while (lexisalpha(ch) || lexisdigit(ch)) {
        ch = linebuf[++linebuf_idx];
        if (isendofline(ch) || iswhitespace(ch)) {
          break;
        }
      }
      idx2 = linebuf_idx;

      // Save token name and type as IDENTIFIER
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp].type = IDENTIFIER;
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (iseof(ch)) {
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

int iseof(char c)
{
  return (c == '\0' ? 1 : 0);
}

int isendofline(char c)
{
  return (c == '\n' ? 1 : 0);
}

int iswhitespace(char c)
{
  if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
    return 1;
  }

  return 0;
}

int isinlinecomment(char c)
{
  if (c == '#') {
    return 1;
  }

  return 0;
}

int isdoublequote(char c)
{
  return (c == '"' ? 1 : 0);
}

int lexisoperator(char c)
{
  int i;
  for (i = 0; single_char_operators[i]; i++) {
    if (c == single_char_operators[i][0]) {
      return 1;
    }
  }

  return 0;
}

int lexisalpha(char c)
{
  if ('A' <= c && c <= 'Z') {
    return 1;
  }

  if ('a' <= c && c <= 'z') {
    return 1;
  }

  return 0;
}

int lexisdigit(char c)
{
  if ('0' <= c && c <= '9') {
    return 1;
  }

  return 0;
}

int lexisbindigit(char c)
{
  if (c == '0' || c == '1') {
    return 1;
  }

  return 0;
}

int lexishexdigit(char c)
{
  if (lexisdigit(c) || ('A' <= c && c <= 'F')
      || ('a' <= c && c <= 'f')) {
    return 1;
  }

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
    default:
      break;
  }
}
#endif
