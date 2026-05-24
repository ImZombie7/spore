#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int uniq_stream(FILE *f, int counts) {
  char prev[1024] = {0};
  char line[1024];
  unsigned long count = 0;
  int have = 0;
  while (fgets(line, sizeof(line), f) != NULL) {
    if (!have || strcmp(prev, line) != 0) {
      if (have) { printf(counts ? "%7lu %s" : "%s", count, prev); }
      snprintf(prev, sizeof(prev), "%s", line);
      count = 1;
      have = 1;
    } else {
      ++count;
    }
  }
  if (have) { printf(counts ? "%7lu %s" : "%s", count, prev); }
  return ferror(f) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  int counts = 0;
  int first = 1;
  if (argc > 1 && streq(argv[1], "-c")) {
    counts = 1;
    first = 2;
  }
  if (first == argc) { return uniq_stream(stdin, counts); }
  FILE *f = fopen(argv[first], "r");
  if (f == NULL) {
    perror(argv[first]);
    return EXIT_FAILURE;
  }
  int rc = uniq_stream(f, counts);
  fclose(f);
  return rc;
}
