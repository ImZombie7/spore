#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int head_stream(FILE *f, long lines) {
  char buf[512];
  long left = lines;
  while (left > 0 && fgets(buf, sizeof(buf), f) != NULL) {
    fputs(buf, stdout);
    if (strchr(buf, '\n') != NULL) { --left; }
  }
  return ferror(f) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  long lines = 10;
  int first = 1;
  if (argc >= 3 && streq(argv[1], "-n")) {
    lines = strtol(argv[2], NULL, 10);
    first = 3;
  }
  if (lines < 0) { return usage("head", "[-n COUNT] [FILE...]"); }
  if (first == argc) { return head_stream(stdin, lines); }
  int rc = EXIT_SUCCESS;
  for (int i = first; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (head_stream(f, lines) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
