#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_LINES = 128, LINE_CAP = 512 };

static char ring[MAX_LINES][LINE_CAP];

static int tail_stream(FILE *f, long lines) {
  long keep = lines > MAX_LINES ? MAX_LINES : lines;
  long seen = 0;
  while (fgets(ring[seen % MAX_LINES], LINE_CAP, f) != NULL) {
    ++seen;
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  long start = seen > keep ? seen - keep : 0;
  for (long i = start; i < seen; ++i) {
    fputs(ring[i % MAX_LINES], stdout);
  }
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  long lines = 10;
  int first = 1;
  if (argc >= 3 && streq(argv[1], "-n")) {
    lines = strtol(argv[2], NULL, 10);
    first = 3;
  }
  if (lines < 0) { return usage("tail", "[-n COUNT] [FILE...]"); }
  if (first == argc) { return tail_stream(stdin, lines); }
  int rc = EXIT_SUCCESS;
  for (int i = first; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (tail_stream(f, lines) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
