
/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include "basic.h"

/* Defines ------------------------------------------------------------------ */
/* Variables ---------------------------------------------------------------- */
/* Constants ---------------------------------------------------------------- */
/* Private Function Prototypes ---------------------------------------------- */
static void PrintErrorMessage(void);

/* Main code ---------------------------------------------------------------- */
int main(int argc, char *argv[])
{
  FILE *fp;
  int result;

  // Make sure we are using the executable correctly.
  if (argc == 1) {
    while ((result = BasicCommandLine()) == rSUCCESS) {
    }
    if (result == rFAILURE) {
      PrintErrorMessage();
    }
  } else if (argc == 2) {
    // Open the provided file.
    fp = fopen(argv[1], "r");
    if (!fp) {
      printf("Could not open file %s!\r\n", argv[1]);
      return EXIT_FAILURE;
    }

    // Let user know we are running the file now.
    printf("Running BASIC script %s\r\n", argv[1]);

    if (BasicInterpret(fp) != rSUCCESS) {
      printf("Error: Line: %d, Column: %d", LexGetCurrentLineCount(), LexGetCurrentColumnCount());
      puts("");

      printf("%s", LexGetCurrentLine());
      int i;
      for (i = 0; i < LexGetCurrentColumnCount() - 1; i++) {
        printf(" ");
      }
      puts("^");
      puts(LexGetErrorMessage());
    }

    // Done! Close file.
    fclose(fp);
    puts("");
    puts("BASIC test program exited successfully.");
  } else {
    puts("Usage: ./basic [filename]");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static void PrintErrorMessage(void)
{
  printf("Error: Line: %d, Column: %d", LexGetCurrentLineCount(), LexGetCurrentColumnCount());
  puts("");

  printf("%s", LexGetCurrentLine());
  int i;
  for (i = 0; i < LexGetCurrentColumnCount() - 1; i++) {
    printf(" ");
  }
  printf("^ ");
  puts(LexGetErrorMessage());
}

/**************************************************************** END OF FILE */

