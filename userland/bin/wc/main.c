#include <ctype.h>
#include <spore.h>
#include <stdio.h>
#include <stdlib.h>

static int wc_stream(FILE *f, const char *name) {
  unsigned long lines = 0;
  unsigned long words = 0;
  unsigned long bytes = 0;
  int in_word = 0;
  for (;;) {
    int c = fgetc(f);
    if (c == EOF) { break; }
    ++bytes;
    if (c == '\n') { ++lines; }
    if (isspace((unsigned char)c)) {
      in_word = 0;
    } else if (!in_word) {
      in_word = 1;
      ++words;
    }
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  printf("%lu %lu %lu", lines, words, bytes);
  if (name != NULL) { printf(" %s", name); }
  putchar('\n');
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc == 1) { return wc_stream(stdin, NULL); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (wc_stream(f, argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
