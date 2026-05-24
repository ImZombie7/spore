#include <spore.h>
#include <stdio.h>
#include <stdlib.h>

static int sum_stream(FILE *f, const char *name) {
  unsigned long sum = 0;
  unsigned long bytes = 0;
  for (;;) {
    int c = fgetc(f);
    if (c == EOF) { break; }
    sum = (sum + (unsigned char)c) % 65535u;
    ++bytes;
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  printf("%lu %lu", sum, bytes);
  if (name != NULL) { printf(" %s", name); }
  putchar('\n');
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc == 1) { return sum_stream(stdin, NULL); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "rb");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (sum_stream(f, argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
