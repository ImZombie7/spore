#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  for (;;) {
    if (argc == 1) {
      puts("y");
    } else {
      for (int i = 1; i < argc; ++i) {
        if (i > 1) { putchar(' '); }
        fputs(argv[i], stdout);
      }
      putchar('\n');
    }
    if (fflush(stdout) != 0) { return EXIT_FAILURE; }
  }
}
