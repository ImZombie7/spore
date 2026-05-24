#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  MAX_LINES = 1024,
  MAX_LINE = 1024,
};

static char lines[MAX_LINES][MAX_LINE];
static char *order[MAX_LINES];
static int reverse_order;

static int cmp_lines(const void *a, const void *b) {
  const char *lhs = *(char *const *)a;
  const char *rhs = *(char *const *)b;
  int rc = strcmp(lhs, rhs);
  return reverse_order ? -rc : rc;
}

static int read_stream(FILE *f, size_t *count) {
  while (*count < MAX_LINES && fgets(lines[*count], sizeof(lines[*count]), f) != NULL) {
    order[*count] = lines[*count];
    ++*count;
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  if (!feof(f)) {
    eprintf("sort: too many lines\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  int first = 1;
  if (argc > 1 && streq(argv[1], "-r")) {
    reverse_order = 1;
    first = 2;
  }
  size_t count = 0;
  int rc = EXIT_SUCCESS;
  if (first == argc) {
    rc = read_stream(stdin, &count);
  } else {
    for (int i = first; i < argc; ++i) {
      FILE *f = fopen(argv[i], "r");
      if (f == NULL) {
        perror(argv[i]);
        rc = EXIT_FAILURE;
        continue;
      }
      if (read_stream(f, &count) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
      fclose(f);
    }
  }
  qsort(order, count, sizeof(order[0]), cmp_lines);
  for (size_t i = 0; i < count; ++i) {
    fputs(order[i], stdout);
    if (strchr(order[i], '\n') == NULL) { putchar('\n'); }
  }
  return rc;
}
