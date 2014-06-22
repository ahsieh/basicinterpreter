
/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include "basic.h"

/* Defines ------------------------------------------------------------------ */
/* Variables ---------------------------------------------------------------- */
/* Constants ---------------------------------------------------------------- */
/* Private Function Prototypes ---------------------------------------------- */

/* Main code ---------------------------------------------------------------- */
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

  if (BasicInterpret(fp) != rSUCCESS) {
    printf("Error: Line: %d, Column: %d", LexCurrentLineCount(), LexCurrentColumnCount());
    puts("");

    printf("%s", LexCurrentLine());
    int i;
    for (i = 0; i < LexCurrentColumnCount() - 1; i++) {
      printf(" ");
    }
    puts("^");
    puts(LexErrorMessage());
  }

  // Done! Close file.
  fclose(fp);
  puts("");
  puts("BASIC test program exited successfully.");
  return EXIT_SUCCESS;
}

/**************************************************************** END OF FILE */

