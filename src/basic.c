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

// Constants -----
const char operators[] = {
  '(', ')', '=', '+', '-', '*', '/'
};

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

// Constants -----
const char *keywords[] = {
  "PRINT",
  ""
};

// Function Prototypes -----
int parseline(FILE *f);
int interpret(void);
int iseof(char c);
int isendofline(char c);
int iswhitespace(char c);
int isdoublequote(char c);
int isoperator(char c);
int isalpha(char c);
int isdigit(char c);
int ishexdigit(char c);

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

  while (parseline(fp) == 0);

  // Done! Close file.
  fclose(fp);
  puts("BASIC test program exited successfully.");
  return EXIT_SUCCESS;
}

// Function Definitions -----
int parseline(FILE *f)
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
  printf("%s", linebuf);
#endif

  // Interpret the line
  if (interpret() == rFAILURE) {
    puts("Parse error!");
    return rFAILURE;
  }

  // Reset line buffer
  linebuf_idx = 0;
  memset(linebuf, 0, LINEBUF_LEN);

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
    } else if (isdigit(ch)) {
      // Check number format (dec, hex, etc.)
      ch = linebuf[linebuf_idx++];
      if (ch == 'x' || ch == 'X') {
        // TODO
      } else {
        // TODO
      }
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

      // Save token name
#if (DEBUG == 1)
      printf("idx1: %d, idx2: %d\r\n", idx1, idx2);
#endif
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
      tokens[tokp++].name[idx2 - idx1] = '\0';
    } else if (isoperator(ch)) {
      strcpy(tokens[tokp++].name, &ch);
      linebuf_idx++;
    } else if (isalpha(ch)) {
      idx1 = linebuf_idx;
      while (!iswhitespace(ch)) {
        ch = linebuf[++linebuf_idx];
        if (isendofline(ch)) {
          break;
        }
      }
      idx2 = linebuf_idx++;

      // Save token name
#if (DEBUG == 1)
      printf("idx1: %d, idx2: %d\r\n", idx1, idx2);
#endif
      memcpy(tokens[tokp].name, linebuf + idx1, idx2 - idx1);
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
    printf("Token %d: %s\r\n", i, tokens[i].name);
  }
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
  if (c == ' ' || c == '\t') {
    return 1;
  }

  return 0;
}

int isdoublequote(char c)
{
  return (c == '"' ? 1 : 0);
}

int isoperator(char c)
{
  int i;
  for (i = 0; i < sizeof(operators); i++) {
    if (c == operators[i]) {
      return 1;
    }
  }

  return 0;
}

int isalpha(char c)
{
  if ('A' <= c && c <= 'Z') {
    return 1;
  }

  if ('a' <= c && c <= 'z') {
    return 1;
  }

  return 0;
}

int isdigit(char c)
{
  if ('0' <= c && c <= '9') {
    return 1;
  }

  return 0;
}

int ishexdigit(char c)
{
  if (isdigit(c) || ('A' <= c && c <= 'F')
      || ('a' <= c && c <= 'f')) {
    return 1;
  }

  return 0;
}

